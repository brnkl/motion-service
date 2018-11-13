MangOH Motion Service
======
This application is designed to operate on a [MongOH Red](https://mangoh.io/mangoh-red-new) WP85 board. Using **general input output** this project uses pthreads to probe the onboard bmi160 chip intermittently in order to detect a significant motion on the board.

## Prerequisites

When cloning use ``git clone --recurse-submodule https://github.com/brnkl/motion-service.git`` 
to include the util module.


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

## Example 


