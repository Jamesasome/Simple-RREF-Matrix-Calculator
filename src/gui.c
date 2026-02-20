#include <gtk/gtk.h>
#include "gui.h"
#include "matrix_operations.h"
#include "r-ref.h"
#include <pango/pangocairo.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>


/* ------------------ 3. Clear old grid entries ------------------ */
void clear_matrix(AppData *app) {

    /* Free entry widget pointer matrix */
    if (app->matrix_entries) {
        for (int i = 0; i < app->rows; i++) {
            free(app->matrix_entries[i]);
        }
        free(app->matrix_entries);
        app->matrix_entries = NULL;
    }

    /* Free numeric matrix data */
    if (app->matrix_data) {
        for (int i = 0; i < app->rows; i++) {
            free(app->matrix_data[i]);
        }
        free(app->matrix_data);
        app->matrix_data = NULL;
    }

    /* Remove GTK grid children */
    GtkWidget *child = gtk_widget_get_first_child(app->grid);
    while (child) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_grid_remove(GTK_GRID(app->grid), child);
        child = next;
    }

    app->rows = 0;
    app->cols = 0;
}


/* ------------------ 4. Create NxM entry grid ------------------ */
void create_matrix(GtkButton *btn, gpointer user_data) {
    AppData *app = user_data;
    clear_matrix(app);

    app->rows = atoi(gtk_editable_get_text(GTK_EDITABLE(app->rows_entry)));
    app->cols = atoi(gtk_editable_get_text(GTK_EDITABLE(app->cols_entry)));
    if (app->rows <= 0 || app->cols <= 0)
        return;

    app->matrix_entries = malloc(app->rows * sizeof(GtkWidget **));
    for (int i = 0; i < app->rows; i++) {
        app->matrix_entries[i] = malloc(app->cols * sizeof(GtkWidget *));
        for (int j = 0; j < app->cols; j++) {
            GtkWidget *entry = gtk_entry_new();
            gtk_widget_set_size_request(entry, 50, -1);
            gtk_grid_attach(GTK_GRID(app->grid), entry, j, i, 1, 1);
            app->matrix_entries[i][j] = entry;
        }
    }
    gtk_widget_set_visible(app->grid, TRUE);
}

/* ------------------ 5. Collect data and redraw ------------------ */
void render_matrix(GtkButton *btn, gpointer user_data) {
    AppData *app = user_data;
    if (!app->matrix_entries) return;

    // Free old numeric data
    if (app->matrix_data) {
        for (int i = 0; i < app->rows; i++)
            free(app->matrix_data[i]);
        free(app->matrix_data);
        app->matrix_data = NULL;
    }

    // Allocate numeric matrix
    app->matrix_data = malloc(app->rows * sizeof(double *));
    for (int i = 0; i < app->rows; i++) {
        app->matrix_data[i] = malloc(app->cols * sizeof(double));
        for (int j = 0; j < app->cols; j++) {
            const char *text = gtk_editable_get_text(GTK_EDITABLE(app->matrix_entries[i][j]));
            char *endptr;
            errno = 0;
            double val = strtod(text, &endptr);
            if (endptr == text || *endptr != '\0' || errno == ERANGE)
                val = 0.0;
            app->matrix_data[i][j] = val;
        }
    }

    // Clear previous steps
    for (int s = 0; s < app->step_list.count; s++) {
        MatrixStep step = app->step_list.steps[s];
        if (step.matrix) {
            for (int i = 0; i < step.rows; i++) free(step.matrix[i]);
            free(step.matrix);
        }
    }
    free(app->step_list.steps);
    app->step_list.steps = NULL;
    app->step_list.count = 0;

    // Record initial matrix as first step (contiguous array)
    double (*temp)[app->cols] = malloc(sizeof(double[app->rows][app->cols]));
    for (int i = 0; i < app->rows; i++)
        for (int j = 0; j < app->cols; j++)
            temp[i][j] = app->matrix_data[i][j];

    record_step(app, app->rows, app->cols, temp);
    free(temp);

    gtk_widget_queue_draw(app->drawing_area);
}


/* ------------------ Render RREF with arrows ------------------ */
void render_rref_matrix(GtkButton *btn, gpointer user_data) {
    AppData *app = user_data;
    if (!app->matrix_entries) return;

    // Clear previous steps
    for (int i = 0; i < app->step_list.count; i++) {
        MatrixStep s = app->step_list.steps[i];
        if (s.matrix) {
            for (int j = 0; j < s.rows; j++)
                free(s.matrix[j]);
            free(s.matrix);
        }
    }
    free(app->step_list.steps);
    app->step_list.steps = NULL;
    app->step_list.count = 0;

    int rows = app->rows;
    int cols = app->cols;
    double M[rows][cols];

    // Copy matrix from entries
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            M[i][j] = atof(gtk_editable_get_text(GTK_EDITABLE(app->matrix_entries[i][j])));

    // **Do not record the initial step here**
    // rref() will record it internally once at start
    rref(app, rows, cols, M);

    gtk_widget_queue_draw(app->drawing_area);
}




/* ------------------------------ Draw Arrows ----------------------------- */
void draw_arrow(cairo_t *cr, double x1, double y1,
                double x2, double y2, double head_size)
{
    double angle = atan2(y2 - y1, x2 - x1);

    /* Draw shaft */
    cairo_move_to(cr, x1, y1);
    cairo_line_to(cr, x2, y2);
    cairo_stroke(cr);

    /* Draw arrowhead */
    cairo_move_to(cr, x2, y2);
    cairo_line_to(cr, x2 - head_size * cos(angle - M_PI/6),
                         y2 - head_size * sin(angle - M_PI/6));
    cairo_line_to(cr, x2 - head_size * cos(angle + M_PI/6),
                         y2 - head_size * sin(angle + M_PI/6));
    cairo_close_path(cr);
    cairo_fill(cr);
}



/* ------------------ 6. Draw function for GtkDrawingArea ------------------ */
// Returns width and sets height via pointer
int draw_matrix(cairo_t *cr, MatrixStep *step, int start_x, int start_y, int *out_height) {
    if (!step->matrix) {
        if (out_height) *out_height = 0;
        return 0;
    }

    PangoLayout *layout = pango_cairo_create_layout(cr);
    PangoFontDescription *desc = pango_font_description_from_string("Sans 18");
    pango_layout_set_font_description(layout, desc);

    int rows = step->rows;
    int cols = step->cols;

    /* ---------------- Measure column widths ---------------- */
    int *col_width = malloc(cols * sizeof(int));
    for (int j = 0; j < cols; j++) col_width[j] = 0;

    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++) {
            char buffer[64];
            format_for_step(step->matrix[i][j], buffer, sizeof(buffer));
            pango_layout_set_text(layout, buffer, -1);
            int tw, th;
            pango_layout_get_pixel_size(layout, &tw, &th);
            if (tw > col_width[j]) col_width[j] = tw;
        }

    for (int j = 0; j < cols; j++) col_width[j] += 20; // horizontal padding

    /* ---------------- Measure row heights ---------------- */
    int *row_height = malloc(rows * sizeof(int));
    for (int i = 0; i < rows; i++) row_height[i] = 0;

    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++) {
            char buffer[64];
            format_for_step(step->matrix[i][j], buffer, sizeof(buffer));
            pango_layout_set_text(layout, buffer, -1);
            int tw, th;
            pango_layout_get_pixel_size(layout, &tw, &th);
            if (th > row_height[i]) row_height[i] = th;
        }

    for (int i = 0; i < rows; i++) row_height[i] += 20; // vertical padding

    /* ---------------- Compute total matrix size ---------------- */
    int matrix_w = 0, matrix_h = 0;
    for (int j = 0; j < cols; j++) matrix_w += col_width[j];
    for (int i = 0; i < rows; i++) matrix_h += row_height[i];

    double pad = 10; // bracket padding
    if (out_height) *out_height = matrix_h + 2 * pad;

    /* ---------------- Draw brackets ---------------- */
    double left_x = start_x - pad;
    double top_y = start_y - pad;
    double bottom_y = start_y + matrix_h + pad;
    double right_x = start_x + matrix_w + pad;

    cairo_set_line_width(cr, 3);
    cairo_set_source_rgb(cr, 0, 0, 0);

    /* Left bracket */
    cairo_move_to(cr, left_x + pad, top_y);
    cairo_line_to(cr, left_x, top_y);
    cairo_line_to(cr, left_x, bottom_y);
    cairo_line_to(cr, left_x + pad, bottom_y);
    cairo_stroke(cr);

    /* Right bracket */
    cairo_move_to(cr, right_x - pad, top_y);
    cairo_line_to(cr, right_x, top_y);
    cairo_line_to(cr, right_x, bottom_y);
    cairo_line_to(cr, right_x - pad, bottom_y);
    cairo_stroke(cr);

    /* ---------------- Draw numbers ---------------- */
    int current_x = start_x;
    for (int j = 0; j < cols; j++) {
        int current_y = start_y;
        for (int i = 0; i < rows; i++) {
            char buffer[64];
            format_for_step(step->matrix[i][j], buffer, sizeof(buffer));

            pango_layout_set_text(layout, buffer, -1);
            int tw, th;
            pango_layout_get_pixel_size(layout, &tw, &th);

            double cx = current_x + col_width[j] / 2.0;
            double cy = current_y + row_height[i] / 2.0;

            cairo_move_to(cr, cx - tw / 2.0, cy - th / 2.0);
            pango_cairo_show_layout(cr, layout);

            current_y += row_height[i];
        }
        current_x += col_width[j];
    }

    free(col_width);
    free(row_height);
    g_object_unref(layout);
    pango_font_description_free(desc);

    return matrix_w + 2 * pad; // include bracket padding
}


/* ------------------ Draw function for GtkDrawingArea ------------------ */
void draw_func(GtkDrawingArea *area, cairo_t *cr,
               int width, int height, gpointer user_data)
{
    if (!cr) return;
    AppData *app = user_data;
    if (!app->step_list.steps) return;

    /* Background */
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    int start_x = 60;
    int start_y = 60;
    int offset_x = start_x;
    int offset_y = start_y;
    int row_spacing = 60;
    int col_spacing = 40;

    int max_row_height = 0;
    int total_width = start_x;
    int total_height = start_y;

    int prev_matrix_h = 0;

    for (int s = 0; s < app->step_list.count; s++) {
        MatrixStep *step = &app->step_list.steps[s];

        int matrix_h = 0;
        int matrix_w = 0;

        if (step->matrix) {
            matrix_w = draw_matrix(cr, step, offset_x, offset_y, &matrix_h);
            prev_matrix_h = matrix_h;

            offset_x += matrix_w + col_spacing;
            if (matrix_h > max_row_height) max_row_height = matrix_h;

        } else {
            /* ---------------- Measure arrow text ---------------- */
            int text_w = 0, text_h = 0;

            if (step->above_arrow) {
                PangoLayout *layout = pango_cairo_create_layout(cr);
                PangoFontDescription *desc =
                        pango_font_description_from_string("Sans 16");

                pango_layout_set_font_description(layout, desc);
                pango_layout_set_text(layout, step->above_arrow, -1);
                pango_layout_get_pixel_size(layout, &text_w, &text_h);

                g_object_unref(layout);
                pango_font_description_free(desc);
            }

            /* ---------------- Compute arrow block width ---------------- */
            int min_arrow_width = 80;
            int padding = 40;

            int arrow_block_width =
                (text_w + padding > min_arrow_width)
                ? text_w + padding
                : min_arrow_width;

            double arrow_start_x = offset_x;
            double arrow_end_x   = offset_x + arrow_block_width;
            double arrow_mid_y   = offset_y + prev_matrix_h / 2.0;

            /* ---------------- Draw Arrow ---------------- */
            draw_arrow(cr,
                       arrow_start_x,
                       arrow_mid_y,
                       arrow_end_x,
                       arrow_mid_y,
                       12);

            /* ---------------- Draw Centered Text ---------------- */
            if (step->above_arrow) {

                PangoLayout *layout = pango_cairo_create_layout(cr);
                PangoFontDescription *desc =
                        pango_font_description_from_string("Sans 16");

                pango_layout_set_font_description(layout, desc);
                pango_layout_set_text(layout, step->above_arrow, -1);

                double text_x =
                    arrow_start_x +
                    (arrow_block_width / 2.0) -
                    (text_w / 2.0);

                double text_y =
                    arrow_mid_y - text_h - 15;

                cairo_move_to(cr, text_x, text_y);
                cairo_set_source_rgb(cr, 0, 0, 0);
                pango_cairo_show_layout(cr, layout);

                g_object_unref(layout);
                pango_font_description_free(desc);
            }

            offset_x += arrow_block_width + col_spacing;
        }

        if (offset_y + max_row_height > total_height)
            total_height = offset_y + max_row_height;

        if (offset_x > total_width)
            total_width = offset_x;
    }

    gtk_widget_set_size_request(GTK_WIDGET(app->drawing_area),
                                total_width + 60,
                                total_height + 60);
}

/* ------------------ Activate function ------------------ */
void activate(GtkApplication *app, gpointer user_data) {
    AppData *data = g_malloc0(sizeof(AppData));

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Matrix Input + Renderer");
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 600);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_hexpand(main_box, TRUE);
    gtk_widget_set_vexpand(main_box, TRUE);

    /* ---------------- Controls ---------------- */
    GtkWidget *controls = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    data->rows_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(data->rows_entry), "Rows");
    data->cols_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(data->cols_entry), "Cols");

    GtkWidget *create_btn = gtk_button_new_with_label("Create Grid");
    GtkWidget *render_btn = gtk_button_new_with_label("Render Matrix");
    GtkWidget *rref_btn = gtk_button_new_with_label("RREF");

    g_signal_connect(create_btn, "clicked", G_CALLBACK(create_matrix), data);
    g_signal_connect(render_btn, "clicked", G_CALLBACK(render_matrix), data);
    g_signal_connect(rref_btn, "clicked", G_CALLBACK(render_rref_matrix), data);

    gtk_box_append(GTK_BOX(controls), data->rows_entry);
    gtk_box_append(GTK_BOX(controls), data->cols_entry);
    gtk_box_append(GTK_BOX(controls), create_btn);
    gtk_box_append(GTK_BOX(controls), render_btn);
    gtk_box_append(GTK_BOX(controls), rref_btn);

    /* ---------------- Input grid with scroll ---------------- */
    data->grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(data->grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(data->grid), 5);

    GtkWidget *grid_frame = gtk_frame_new("Matrix Input");
    gtk_frame_set_child(GTK_FRAME(grid_frame), data->grid);

    GtkWidget *grid_scrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(grid_scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(grid_scrolled), grid_frame);
    gtk_widget_set_hexpand(grid_scrolled, TRUE);
    gtk_widget_set_vexpand(grid_scrolled, TRUE);

    /* ---------------- Drawing area with scroll ---------------- */
    data->drawing_area = gtk_drawing_area_new();
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(data->drawing_area),
                                   draw_func, data, NULL);

    // Allow scrolled window to control drawing area size for scrollbars
    gtk_widget_set_hexpand(data->drawing_area, FALSE);
    gtk_widget_set_vexpand(data->drawing_area, FALSE);

    GtkWidget *draw_frame = gtk_frame_new("Rendered Output");
    gtk_frame_set_child(GTK_FRAME(draw_frame), data->drawing_area);

    GtkWidget *draw_scrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(draw_scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(draw_scrolled), draw_frame);
    gtk_widget_set_hexpand(draw_scrolled, TRUE);
    gtk_widget_set_vexpand(draw_scrolled, TRUE);

    /* ---------------- Horizontal content box ---------------- */
    GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_box_append(GTK_BOX(content_box), grid_scrolled);
    gtk_box_append(GTK_BOX(content_box), draw_scrolled);

    /* Pack main layout */
    gtk_box_append(GTK_BOX(main_box), controls);
    gtk_box_append(GTK_BOX(main_box), content_box);

    gtk_window_set_child(GTK_WINDOW(window), main_box);
    gtk_window_present(GTK_WINDOW(window));
}



/* ------------------ 8. Main ------------------ */
int main(int argc, char *argv[]) {
    GtkApplication *app =
        gtk_application_new("com.example.MatrixResponsive",
                            G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}

