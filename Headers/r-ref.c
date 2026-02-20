#include "r-ref.h"
#include "matrix_operations.h"
#include <stdio.h>
#include <math.h>

/* ---------------- REF ---------------- */
void ref(AppData *app, int rows, int cols, double M[rows][cols]) {
    double prev[rows][cols];
    copy_matrix(rows, cols, M, prev);
    record_step(app, rows, cols, M);

    int r = 0;
    for (int c = 0; c < cols && r < rows; c++) {
        int pivot = r;
        double max_val = fabs(M[r][c]);
        for (int i = r+1; i<rows; i++)
            if (fabs(M[i][c]) > max_val) { max_val = fabs(M[i][c]); pivot = i; }

        if (max_val < EPS) continue;

        if (pivot != r) {
            char op[50]; sprintf(op,"R%d <-> R%d", r+1,pivot+1);
            swap_rows(rows, cols, M, r, pivot); clean_matrix(rows, cols, M);
            if (matrix_changed(rows, cols, M, prev)) {
                record_arrow(app, op); record_step(app, rows, cols, M);
                copy_matrix(rows, cols, M, prev);
            }
        }

        for (int i = r+1;i<rows;i++) {
            double factor = -M[i][c]/M[r][c];
            if (fabs(factor) < EPS) continue;
            char op[100], coeff[32];
            format_for_step(factor, coeff, sizeof(coeff));
            sprintf(op,"R%d -> R%d + (%s)R%d", i+1,i+1,coeff,r+1);
            add_row(rows, cols, M, factor, r, i); clean_matrix(rows, cols, M);
            if (matrix_changed(rows, cols, M, prev)) {
                record_arrow(app, op); record_step(app, rows, cols, M);
                copy_matrix(rows, cols, M, prev);
            }
        }
        r++;
    }
}

/* ---------------- RREF ---------------- */
void rref(AppData *app, int rows, int cols, double M[rows][cols]) {
    ref(app, rows, cols, M);
    double prev[rows][cols]; copy_matrix(rows, cols, M, prev);

    for (int i = rows-1; i>=0; i--) {
        int pivot_col = -1;
        for (int j=0;j<cols;j++) if(fabs(M[i][j])>EPS) { pivot_col=j; break; }
        if (pivot_col==-1) continue;

        double pivot_val = M[i][pivot_col];
        if (fabs(pivot_val-1.0)>EPS) {
            double scale = 1.0/pivot_val;
            char op[100], coeff[32];
            format_for_step(scale, coeff, sizeof(coeff));
            sprintf(op,"R%d -> (%s)R%d", i+1, coeff, i+1);
            scale_row(rows, cols, M, scale, i); clean_matrix(rows, cols, M);
            if (matrix_changed(rows, cols, M, prev)) {
                record_arrow(app, op); record_step(app, rows, cols, M);
                copy_matrix(rows, cols, M, prev);
            }
        }

        for (int k=0;k<i;k++) {
            double factor=-M[k][pivot_col];
            if (fabs(factor)<EPS) continue;
            char op[100], coeff[32];
            format_for_step(factor, coeff, sizeof(coeff));
            sprintf(op,"R%d -> R%d + (%s)R%d", k+1, k+1, coeff, i+1);
            add_row(rows, cols, M, factor, i, k); clean_matrix(rows, cols, M);
            if (matrix_changed(rows, cols, M, prev)) {
                record_arrow(app, op); record_step(app, rows, cols, M);
                copy_matrix(rows, cols, M, prev);
            }
        }
    }

    // Remove trailing arrow if exists
    if (app->step_list.count>0 && app->step_list.steps[app->step_list.count-1].matrix==NULL) {
        free(app->step_list.steps[app->step_list.count-1].above_arrow);
        app->step_list.count--;
    }
}
