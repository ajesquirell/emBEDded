#ifndef MPUHANDLER_H
#define MPUHANDLER_H

#define M_PI		3.14159265358979323846 // Because the incorrect errors are annoying

#include "MPU6050_6Axis_MotionApps_V6_12_INLINED.h"
//#include "MPU6050_6Axis_MotionApps_V6_12_OOP.h"


class MpuHandler {
    public:
    //MpuHandler();

    float* GetYprData();
    void Update(); // To be called each loop to update
    void Init();

    private:

    // class default I2C address is 0x68
    MPU6050 mpu; // Could make this pointer and allocate it with 'new' in constructor with specific I2C address if I want to use the 0x69 address in the future (Add destructor too)
    //MPU6050 mpu(0x69); // <-- use for AD0 high


    // MPU control/status vars
    bool dmpReady = false;  // set true if DMP init was successful
    uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
    uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
    uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
    uint16_t fifoCount;     // count of all bytes currently in FIFO
    uint8_t fifoBuffer[64]; // FIFO storage buffer

    // orientation/motion vars
    Quaternion q;           // [w, x, y, z]         quaternion container
    VectorInt16 aa;         // [x, y, z]            accel sensor measurements
    VectorInt16 gy;         // [x, y, z]            gyro sensor measurements
    VectorInt16 aaReal;     // [x, y, z]            gravity-free accel sensor measurements
    VectorInt16 aaWorld;    // [x, y, z]            world-frame accel sensor measurements
    VectorFloat gravity;    // [x, y, z]            gravity vector
    float euler[3];         // [psi, theta, phi]    Euler angle container
    float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector
};






#endif