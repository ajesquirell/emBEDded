// Handler for MPU6050 class using DMP (MotionApps v6.12)
// Code from the MPU6050 I2C device class by Jeff Rowberg

#include "MpuHandler.h"
#include "MpuDefinitions.h"

#include "I2Cdev.h"

// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
// is used in I2Cdev.h
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
#include "Wire.h"
#endif



void MpuHandler::Init()
{
  // ================================================================
  // ===                      INITIAL SETUP                       ===
  // ================================================================

  // initialize serial communication if not already
  if (!Serial)
    Serial.begin(115200);
  while (!Serial);

  // join I2C bus (For MPU6050 - I2Cdev library doesn't do this automatically)
  #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    Wire.begin();
    Wire.setClock(400000); // 400kHz I2C clock. Comment this line if having compilation difficulties
  #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
    Fastwire::setup(400, true);
  #endif

  // initialize device
  Serial.println(F("Initializing I2C devices..."));
  mpu.initialize();

  // verify connection
  Serial.println(F("Testing device connections..."));
  Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));

  // load and configure the DMP
  Serial.println(F("Initializing DMP..."));
  devStatus = mpu.dmpInitialize();

  // supply your own gyro offsets here, scaled for min sensitivity
  // For our purposes, it doesn't really matter what these are, as long as they don't change
  mpu.setXAccelOffset(-428);
  mpu.setYAccelOffset(2631);
  mpu.setZAccelOffset(1080);
  mpu.setXGyroOffset(22);
  mpu.setYGyroOffset(24);
  mpu.setZGyroOffset(60);
  // make sure it worked (returns 0 if so)
  if (devStatus == 0) {
    // Calibration Time: generate offsets and calibrate our MPU6050
    //mpu.CalibrateAccel(6);
    //mpu.CalibrateGyro(6);
    Serial.println();
    mpu.PrintActiveOffsets();

    // turn on the DMP, now that it's ready
    Serial.println(F("Enabling DMP..."));
    mpu.setDMPEnabled(true);

    // set our DMP Ready flag so the main loop() function knows it's okay to use it
    Serial.println(F("DMP ready! Waiting for first interrupt..."));
    dmpReady = true;

    // get expected DMP packet size for later comparison
    packetSize = mpu.dmpGetFIFOPacketSize();
  } else {
    // ERROR!
    // 1 = initial memory load failed
    // 2 = DMP configuration updates failed
    // (if it's going to break, usually the code will be 1)
    Serial.print(F("DMP Initialization failed (code "));
    Serial.print(devStatus);
    Serial.println(F(")"));
  }

  
}

const float* MpuHandler::GetYprData()
{
  mpu.dmpGetQuaternion(&q, fifoBuffer);
  mpu.dmpGetGravity(&gravity, &q);
  mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
  ypr[0] = ypr[0] * 180 / M_PI;
  ypr[1] = ypr[1] * 180 / M_PI;
  ypr[2] = ypr[2] * 180 / M_PI;
  return ypr;
}

void MpuHandler::Update() 
{
  // Simply read new data into fifo, and output if desired

  // if programming failed, don't try to do anything
  if (!dmpReady) return;
  // read a packet from FIFO
  if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) { // Get the Latest packet 

      #ifdef OUTPUT_READABLE_QUATERNION
          // display quaternion values in easy matrix form: w x y z
          mpu.dmpGetQuaternion(&q, fifoBuffer);
          Serial.print("quat\t");
          Serial.print(q.w);
          Serial.print("\t");
          Serial.print(q.x);
          Serial.print("\t");
          Serial.print(q.y);
          Serial.print("\t");
          Serial.println(q.z);
      #endif

      #ifdef OUTPUT_READABLE_EULER
          // display Euler angles in degrees
          mpu.dmpGetQuaternion(&q, fifoBuffer);
          mpu.dmpGetEuler(euler, &q);
          Serial.print("euler\t");
          Serial.print(euler[0] * 180 / M_PI);
          Serial.print("\t");
          Serial.print(euler[1] * 180 / M_PI);
          Serial.print("\t");
          Serial.println(euler[2] * 180 / M_PI);
      #endif

      #ifdef OUTPUT_READABLE_YAWPITCHROLL
          // display Euler angles in degrees
          mpu.dmpGetQuaternion(&q, fifoBuffer);
          mpu.dmpGetGravity(&gravity, &q);
          mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
          Serial.print("ypr\t");
          Serial.print(ypr[0] * 180 / M_PI);
          Serial.print("\t");
          Serial.print(ypr[1] * 180 / M_PI);
          Serial.print("\t");
          Serial.print(ypr[2] * 180 / M_PI);
          /*
          mpu.dmpGetAccel(&aa, fifoBuffer);
          Serial.print("\tRaw Accl XYZ\t");
          Serial.print(aa.x);
          Serial.print("\t");
          Serial.print(aa.y);
          Serial.print("\t");
          Serial.print(aa.z);
          mpu.dmpGetGyro(&gy, fifoBuffer);
          Serial.print("\tRaw Gyro XYZ\t");
          Serial.print(gy.x);
          Serial.print("\t");
          Serial.print(gy.y);
          Serial.print("\t");
          Serial.print(gy.z);
          */
          Serial.println();

      #endif

      #ifdef OUTPUT_READABLE_REALACCEL
          // display real acceleration, adjusted to remove gravity
          mpu.dmpGetQuaternion(&q, fifoBuffer);
          mpu.dmpGetAccel(&aa, fifoBuffer);
          mpu.dmpGetGravity(&gravity, &q);
          mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
          Serial.print("areal\t");
          Serial.print(aaReal.x);
          Serial.print("\t");
          Serial.print(aaReal.y);
          Serial.print("\t");
          Serial.println(aaReal.z);
      #endif

      #ifdef OUTPUT_READABLE_WORLDACCEL
          // display initial world-frame acceleration, adjusted to remove gravity
          // and rotated based on known orientation from quaternion
          mpu.dmpGetQuaternion(&q, fifoBuffer);
          mpu.dmpGetAccel(&aa, fifoBuffer);
          mpu.dmpGetGravity(&gravity, &q);
          mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
          mpu.dmpGetLinearAccelInWorld(&aaWorld, &aaReal, &q);
          Serial.print("aworld\t");
          Serial.print(aaWorld.x);
          Serial.print("\t");
          Serial.print(aaWorld.y);
          Serial.print("\t");
          Serial.println(aaWorld.z);
      #endif

      #ifdef OUTPUT_TEAPOT
          // display quaternion values in InvenSense Teapot demo format:
          teapotPacket[2] = fifoBuffer[0];
          teapotPacket[3] = fifoBuffer[1];
          teapotPacket[4] = fifoBuffer[4];
          teapotPacket[5] = fifoBuffer[5];
          teapotPacket[6] = fifoBuffer[8];
          teapotPacket[7] = fifoBuffer[9];
          teapotPacket[8] = fifoBuffer[12];
          teapotPacket[9] = fifoBuffer[13];
          Serial.write(teapotPacket, 14);
          teapotPacket[11]++; // packetCount, loops at 0xFF on purpose
      #endif
  }
}