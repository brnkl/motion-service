#include "interfaces.h"
#include "legato.h"
#include "util.h"
#include <pthread.h>
#include <semaphore.h>

#define N_CHANGE_BLOCKS 200

void *impactMonitor(void *);

typedef struct{
  double x;
  double y;
  double z;
} Acceleration;

double xAccImpact [N_CHANGE_BLOCKS];
double yAccImpact [N_CHANGE_BLOCKS];
double zAccImpact [N_CHANGE_BLOCKS];
uint64_t timestamps [N_CHANGE_BLOCKS];

static const char FormatStr[] = "/sys/devices/i2c-0/0-0068/iio:device0/in_%s_%s";
static const char AccType[]   = "accel";
static const char GyroType[]  = "anglvel";
static const char CompX[]     = "x_raw";
static const char CompY[]     = "y_raw";
static const char CompZ[]     = "z_raw";
static const char CompScale[] = "scale";
int totalImpacts              = 0; 
static const double impactThreshold = 20.0;

Acceleration suddenImpact = {0, 0, 0};

pthread_t impact_thread;
sem_t impact_mutex;


/*
 * Reports the x, y and z accelerometer readings in meters per second squared.
*/

le_result_t brnkl_motion_getCurrentAcceleration(
    double *xAcc,
    double *yAcc,
    double *zAcc
)
{
    le_result_t r;
    char path[256];

    double scaling = 0.0;
    int pathLen = snprintf(path, sizeof(path), FormatStr, AccType, CompScale);
    LE_ASSERT(pathLen < sizeof(path));
    r = ioutil_readDoubleFromFile(path, &scaling);
    if (r != LE_OK)
    {
        goto done;
    }

    pathLen = snprintf(path, sizeof(path), FormatStr, AccType, CompX);
    LE_ASSERT(pathLen < sizeof(path));
    r = ioutil_readDoubleFromFile(path, xAcc);
    if (r != LE_OK)
    { 
        goto done;
    }
    *xAcc *= scaling;

    pathLen = snprintf(path, sizeof(path), FormatStr, AccType, CompY);
    LE_ASSERT(pathLen < sizeof(path));
    r = ioutil_readDoubleFromFile(path, yAcc);
    if (r != LE_OK)
    {
        goto done;
    }
    *yAcc *= scaling;

    pathLen = snprintf(path, sizeof(path), FormatStr, AccType, CompZ);
    LE_ASSERT(pathLen < sizeof(path));
    r = ioutil_readDoubleFromFile(path, zAcc);
    *zAcc *= scaling;

done:
    LE_INFO("Showing accel X: %f Y: %f Z: %f ", *xAcc, *yAcc, *zAcc);
    return r;
}

void recordImpact(double* xAcc, double* yAcc, double* zAcc){
  timestamps[totalImpacts] = (unsigned long)time(NULL);
  xAccImpact[totalImpacts] = *xAcc;
  yAccImpact[totalImpacts] = *yAcc;
  zAccImpact[totalImpacts] = *zAcc;
  totalImpacts++;

  LE_INFO("New Impact, totalImpacts: %d", totalImpacts);
}

le_result_t brnkl_motion_getSuddenImpact(double* xAcc, size_t *xSize,
              double* yAcc, size_t *ySize,
              double* zAcc, size_t *zSize) {

  if(!totalImpacts)
    LE_INFO("No Sudden Impacts to Report");

  *xSize = *ySize = *zSize = totalImpacts;

  xAcc = xAccImpact;
  yAcc = yAccImpact;
  zAcc = zAccImpact;

  totalImpacts = 0;

  return LE_OK;
}

/*
*Monitors accelerometer from iio on 100ms intervals
*Sets hasSuddenImpact flag when accelerometer surpasses threshold
*/
void *impactMonitor(void * ptr){
  double x, y, z;
  for(;;){
    brnkl_motion_getCurrentAcceleration(&x, &y, &z);

    double euclidian = sqrt(x*x + y*y + z*z);

    if(euclidian > impactThreshold){
      LE_INFO("euclidian : %f", euclidian);
      //3. add x, y, z to impact array
      sem_wait(&impact_mutex);
      recordImpact(&x, &y, &z);
      sem_post(&impact_mutex);
      }
    usleep(100*1000);

  }
  return ptr;
}

/*
*Create thread to monitor accelerometer iio
*/
void initThread(){
  int thread, mutx;

  thread = pthread_create( &impact_thread, NULL, impactMonitor, NULL);
  mutx   = sem_init(&impact_mutex, 0, 1);

  if(thread && mutx){
    LE_INFO("Reader Thread Created");
  }else{
    LE_ERROR("Reader Thread or Mutex Creation Failed");
  }
}

COMPONENT_INIT {
  initThread();
}