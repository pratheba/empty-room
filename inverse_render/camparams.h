#ifndef _CAM_PARAMS_H
#define _CAM_PARAMS_H
#include "R3Shapes/R3Shapes.h"

struct CameraParams {
    R3Point pos;
    R3Vector up;
    R3Vector towards;
    R3Vector right;

    int width;
    int height;
    double focal_length;
    double fov;
};

#endif
