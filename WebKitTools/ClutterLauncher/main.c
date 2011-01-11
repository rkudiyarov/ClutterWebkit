#include <clutter/clutter.h>
#include <webkit/webkit.h>
#include <stdlib.h>
#include <cairo.h>

static const ClutterColor stage_color = { 0xff, 0xff, 0xff, 0xff };
static const guint statusBarHeight = 16;
static const guint toolbarHeight = 31;

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

/*
static gboolean
delete_cb (ClutterStage* stage, ClutterEvent* event, gpointer data)
{
    g_assert(WEBKIT_IS_WEB_VIEW(data));
    
    printf("Stage delete\n");
    
    clutter_container_remove_actor(CLUTTER_CONTAINER(stage), CLUTTER_ACTOR(data));
    
    return FALSE;
}
*/

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

gboolean
on_back_release_cb(ClutterActor *actor,
                   ClutterEvent *event,
                   WebKitWebView *web_view)
{
    if (webkit_web_view_can_go_back(web_view)) {
        webkit_web_view_go_back(web_view);
    }

    return TRUE;
}

gboolean
on_fwd_release_cb (ClutterActor *actor,
                   ClutterEvent *event,
                   WebKitWebView *web_view)
{
    if (webkit_web_view_can_go_forward(web_view)) {
        webkit_web_view_go_forward(web_view);
    }
    
    return TRUE;
}

void on_uri_activate_cb(ClutterText *self, WebKitWebView *web_view)
{
    const gchar *uri = clutter_text_get_text(self);

    webkit_web_view_load_uri(web_view, uri);
}

int main(int argc, char *argv[])
{
    ClutterActor *stage;
    WebKitWebView *web_view;
    
    ClutterConstraint *width_binding;
    ClutterConstraint *height_binding;
    ClutterConstraint *web_view_height_binding;
    
    gfloat stageWidth, stageHeight;
    ClutterActorBox stageAllocation;
    
    ClutterLayoutManager  *mainLayout;
    ClutterActor          *mainLayoutContainer;
    ClutterLayoutManager  *toolbarLayout;
    ClutterActor          *toolbarContainer;
    ClutterLayoutManager  *toolbarBinLayout;
    ClutterActor          *toolbarBinContainer;
    ClutterActor          *toolbarBgr;
    ClutterActor          *statusBar;
    ClutterActor *backFwdBtns;
    ClutterActor *backBtn;
    ClutterActor *fwdBtn;
    ClutterActor *uriGroup;
    ClutterActor *uriBgr;
    ClutterActor *uriText;
    ClutterActor *spacer;
    
    GError *error = NULL;
    
    ClutterColor whiteColor = { 255, 255, 255, 255 };
    ClutterColor blackColor = { 0, 0, 0, 255 };
    ClutterColor grayColor =  { 200, 200, 200, 255 };
    ClutterColor transparentColor = { 0, 0, 0, 0 };
    
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
    
    mainLayout = clutter_box_layout_new();
    clutter_box_layout_set_vertical(CLUTTER_BOX_LAYOUT(mainLayout), TRUE);
    
    
    clutter_actor_get_allocation_box(stage, &stageAllocation);
    stageWidth = stageAllocation.x2 - stageAllocation.x1;
    stageHeight = stageAllocation.y2 - stageAllocation.y1;
    
    web_view = WEBKIT_WEB_VIEW(webkit_web_view_new((guint)stageWidth, (guint)stageHeight - (toolbarHeight + statusBarHeight)));
    g_object_set(web_view, "reactive", TRUE, NULL);

    g_signal_connect(web_view, "webkit-load-finished", G_CALLBACK(load_finished_cb), web_view);
    g_signal_connect(web_view, "notify::progress", G_CALLBACK (notify_progress_cb), web_view);
/*    g_signal_connect(stage, "delete-event", G_CALLBACK(delete_cb), web_view);*/
    
    mainLayoutContainer = clutter_box_new(mainLayout);
    clutter_actor_set_size(mainLayoutContainer, stageWidth, stageHeight);
    
    width_binding = clutter_bind_constraint_new(stage, CLUTTER_BIND_WIDTH, 0);
    height_binding = clutter_bind_constraint_new(stage, CLUTTER_BIND_HEIGHT, 0);
/*    web_view_height_binding = clutter_bind_constraint_new(stage, CLUTTER_BIND_HEIGHT, -(toolbarHeight + statusBarHeight));
  */  
    clutter_actor_add_constraint(mainLayoutContainer, width_binding);
    clutter_actor_add_constraint(mainLayoutContainer, height_binding);
/*    clutter_actor_add_constraint(CLUTTER_ACTOR(web_view), web_view_height_binding);
  */  
    toolbarBinLayout = clutter_bin_layout_new(CLUTTER_BIN_ALIGNMENT_FILL, CLUTTER_BIN_ALIGNMENT_CENTER);
    toolbarBinContainer = clutter_box_new(toolbarBinLayout);
    
    toolbarBgr = clutter_texture_new_from_file("./toolbar_bgr.png", &error);
    if (toolbarBgr == NULL) {
      fprintf(stderr, "Can't load file: toolbar_bgr.png. Aborting...\n");
      fprintf(stderr, "%s\n", error->message);
      fprintf(stdout, "Working directory is: %s\n", getcwd(NULL, 0));
      exit(1);
    }
    clutter_actor_set_height(toolbarBgr, toolbarHeight);
    clutter_texture_set_repeat(CLUTTER_TEXTURE(toolbarBgr), TRUE, FALSE);
    clutter_box_pack(CLUTTER_BOX(toolbarBinContainer), toolbarBgr, NULL, NULL);
    
    toolbarLayout = clutter_box_layout_new();
    clutter_box_layout_set_vertical(CLUTTER_BOX_LAYOUT(toolbarLayout), FALSE);
    clutter_box_layout_set_spacing(CLUTTER_BOX_LAYOUT(toolbarLayout), 16);
    toolbarContainer = clutter_box_new(toolbarLayout);
    
    spacer = clutter_rectangle_new_with_color(&transparentColor);
    clutter_actor_set_size(spacer, 1, 1);
    clutter_box_pack(CLUTTER_BOX(toolbarContainer), spacer, NULL, NULL);
    
    backFwdBtns = clutter_group_new();
    
    backBtn = clutter_texture_new_from_file("./back_btn.png", &error);
    if (backBtn == NULL) {
      fprintf(stderr, "Can't load file: back_btn.png. Aborting...\n");
      exit(1);
    }
    clutter_actor_set_reactive(backBtn, TRUE);
    /* connect the release event */
    g_signal_connect (backBtn,
                      "button-release-event",
                      G_CALLBACK (on_back_release_cb),
                      web_view);
    
    fwdBtn = clutter_texture_new_from_file("./fwd_btn.png", &error);
    if (fwdBtn == NULL) {
      fprintf(stderr, "Can't load file: fwd_btn.png. Aborting...\n");
      exit(1);
    }
    clutter_actor_set_reactive(fwdBtn, TRUE);
    /* connect the release event */
    g_signal_connect (fwdBtn,
                      "button-release-event",
                      G_CALLBACK (on_fwd_release_cb),
                      web_view);
    
    clutter_actor_set_position(fwdBtn, 
                               clutter_actor_get_width(backBtn), 0);
    clutter_container_add(CLUTTER_CONTAINER(backFwdBtns), backBtn, fwdBtn, NULL);
    clutter_box_pack(CLUTTER_BOX(toolbarContainer), backFwdBtns, NULL, NULL);
    
    uriGroup = clutter_group_new();
    
    uriBgr = clutter_rectangle_new_with_color(&whiteColor);
    clutter_rectangle_set_border_color(CLUTTER_RECTANGLE(uriBgr), &blackColor);
    clutter_rectangle_set_border_width(CLUTTER_RECTANGLE(uriBgr), 1);
    clutter_actor_set_size(uriBgr, 400, 25);
    
    uriText = clutter_text_new_full("Helvetica 16px", "http://www.google.com", &blackColor);
    clutter_text_set_editable(CLUTTER_TEXT(uriText), TRUE);
    clutter_text_set_single_line_mode(CLUTTER_TEXT(uriText), TRUE);
    clutter_actor_set_position(uriText, 5, 4);
    clutter_actor_set_size(uriText, 390, 17);
    clutter_actor_set_reactive(uriText, TRUE);
    g_signal_connect(uriText, "activate", G_CALLBACK(on_uri_activate_cb), web_view);
    
    clutter_container_add(CLUTTER_CONTAINER(uriGroup), uriBgr, uriText, NULL);
    clutter_box_pack(CLUTTER_BOX(toolbarContainer), uriGroup, NULL, NULL);
    
    clutter_box_pack(CLUTTER_BOX(toolbarBinContainer), toolbarContainer, NULL, NULL);
    
    clutter_box_pack(CLUTTER_BOX(mainLayoutContainer), toolbarBinContainer, 
                     "y-align", CLUTTER_BOX_ALIGNMENT_START, NULL);
    clutter_box_layout_set_expand(CLUTTER_BOX_LAYOUT(mainLayout), toolbarBinContainer, TRUE);
    clutter_box_layout_set_fill(CLUTTER_BOX_LAYOUT(mainLayout), toolbarBinContainer, TRUE, FALSE);
    
    statusBar = clutter_rectangle_new_with_color(&grayColor);
    clutter_actor_set_height(statusBar, statusBarHeight);
    
    clutter_box_pack(CLUTTER_BOX(mainLayoutContainer), statusBar, 
                     "y-align", CLUTTER_BOX_ALIGNMENT_END, NULL);
    clutter_box_layout_set_expand(CLUTTER_BOX_LAYOUT(mainLayout), statusBar, TRUE);
    clutter_box_layout_set_fill(CLUTTER_BOX_LAYOUT(mainLayout), statusBar, TRUE, FALSE);
    
    clutter_box_pack_after(CLUTTER_BOX(mainLayoutContainer), CLUTTER_ACTOR(web_view), toolbarBinContainer, 
                           "y-align", CLUTTER_BOX_ALIGNMENT_START, NULL);
    clutter_box_layout_set_expand(CLUTTER_BOX_LAYOUT(mainLayout), CLUTTER_ACTOR(web_view), TRUE);
    clutter_box_layout_set_fill(CLUTTER_BOX_LAYOUT(mainLayout), CLUTTER_ACTOR(web_view), TRUE, TRUE);

    clutter_container_add(CLUTTER_CONTAINER(stage), mainLayoutContainer, NULL);
    
    gchar *uri = (gchar*) (argc > 1 ? argv[1] : "http://www.google.com/");
    gchar *fileURL = filenameToURL(uri);

    webkit_web_view_load_uri(web_view, fileURL ? fileURL : uri);
    printf("%s\n", fileURL);
    g_free(fileURL);
        
    g_timeout_add_full(G_PRIORITY_DEFAULT, 3000, timeout_cb, web_view, 0);
    
    clutter_threads_enter ();
    clutter_main();
    clutter_threads_leave ();
    
    return EXIT_SUCCESS;
}