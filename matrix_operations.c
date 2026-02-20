#include "matrix_operations.h"
#include <string.h>
#include <errno.h>

/* ---------------- Row operations ---------------- */
void swap_rows(int r, int c, double M[r][c], int i, int j) {
    if (i == j) return;
    for (int n = 0; n < c; n++) {
        double tmp = M[i][n]; M[i][n] = M[j][n]; M[j][n] = tmp;
    }
}

void scale_row(int r, int c, double M[r][c], double k, int row) {
    for (int n = 0; n < c; n++)
        M[row][n] *= k;
}

void add_row(int r, int c, double M[r][c], double k, int src, int dest) {
    for (int n = 0; n < c; n++)
        M[dest][n] += k * M[src][n];
}

void clean_matrix(int r, int c, double M[r][c]) {
    for (int i = 0; i < r; i++)
        for (int j = 0; j < c; j++)
            if (fabs(M[i][j]) < EPS)
                M[i][j] = 0.0;
}

void copy_matrix(int r, int c, double M[r][c], double prev[r][c]) {
    for (int i = 0; i < r; i++)
        for (int j = 0; j < c; j++)
            prev[i][j] = M[i][j];
}

int matrix_changed(int r, int c, double M[r][c], double prev[r][c]) {
    for (int i = 0; i < r; i++)
        for (int j = 0; j < c; j++)
            if (fabs(M[i][j] - prev[i][j]) > EPS)
                return 1;
    return 0;
}

/* ---------------- Record matrix steps ---------------- */
void record_step(AppData *app, int r, int c, double M[r][c]) {
    app->step_list.steps = realloc(app->step_list.steps,
                                   (app->step_list.count + 1) * sizeof(MatrixStep));
    MatrixStep *step = &app->step_list.steps[app->step_list.count];
    step->rows = r;
    step->cols = c;
    step->matrix = malloc(r * sizeof(double*));
    for (int i = 0; i < r; i++) {
        step->matrix[i] = malloc(c * sizeof(double));
        for (int j = 0; j < c; j++)
            step->matrix[i][j] = M[i][j];
    }
    step->above_arrow = NULL;
    app->step_list.count++;
}

void record_arrow(AppData *app, const char *text) {
    app->step_list.steps = realloc(app->step_list.steps,
                                   (app->step_list.count + 1) * sizeof(MatrixStep));
    MatrixStep *step = &app->step_list.steps[app->step_list.count];
    step->matrix = NULL;
    step->rows = 0;
    step->cols = 0;
    step->above_arrow = text ? strdup(text) : NULL;
    app->step_list.count++;
}

/* ---------------- Utilities ---------------- */
int gcd(int a, int b) {
    a = abs(a); b = abs(b);
    while (b != 0) { int t = b; b = a % b; a = t; }
    return a;
}

void format_fraction(double value, char *buffer, size_t size) {
    int sign = value < 0 ? -1 : 1;
    value = fabs(value);

    if (value < 1e-16) {  // treat extremely tiny numbers as nonzero decimals
        snprintf(buffer, size, "%.3g", sign * value); // 3 significant digits
        return;
    }

    int best_num = 0, best_den = 1;
    double best_err = 1e9;

    for (int den = 1; den <= MAX_DEN; den++) {
        int num = (int)round(value * den);
        if (num == 0) continue; // skip denominator that makes zero
        double approx = (double)num / den;
        double err = fabs(value - approx);
        if (err < best_err) {
            best_err = err;
            best_num = num;
            best_den = den;
        }
        if (err < 1e-16) break;
    }

    // fallback: tiny number couldn't be represented as fraction
    if (best_num == 0) {
        snprintf(buffer, size, "%.3g", sign * value);
        return;
    }

    int g = gcd(best_num, best_den);
    best_num /= g; best_den /= g;
    best_num *= sign;

    if (best_den == 1) snprintf(buffer, size, "%d", best_num);
    else snprintf(buffer, size, "%d/%d", best_num, best_den);
}

void format_for_step(double value, char *buffer, size_t size) {
    format_fraction(value, buffer, size);
}
