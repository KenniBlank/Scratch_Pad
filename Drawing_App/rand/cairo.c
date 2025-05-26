#include <cairo/cairo.h>
#include <gtk/gtk.h>
#include <stdlib.h>

typedef struct {
    GList *points;
    GList *paths;
    cairo_t *cr;
    GtkWidget *drawing_area;
    gboolean is_drawing;
    GdkRGBA color;
    double line_width;
} DrawingApp;

static void clear_surface(DrawingApp *app) {
    cairo_t *cr = app->cr;
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
    app->points = NULL;
    app->paths = NULL;
}

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, DrawingApp *app) {
    // Draw background
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    // Draw all saved paths
    GList *iter;
    for (iter = app->paths; iter != NULL; iter = iter->next) {
        GList *path = (GList *)iter->data;
        GList *point_iter;
        gboolean first = TRUE;

        cairo_set_source_rgba(cr, app->color.red, app->color.green, app->color.blue, app->color.alpha);
        cairo_set_line_width(cr, app->line_width);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);

        for (point_iter = path; point_iter != NULL; point_iter = point_iter->next) {
            GdkPoint *p = (GdkPoint *)point_iter->data;
            if (first) {
                cairo_move_to(cr, p->x, p->y);
                first = FALSE;
            } else {
                cairo_line_to(cr, p->x, p->y);
            }
        }
        cairo_stroke(cr);
    }

    // Draw current path
    if (app->points != NULL) {
        GList *point_iter;
        gboolean first = TRUE;

        cairo_set_source_rgba(cr,
            app->color.red,
            app->color.green,
            app->color.blue,
            app->color.alpha);
        cairo_set_line_width(cr, app->line_width);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);

        for (point_iter = app->points; point_iter != NULL; point_iter = point_iter->next) {
            GdkPoint *p = (GdkPoint *)point_iter->data;
            if (first) {
                cairo_move_to(cr, p->x, p->y);
                first = FALSE;
            } else {
                cairo_line_to(cr, p->x, p->y);
            }
        }
        cairo_stroke(cr);
    }

    return FALSE;
}

static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, DrawingApp *app) {
    if (event->button == GDK_BUTTON_PRIMARY) {
        app->is_drawing = TRUE;
        GdkPoint *point = g_new(GdkPoint, 1);
        point->x = event->x;
        point->y = event->y;
        app->points = g_list_append(app->points, point);
    }
    return TRUE;
}

static gboolean on_motion(GtkWidget *widget, GdkEventMotion *event, DrawingApp *app) {
    if (app->is_drawing) {
        GdkPoint *point = g_new(GdkPoint, 1);
        point->x = event->x;
        point->y = event->y;
        app->points = g_list_append(app->points, point);
        gtk_widget_queue_draw(widget);
    }
    return TRUE;
}

static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, DrawingApp *app) {
    if (event->button == GDK_BUTTON_PRIMARY && app->is_drawing) {
        app->is_drawing = FALSE;
        app->paths = g_list_append(app->paths, app->points);
        app->points = NULL;
    }
    return TRUE;
}

static void on_color_set(GtkColorButton *button, DrawingApp *app) {
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(button), &app->color);
}

static void on_line_width_changed(GtkSpinButton *button, DrawingApp *app) {
    app->line_width = gtk_spin_button_get_value(button);
}

static void on_clear_clicked(GtkButton *button, DrawingApp *app) {
    clear_surface(app);
    gtk_widget_queue_draw(app->drawing_area);
}

static void close_window(GtkWidget *widget, DrawingApp *app) {
    gtk_main_quit();
}

int main(int argc, char *argv[]) {
    GtkWidget *window;
    GtkWidget *frame;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *drawing_area;
    GtkWidget *color_button;
    GtkWidget *line_width_spin;
    GtkWidget *clear_button;

    DrawingApp app = {0};
    app.color = (GdkRGBA){0, 1, 1, 1}; // Black by default
    app.line_width = 1.0;

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Vector Drawing App");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(close_window), &app);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Toolbar
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    color_button = gtk_color_button_new();
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(color_button), &app.color);
    g_signal_connect(color_button, "color-set", G_CALLBACK(on_color_set), &app);
    gtk_box_pack_start(GTK_BOX(hbox), color_button, FALSE, FALSE, 0);

    line_width_spin = gtk_spin_button_new_with_range(1, 20, 0.5);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(line_width_spin), app.line_width);
    g_signal_connect(line_width_spin, "value-changed", G_CALLBACK(on_line_width_changed), &app);
    gtk_box_pack_start(GTK_BOX(hbox), line_width_spin, FALSE, FALSE, 0);

    clear_button = gtk_button_new_with_label("Clear");
    g_signal_connect(clear_button, "clicked", G_CALLBACK(on_clear_clicked), &app);
    gtk_box_pack_start(GTK_BOX(hbox), clear_button, FALSE, FALSE, 0);

    // Drawing area
    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
    gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);

    drawing_area = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(frame), drawing_area);
    app.drawing_area = drawing_area;

    g_signal_connect(drawing_area, "draw", G_CALLBACK(on_draw), &app);
    g_signal_connect(drawing_area, "motion-notify-event", G_CALLBACK(on_motion), &app);
    g_signal_connect(drawing_area, "button-press-event", G_CALLBACK(on_button_press), &app);
    g_signal_connect(drawing_area, "button-release-event", G_CALLBACK(on_button_release), &app);

    gtk_widget_set_events(drawing_area, gtk_widget_get_events(drawing_area) |
                         GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK);

    gtk_widget_show_all(window);

    gtk_main();

    return 0;
}
