#include "interfaces.h"
#include "legato.h"
#include "util.h"

#define N_CHANGE_BLOCKS 200
#define DEFAULT_THRESHOLD_MS2 17
// only used if -DMOTION_MONITOR_USE_THREAD is set
#define SAMPLE_PERIOD_MS 100

static const char FormatStr[] = "/sys/bus/iio/devices/iio:device0/in_%s_%s";
static const char AccType[] = "accel";
static const char GyroType[] = "anglvel";
static const char CompX[] = "x_raw";
static const char CompY[] = "y_raw";
static const char CompZ[] = "z_raw";
static const char CompScale[] = "scale";

struct suddenImpacts_t {
  int nValues;
  double threshold;
  double x[N_CHANGE_BLOCKS];
  double y[N_CHANGE_BLOCKS];
  double z[N_CHANGE_BLOCKS];
  uint64_t timestamps[N_CHANGE_BLOCKS];
#ifdef MOTION_MONITOR_USE_THREAD
  pthread_mutex_t lock;
#endif
} impacts = {0, DEFAULT_THRESHOLD_MS2};

/*
 * Reports the x, y and z accelerometer readings in meters per second squared.
 */
le_result_t brnkl_motion_getCurrentAcceleration(double* xAcc,
                                                double* yAcc,
                                                double* zAcc) {
  le_result_t r;
  char path[256];

  double scaling = 0.0;
  snprintf(path, sizeof(path), FormatStr, AccType, CompScale);
  r = ioutil_readDoubleFromFile(path, &scaling);
  if (r != LE_OK) {
    goto done;
  }

  snprintf(path, sizeof(path), FormatStr, AccType, CompX);
  r = ioutil_readDoubleFromFile(path, xAcc);
  if (r != LE_OK) {
    goto done;
  }
  *xAcc *= scaling;

  snprintf(path, sizeof(path), FormatStr, AccType, CompY);
  r = ioutil_readDoubleFromFile(path, yAcc);
  if (r != LE_OK) {
    goto done;
  }
  *yAcc *= scaling;

  snprintf(path, sizeof(path), FormatStr, AccType, CompZ);
  r = ioutil_readDoubleFromFile(path, zAcc);
  *zAcc *= scaling;

done:
  return r;
}

le_result_t recordImpact(struct suddenImpacts_t* it,
                         double xAcc,
                         double yAcc,
                         double zAcc) {
  LE_INFO("Recording impact...");
  if (it->nValues > N_CHANGE_BLOCKS || it->nValues > N_CHANGE_BLOCKS ||
      it->nValues > N_CHANGE_BLOCKS)
    return LE_OUT_OF_RANGE;

  it->timestamps[it->nValues] = GetCurrentTimestamp();
  it->x[it->nValues] = xAcc;
  it->y[it->nValues] = yAcc;
  it->z[it->nValues] = zAcc;
  it->nValues++;

  return LE_OK;
}

int8_t brnkl_motion_hasSuddenImpact() {
  return impacts.nValues > 0;
}

le_result_t brnkl_motion_getSuddenImpact(double* xAcc,
                                         size_t* xSize,
                                         double* yAcc,
                                         size_t* ySize,
                                         double* zAcc,
                                         size_t* zSize,
                                         uint64_t* timestampssOut,
                                         size_t* timeSize) {
  if (!impacts.nValues)
    LE_INFO("No Sudden Impacts to Report");
  else {
#ifdef MOTION_MONITOR_USE_THREAD
    pthread_mutex_lock(&impacts.lock);
#endif
    if (impacts.nValues > *xSize || impacts.nValues > *ySize ||
        impacts.nValues > *zSize)
      return LE_OUT_OF_RANGE;

    for (int i = 0; i < impacts.nValues; i++) {
      xAcc[i] = impacts.x[i];
      yAcc[i] = impacts.y[i];
      zAcc[i] = impacts.y[i];
      timestampssOut[i] = impacts.timestamps[i];
    }

    *xSize = *ySize = *zSize = impacts.nValues;

    impacts.nValues = 0;
#ifdef MOTION_MONITOR_USE_THREAD
    pthread_mutex_unlock(&impacts.lock);
#endif
  }

  return LE_OK;
}

#ifdef MOTION_MONITOR_USE_THREAD

/*
 * Monitors accelerometer from iio on 100ms intervals
 *
 * We use a context pointer here such that this routine
 * is not coupled to the global scope. This allows us to pass in
 * a pointer to a struct that contains the data we care about
 * instead of storing it globally. Ultimately, the struct we point to
 * will likely be in the global scope, but this is still a good practice.
 */
void* impactMonitor(void* ctx) {
  double x, y, z;
  le_result_t r = LE_OK;
  struct suddenImpacts_t* it = ctx;
  for (;;) {
    brnkl_motion_getCurrentAcceleration(&x, &y, &z);

    double impactMagnitude = sqrt(x * x + y * y + z * z);

    if (impactMagnitude > it->threshold) {
      // 3. add x, y, z to impact array
      pthread_mutex_lock(&it->lock);
      r = recordImpact(it, x, y, z);
      pthread_mutex_unlock(&it->lock);
    }
    if (r != LE_OK)
      LE_ERROR("Impact Not Recorded");

    usleep(SAMPLE_PERIOD_MS * 1000);
  }
  // should never get here
  return NULL;
}

/*
 *Create thread to monitor accelerometer iio
 */
void initThread() {
  pthread_t impactThread;
  int thread, mutx;
  mutx = pthread_mutex_init(&impacts.lock, NULL);
  thread = pthread_create(&impactThread, NULL, impactMonitor, &impacts);
  if (thread || mutx) {
    LE_ERROR("Reader Thread or Mutex Creation Failed");
  } else {
    LE_INFO("Reader Thread Created");
  }
}

COMPONENT_INIT {
  initThread();
}

#else

void interruptHandler(bool val, void* ctx) {
  double x, y, z;
  le_result_t r = LE_OK;
  struct suddenImpacts_t* it = ctx;
  if (ctx == NULL) {
    LE_ERROR("No context passed");
  } else {
    brnkl_motion_getCurrentAcceleration(&x, &y, &z);

    double impactMagnitude = sqrt(x * x + y * y + z * z);

    if (impactMagnitude > it->threshold) {
      // 3. add x, y, z to impact array
      r = recordImpact(it, x, y, z);
    }
    if (r != LE_OK)
      LE_ERROR("Impact Not Recorded");
  }
}

void initGpio() {
  interrupt_SetInput(INTERRUPT_ACTIVE_HIGH);
  interrupt_AddChangeEventHandler(INTERRUPT_EDGE_RISING, interruptHandler,
                                  &impacts, 0);
}

COMPONENT_INIT {
  initGpio();
}

#endif
