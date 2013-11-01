#ifndef GLOB_h
#define GLOB_h

#include "RPiAPMCopterDefs.h"


// ArduPilot Hardware Abstraction Layer
const AP_HAL::HAL& hal = AP_HAL_AVR_APM2;
// MPU6050 accel/gyro chip
AP_InertialSensor_MPU6000 inertial;

// Magnetometer aka compass
AP_Compass_HMC5843 compass;
bool COMPASS_INITIALIZED = 0;

// Motor control 
// PID array (6 pids, two for each axis)
PID PIDS[6];

// Gyrometer calibration
bool  GYRO_CALI         = 1;
float GYRO_ROL_DRIFT    = 0;
float GYRO_PIT_DRIFT    = 0;
float GYRO_YAW_DRIFT    = 0;

float GYRO_ROL_OFFS     = 0;
float GYRO_PIT_OFFS     = 0;
float GYRO_YAW_OFFS     = 0;

// Remote control
uint32_t RC_PACKET_T = 0;
int16_t  RC_CHANNELS[APM_IOCHANNEL_COUNT] = { 0,0,0,0,0,0,0,0 };

#endif