#include <clutter/clutter.h>
#include <webkit/webkit.h>
#include <stdlib.h>
#include <cairo.h>

static const ClutterColor stage_color = { 0xff, 0xff, 0xff, 0xff };

static gchar* filenameToURL(const char* filename)
{
    if (!g_file_test(filename, G_FILE_TEST_EXISTS))
        return 0;

    GFile *gfile = g_file_new_for_path(filename);
    gchar *fileURL = g_file_get_uri(gfile);
    g_object_unref(gfile);

    return fileURL;
}

void paint(cairo_t *c)
{
	cairo_rectangle(c, 0.0, 10.0, 512, 384);
	cairo_set_source_rgb(c, 0.0, 0.0, 0.5);
	cairo_fill(c);


	cairo_move_to(c, 10.0, 10.0);
	cairo_set_source_rgb(c, 1.0, 1.0, 0.0);
	cairo_show_text(c, "Hello World!");

	
	cairo_show_page(c);

	cairo_destroy(c);
}

static void
notify_progress_cb (WebKitWebView* web_view, GParamSpec* pspec, gpointer data)
{
    gdouble load_progress = webkit_web_view_get_progress (web_view) * 100;
    printf("Load progress: %.2f%%\n", load_progress);
}

static void
load_finished_cb (WebKitWebView* web_view, GParamSpec* pspec, gpointer data)
{
    printf("Load finished.\n");
}

static gboolean
delete_cb (ClutterStage* stage, ClutterEvent* event, gpointer data)
{
    g_assert(WEBKIT_IS_WEB_VIEW(data));
    
    printf("Stage delete\n");
    
    clutter_container_remove_actor(CLUTTER_CONTAINER(stage), CLUTTER_ACTOR(data));
    /* g_object_unref(WEBKIT_WEB_VIEW(data)); */
    
    return FALSE;
}

static gboolean
timeout_cb(gpointer data)
{
/*    
    WebKitWebView *web_view = (WebKitWebView*)data;
    gfloat posX, posY;
    double offsX, offsY;
*/
/*
    WebkitActorRectangle uRect;
    uRect.x = 0;
    uRect.y = 0;
    uRect.width = 1024;
    uRect.height = 768;
    
    printf("Screen refresh.\n");
    
    webkit_actor_queue_expose(WEBKIT_ACTOR(web_view), &uRect);
*/    
/*

    cairo_t *ctx = clutter_cairo_texture_create(CLUTTER_CAIRO_TEXTURE(web_view));
    cairo_surface_t *surf = cairo_get_target(ctx);
    cairo_surface_get_device_offset(surf, &offsX, &offsY);
    printf("Surface device offset: %.2f %.2f\n", offsX, offsY);
    
    cairo_matrix_t m;
    cairo_get_matrix(ctx, &m);
    
    printf("Transformation matrix:\n%7.2f %7.2f\n%7.2f %7.2f\n%7.2f %7.2f\n", m.xx, m.yx, m.xy, m.yy, m.x0, m.y0);
    
    paint(ctx);
         
    clutter_actor_get_position(CLUTTER_ACTOR(web_view), &posX, &posY);
    printf("Position: %.2f %.2f\n", posX, posY);
*/
    return TRUE;
}

int main(int argc, char *argv[])
{
    ClutterActor *stage;
    WebKitWebView *web_view;
    
    ClutterConstraint *width_binding;
    ClutterConstraint *height_binding;
    
    gfloat stageWidth, stageHeight;
    ClutterActorBox stageAllocation;
    
    g_thread_init(NULL);
    clutter_threads_init();
    
    clutter_init(&argc, &argv);
    
    stage = clutter_stage_get_default();
    clutter_actor_set_size(stage, 1024, 768);
    clutter_stage_set_color(CLUTTER_STAGE(stage), &stage_color);
    g_signal_connect (stage, "destroy", G_CALLBACK(clutter_main_quit), NULL);
    
    /* make the stage resizable */
    clutter_stage_set_user_resizable(CLUTTER_STAGE(stage), TRUE);
    
    clutter_actor_show(stage);
    
    clutter_actor_get_allocation_box(stage, &stageAllocation);
    stageWidth = stageAllocation.x2 - stageAllocation.x1;
    stageHeight = stageAllocation.y2 - stageAllocation.y1;
    
    web_view = WEBKIT_WEB_VIEW(webkit_web_view_new((guint)stageWidth, (guint)stageHeight));
    g_object_set(web_view, "reactive", TRUE, NULL);
    
    g_signal_connect(web_view, "webkit-load-finished", G_CALLBACK(load_finished_cb), web_view);
    g_signal_connect (web_view, "notify::progress", G_CALLBACK (notify_progress_cb), web_view);
    g_signal_connect(stage, "delete-event", G_CALLBACK(delete_cb), web_view);
    
    width_binding = clutter_bind_constraint_new(stage, CLUTTER_BIND_WIDTH, 0);
    height_binding = clutter_bind_constraint_new(stage, CLUTTER_BIND_HEIGHT, 0);
    
    clutter_actor_add_constraint(CLUTTER_ACTOR(web_view), width_binding);
    clutter_actor_add_constraint(CLUTTER_ACTOR(web_view), height_binding);
    
    clutter_container_add_actor(CLUTTER_CONTAINER(stage), CLUTTER_ACTOR(web_view));
    
    gchar *uri = (gchar*) (argc > 1 ? argv[1] : "http://www.google.com/");
    gchar *fileURL = filenameToURL(uri);

    webkit_web_view_load_uri(web_view, fileURL ? fileURL : uri);
    printf("%s\n", fileURL);
    g_free(fileURL);
    
    clutter_actor_show(CLUTTER_ACTOR(web_view));
    
    g_timeout_add_full(G_PRIORITY_DEFAULT, 3000, timeout_cb, web_view, 0);
    
    clutter_threads_enter ();
    clutter_main();
    clutter_threads_leave ();
    
    return EXIT_SUCCESS;
}