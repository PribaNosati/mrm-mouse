#include "common.h"


uint8_t errorCode;


float constrain(float value, float minimum, float maximum){
	if (value < minimum)
		return minimum;
	else if (value > maximum)
		return maximum;
	else
		return value;
}

uint8_t error(){
	return errorCode > WARNING_LIMIT;
}

float map(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
