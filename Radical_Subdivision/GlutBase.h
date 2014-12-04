#pragma once
#ifndef GLUT_BASE_H
#define GLUT_BASE_H

#include <GL/glut.h>
#include "RadicalLOD.h"
#include "LoopLOD.h"
#include "RingLOD.h"

void initGL();

void display();

void reshape(int w,int h);

void processSpecialKeys(int key, int x, int y);

void keyboard(unsigned char key, int x, int y);

void mouse(int btn, int state, int x, int y);

void mousemove(int x, int y);

extern LODBase* g_LOD;

#endif