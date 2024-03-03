// ai.cpp
//

#include "../quakedef.h"
#include "game_local.h"

float anglemod(float v) {
	while (v >= 360)
		v -= 360;
	while (v < 0)
		v += 360;
	return v;
}
