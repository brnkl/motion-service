MangOH Motion Service
======
This application is designed to operate on a [MongOH Red](https://mangoh.io/mangoh-red-new) WP85 board. Using **general input output** this project uses pthreads to probe the onboard bmi160 chip intermittently in order to detect a significant motion on the board.

## Prerequisites

When cloning use ``git clone https://github.com/brnkl/motion-service.git``.


## Building
Compile the project using 

wp85: 	``make wp85``

## Setting Variables
To change the sensitivity of impact detection you must edit the `motionMonitor/motionMonitor.c` file.
The Accelerometer measures acceleration in 3 dimensions, X, Y, and Z. These dimensions of acceleration are recorded and the magnitude of their resulting vector is calculated using ``double impactMagnitude = sqrt(x * x + y * y + z * z);``. As visualized in the image below.

![Magnitude](https://www.intmath.com/vectors/img/235-3D-vector.png)


`#define DEFAULT_THRESHOLD_MS2 17` determines the magnitude of the resulting vector that will trigger the application to detect a sudden impact. 

If adjusting the value of `DEFAUTL_THRESHOLD_MS2` keep in mind that gravity implies a motionless magnitude of -9.8m/s^2.

## Bindings 
App.adef
```
...
bindings:
{
  myClientApp.myComponent.brnkl_motion -> motionService.brnkl_motion
}
...
```
Component.cdef
```
...
requires:
{
  api:
  {
    brnkl_motion.api
  }
}
...
```


## Examples
For getting a list of impact information you may follow this example.
```
double x_arr [array_size];
double y_arr [array_size];
double z_arr [array_size];
uint64 timestamps[array_size];

// Fills arrays with x, y, and z acceleration that have been recorded at each impact.

le_result_t result = getSuddenImpact(x_arr, y_arr, z_arr, timestamps);

for (int i = 0; i < array_size; i++)
    LE_INFO("X: %d - Y: %d - Z: %d - time: %ul", x_arr[i], 
                                                    y_arr[i], 
                                                    z_arr[i],
                                                    timestamps[i]);
```
For receiving the current acceleration only

```
double x_val;
double y_val;
double z_val;

le_result_t result = getCurrentAcceleration(x_val, y_val, z_val);

LE_INFO("Current Acceleration: X: %d - Y: %d - Z: %d");

```
