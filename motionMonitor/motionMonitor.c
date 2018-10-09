#include "interfaces.h"
#include "legato.h"
#include "util.h"

static const char FormatStr[] =
    "/sys/bus/i2c/devices/3-0068/iio:device0/in_%s_%s";
static const char AccType[] = "accel";
static const char GyroType[] = "anglvel";
static const char CompX[] = "x_raw";
static const char CompY[] = "y_raw";
static const char CompZ[] = "z_raw";
static const char CompScale[] = "scale";

typedef struct {
  double x;
  double y;
  double z;
} Acceleration;

bool hasSuddenImpact = false;
Acceleration suddenImpact = {0, 0, 0};

/**
 * Reports the x, y and z accelerometer readings in meters per second squared.
 */
le_result_t brnkl_motion_getCurrentAcceleration(double* xAcc,
                                                double* yAcc,
                                                double* zAcc) {
  le_result_t r;
  char path[256];

  double scaling = 0.0;
  int pathLen = snprintf(path, sizeof(path), FormatStr, AccType, CompScale);
  LE_ASSERT(pathLen < sizeof(path));
  r = ioutil_readDoubleFromFile(path, &scaling);
  if (r != LE_OK) {
    goto done;
  }

  pathLen = snprintf(path, sizeof(path), FormatStr, AccType, CompX);
  LE_ASSERT(pathLen < sizeof(path));
  r = ioutil_readDoubleFromFile(path, xAcc);
  if (r != LE_OK) {
    goto done;
  }
  *xAcc *= scaling;

  pathLen = snprintf(path, sizeof(path), FormatStr, AccType, CompY);
  LE_ASSERT(pathLen < sizeof(path));
  r = ioutil_readDoubleFromFile(path, yAcc);
  if (r != LE_OK) {
    goto done;
  }
  *yAcc *= scaling;

  pathLen = snprintf(path, sizeof(path), FormatStr, AccType, CompZ);
  LE_ASSERT(pathLen < sizeof(path));
  r = ioutil_readDoubleFromFile(path, zAcc);
  *zAcc *= scaling;

done:
  LE_INFO("Showing accel: x: %f, y: %f, z: %f", *xAcc, *yAcc, *zAcc);
  return r;
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

int8_t brnkl_motion_hasSuddenImpact() {
  return hasSuddenImpact;
}

// take a reading but make sure its "larger" than
// whatever is already in the buffer
void interruptChangeHandler(bool state, void* ctx) {
  double x, y, z;
  le_result_t r = brnkl_motion_getCurrentAcceleration(&x, &y, &z);
  LE_INFO("TRIGGERED x:%f y:%f z:%f", x, y, z);
  if (r == LE_OK) {
    bool updateX = fabs(x) > fabs(suddenImpact.x);
    bool updateY = fabs(y) > fabs(suddenImpact.y);
    bool updateZ = fabs(z) > fabs(suddenImpact.z);
    if (updateX) {
      suddenImpact.x = x;
    }
    if (updateY) {
      suddenImpact.y = y;
    }
    if (updateZ) {
      suddenImpact.z = z;
    }
    if (updateX || updateY || updateZ) {
      hasSuddenImpact = true;
    }
  }
}

void initGpio() {
  interrupt_SetInput(INTERRUPT_ACTIVE_HIGH);
  interrupt_AddChangeEventHandler(INTERRUPT_EDGE_RISING, interruptChangeHandler,
                                  NULL, 0);
}

COMPONENT_INIT {
  initGpio();
}
