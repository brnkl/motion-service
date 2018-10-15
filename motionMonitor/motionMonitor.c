#include "interfaces.h"
#include "legato.h"
#include "util.h"
#include <pthread.h>

void *impactMonitor(void *);

static const char FormatStr[] = "/sys/devices/i2c-0/0-0068/iio:device0/in_%s_%s";
static const char AccType[]   = "accel";
static const char GyroType[]  = "anglvel";
static const char CompX[]     = "x_raw";
static const char CompY[]     = "y_raw";
static const char CompZ[]     = "z_raw";
static const char CompScale[] = "scale";
bool hasSuddenImpact          = false;
static const int  impactThreshold = 20;

typedef struct {
  double x;
  double y;
  double z;
} Acceleration;

pthread_t impact_thread;

Acceleration suddenImpact = {0, 0, 0};

/**
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
    if(hasSuddenImpact){
      LE_INFO("hasSuddenImpact");
      hasSuddenImpact = false;
    }

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

/*
*return 1 if hardware has been in a sudden impact.
*/
int8_t brnkl_motion_hasSuddenImpact() {
  if(hasSuddenImpact){
    hasSuddenImpact = false;
    return 1;
  }
  return 0;
}

le_result_t brnkl_motion_getSuddenImpact(double* xAcc,
                                         double* yAcc,
                                         double* zAcc) {
  *xAcc = suddenImpact.x;
  *yAcc = suddenImpact.y;
  *zAcc = suddenImpact.z;
  suddenImpact.x = 0;
  suddenImpact.y = 0;
  suddenImpact.z = 0;
  hasSuddenImpact = false;
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

    if(
      abs(x) + 
      abs(y) + 
      abs(z) > 
      impactThreshold
      )
      hasSuddenImpact = true;
    

    usleep(100*1000);

  }
  return ptr;
}

/*
*Create thread to monitor accelerometer iio
*/
void initThread(){
  int thread;

  thread = pthread_create( &impact_thread, NULL, impactMonitor, NULL);

  if(thread){
    LE_INFO("Reader Thread Created");
  }else{
    LE_ERROR("Reader Thread Creation Failed");
  }
}

COMPONENT_INIT {
  initThread();
}
