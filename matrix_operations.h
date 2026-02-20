#ifndef MATRIX_OPERATIONS_H_INCLUDED
#define MATRIX_OPERATIONS_H_INCLUDED

#include "gui.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define FRACTION_TOL 1e-100
#define MAX_DEN 1000   // max denominator size
#define EPS 1e-12

void swap_rows(int r, int c, double M[r][c], int i, int j);
void scale_row(int r, int c, double M[r][c], double k, int row);
void add_row(int r, int c, double M[r][c], double k, int src, int dest);
void print_matrix(int r, int c, double M[r][c]);
void clean_matrix(int r, int c, double M[r][c]);
void copy_matrix(int rows, int cols, double M[rows][cols], double prev[rows][cols]);
int matrix_changed(int rows, int cols, double M[rows][cols], double prev[rows][cols]);
void record_step(AppData *app, int rows, int cols, double M[rows][cols]);
void record_arrow(AppData *app, const char *text);
int gcd(int a, int b);
void format_fraction(double value, char *buffer, size_t size);
void format_for_step(double value, char *buffer, size_t size);

#endif // MATRIX_OPERATIONS_H_INCLUDED
