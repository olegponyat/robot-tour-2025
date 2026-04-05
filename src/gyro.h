#ifndef GYRO_H
#define GYRO_H

#include <Arduino.h>
#include <Wire.h>
#define LSM6DS33_ADDRESS 0x6B
#define CTRL1_XL 0x10
#define CTRL2_G 0x11
#define OUTZ_L_G 0x26
#define OUTZ_H_G 0x27

class Gyro {
public:
    Gyro();
    bool init();
    void calibrate(unsigned int samples = 1000);
    void update();
    float getAngle() { return angle; }
    void reset();

private:
    void readGyro();
    
    float gyroZ;

    float Q_angle;
    float Q_bias;
    float R_measure;
    float angle;
    float bias;
    float rate;

    float P[2][2];

    unsigned long timer;
};

#endif
