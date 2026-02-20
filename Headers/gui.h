#ifndef GUI_H_INCLUDED
#define GUI_H_INCLUDED

#include <gtk/gtk.h>
#include <pango/pangocairo.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>

/* -------------------- Data Structures -------------------- */

typedef struct {
    double **matrix;   // NULL for arrow step
    int rows;
    int cols;
    char *above_arrow; // NULL if this is a matrix step
} MatrixStep;

typedef struct {
    MatrixStep *steps;
    int count;
} StepList;

typedef struct {
    GtkWidget *grid;
    GtkWidget *drawing_area;
    GtkWidget *rows_entry;
    GtkWidget *cols_entry;
    GtkWidget ***matrix_entries;
    double **matrix_data;
    char *above_arrow;
    StepList step_list;  // store all matrices & arrows
    int rows;
    int cols;
} AppData;

/* -------------------- Function prototypes -------------------- */
void clear_matrix(AppData *app);
void create_matrix(GtkButton *btn, gpointer user_data);
void render_matrix(GtkButton *btn, gpointer user_data);
void render_rref_matrix(GtkButton *btn, gpointer user_data);
// Draw a single matrix; returns total width in pixels for layout purposes
int draw_matrix(cairo_t *cr, MatrixStep *step, int start_x, int start_y, int *out_height);

// Draw all steps (matrices + arrows) in the drawing area
void draw_func(GtkDrawingArea *area, cairo_t *cr,
               int width, int height, gpointer user_data);
void draw_arrow(cairo_t *cr, double x1, double y1,
                double x2, double y2, double head_size);
void activate(GtkApplication *app, gpointer user_data);

#endif


