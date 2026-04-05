#pragma once

#include <Arduino.h>
#include "Romi32U4Motors.h"

class Chassis {
public:
  LeftMotor leftMotor;
  RightMotor rightMotor;

protected:
  const float cmPerEncoderTick;
  const float robotRadius;
  const uint16_t ctrlIntervalMS = 16;

public:
  Chassis(float wheelDiam = 7, float ticksPerRevolution = 1440, float wheelTrack = 14.7)
    : cmPerEncoderTick(wheelDiam * M_PI / ticksPerRevolution), robotRadius(wheelTrack / 2.0) {}

  void init(void);

  void setMotorPIDcoeffs(float kp, float ki);

  void idle(void);

  void setMotorEfforts(int leftEffort, int rightEffort);

  void setWheelSpeeds(float leftSpeed, float rightSpeed);

  void setTwist(float forwardSpeed, float turningSpeed);

  void driveFor(float forwardDistance, float forwardSpeed, bool block = true);

  void driveWithTime(float forwardDistance, float targetSeconds);

  void turnFor(float turnAngle, float turningSpeed, bool block = true);

  void turnTo(float turnAngle, float targetSeconds, bool block = true);

  void turnWithTimePosPid(int targetCount, float targetSeconds);

  bool checkMotionComplete(void);

  void printSpeeds(void);
  void printEncoderCounts(void);

  inline void updateEncoderDeltas();

  long getLeftEncoderCount(bool reset = false) {
    return reset ? leftMotor.getAndResetCount() : leftMotor.getCount();
  }

  long getRightEncoderCount(bool reset = false) {
    return reset ? rightMotor.getAndResetCount() : rightMotor.getCount();
  }
};

extern Chassis chassis;
