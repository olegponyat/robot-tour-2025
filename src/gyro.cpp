#include "gyro.h"

Gyro::Gyro() { 
    Q_angle = 0.0001;
    Q_bias = 0.01;
    R_measure = 0.5;

    angle = 0;
    bias = 0;

    P[0][0] = 0;
    P[0][1] = 0;
    P[1][0] = 0;
    P[1][1] = 0;

    timer = micros();
}

bool Gyro::init() {
    Wire.begin();
    
    Wire.beginTransmission(LSM6DS33_ADDRESS);
    if (Wire.endTransmission() != 0) {
        return false;
    }

    Wire.beginTransmission(LSM6DS33_ADDRESS);
    Wire.write(CTRL2_G);
    Wire.write(0b01001000);
    Wire.endTransmission();

    return true;
}

void Gyro::readGyro() {
    Wire.beginTransmission(LSM6DS33_ADDRESS);
    Wire.write(OUTZ_L_G);
    Wire.endTransmission();

    Wire.requestFrom(LSM6DS33_ADDRESS, 2);
    if (Wire.available() >= 2) {
        uint8_t z_l = Wire.read();
        uint8_t z_h = Wire.read();
        int16_t z = (int16_t)((z_h << 8) | z_l);
        gyroZ = z * 42.5 / 1000.0;
    } else {
        gyroZ = 0;
    }
}

void Gyro::update() {
    readGyro();

    unsigned long now = micros();
    float dt = (now - timer) / 1000000.0;
    timer = now;

    rate = gyroZ - bias;
    angle += dt * rate;

    P[0][0] += dt * (dt * P[1][1] - P[0][1] - P[1][0] + Q_angle);
    P[0][1] -= dt * P[1][1];
    P[1][0] -= dt * P[1][1];
    P[1][1] += Q_bias * dt;

}

void Gyro::reset() {
    angle = 0;
    bias = 0;
}
