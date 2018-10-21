#include "interfaces.h"
#include "legato.h"
#include "util.h"
#include <pthread.h>

#define N_CHANGE_BLOCKS 200
#define SAMPLE_PERIOD_MS 100

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

pthread_t impactThread;
pthread_mutex_t impactMutex;


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
    r = ioutil_readDoubleFromFile(path, &scaling);
    if (r != LE_OK)
    {
        goto done;
    }

    r = ioutil_readDoubleFromFile(path, xAcc);
    if (r != LE_OK)
    { 
        goto done;
    }
    *xAcc *= scaling;

    r = ioutil_readDoubleFromFile(path, yAcc);
    if (r != LE_OK)
    {
        goto done;
    }
    *yAcc *= scaling;

    r = ioutil_readDoubleFromFile(path, zAcc);
    *zAcc *= scaling;

done:
    LE_INFO("Showing accel X: %f Y: %f Z: %f ", *xAcc, *yAcc, *zAcc);
    return r;
}

le_result_t recordImpact(double* xAcc, double* yAcc, double* zAcc){
  if( 
    totalImpacts > N_CHANGE_BLOCKS ||
    totalImpacts > N_CHANGE_BLOCKS ||
    totalImpacts > N_CHANGE_BLOCKS
    )
    return LE_OUT_OF_RANGE;

  timestamps[totalImpacts] = GetCurrentTimestamp();
  xAccImpact[totalImpacts] = *xAcc;
  yAccImpact[totalImpacts] = *yAcc;
  zAccImpact[totalImpacts] = *zAcc;
  totalImpacts++;
  LE_INFO("New Impact, totalImpacts: %d", totalImpacts);

  return LE_OK;
}

le_result_t brnkl_motion_getSuddenImpact(double* xAcc, size_t *xSize,
              double* yAcc, size_t *ySize,
              double* zAcc, size_t *zSize) {

  if(!totalImpacts)
    LE_INFO("No Sudden Impacts to Report");
  else {
    pthread_mutex_lock(&impactMutex);
    //check 

    if(
        totalImpacts < *xSize ||
        totalImpacts < *ySize ||
        totalImpacts < *zSize
        )
        return LE_OUT_OF_RANGE;

    for(int i = 0; i < totalImpacts; i++){
      xAcc[i] = xAccImpact[i];
      yAcc[i] = yAccImpact[i];
      zAcc[i] = zAccImpact[i];
    }
   
    *xSize = *ySize = *zSize = totalImpacts;


    totalImpacts = 0;

    pthread_mutex_unlock(&impactMutex);
  }

  return LE_OK;
}

/*
*Monitors accelerometer from iio on 100ms intervals
*Sets hasSuddenImpact flag when accelerometer surpasses threshold
*/
void *impactMonitor(void * ptr){
  double x, y, z;
  le_result_t r = LE_OK;
  for(;;){
    brnkl_motion_getCurrentAcceleration(&x, &y, &z);

    double impactMagnitude = sqrt(x*x + y*y + z*z);

    if(impactMagnitude > impactThreshold){
      //3. add x, y, z to impact array
      pthread_mutex_lock(&impactMutex);
      r = recordImpact(&x, &y, &z);
      pthread_mutex_unlock(&impactMutex);
      }
    if(r != LE_OK)
      LE_ERROR("Impact Not Recorded");

    usleep(SAMPLE_PERIOD_MS*1000);

  }
  return ptr;
}

/*
*Create thread to monitor accelerometer iio
*/
void initThread(){
  int thread, mutx;
  LE_INFO("initThread called");
  mutx   = pthread_mutex_init(&impactMutex, NULL);
  thread = pthread_create( &impactThread, NULL, impactMonitor, NULL);
  LE_INFO("mutexResult: %d", mutx);
  if(thread || mutx){
    LE_ERROR("Reader Thread or Mutex Creation Failed");
  }else{
    LE_INFO("Reader Thread Created");
  }
}

COMPONENT_INIT {
  initThread();
}
