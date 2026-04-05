#include "Chassis.h"
#include "gyro.h"

extern Gyro gyro;
float calculateSpeed(float forwardDistance, float targetSeconds, float elapsedSeconds, float minSpeed = 15) {
  float speedMultiplier = 1.05;
  if (forwardDistance < 0) speedMultiplier = -1.05;

  float percentComplete = elapsedSeconds / targetSeconds;
  float delta = (fabs(forwardDistance) - minSpeed * targetSeconds) / (0.6 * targetSeconds);
  float s = delta + minSpeed;

  if (percentComplete < 0.4) {
    s = delta * percentComplete / 0.4 + minSpeed;
  } else if (percentComplete > 0.6) {
    s = delta * (1 - percentComplete) / 0.4 + minSpeed;
  }

  return s * speedMultiplier;
}

int calculateIntermediateTargetLinear(int target, float finishSeconds, float elapsedSeconds) {
  if (elapsedSeconds >= finishSeconds) return target;
  return int(elapsedSeconds * (float(target) / finishSeconds));
}

void Chassis::init(void) {
  Romi32U4Motor::init();

  noInterrupts();

  TCCR4A = 0x00;
  TCCR4B = 0x0B;
  TCCR4C = 0x00;
  TCCR4D = 0x00;

  OCR4C = 249;

  TIMSK4 = 0x04;

  interrupts();
}

void Chassis::setMotorPIDcoeffs(float kp, float ki) {
  leftMotor.setSpeedPIDCoeffients(kp, ki);
  rightMotor.setSpeedPIDCoeffients(kp, ki);
}

void Chassis::idle(void) {
  setMotorEfforts(0, 0);
}

void Chassis::setMotorEfforts(int leftEffort, int rightEffort) {
  leftMotor.setMotorEffort(leftEffort);
  rightMotor.setMotorEffort(rightEffort);
}

void Chassis::setWheelSpeeds(float leftSpeed, float rightSpeed) {
  int16_t leftTicksPerInterval = (leftSpeed * (ctrlIntervalMS / 1000.0)) / cmPerEncoderTick;
  int16_t rightTicksPerInterval = (rightSpeed * (ctrlIntervalMS / 1000.0)) / cmPerEncoderTick;

  leftMotor.setTargetSpeed(leftTicksPerInterval);
  rightMotor.setTargetSpeed(rightTicksPerInterval);
}

void Chassis::setTwist(float forwardSpeed, float turningSpeed) {
  int16_t ticksPerIntervalFwd = (forwardSpeed * (ctrlIntervalMS / 1000.0)) / cmPerEncoderTick;
  int16_t ticksPerIntervalTurn = (robotRadius * 3.14 / 180.0) * (turningSpeed * (ctrlIntervalMS / 1000.0)) / cmPerEncoderTick;

  leftMotor.setTargetSpeed(ticksPerIntervalFwd - ticksPerIntervalTurn);
  rightMotor.setTargetSpeed(ticksPerIntervalFwd + ticksPerIntervalTurn);
}

void Chassis::driveFor(float forwardDistance, float forwardSpeed, bool block) {
  forwardSpeed = forwardDistance > 0 ? fabs(forwardSpeed) : -fabs(forwardSpeed);
  setTwist(forwardSpeed, 0);

  long delta = forwardDistance / cmPerEncoderTick;

  leftMotor.moveFor(delta);
  rightMotor.moveFor(delta);

  if (block) {
    while (!checkMotionComplete()) { delay(1); }
  }
}

void Chassis::driveWithTime(float forwardDistance, float targetSeconds) {
  gyro.reset();
  gyro.update();
  float initialangle = gyro.getAngle();
  unsigned long startTime = millis();
  float targetSeconds2 = targetSeconds - 0.02;
  float forwardSpeed = calculateSpeed(forwardDistance, targetSeconds2, 0);
  setTwist(forwardSpeed, 0);
  long delta = forwardDistance / cmPerEncoderTick;
  leftMotor.moveFor(delta);
  rightMotor.moveFor(delta);
  float adjustment = 0.45;
  if (forwardSpeed < 0) {
    adjustment = -adjustment;
  }
  while (true) {
    delay(1);
    if (checkMotionComplete()) break;
    float elapsedSeconds = (millis() - startTime) / 1000.0;
    forwardSpeed = calculateSpeed(forwardDistance, targetSeconds2, elapsedSeconds);
    int ticksPerIntervalFwd = (forwardSpeed * (ctrlIntervalMS / 1000.0)) / cmPerEncoderTick;
    if (gyro.getAngle() > initialangle + 0.05 and forwardSpeed > 0) {
      while (gyro.getAngle() > initialangle - 0.5) {
        leftMotor.targetSpeed = ticksPerIntervalFwd + 8.2;
        delay(5);
      gyro.update();
    }
      initialangle = gyro.getAngle();
      leftMotor.targetSpeed = ticksPerIntervalFwd;
    } else if (gyro.getAngle() < initialangle - 0.05 and forwardSpeed > 0) {
      while (gyro.getAngle() < initialangle + 0.5) {
        leftMotor.targetSpeed = ticksPerIntervalFwd - 8.2;
        delay(5);
        gyro.update();
      }
      initialangle = gyro.getAngle();
      leftMotor.targetSpeed = ticksPerIntervalFwd;
    } else if (gyro.getAngle() > initialangle + 0.05 and forwardSpeed < 0) {
      while (gyro.getAngle() > initialangle - 0.5) {
        leftMotor.targetSpeed = ticksPerIntervalFwd - 8.2;
        delay(5);
        gyro.update();
      }
      initialangle = gyro.getAngle();
      leftMotor.targetSpeed = ticksPerIntervalFwd;     
    } else if (gyro.getAngle() < initialangle - 0.05 and forwardSpeed < 0) {
      while (gyro.getAngle() < initialangle + 0.5) {
        leftMotor.targetSpeed = ticksPerIntervalFwd + 8.2;
        delay(5);
        gyro.update();
      }
      initialangle = gyro.getAngle();
      leftMotor.targetSpeed = ticksPerIntervalFwd;
    } else {
    leftMotor.targetSpeed = ticksPerIntervalFwd - adjustment;
    rightMotor.targetSpeed = ticksPerIntervalFwd;
  }
  delay(20);
  Serial.print("Current angle: ");
  Serial.println(gyro.getAngle());
  }

}

void Chassis::turnFor(float turnAngle, float turningSpeed, bool block) {
  turningSpeed = turnAngle > 0 ? fabs(turningSpeed) : -fabs(turningSpeed);
  setTwist(0, turningSpeed);

  long delta = turnAngle * (robotRadius * 3.14 / 180.0) / cmPerEncoderTick;

  leftMotor.moveFor(-delta);
  rightMotor.moveFor(delta);

  if (block) {
    while (!checkMotionComplete()) { delay(1); }
  }
}

void Chassis::turnTo(float turnAngle, float targetSeconds, bool block) {
    float turningSpeed = 1.545 * (90 / targetSeconds);
    turningSpeed = turnAngle > 0 ? fabs(turningSpeed) : -fabs(turningSpeed);
    setTwist(0, turningSpeed);

    if (block) {
        unsigned long startTime = millis();
        while (true) {
            gyro.update();
            Serial.print("Target: ");
            Serial.print(turnAngle);
            Serial.print(" | Current: ");
            Serial.println(gyro.getAngle());

            if ((millis() - startTime) / 1000.0 >= targetSeconds) {
                Serial.println("Turn timed out.");
                break;
            }
            if (turnAngle > 0) {
                if (gyro.getAngle() >= turnAngle) {
                    break;
                }
            } else {
                if (gyro.getAngle() <= turnAngle) {
                    break;
                }
            }
            // delay(5);
        }
        idle();
        Serial.print("Final Angle: ");
        Serial.println(gyro.getAngle());
    }
}

void Chassis::turnWithTimePosPid(int targetCount, float targetSeconds) {
  unsigned long startTime = millis();
  targetSeconds = targetSeconds;
  leftMotor.setTargetCount(0);
  rightMotor.setTargetCount(0);
  while (true) {
    delay(1);
    float elapsedSeconds = (millis() - startTime) / 1000.0;
    int thisTarget = calculateIntermediateTargetLinear(targetCount, targetSeconds - 0.1, elapsedSeconds);
    leftMotor.targetCount = thisTarget;
    rightMotor.targetCount = -thisTarget;
    if (elapsedSeconds > targetSeconds)
      break;
  }
  delay(100);
  setMotorEfforts(0, 0);
}

bool Chassis::checkMotionComplete(void) {
  bool complete = leftMotor.checkComplete() && rightMotor.checkComplete();
  return complete;
}

ISR(TIMER4_OVF_vect) {
  chassis.updateEncoderDeltas();
}

void Chassis::updateEncoderDeltas(void) {
  leftMotor.calcEncoderDelta();
  rightMotor.calcEncoderDelta();

  leftMotor.update();
  rightMotor.update();
}

void Chassis::printSpeeds(void) {
  Serial.print(leftMotor.speed);
  Serial.print('\t');
  Serial.print(rightMotor.speed);
  Serial.print('\n');
}

void Chassis::printEncoderCounts(void) {
  Serial.print(leftMotor.getCount());
  Serial.print('\t');
  Serial.print(rightMotor.getCount());
  Serial.print('\n');
}
