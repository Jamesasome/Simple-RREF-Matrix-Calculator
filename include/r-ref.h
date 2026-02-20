#ifndef R_REF_H
#define R_REF_H

#include "gui.h"   // AppData typedef
#include <stdio.h>
#include <math.h>

#define EPS 1e-12

// REF/RREF
void ref(AppData *app, int rows, int cols, double M[rows][cols]);
void rref(AppData *app, int rows, int cols, double M[rows][cols]);

#endif

