#include "PIDcontroller.h"
#include <math.h>

float PIDController::calcEffort(float error)
{
    currError = error;
    sumError += currError;

    if(errorBound > 0)
    {
        if(fabs(sumError) > errorBound) 
        {
            sumError -= currError;
        }
    }

    float derivError = currError - prevError;
    prevError = currError;

    currEffort = Kp * currError + Ki * sumError + Kd * derivError;

    return currEffort;
}
