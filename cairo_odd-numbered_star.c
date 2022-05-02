#include <math.h> // for M_PI, cos, sin

#include <cairo-svg.h>
#include <gtk/gtk.h>

// Types
typedef struct { double x;
                 double y; }
    Point;

typedef struct { cairo_t *cr;
                 int      num_points;
                 Point   *points; }
    Star;

// Global constants
static       int        gNum_points         = 5;
static const double     gFull_circle_angle  = 2 * M_PI;
static const double     gHalf_circle_angle  = M_PI;
static const double     gRight_angle        = M_PI / 2.0;
static const double     gStar_radius        = 200.0;
static const double     gCanvas_dim         = gStar_radius * 2 * 1.1;
static const Point      gOrigin             = { 0, 0 };

// Helper functions/macros

// To silence errors when we have unused things and use -Werror
#define UNUSED __attribute__((unused))

static Point
point_from_origin_angle_radius ( Point  origin,
                                 double angle,
                                 double radius )
{
    return (Point){
        origin.x + cos(angle) * radius,
        origin.y + sin(angle) * radius
    };
}

void
draw_axes ( cairo_t *cr )
{
    cairo_move_to(cr, gOrigin.x, gOrigin.y);
    cairo_line_to(cr, gCanvas_dim, 0);
    cairo_close_path(cr);
    cairo_stroke(cr);

    cairo_move_to(cr, gOrigin.x, gOrigin.y);
    cairo_line_to(cr, 0, gCanvas_dim);
    cairo_close_path(cr);
    cairo_stroke(cr);
}

Star
star_new ( cairo_t *cr,
           int num_points )
{
    // TODO: Handle non-odd number of points
    // Create star points
    const int
        point_increment = ((int)num_points / 2);

    const double
        angle_increment = ((double)gFull_circle_angle / num_points);

    Point *
        star_points = malloc(num_points * sizeof(Point));
    for (int i = 0; i < num_points; ++ i) {
        const int
            point_num = (point_increment * i) % num_points;

        star_points[i] = point_from_origin_angle_radius(
                gOrigin,
                angle_increment * point_num,
                gStar_radius
        );
    }

    cairo_reference(cr);
    Star
        result = { cr, num_points, star_points };
    return result;
}

void
star_destroy ( Star star )
{
    free(star.points);
    cairo_destroy(star.cr);
}

void
star_draw ( Star star )
{
    for (int i = 0; i < star.num_points; ++ i)
        cairo_line_to(star.cr, star.points[i].x, star.points[i].y);

    cairo_close_path(star.cr);
    cairo_stroke(star.cr);
}

void
process_environ (void)
{
    int i = 0;
    if (environ) {
        while (environ[i]) {
            char *
                environ_entry_copy = strdup(environ[i]);
            char *
                haystack = environ_entry_copy;
            char *
                env_var;
            while ( (env_var = strtok(haystack, "=")) ) {
                haystack = NULL;
                char *
                    env_var_value = strtok(NULL, "");

                if (strcmp(env_var, "num_points") == 0) {
                    gNum_points = atoi(env_var_value);
                    break;
                }
            }

            free(environ_entry_copy);
            ++ i;
        }
    }
}

// TODO
void
shrink_and_rotate ( cairo_t *cr,
                    double   scale_amount )
{
    cairo_scale(cr, 1 * scale_amount, -1 * scale_amount);
    cairo_rotate(cr, gHalf_circle_angle);
}

// Gtk setup
// Forward declarations - these are defined in this file.
static gboolean on_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data);
static gboolean time_handler(GtkWidget *widget);

int
gtk_setup ( int   argc,
            char *argv[] )
{
        gtk_init(&argc, &argv);

        GtkWidget *
            window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

        gtk_widget_add_events(window, GDK_BUTTON_PRESS_MASK);

        g_signal_connect(window, "expose-event", G_CALLBACK(on_expose_event), NULL);
        g_signal_connect(window, "destroy",      G_CALLBACK(gtk_main_quit),   NULL);

        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
        gtk_window_set_title(GTK_WINDOW(window), "Star");
        gtk_window_set_default_size(GTK_WINDOW(window), gCanvas_dim, gCanvas_dim);
        gtk_widget_set_app_paintable(window, TRUE);

        //g_timeout_add(10, (GSourceFunc)time_handler, (gpointer)window);

        gtk_widget_show_all(window);

        gtk_main();
        return 0;
}

// Gtk event handlers
// TODO: Customize
static gboolean
on_expose_event ( GtkWidget      *widget,
                  GdkEventExpose *event   UNUSED,
                  gpointer        data    UNUSED )
{
    // TODO: Seems we can't make this static; we get 'error: initializer element is not constant'
    cairo_t *
        cr = gdk_cairo_create(widget->window);

    // Set up coordinate system
    // TODO: Investigate whether we should keep the default coordinate system
    // and just generate the points differently
    cairo_translate(cr, gCanvas_dim / 2.0, gCanvas_dim / 2.0);
    cairo_rotate(cr, gRight_angle);
    cairo_scale(cr, -1, 1);

    // Set black stroke color
    cairo_set_source_rgb(cr, 0, 0, 0);

    // Draw axes
    //draw_axes(cr);

    // Create star points
    Star
        star = star_new(cr, gNum_points);

    // Draw star
    star_draw(star);

    // Draw circle
    // For now, make the circle's radius the average of the canvas size and the
    // star's radius; that way, the circle is just a little bit outside the
    // star.
    // TODO: The reason for this was that I didn't find it pleasing to have the
    // points sticking out just barely beyond the circle, which is how it works
    // right now if I make the circle radius the star radius. I could probably
    // change the line thickness to change that, but I like the thickness at
    // the default level; it would be better to somehow figure out how to make
    // the very point just touch the circle.
    cairo_arc(cr, gOrigin.x, gOrigin.y,
                  (gStar_radius + (gCanvas_dim / 2.0)) / 2.0,
                  0, gFull_circle_angle);
    cairo_close_path(cr);
    cairo_stroke(cr);

    star_destroy(star);

    // Clean up Cairo
    cairo_destroy(cr);

    return FALSE;
}

UNUSED
static gboolean
time_handler ( GtkWidget *widget )
{
    if (widget->window == NULL)
        return FALSE;

    gtk_widget_queue_draw(widget);
    return TRUE;
}

// Create SVG file
void
create_svg (int num_points)
{
    // Set up surface and Cairo
    // TODO: Don't hardcode path
    cairo_surface_t *
        surface = cairo_svg_surface_create("star.svg", gCanvas_dim, gCanvas_dim);
    cairo_t *
        cr = cairo_create(surface);
    // No need to keep surface any longer; clean it up
    cairo_surface_destroy(surface);

    // Set up coordinate system
    // TODO: Investigate whether we should keep the default coordinate system
    // and just generate the points differently
    cairo_translate(cr, gCanvas_dim / 2.0, gCanvas_dim / 2.0);
    cairo_rotate(cr, gRight_angle);
    cairo_scale(cr, -1, 1);

    // Set black stroke color
    cairo_set_source_rgb(cr, 0, 0, 0);

    // Draw axes
    //draw_axes(cr);

    // Create star points
    Star
        star = star_new(cr, num_points);

    // Draw star
    star_draw(star);

    // Draw circle
    // For now, make the circle's radius the average of the canvas size and the
    // star's radius; that way, the circle is just a little bit outside the
    // star.
    // TODO: The reason for this was that I didn't find it pleasing to have the
    // points sticking out just barely beyond the circle, which is how it works
    // right now if I make the circle radius the star radius. I could probably
    // change the line thickness to change that, but I like the thickness at
    // the default level; it would be better to somehow figure out how to make
    // the very point just touch the circle.
    cairo_arc(cr, gOrigin.x, gOrigin.y,
                  (gStar_radius + (gCanvas_dim / 2.0)) / 2.0,
                  0, gFull_circle_angle);
    cairo_close_path(cr);
    cairo_stroke(cr);

    // TODO: cr is now held by the Star; we should probably reference it there
    // and clean it up when we're done, and decide whether to also clean it up
    // here.
    // Clean up star
    free(star.points);

    // Clean up Cairo
    cairo_destroy(cr);
}

// Main
int
main ( int   argc,
       char *argv[] )
{
    // TODO: For now, get number of points to draw from the num_points
    // environment variable.
    process_environ();

    create_svg(gNum_points);

    // Set up X window via Gtk, and finish up
    return gtk_setup(argc, argv);
}

// vim:set sw=4 ts=4 sts=4 expandtab scrolloff=0:
