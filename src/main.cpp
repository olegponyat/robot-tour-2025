#include <Arduino.h>
#include "Chassis.h"
#include "Romi32U4Buttons.h"
#include "gyro.h"

Gyro gyro;

// Bottle state tracking
bool hasBottle = false;
// Speed multipliers when carrying bottle (75% forward, 40% turns)
const float BOTTLE_DRIVE_SPEED_MULTIPLIER = 1;
// Bottle friction didn't really affect the normal movements at all
const float BOTTLE_DRIVE_COUNT_MULTIPLIER = 1.014;
const float BOTTLE_TURN_SPEED_MULTIPLIER = 0.4;
// Bottle friction with the floor affected the turns slightly
const float BOTTLE_TURN_COUNT_MULTIPLIER = 1.032; // NB Home (Smoothed Wood Floor)
// const float BOTTLE_TURN_COUNT_MULTIPLIER = 1.06; // gym floor
// const float BOTTLE_TURN_COUNT_MULTIPLIER = 1.055; // Ferrigno Floor (Tiles)


// encoder count targets, tune by turning 16 times and changing numbers untill offset is 0
#define NIGHTY_LEFT_TURN_COUNT -718
#define NIGHTY_RIGHT_TURN_COUNT 713


// F and b go forward/backwards 50 cm by default, but other distances can be easily specified by adding a number after the letter
// S and E go the start/end distance, thought I dont really recommend using them
// L and R are left and right
// B picks up bottle (slows down movement for accuracy)
// N drops bottle (returns to normal speed)
// targetTime is target time 
// char moves[200] = "F25 R F F L F B L F R F F R F L F B L F F F F L F F L F L F F F R F B";

// USE THE FOLLOWING MOVESETS TO TEST CHASSIS EFFECTIVENESS
char moves[200] = "F32 L F R F B F R R F F L F L F F F L F F F45 L F F F L F N b b L F F F b b R F40";
// char moves[200] = "F9 F F F25 B F25 L F L F N b L F F F R F25 B F25 L L F R F N b b R F b L F R F F R F F25 B F25 F L F N b L F R F L F F F L F"; // boyceville
// char moves[200] = "F25 L F F R F F L F R B F L L F F F25 N b b L F B F L L F L F N b b L F B F R F L F L F N b b b F L F ";
// char moves[200] = "F32 B F F F L F R R F N b R B F F N b b R F L F F R F B F L F L F N b L F R F L F L B F R F N b R F L F b9";
// char moves[200] = "F32 L F F R F F L F R B F L L F F F N b b L F B F L L F L F N b b L F B F R F L F L F N b b b F L F";
// char moves[200] = "B R R R R R R R R R R R R R R R R";

// BOTTLE TEST
// char moves[200] = "F L L F R R F B F L F R N F";
// char moves[200] = "F L F L F L F L B";
double targetTime = 65;
double endDist = -9;
double startDist = -16;


// parameters are wheel diam, encoder counts, wheel track
// default values of 7, 1440, 14 can't go wrong
Chassis chassis(6.994936972, 1440, 14.0081);
//Chassis chassis(7, 1440, 14);

Romi32U4ButtonA buttonA;

enum ROBOT_STATE { ROBOT_IDLE,
                   ROBOT_MOVE,
                   MOVING };
ROBOT_STATE robotState = ROBOT_IDLE;

void idle(void) {
  Serial.println("idle()");
  chassis.idle();
  robotState = ROBOT_IDLE;
}

void setup() {
  Serial.begin(115200);
  chassis.init(); 
  gyro.init();
  idle();

  // Good numbers to change
  // PI controller where first number is P and second is I
  // Might not be a bad idea to go DOWN in P, since it seems to always go left
  chassis.setMotorPIDcoeffs(4.45, .3);
}

void turnLeft() {
  gyro.update();
  float angle = gyro.getAngle();
  float time = hasBottle ? 0.55 * BOTTLE_TURN_SPEED_MULTIPLIER : 0.55;
  chassis.turnTo(angle+85, time);
  delay(50);
}

void turnRight() {
  float speed = hasBottle ? 60 * BOTTLE_TURN_SPEED_MULTIPLIER : 60;
  chassis.turnFor(-89, speed);
  delay(50);
}

void left(float seconds) {
  float adjustedTime = hasBottle ? seconds / BOTTLE_TURN_SPEED_MULTIPLIER : seconds;
  float adjustedLeftTurn = hasBottle ? NIGHTY_LEFT_TURN_COUNT * BOTTLE_TURN_COUNT_MULTIPLIER : NIGHTY_LEFT_TURN_COUNT;
  chassis.turnWithTimePosPid(adjustedLeftTurn, adjustedTime);
  // gyro.reset();
  // gyro.update();
  // float angle = gyro.getAngle();
  // float time = hasBottle ? 0.55 / BOTTLE_TURN_SPEED_MULTIPLIER : 0.55;
  // float bottle_adder = hasBottle? 2.4 : 0;
  // chassis.turnTo(angle+87 + bottle_adder, time);
  // delay(50);
}

void right(float seconds) {
  float adjustedTime = hasBottle ? seconds / BOTTLE_TURN_SPEED_MULTIPLIER : seconds;
  float adjustedRightTurn = hasBottle ? NIGHTY_RIGHT_TURN_COUNT * BOTTLE_TURN_COUNT_MULTIPLIER : NIGHTY_RIGHT_TURN_COUNT;
  chassis.turnWithTimePosPid(adjustedRightTurn, adjustedTime);
//   gyro.reset();
//   gyro.update();
//   float angle = gyro.getAngle();
//   float time = hasBottle ? 0.55 / BOTTLE_TURN_SPEED_MULTIPLIER : 0.55;
//   float bottle_adder = hasBottle? 18.5 : 0;
//   chassis.turnTo(angle - 97.3 - bottle_adder, time);
//   delay(50);
}

void loop() {
  gyro.update();
  if (buttonA.getSingleDebouncedPress()) {
    delay(300);
    robotState = ROBOT_MOVE;
    Serial.println("Starting robot sequence...");
  }

  if (robotState == ROBOT_MOVE) {
    int count = 1; // count the number of moves (turns and straights)
    for (int i = 0; i < strlen(moves); i++) 
      if (isSpace(moves[i])) count++;

    // constucts *movesList, each element is pointer to the first character of a move string
    // i.e. if moves is "S R F100 B L E" then *movesList[2] is a pointer to "F" and moveslist[2] is "F100"
    char *movesList[count];
    char *ptr = NULL;

    byte index = 0;
    ptr = strtok(moves, " ");
    while (ptr != NULL) {
      movesList[index] = ptr;
      index++;
      ptr = strtok(NULL, " ");
    }

    int numTurns = 0;
    int numBottleTurns = 0;
    double totalDist = 0;
    char currentChar;
    String st;

    // count number of turns and total distance travelled
    for (int i = 0; i < count; i++) {
      currentChar = *movesList[i];
      st = movesList[i];
      if (currentChar == 'B') {
        hasBottle = true;
      }
      else if (currentChar == 'N') {
        hasBottle = false;
      }
      else if (currentChar == 'R' || currentChar == 'L') {
        if (hasBottle) {
          numBottleTurns++;
        }
        else {
          numTurns++;
        }
      }
      else if (currentChar == 'F' || currentChar == 'b') {   
        if (st.length() > 1) {
          totalDist += st.substring(1).toDouble();
        } else {
          totalDist += 49.83;
        }
      } else if (currentChar == 'S') {
        totalDist += abs(startDist);
      } else if (currentChar == 'E') {
        totalDist += abs(endDist);
      }
    }
    hasBottle = false; // reset bottle state for actual execution
    double turnTime = 0.55; // target time for a turn is 0.55 seconds
    double totalTurnTime = 0.65 * numTurns + 0.65 * numBottleTurns / BOTTLE_TURN_SPEED_MULTIPLIER; // but the code doesn't work so the actual time for a turn is 0.65 seconds
    double totalDriveTime = targetTime - totalTurnTime - 0.0049*totalDist; // this also always went over hence the 0.0029*totalDist
    double dist;

    // execute the moves 
    for (int i = 0; i < count; i++) {
      currentChar = *movesList[i];
      st = movesList[i];

      if (currentChar == 'B') {
        hasBottle = true;
      } else if (currentChar == 'N') {
        hasBottle = false;
      } else if (currentChar == 'R') {
        if (st.length() > 1) {
          float degrees = st.substring(1).toFloat();
          float adjustedTime = hasBottle ? turnTime / BOTTLE_TURN_SPEED_MULTIPLIER : turnTime;
          chassis.turnTo(-degrees, adjustedTime, true);
          delay(50);
        } else {
          right(turnTime);
          delay(50);
        }
      } else if (currentChar == 'L') {
        if (st.length() > 1) {
          float degrees = st.substring(1).toFloat();
          float adjustedTime = hasBottle ? turnTime / BOTTLE_TURN_SPEED_MULTIPLIER : turnTime;
          chassis.turnTo(degrees, adjustedTime, true);
          delay(50);
        } else {
          left(turnTime);
          delay(50);
        }
      } else if (currentChar == 'F' || currentChar == 'b') {
        if (st.length() > 1) {
          dist = st.substring(1).toDouble();
        } else {
          dist = 49.83;
        }

        
        float driveTimeForThisMove = dist/totalDist * totalDriveTime;
        if (hasBottle) {
          driveTimeForThisMove = driveTimeForThisMove / BOTTLE_DRIVE_SPEED_MULTIPLIER;
          dist = dist * BOTTLE_DRIVE_COUNT_MULTIPLIER;
        }
        
        if (currentChar == 'F') {
          chassis.driveWithTime(dist, driveTimeForThisMove);
        } else {
          chassis.driveWithTime(0-dist, driveTimeForThisMove);
        }
        Serial.print("Left encoder: ");
        Serial.print(chassis.getLeftEncoderCount());
        Serial.print("  Right encoder: ");
        Serial.println(chassis.getRightEncoderCount());
      } else if (currentChar == 'S') {
        float driveTimeForThisMove = abs(startDist)/totalDist * totalDriveTime;
        if (hasBottle) {
          driveTimeForThisMove = driveTimeForThisMove / BOTTLE_DRIVE_SPEED_MULTIPLIER;
        }
        chassis.driveWithTime(startDist, driveTimeForThisMove);
      } else if (currentChar == 'E') {
        float driveTimeForThisMove = abs(endDist)/totalDist * totalDriveTime;
        if (hasBottle) {
          driveTimeForThisMove = driveTimeForThisMove / BOTTLE_DRIVE_SPEED_MULTIPLIER;
        }
        chassis.driveWithTime(endDist, driveTimeForThisMove);
      }
    }
    
    hasBottle = false;
    idle();
  }
}
