/*
 *  Copyright (C) 2007, 2008 Holger Hans Peter Freyther
 *  Copyright (C) 2007, 2008, 2009 Christian Dywan <christian@imendio.com>
 *  Copyright (C) 2007 Xan Lopez <xan@gnome.org>
 *  Copyright (C) 2007, 2008 Alp Toker <alp@atoker.com>
 *  Copyright (C) 2008 Jan Alonzo <jmalonzo@unpluggable.com>
 *  Copyright (C) 2008 Gustavo Noronha Silva <gns@gnome.org>
 *  Copyright (C) 2008 Nuanti Ltd.
 *  Copyright (C) 2008, 2009, 2010 Collabora Ltd.
 *  Copyright (C) 2009, 2010 Igalia S.L.
 *  Copyright (C) 2009 Movial Creative Technologies Inc.
 *  Copyright (C) 2009 Bobby Powers
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "webkitwebview.h"

#include "webkitdownload.h"
#include "webkitenumtypes.h"
#include "webkitgeolocationpolicydecision.h"
#include "webkitmarshal.h"
#include "webkitnetworkrequest.h"
#include "webkitnetworkresponse.h"
#include "webkitprivate.h"
#include "webkitwebinspector.h"
#include "webkitwebbackforwardlist.h"
#include "webkitwebhistoryitem.h"

#include "AXObjectCache.h"
#include "AbstractDatabase.h"
#include "BackForwardList.h"
#include "Cache.h"
#include "ChromeClientClutter.h"
//#include "ClipboardUtilitiesClutter.h"
#include "ContextMenuClientClutter.h"
#include "ContextMenuController.h"
#include "ContextMenu.h"
#include "Cursor.h"
#include "Document.h"
#include "DocumentLoader.h"
#include "DragActions.h"
#include "DragClientClutter.h"
#include "DragController.h"
#include "DragData.h"
#include "EditorClientClutter.h"
#include "Editor.h"
#include "EventHandler.h"
#include "FloatQuad.h"
#include "FocusController.h"
#include "FrameLoader.h"
#include "FrameLoaderTypes.h"
#include "FrameView.h"
#include <glib/gi18n-lib.h>
#include <GOwnPtr.h>
#include "GraphicsContext.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "IconDatabase.h"
#include "InspectorClientClutter.h"
#include "MouseEventWithHitTestResults.h"
#include "NotImplemented.h"
#include "PageCache.h"
#include "Pasteboard.h"
#include "PasteboardHelperClutter.h"
#include "PasteboardHelperWebCoreClutter.h"
#include "PlatformKeyboardEvent.h"
#include "PlatformWheelEvent.h"
#include "ProgressTracker.h"
#include "RenderLayer.h"
#include "RenderView.h"
#include "ResourceHandle.h"
#include "ScriptValue.h"
#include "Scrollbar.h"
#include "webkit/WebKitDOMDocumentPrivate.h"
#include <wtf/text/CString.h>

//#include <gdk/gdkkeysyms.h>

/**
 * SECTION:webkitwebview
 * @short_description: The central class of the WebKitGTK+ API
 * @see_also: #WebKitWebSettings, #WebKitWebFrame
 *
 * #WebKitWebView is the central class of the WebKitGTK+ API. It is a
 * #GtkWidget implementing the scrolling interface which means you can
 * embed in a #GtkScrolledWindow. It is responsible for managing the
 * drawing of the content, forwarding of events. You can load any URI
 * into the #WebKitWebView or any kind of data string. With #WebKitWebSettings
 * you can control various aspects of the rendering and loading of the content.
 * Each #WebKitWebView has exactly one #WebKitWebFrame as main frame. A
 * #WebKitWebFrame can have n children.
 *
 * <programlisting>
 * /<!-- -->* Create the widgets *<!-- -->/
 * GtkWidget *main_window = gtk_window_new (GTK_WIDGET_TOPLEVEL);
 * GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);
 * GtkWidget *web_view = webkit_web_view_new ();
 *
 * /<!-- -->* Place the WebKitWebView in the GtkScrolledWindow *<!-- -->/
 * gtk_container_add (GTK_CONTAINER (scrolled_window), web_view);
 * gtk_container_add (GTK_CONTAINER (main_window), scrolled_window);
 *
 * /<!-- -->* Open a webpage *<!-- -->/
 * webkit_web_view_load_uri (WEBKIT_WEB_VIEW (web_view), "http://www.gnome.org");
 *
 * /<!-- -->* Show the result *<!-- -->/
 * gtk_window_set_default_size (GTK_WINDOW (main_window), 800, 600);
 * gtk_widget_show_all (main_window);
 * </programlisting>
 */

static const double defaultDPI = 96.0;
static WebKitCacheModel cacheModel;
//static IntPoint globalPointForClientPoint(GdkWindow* window, const IntPoint& clientPoint);

using namespace WebKit;
using namespace WebCore;

enum {
    /* normal signals */
    NAVIGATION_REQUESTED,
    NEW_WINDOW_POLICY_DECISION_REQUESTED,
    NAVIGATION_POLICY_DECISION_REQUESTED,
    MIME_TYPE_POLICY_DECISION_REQUESTED,
    CREATE_WEB_VIEW,
    WEB_VIEW_READY,
    WINDOW_OBJECT_CLEARED,
    LOAD_STARTED,
    LOAD_COMMITTED,
    LOAD_PROGRESS_CHANGED,
    LOAD_ERROR,
    LOAD_FINISHED,
    TITLE_CHANGED,
    HOVERING_OVER_LINK,
    POPULATE_POPUP,
    STATUS_BAR_TEXT_CHANGED,
    ICON_LOADED,
    SELECTION_CHANGED,
    CONSOLE_MESSAGE,
    SCRIPT_ALERT,
    SCRIPT_CONFIRM,
    SCRIPT_PROMPT,
    SELECT_ALL,
    COPY_CLIPBOARD,
    PASTE_CLIPBOARD,
    CUT_CLIPBOARD,
    DOWNLOAD_REQUESTED,
    MOVE_CURSOR,
    PRINT_REQUESTED,
    PLUGIN_WIDGET,
    CLOSE_WEB_VIEW,
    UNDO,
    REDO,
    DATABASE_QUOTA_EXCEEDED,
    RESOURCE_REQUEST_STARTING,
    DOCUMENT_LOAD_FINISHED,
    GEOLOCATION_POLICY_DECISION_REQUESTED,
    GEOLOCATION_POLICY_DECISION_CANCELLED,
    ONLOAD_EVENT,
    FRAME_CREATED,
    LAST_SIGNAL
};

enum {
    PROP_0,

    PROP_TITLE,
    PROP_URI,
    PROP_COPY_TARGET_LIST,
    PROP_PASTE_TARGET_LIST,
    PROP_EDITABLE,
    PROP_SETTINGS,
    PROP_WEB_INSPECTOR,
    PROP_WINDOW_FEATURES,
    PROP_TRANSPARENT,
    PROP_ZOOM_LEVEL,
    PROP_FULL_CONTENT_ZOOM,
    PROP_LOAD_STATUS,
    PROP_PROGRESS,
    PROP_ENCODING,
    PROP_CUSTOM_ENCODING,
    PROP_ICON_URI,
//    PROP_IM_CONTEXT,
    PROP_VIEW_MODE,
    PROP_ZOOM_PADDING,
    PROP_TRANSITION_TIME
};

static guint webkit_web_view_signals[LAST_SIGNAL] = { 0, };

static void webkit_web_view_container_iface_init (ClutterContainerIface *iface);
G_DEFINE_TYPE_WITH_CODE(WebKitWebView, webkit_web_view, WEBKIT_TYPE_ACTOR,
			G_IMPLEMENT_INTERFACE(CLUTTER_TYPE_CONTAINER,
					      webkit_web_view_container_iface_init))

static void webkit_web_view_settings_notify(WebKitWebSettings* webSettings, GParamSpec* pspec, WebKitWebView* webView);
static void webkit_web_view_set_window_features(WebKitWebView* webView, WebKitWebWindowFeatures* webWindowFeatures);


static gboolean webkit_web_view_forward_context_menu_event(WebKitWebView* webView, const PlatformMouseEvent& event)
{
    Page* page = core(webView);
    page->contextMenuController()->clearContextMenu();
    Frame* focusedFrame;
    Frame* mainFrame = page->mainFrame();
    gboolean mousePressEventResult = FALSE;

    if (!mainFrame->view())
        return FALSE;

    mainFrame->view()->setCursor(pointerCursor());
    if (page->frameCount()) {
        HitTestRequest request(HitTestRequest::Active);
        IntPoint point = mainFrame->view()->windowToContents(event.pos());
        MouseEventWithHitTestResults mev = mainFrame->document()->prepareMouseEvent(request, point, event);

        Frame* targetFrame = EventHandler::subframeForTargetNode(mev.targetNode());
        if (!targetFrame)
            targetFrame = mainFrame;

        focusedFrame = page->focusController()->focusedOrMainFrame();
        if (targetFrame != focusedFrame) {
            page->focusController()->setFocusedFrame(targetFrame);
            focusedFrame = targetFrame;
        }
    } else
        focusedFrame = mainFrame;

    if (focusedFrame->view() && focusedFrame->eventHandler()->handleMousePressEvent(event))
        mousePressEventResult = TRUE;


    bool handledEvent = focusedFrame->eventHandler()->sendContextMenuEvent(event);
    if (!handledEvent)
        return FALSE;

    // If coreMenu is NULL, this means WebCore decided to not create
    // the default context menu; this may happen when the page is
    // handling the right-click for reasons other than the context menu.
    ContextMenu* coreMenu = page->contextMenuController()->contextMenu();
    if (!coreMenu)
        return mousePressEventResult;

    // If we reach here, it's because WebCore is going to show the
    // default context menu. We check our setting to figure out
    // whether we want it or not.
    WebKitWebSettings* settings = webkit_web_view_get_settings(webView);
    gboolean enableDefaultContextMenu;
    g_object_get(settings, "enable-default-context-menu", &enableDefaultContextMenu, NULL);

    if (!enableDefaultContextMenu)
        return FALSE;

    GList* menu = (GList*)(coreMenu->platformDescription());
    if (!menu)
        return FALSE;

    //g_signal_emit(webView, webkit_web_view_signals[POPULATE_POPUP], 0, menu);

    GList* items = menu;
    bool empty = !g_list_nth(items, 0);
    //g_list_free(items);
    if (empty)
        return FALSE;

    WebKitWebViewPrivate* priv = WEBKIT_WEB_VIEW_GET_PRIVATE(webView);
    priv->currentMenu = menu;
    priv->lastPopupXPosition = event.globalX();
    priv->lastPopupYPosition = event.globalY();

    // gtk_menu_popup(menu, NULL, NULL,
    //                &PopupMenuPositionFunc,
    //                webView, event.button() + 1, gtk_get_current_event_time());
    return TRUE;
}

static gboolean webkit_web_view_popup_menu_handler(ClutterActor* widget)
{
    static const int contextMenuMargin = 1;

    // The context menu event was generated from the keyboard, so show the context menu by the current selection.
    Page* page = core(WEBKIT_WEB_VIEW(widget));
    Frame* frame = page->focusController()->focusedOrMainFrame();
    FrameView* view = frame->view();
    if (!view)
        return FALSE;    

    Position start = frame->selection()->selection().start();
    Position end = frame->selection()->selection().end();

    int rightAligned = FALSE;
    IntPoint location;

    if (!start.node() || !end.node()
        || (frame->selection()->selection().isCaret() && !frame->selection()->selection().isContentEditable()))
        location = IntPoint(rightAligned ? view->contentsWidth() - contextMenuMargin : contextMenuMargin, contextMenuMargin);
    else {
        RenderObject* renderer = start.node()->renderer();
        if (!renderer)
            return FALSE;

        // Calculate the rect of the first line of the selection (cribbed from -[WebCoreFrameBridge firstRectForDOMRange:],
        // now Frame::firstRectForRange(), which perhaps this should call).
        int extraWidthToEndOfLine = 0;

        InlineBox* startInlineBox;
        int startCaretOffset;
        start.getInlineBoxAndOffset(DOWNSTREAM, startInlineBox, startCaretOffset);
        IntRect startCaretRect = renderer->localCaretRect(startInlineBox, startCaretOffset, &extraWidthToEndOfLine);
        if (startCaretRect != IntRect())
            startCaretRect = renderer->localToAbsoluteQuad(FloatRect(startCaretRect)).enclosingBoundingBox();

        InlineBox* endInlineBox;
        int endCaretOffset;
        end.getInlineBoxAndOffset(UPSTREAM, endInlineBox, endCaretOffset);
        IntRect endCaretRect = renderer->localCaretRect(endInlineBox, endCaretOffset);
        if (endCaretRect != IntRect())
            endCaretRect = renderer->localToAbsoluteQuad(FloatRect(endCaretRect)).enclosingBoundingBox();

        IntRect firstRect;
        if (startCaretRect.y() == endCaretRect.y())
            firstRect = IntRect(MIN(startCaretRect.x(), endCaretRect.x()),
                                startCaretRect.y(),
                                abs(endCaretRect.x() - startCaretRect.x()),
                                MAX(startCaretRect.height(), endCaretRect.height()));
        else
            firstRect = IntRect(startCaretRect.x(),
                                startCaretRect.y(),
                                startCaretRect.width() + extraWidthToEndOfLine,
                                startCaretRect.height());

        location = IntPoint(rightAligned ? firstRect.right() : firstRect.x(), firstRect.bottom());
    }

    // FIXME: The IntSize(0, -1) is a hack to get the hit-testing to result in the selected element.
    // Ideally we'd have the position of a context menu event be separate from its target node.
    location = view->contentsToWindow(location) + IntSize(0, -1);
    if (location.y() < 0)
        location.setY(contextMenuMargin);
    else if (location.y() > view->height())
        location.setY(view->height() - contextMenuMargin);
    if (location.x() < 0)
        location.setX(contextMenuMargin);
    else if (location.x() > view->width())
        location.setX(view->width() - contextMenuMargin);
    // FIXME: the location is *probably* already in stage coordinates so we don't convert global
    IntPoint global(location);

    PlatformMouseEvent event(location, global, RightButton, MouseEventPressed, 0, false, false, false, false, clutter_get_current_event_time());

    return webkit_web_view_forward_context_menu_event(WEBKIT_WEB_VIEW(widget), event);
}

static void webkit_web_view_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(object);

    switch(prop_id) {
    case PROP_TITLE:
        g_value_set_string(value, webkit_web_view_get_title(webView));
        break;
    case PROP_URI:
        g_value_set_string(value, webkit_web_view_get_uri(webView));
        break;
    // case PROP_COPY_TARGET_LIST:
    //     g_value_set_boxed(value, webkit_web_view_get_copy_target_list(webView));
    //     break;
    // case PROP_PASTE_TARGET_LIST:
    //     g_value_set_boxed(value, webkit_web_view_get_paste_target_list(webView));
    //     break;
    case PROP_EDITABLE:
        g_value_set_boolean(value, webkit_web_view_get_editable(webView));
        break;
    case PROP_SETTINGS:
        g_value_set_object(value, webkit_web_view_get_settings(webView));
        break;
    case PROP_WEB_INSPECTOR:
        g_value_set_object(value, webkit_web_view_get_inspector(webView));
        break;
    case PROP_WINDOW_FEATURES:
        g_value_set_object(value, webkit_web_view_get_window_features(webView));
        break;
    case PROP_TRANSPARENT:
        g_value_set_boolean(value, webkit_web_view_get_transparent(webView));
        break;
    case PROP_ZOOM_LEVEL:
        g_value_set_float(value, webkit_web_view_get_zoom_level(webView));
        break;
    case PROP_FULL_CONTENT_ZOOM:
        g_value_set_boolean(value, webkit_web_view_get_full_content_zoom(webView));
        break;
    case PROP_ENCODING:
        g_value_set_string(value, webkit_web_view_get_encoding(webView));
        break;
    case PROP_CUSTOM_ENCODING:
        g_value_set_string(value, webkit_web_view_get_custom_encoding(webView));
        break;
    case PROP_LOAD_STATUS:
        g_value_set_enum(value, webkit_web_view_get_load_status(webView));
        break;
    case PROP_PROGRESS:
        g_value_set_double(value, webkit_web_view_get_progress(webView));
        break;
    case PROP_ICON_URI:
        g_value_set_string(value, webkit_web_view_get_icon_uri(webView));
        break;
//    case PROP_IM_CONTEXT:
//        g_value_set_object(value, webkit_web_view_get_im_context(webView));
        break;
    case PROP_VIEW_MODE:
        g_value_set_enum(value, webkit_web_view_get_view_mode(webView));
        break;
    case PROP_ZOOM_PADDING:
        g_value_set_int(value, webkit_web_view_get_zoom_padding(webView));
        break;
    case PROP_TRANSITION_TIME:
        g_value_set_uint(value, webkit_web_view_get_transition_time(webView));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void webkit_web_view_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec *pspec)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(object);

    switch(prop_id) {
    case PROP_EDITABLE:
        webkit_web_view_set_editable(webView, g_value_get_boolean(value));
        break;
    case PROP_SETTINGS:
        webkit_web_view_set_settings(webView, WEBKIT_WEB_SETTINGS(g_value_get_object(value)));
        break;
    case PROP_WINDOW_FEATURES:
        webkit_web_view_set_window_features(webView, WEBKIT_WEB_WINDOW_FEATURES(g_value_get_object(value)));
        break;
    case PROP_TRANSPARENT:
        webkit_web_view_set_transparent(webView, g_value_get_boolean(value));
        break;
    case PROP_ZOOM_LEVEL:
        webkit_web_view_set_zoom_level(webView, g_value_get_float(value));
        break;
    case PROP_FULL_CONTENT_ZOOM:
        webkit_web_view_set_full_content_zoom(webView, g_value_get_boolean(value));
        break;
    case PROP_CUSTOM_ENCODING:
        webkit_web_view_set_custom_encoding(webView, g_value_get_string(value));
        break;
    case PROP_VIEW_MODE:
        webkit_web_view_set_view_mode(webView, static_cast<WebKitWebViewViewMode>(g_value_get_enum(value)));
        break;
    case PROP_ZOOM_PADDING:
        webkit_web_view_set_zoom_padding (webView, g_value_get_int (value));
        break;
    case PROP_TRANSITION_TIME:
        webkit_web_view_set_transition_time (webView, g_value_get_uint (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static bool shouldCoalesce(const IntRect& rect, const Vector<IntRect>& rects)
{
    const unsigned int cRectThreshold = 10;
    const float cWastedSpaceThreshold = 0.75f;
    bool useUnionedRect = (rects.size() <= 1) || (rects.size() > cRectThreshold);
    if (useUnionedRect)
        return true;
    // Attempt to guess whether or not we should use the unioned rect or the individual rects.
    // We do this by computing the percentage of "wasted space" in the union.  If that wasted space
    // is too large, then we will do individual rect painting instead.
    float unionPixels = (rect.width() * rect.height());
    float singlePixels = 0;
    for (size_t i = 0; i < rects.size(); ++i)
        singlePixels += rects[i].width() * rects[i].height();
    float wastedSpace = 1 - (singlePixels / unionPixels);
    if (wastedSpace <= cWastedSpaceThreshold)
        useUnionedRect = true;
    return useUnionedRect;
}

static void paintWebView(Frame* frame, gboolean transparent, GraphicsContext& context, const IntRect& clipRect, const Vector<IntRect>& rects)
{
    bool coalesce = true;

    if (rects.size() > 0)
        coalesce = shouldCoalesce(clipRect, rects);

    if (coalesce) {
        context.clip(clipRect);
        if (transparent)
            context.clearRect(clipRect);
        frame->view()->paint(&context, clipRect);
    } else {
        for (size_t i = 0; i < rects.size(); i++) {
            IntRect rect = rects[i];
            context.save();
            context.clip(rect);
            if (transparent)
                context.clearRect(rect);
            frame->view()->paint(&context, rect);
            context.restore();
        }
    }

    context.save();
    context.clip(clipRect);
    frame->page()->inspectorController()->drawNodeHighlight(context);
    context.restore();
}

static void 
webkit_web_view_expose(WebkitActor* actor, WebkitActorRectangle* rect)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(actor);
    WebKitWebViewPrivate* priv = webView->priv;

    Frame* frame = core(webView)->mainFrame();
    
    if ((rect->width == 0) || (rect->height == 0))
        return;
        
//    Frame* frame = core(webView)->mainFrame();
    cairo_t* cr;
    if (frame->contentRenderer() && frame->view()) {
        frame->view()->updateLayoutAndStyleIfNeededRecursive();

        cr = clutter_cairo_texture_create_region(CLUTTER_CAIRO_TEXTURE(actor),
            rect->x, rect->y,
            rect->width, rect->height);
        GraphicsContext ctx(cr);
        cairo_destroy(cr);
        
        //int rectCount;
        //GOwnPtr<GdkRectangle> rects;
        //gdk_region_get_rectangles(event->region, &rects.outPtr(), &rectCount);
        Vector<IntRect> paintRects;
        paintRects.append(IntRect(*rect));
        //for (int i = 0; i < rectCount; i++)
        //    paintRects.append(IntRect(rects.get()[i]));

        paintWebView(frame, priv->transparent, ctx, IntRect(*rect), paintRects);
    }
}

static double
get_zoom_factor (WebKitWebView *webView)
{
    WebKitWebViewPrivate *priv = webView->priv;

    if (priv->zoom_rect.width > priv->zoom_rect.height)
        return ((double) priv->zoom_rect_pre.width) /
               ((double) priv->zoom_rect.width);
    else
        return ((double) priv->zoom_rect_pre.height) /
               ((double) priv->zoom_rect.height);
}

static void
zoom_new_frame_cb (ClutterTimeline *timeline, gint frame_num, WebKitWebView *webView)
{
    WebKitWebViewPrivate *priv = webView->priv;
    double alpha = (double)clutter_alpha_get_alpha(priv->pan_alpha);
    
    // Set position
    priv->zoom_x = (int)
                   (((double)priv->zoom_rect.x * alpha) +
                    ((double)priv->zoom_rect_pre.x * (1.0 - alpha)));
    priv->zoom_x -= priv->zoom_rect_pre.x;

    priv->zoom_y = (int)
                   (((double)priv->zoom_rect.y * alpha) +
                    ((double)priv->zoom_rect_pre.y * (1.0 - alpha)));
    priv->zoom_y -= priv->zoom_rect_pre.y;

    alpha = (double)clutter_alpha_get_alpha (priv->zoom_alpha);
    
    // Set zoom
    priv->zoom_factor = (1.0 - alpha) + (get_zoom_factor (webView) * alpha);
    
    // Queue a redraw
    clutter_actor_queue_redraw (CLUTTER_ACTOR (webView));
}

static void
zoom_completed_cleanup_cb (ClutterTimeline *timeline, WebKitWebView *webView)
{
    WebKitWebViewPrivate *priv = webView->priv;

    priv->zoom_timeline = NULL;
    g_object_unref (timeline);
    
    priv->zoom_factor = 1.0;
    priv->zoom_x = 0;
    priv->zoom_y = 0;
}

static void
zoom_completed_transition_cb (ClutterTimeline *timeline, WebKitWebView *webView)
{
    WebKitWebViewPrivate *priv = webView->priv;
    Frame* frame = core(webView)->mainFrame();
    double zoom_factor = get_zoom_factor(webView);

    // Invalidate buffer
    // TODO: check if this works without it
//    if (priv->slide_init > 0)
//        priv->slide_init = 0;

    frame->setPageZoomFactor(frame->pageZoomFactor() * zoom_factor);

    WebkitActorPoint pt;
    pt.x = (int)(priv->zoom_rect.x * zoom_factor);
    pt.y = (int)(priv->zoom_rect.y * zoom_factor);
    frame->view()->setScrollPosition(pt);

    if (priv->zoom_node.get()) {
	Document* document = frame->document();
	document->updateLayoutIgnorePendingStylesheets();

	WebkitActorRectangle bounds = priv->zoom_node->getRect();
	bounds.x -= priv->zoom_pad;
	bounds.y -= priv->zoom_pad;
	bounds.width += priv->zoom_pad * 2;
	bounds.height += priv->zoom_pad * 2;

	priv->zoom_node->renderer()->enclosingLayer()->scrollRectToVisible(bounds, false, ScrollAlignment::alignCenterAlways, ScrollAlignment::alignTopAlways);
    }
}

static void zoom_on_node(WebKitWebView* webView, Node* node)
{
    WebKitWebViewPrivate *priv = webView->priv;
    Frame* frame = core(webView)->mainFrame();
    
    if (priv->zoom_timeline) {
        clutter_timeline_stop (priv->zoom_timeline);
        g_object_unref (priv->zoom_timeline);
    }
    
    // Store pre-zoom and post-zoom rectangles
    priv->zoom_rect_pre = frame->view()->windowClipRect();
    priv->zoom_rect_pre.x = frame->view()->scrollX();
    priv->zoom_rect_pre.y = frame->view()->scrollY();
    
    if (node) {
        priv->zoom_node = node;
	
        priv->zoom_rect = node->getRect();
        priv->zoom_rect.x -= priv->zoom_pad;
        priv->zoom_rect.y -= priv->zoom_pad;
        priv->zoom_rect.width += priv->zoom_pad * 2;
        priv->zoom_rect.height += priv->zoom_pad * 2;
        
        //printf("zoom_rect.x: %d, zoom_rect.y: %d\n", priv->zoom_rect.x, priv->zoom_rect.y);
    } else {
        // Zoom-out case - try to keep the current page x,y if possible and
        // zoom out to a 1.0 zoom
        double zoom_factor;
        
        priv->zoom_node.clear ();
        
        priv->zoom_rect = frame->view()->windowClipRect();
        priv->zoom_rect.width = (int)(priv->zoom_rect_pre.width *
                                      frame->pageZoomFactor());
        priv->zoom_rect.height = (int)(priv->zoom_rect_pre.height *
                                       frame->pageZoomFactor());
        
        zoom_factor = get_zoom_factor (webView);
        priv->zoom_rect.x = (int)
            (MIN (frame->view()->scrollX() / frame->pageZoomFactor(),
                  (frame->view()->contentsWidth() / frame->pageZoomFactor()) -
                  priv->zoom_rect_pre.width) /
             zoom_factor);
        priv->zoom_rect.y = (int)
            (MIN (frame->view()->scrollY() / frame->pageZoomFactor(),
                  (frame->view()->contentsHeight() / frame->pageZoomFactor()) -
                  priv->zoom_rect_pre.height) /
             zoom_factor);
    }

    // Set up the zooming timeline
    priv->zoom_timeline = clutter_timeline_new (priv->transition_time);
    priv->pan_alpha = clutter_alpha_new_full (priv->zoom_timeline,
                                              CLUTTER_LINEAR);

    priv->zoom_alpha = clutter_alpha_new_full (priv->zoom_timeline,
                                               CLUTTER_EASE_IN_SINE);

    
    g_signal_connect (priv->zoom_timeline, "new-frame",
                      G_CALLBACK (zoom_new_frame_cb), webView);
    g_signal_connect (priv->zoom_timeline, "completed",
                      G_CALLBACK (zoom_completed_transition_cb), webView);
    g_signal_connect_after (priv->zoom_timeline, "completed",
                            G_CALLBACK (zoom_completed_cleanup_cb), webView);

    clutter_timeline_start (priv->zoom_timeline);
}

static gboolean webkit_web_view_key_press_event(ClutterActor* actor, ClutterKeyEvent* event)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(actor);

    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    PlatformKeyboardEvent keyboardEvent(event);

    if (!frame->view())
        return FALSE;

    if (frame->eventHandler()->keyEvent(keyboardEvent))
        return TRUE;

    return FALSE;
}

static gboolean webkit_web_view_key_release_event(ClutterActor* actor, ClutterKeyEvent* event)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(actor);

    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    PlatformKeyboardEvent keyboardEvent(event);

    if (!frame->view())
	return FALSE;

    if (frame->eventHandler()->keyEvent(keyboardEvent))
	return TRUE;

    // FIXME: Handle other keys
    return FALSE;
}

// static guint32 getEventTime(GdkEvent* event)
// {
//     guint32 time = gdk_event_get_time(event);
//     if (time)
//         return time;
// 
//     // Real events always have a non-zero time, but events synthesized
//     // by the DRT do not and we must calculate a time manually. This time
//     // is not calculated in the DRT, because GTK+ does not work well with
//     // anything other than GDK_CURRENT_TIME on synthesized events.
//     GTimeVal timeValue;
//     g_get_current_time(&timeValue);
//     return (timeValue.tv_sec * 1000) + (timeValue.tv_usec / 1000);
// } 

static gboolean webkit_web_view_button_press_event(ClutterActor* actor, ClutterButtonEvent* event)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(actor);
    WebKitWebViewPrivate* priv = webView->priv;
    Frame* frame = core(webView)->mainFrame();
    gboolean return_val, is_link;

    if (!frame->view())
	    return FALSE;

    // Handle the click and get the clicked node
    core(webView)->focusController()->setActive(frame);
    return_val = frame->eventHandler()->handleMousePressEvent(PlatformMouseEvent(event));
    is_link = frame->eventHandler()->mousePressNode()->isLink();

    // If we've double-clicked, zoom into the clicked element (but not for links)
    if ((event->click_count == 2) && (!is_link)) {
        gfloat ux, uy;
        Document* document = frame->document();

        // Get the element at the page coordinates
        clutter_actor_transform_stage_point(actor, event->x, event->y, &ux, &uy);

        IntPoint windowPoint((int)ux, (int)uy);
        IntPoint point = document->view()->windowToContents(windowPoint);
        Element* element = document->elementFromPoint(point.x(), point.y());
        
        // Clear selection
        frame->selection()->clear();
        
        // Zoom in/out element or reset zoom if it's the previously zoomed element
        if (element == priv->zoom_node.get())
            zoom_on_node(webView, NULL);
        else
            zoom_on_node(webView, element);
        
        return TRUE;
    }
    
    return return_val;
}

static gboolean webkit_web_view_button_release_event(ClutterActor* actor, ClutterButtonEvent* event)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(actor);

    Frame* mainFrame = core(webView)->mainFrame();
    if (!mainFrame->view())
   	    return FALSE;

    return mainFrame->eventHandler()->handleMouseReleaseEvent(PlatformMouseEvent(event));
}

static gboolean webkit_web_view_motion_event(ClutterActor* actor, ClutterMotionEvent* event)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(actor);

    Frame* frame = core(webView)->mainFrame();
    if (!frame->view())
        return FALSE;

    return frame->eventHandler()->mouseMoved(PlatformMouseEvent(event));
}

static gboolean webkit_web_view_scroll_event(ClutterActor* actor, ClutterScrollEvent* event)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(actor);

    Frame* frame = core(webView)->mainFrame();
    if (!frame->view())
        return FALSE;

    PlatformWheelEvent wheelEvent(event);
    return frame->eventHandler()->handleWheelEvent(wheelEvent);
}

static gboolean webkit_web_view_enter_event(ClutterActor* actor, ClutterCrossingEvent *event) {
    WebKitWebView* webView = WEBKIT_WEB_VIEW(actor);

    Frame* frame = core(webView)->mainFrame();
    if (!frame->view())
        return FALSE;
    
    ClutterMotionEvent motionEvent;
    motionEvent.type = CLUTTER_MOTION;
    motionEvent.time = event->time;
    motionEvent.flags = CLUTTER_EVENT_FLAG_SYNTHETIC;
    motionEvent.stage = event->stage;
    motionEvent.source = event->source;
    motionEvent.x = event->x;
    motionEvent.y = event->y;
    motionEvent.modifier_state = CLUTTER_MODIFIER_MASK;
    motionEvent.axes = 0;
    motionEvent.device = event->device;
    
    return frame->eventHandler()->mouseMoved(PlatformMouseEvent(&motionEvent));
}

static gboolean webkit_web_view_leave_event(ClutterActor* actor, ClutterCrossingEvent *event) {
    printf("leave event\n");
    
    WebKitWebView* webView = WEBKIT_WEB_VIEW(actor);

    Frame* frame = core(webView)->mainFrame();
    if (!frame->view())
        return FALSE;

    ClutterMotionEvent motionEvent;
    motionEvent.type = CLUTTER_MOTION;
    motionEvent.time = event->time;
    motionEvent.flags = CLUTTER_EVENT_FLAG_SYNTHETIC;
    motionEvent.stage = event->stage;
    motionEvent.source = event->source;
    motionEvent.x = -1;
    motionEvent.y = -1;
    motionEvent.modifier_state = CLUTTER_MODIFIER_MASK;
    motionEvent.axes = 0;
    motionEvent.device = event->device;
    
    return frame->eventHandler()->mouseMoved(PlatformMouseEvent(&motionEvent));
}

static WebKitWebView* webkit_web_view_real_create_web_view(WebKitWebView*, WebKitWebFrame*)
{
    return 0;
}

static gboolean webkit_web_view_real_can_take_focus(WebKitWebView*)
{
    notImplemented();
    return TRUE;
}

static void webkit_web_view_real_get_page_rect (WebKitWebView* webView, int* x, int* y, int* width, int* height)
{
    Frame *frame = core(webView)->mainFrame();
    
    *x = 0;
    *y = 0;
    *width = frame->view()->visibleWidth();
    *height = frame->view()->visibleHeight();
}

static void webkit_web_view_real_request_focus (WebKitWebView*)
{
    notImplemented();
}

static void webkit_web_view_focus(WebkitActor* actor)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(actor);
    Frame* frame = core(webView)->mainFrame();

    core(webView)->focusController()->setActive(frame);
}

static void webkit_web_view_real_lose_focus (WebKitWebView*)
{
    notImplemented();
}

static gboolean webkit_web_view_real_web_view_ready(WebKitWebView*)
{
    return FALSE;
}

static gboolean webkit_web_view_real_close_web_view(WebKitWebView*)
{
    return FALSE;
}

static WebKitNavigationResponse webkit_web_view_real_navigation_requested(WebKitWebView*, WebKitWebFrame*, WebKitNetworkRequest*)
{
    return WEBKIT_NAVIGATION_RESPONSE_ACCEPT;
}

static void webkit_web_view_real_window_object_cleared(WebKitWebView*, WebKitWebFrame*, JSGlobalContextRef context, JSObjectRef window_object)
{
    notImplemented();
}

static gchar* webkit_web_view_real_choose_file(WebKitWebView*, WebKitWebFrame*, const gchar* old_name)
{
    notImplemented();
    return g_strdup(old_name);
}

typedef enum {
    WEBKIT_SCRIPT_DIALOG_ALERT,
    WEBKIT_SCRIPT_DIALOG_CONFIRM,
    WEBKIT_SCRIPT_DIALOG_PROMPT
 } WebKitScriptDialogType;

static void webkit_web_view_real_load_committed(WebKitWebView *webView, WebKitWebFrame *frame)
{
    // Reset zoom
    core(webView)->mainFrame()->setPageZoomFactor(1.0);
}

#if 0
static gboolean webkit_web_view_script_dialog(WebKitWebView* webView, WebKitWebFrame* frame, const gchar* message, WebKitScriptDialogType type, const gchar* defaultValue, gchar** value)
{
    return FALSE;
}
#endif

static gboolean webkit_web_view_real_script_alert(WebKitWebView* webView, WebKitWebFrame* frame, const gchar* message)
{
    return TRUE;
}

static gboolean webkit_web_view_real_script_confirm(WebKitWebView* webView, WebKitWebFrame* frame, const gchar* message, gboolean* didConfirm)
{
    *didConfirm = FALSE;
    return TRUE;
}

static gboolean webkit_web_view_real_script_prompt(WebKitWebView* webView, WebKitWebFrame* frame, const gchar* message, const gchar* defaultValue, gchar** value)
{

    *value = NULL;
    return TRUE;
}

static gboolean webkit_web_view_real_console_message(WebKitWebView* webView, const gchar* message, unsigned int line, const gchar* sourceId)
{
    g_message("console message: %s @%d: %s\n", sourceId, line, message);
    return TRUE;
}

static void webkit_web_view_real_select_all(WebKitWebView* webView)
{
    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    frame->editor()->command("SelectAll").execute();
}

static void webkit_web_view_real_cut_clipboard(WebKitWebView* webView)
{
    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    frame->editor()->command("Cut").execute();
}

static void webkit_web_view_real_copy_clipboard(WebKitWebView* webView)
{
    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    frame->editor()->command("Copy").execute();
}

static void webkit_web_view_real_undo(WebKitWebView* webView)
{
    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    frame->editor()->command("Undo").execute();
}

static void webkit_web_view_real_redo(WebKitWebView* webView)
{
    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    frame->editor()->command("Redo").execute();
}

static void webkit_web_view_real_paste_clipboard(WebKitWebView* webView)
{
    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    frame->editor()->command("Paste").execute();
}

static void webkit_web_view_dispose(GObject* object)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(object);
    WebKitWebViewPrivate* priv = webView->priv;

    priv->disposing = TRUE;
    
    if (priv->zoom_timeline) {
        clutter_timeline_stop (priv->zoom_timeline);
        g_object_unref (priv->zoom_timeline);
    }
    
    priv->zoom_node.clear();

    // These smart pointers are cleared manually, because some cleanup operations are
    // very sensitive to their value. We may crash if these are done in the wrong order.
//    priv->horizontalAdjustment.clear();
//    priv->verticalAdjustment.clear();
    priv->backForwardList.clear();

    if (priv->corePage) {
        webkit_web_view_stop_loading(WEBKIT_WEB_VIEW(object));
        core(priv->mainFrame)->loader()->detachFromParent();
        delete priv->corePage;
        priv->corePage = 0;
    }

    if (priv->webSettings) {
        g_signal_handlers_disconnect_by_func(priv->webSettings.get(), (gpointer)webkit_web_view_settings_notify, webView);
        priv->webSettings.clear();
    }

    priv->webInspector.clear();
    priv->webWindowFeatures.clear();
    priv->mainResource.clear();
    priv->subResources.clear();

//    HashMap<GdkDragContext*, DroppingContext*>::iterator endDroppingContexts = priv->droppingContexts.end();
//    for (HashMap<GdkDragContext*, DroppingContext*>::iterator iter = priv->droppingContexts.begin(); iter != endDroppingContexts; ++iter)
//        delete (iter->second);
//    priv->droppingContexts.clear();

    G_OBJECT_CLASS(webkit_web_view_parent_class)->dispose(object);
}

static void webkit_web_view_finalize(GObject* object)
{
    // We need to manually call the destructor here, since this object's memory is managed
    // by GLib. This calls all C++ members' destructors and prevents memory leaks.
    WEBKIT_WEB_VIEW(object)->priv->~WebKitWebViewPrivate();
    G_OBJECT_CLASS(webkit_web_view_parent_class)->finalize(object);
}

static gboolean webkit_signal_accumulator_object_handled(GSignalInvocationHint* ihint, GValue* returnAccu, const GValue* handlerReturn, gpointer dummy)
{
    gpointer newWebView = g_value_get_object(handlerReturn);
    g_value_set_object(returnAccu, newWebView);

    // Continue if we don't have a newWebView
    return !newWebView;
}

static gboolean webkit_navigation_request_handled(GSignalInvocationHint* ihint, GValue* returnAccu, const GValue* handlerReturn, gpointer dummy)
{
    WebKitNavigationResponse navigationResponse = (WebKitNavigationResponse)g_value_get_enum(handlerReturn);
    g_value_set_enum(returnAccu, navigationResponse);

    if (navigationResponse != WEBKIT_NAVIGATION_RESPONSE_ACCEPT)
        return FALSE;

    return TRUE;
}

//static AtkObject* webkit_web_view_get_accessible(GtkWidget* widget)
//{
//     WebKitWebView* webView = WEBKIT_WEB_VIEW(widget);
//     if (!core(webView))
//         return NULL;
// 
//     AXObjectCache::enableAccessibility();
// 
//     Frame* coreFrame = core(webView)->mainFrame();
//     if (!coreFrame)
//         return NULL;
// 
//     Document* doc = coreFrame->document();
//     if (!doc)
//         return NULL;
// 
//     AccessibilityObject* coreAccessible = doc->axObjectCache()->getOrCreate(doc->renderer());
//     if (!coreAccessible || !coreAccessible->wrapper())
//         return NULL;
// 
//     return coreAccessible->wrapper();
// }

static gdouble webViewGetDPI(WebKitWebView *webView)
{
    WebKitWebViewPrivate* priv = webView->priv;
    WebKitWebSettings* webSettings = priv->webSettings.get();
    gboolean enforce96DPI;
    g_object_get(webSettings, "enforce-96-dpi", &enforce96DPI, NULL);
    if (enforce96DPI)
        return 96.0;

    gdouble DPI = defaultDPI;
    ClutterBackend* backend = clutter_get_default_backend();
    if (backend) {
        DPI = clutter_backend_get_resolution(clutter_get_default_backend());
        if (DPI == -1)
            DPI = defaultDPI;
    }
    ASSERT(DPI > 0);
    return DPI;
}

static void webkit_web_view_screen_changed(ClutterActor* widget)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(widget);
    WebKitWebViewPrivate* priv = webView->priv;

    if (priv->disposing)
        return;

    WebKitWebSettings* webSettings = priv->webSettings.get();
    Settings* settings = core(webView)->settings();
    gdouble DPI = webViewGetDPI(webView);

    guint defaultFontSize, defaultMonospaceFontSize, minimumFontSize, minimumLogicalFontSize;

    g_object_get(webSettings,
                 "default-font-size", &defaultFontSize,
                 "default-monospace-font-size", &defaultMonospaceFontSize,
                 "minimum-font-size", &minimumFontSize,
                 "minimum-logical-font-size", &minimumLogicalFontSize,
                 NULL);

    settings->setDefaultFontSize(defaultFontSize / 72.0 * DPI);
    settings->setDefaultFixedFontSize(defaultMonospaceFontSize / 72.0 * DPI);
    settings->setMinimumFontSize(minimumFontSize / 72.0 * DPI);
    settings->setMinimumLogicalFontSize(minimumLogicalFontSize / 72.0 * DPI);
}

//static IntPoint globalPointForClientPoint(GdkWindow* window, const IntPoint& clientPoint)
//{
//    int x, y;
//    gdk_window_get_origin(window, &x, &y);
//    return clientPoint + IntSize(x, y);
//}

static void webkit_web_view_paint(ClutterActor* actor)
{   
    WebKitWebView* webView = WEBKIT_WEB_VIEW(actor);
    WebKitWebViewPrivate* priv = webView->priv;

    //printf("zoom_x: %d, zoom_y: %d\n", priv->zoom_x, priv->zoom_y);

    gfloat width, height;
    ClutterActorBox box;

    clutter_actor_get_allocation_box(actor, &box);

    width = box.x2 - box.x1;
    height = box.y2 - box.y1;
    cogl_clip_push_rectangle (0, 0, width, height);
    
    cogl_scale (priv->zoom_factor, priv->zoom_factor, 1.0);
    cogl_translate (-priv->zoom_x, -priv->zoom_y, 0);

    CLUTTER_ACTOR_CLASS(webkit_web_view_parent_class)->paint(actor);

    HashSet<ClutterActor*> children = priv->children;
    HashSet<ClutterActor*>::const_iterator end = children.end();
    for (HashSet<ClutterActor*>::const_iterator current = children.begin(); current != end; ++current) {
        if (CLUTTER_ACTOR_IS_VISIBLE(*current)) {
            clutter_actor_paint(*current);
        }
    }
    
    cogl_clip_pop();
}

static void
webkit_web_view_allocate (ClutterActor          *actor,
                          const ClutterActorBox *box,
                          ClutterAllocationFlags flags)
{
    gint                  width, height;
    WebKitWebView        *webView = WEBKIT_WEB_VIEW (actor);
    WebKitWebViewPrivate *priv = webView->priv;

    width  = int (box->x2 - box->x1);
    height = int (box->y2 - box->y1);
    
    // Set the size of the texture
    // if (priv->slide_init >= 0)
    //     clutter_cairo_surface_resize (CLUTTER_CAIRO (actor),
    //                                   width  + priv->slide_width,
    //                                   height + priv->slide_height);
    // else
        clutter_cairo_texture_set_surface_size (CLUTTER_CAIRO_TEXTURE (actor), width, height);

    // Set the size of the web view
    Frame* frame = core(webView)->mainFrame();

    if (!frame->view()) {
	CLUTTER_ACTOR_CLASS (webkit_web_view_parent_class)->allocate(actor, box, flags);
	return;
    }

    frame->view()->resize(width, height);
    frame->view()->forceLayout();
    frame->view()->adjustViewSize();
    
    CLUTTER_ACTOR_CLASS (webkit_web_view_parent_class)->allocate (actor, box, flags);

    // Redraw page
    //frame->view()->addToDirtyRegion(frame->view()->windowClipRect());
    //frame->view()->updateBackingStore();
}

static void webkit_web_view_class_init(WebKitWebViewClass* webViewClass)
{
    //GtkBindingSet* binding_set;

    webkit_init();

    /*
     * Signals
     */

    /**
     * WebKitWebView::create-web-view:
     * @web_view: the object on which the signal is emitted
     * @frame: the #WebKitWebFrame
     *
     * Emitted when the creation of a new window is requested.
     * If this signal is handled the signal handler should return the
     * newly created #WebKitWebView.
     *
     * The new #WebKitWebView should not be displayed to the user
     * until the #WebKitWebView::web-view-ready signal is emitted.
     *
     * The signal handlers should not try to deal with the reference count for
     * the new #WebKitWebView. The widget to which the widget is added will
     * handle that.
     *
     * Return value: (transfer full): a newly allocated #WebKitWebView, or %NULL
     *
     * Since: 1.0.3
     */
    webkit_web_view_signals[CREATE_WEB_VIEW] = g_signal_new("create-web-view",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (WebKitWebViewClass, create_web_view),
            webkit_signal_accumulator_object_handled,
            NULL,
            webkit_marshal_OBJECT__OBJECT,
            WEBKIT_TYPE_WEB_VIEW , 1,
            WEBKIT_TYPE_WEB_FRAME);

    /**
     * WebKitWebView::web-view-ready:
     * @web_view: the object on which the signal is emitted
     *
     * Emitted after #WebKitWebView::create-web-view when the new #WebKitWebView
     * should be displayed to the user. When this signal is emitted
     * all the information about how the window should look, including
     * size, position, whether the location, status and scroll bars
     * should be displayed, is already set on the
     * #WebKitWebWindowFeatures object contained by the #WebKitWebView.
     *
     * Notice that some of that information may change during the life
     * time of the window, so you may want to connect to the ::notify
     * signal of the #WebKitWebWindowFeatures object to handle those.
     *
     * Return value: %TRUE to stop handlers from being invoked for the event or
     * %FALSE to propagate the event furter
     *
     * Since: 1.0.3
     */
    webkit_web_view_signals[WEB_VIEW_READY] = g_signal_new("web-view-ready",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (WebKitWebViewClass, web_view_ready),
            g_signal_accumulator_true_handled,
            NULL,
            webkit_marshal_BOOLEAN__VOID,
            G_TYPE_BOOLEAN, 0);

    /**
     * WebKitWebView::close-web-view:
     * @web_view: the object on which the signal is emitted
     *
     * Emitted when closing a #WebKitWebView is requested. This occurs when a
     * call is made from JavaScript's window.close function. The default
     * signal handler does not do anything. It is the owner's responsibility
     * to hide or delete the web view, if necessary.
     *
     * Return value: %TRUE to stop handlers from being invoked for the event or
     * %FALSE to propagate the event furter
     *
     * Since: 1.1.11
     */
    webkit_web_view_signals[CLOSE_WEB_VIEW] = g_signal_new("close-web-view",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (WebKitWebViewClass, close_web_view),
            g_signal_accumulator_true_handled,
            NULL,
            webkit_marshal_BOOLEAN__VOID,
            G_TYPE_BOOLEAN, 0);

    /**
     * WebKitWebView::navigation-requested:
     * @web_view: the object on which the signal is emitted
     * @frame: the #WebKitWebFrame that required the navigation
     * @request: a #WebKitNetworkRequest
     *
     * Emitted when @frame requests a navigation to another page.
     *
     * Return value: a #WebKitNavigationResponse
     *
     * Deprecated: Use WebKitWebView::navigation-policy-decision-requested
     * instead
     */
    webkit_web_view_signals[NAVIGATION_REQUESTED] = g_signal_new("navigation-requested",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (WebKitWebViewClass, navigation_requested),
            webkit_navigation_request_handled,
            NULL,
            webkit_marshal_ENUM__OBJECT_OBJECT,
            WEBKIT_TYPE_NAVIGATION_RESPONSE, 2,
            WEBKIT_TYPE_WEB_FRAME,
            WEBKIT_TYPE_NETWORK_REQUEST);

    /**
     * WebKitWebView::new-window-policy-decision-requested:
     * @web_view: the object on which the signal is emitted
     * @frame: the #WebKitWebFrame that required the navigation
     * @request: a #WebKitNetworkRequest
     * @navigation_action: a #WebKitWebNavigationAction
     * @policy_decision: a #WebKitWebPolicyDecision
     *
     * Emitted when @frame requests opening a new window. With this
     * signal the browser can use the context of the request to decide
     * about the new window. If the request is not handled the default
     * behavior is to allow opening the new window to load the URI,
     * which will cause a create-web-view signal emission where the
     * browser handles the new window action but without information
     * of the context that caused the navigation. The following
     * navigation-policy-decision-requested emissions will load the
     * page after the creation of the new window just with the
     * information of this new navigation context, without any
     * information about the action that made this new window to be
     * opened.
     *
     * Notice that if you return TRUE, meaning that you handled the
     * signal, you are expected to have decided what to do, by calling
     * webkit_web_policy_decision_ignore(),
     * webkit_web_policy_decision_use(), or
     * webkit_web_policy_decision_download() on the @policy_decision
     * object.
     *
     * Return value: %TRUE if a decision was made, %FALSE to have the
     * default behavior apply
     *
     * Since: 1.1.4
     */
    webkit_web_view_signals[NEW_WINDOW_POLICY_DECISION_REQUESTED] =
        g_signal_new("new-window-policy-decision-requested",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)G_SIGNAL_RUN_LAST,
            0,
            g_signal_accumulator_true_handled,
            NULL,
            webkit_marshal_BOOLEAN__OBJECT_OBJECT_OBJECT_OBJECT,
            G_TYPE_BOOLEAN, 4,
            WEBKIT_TYPE_WEB_FRAME,
            WEBKIT_TYPE_NETWORK_REQUEST,
            WEBKIT_TYPE_WEB_NAVIGATION_ACTION,
            WEBKIT_TYPE_WEB_POLICY_DECISION);

    /**
     * WebKitWebView::navigation-policy-decision-requested:
     * @web_view: the object on which the signal is emitted
     * @frame: the #WebKitWebFrame that required the navigation
     * @request: a #WebKitNetworkRequest
     * @navigation_action: a #WebKitWebNavigationAction
     * @policy_decision: a #WebKitWebPolicyDecision
     *
     * Emitted when @frame requests a navigation to another page.
     * If this signal is not handled, the default behavior is to allow the
     * navigation.
     *
     * Notice that if you return TRUE, meaning that you handled the
     * signal, you are expected to have decided what to do, by calling
     * webkit_web_policy_decision_ignore(),
     * webkit_web_policy_decision_use(), or
     * webkit_web_policy_decision_download() on the @policy_decision
     * object.
     *
     * Return value: %TRUE if a decision was made, %FALSE to have the
     * default behavior apply
     *
     * Since: 1.0.3
     */
    webkit_web_view_signals[NAVIGATION_POLICY_DECISION_REQUESTED] = g_signal_new("navigation-policy-decision-requested",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)G_SIGNAL_RUN_LAST,
            0,
            g_signal_accumulator_true_handled,
            NULL,
            webkit_marshal_BOOLEAN__OBJECT_OBJECT_OBJECT_OBJECT,
            G_TYPE_BOOLEAN, 4,
            WEBKIT_TYPE_WEB_FRAME,
            WEBKIT_TYPE_NETWORK_REQUEST,
            WEBKIT_TYPE_WEB_NAVIGATION_ACTION,
            WEBKIT_TYPE_WEB_POLICY_DECISION);

    /**
     * WebKitWebView::mime-type-policy-decision-requested:
     * @web_view: the object on which the signal is emitted
     * @frame: the #WebKitWebFrame that required the policy decision
     * @request: a WebKitNetworkRequest
     * @mimetype: the MIME type attempted to load
     * @policy_decision: a #WebKitWebPolicyDecision
     *
     * Decide whether or not to display the given MIME type.  If this
     * signal is not handled, the default behavior is to show the
     * content of the requested URI if WebKit can show this MIME
     * type and the content disposition is not a download; if WebKit
     * is not able to show the MIME type nothing happens.
     *
     * Notice that if you return TRUE, meaning that you handled the
     * signal, you are expected to be aware of the "Content-Disposition"
     * header. A value of "attachment" usually indicates a download
     * regardless of the MIME type, see also
     * soup_message_headers_get_content_disposition(). And you must call
     * webkit_web_policy_decision_ignore(),
     * webkit_web_policy_decision_use(), or
     * webkit_web_policy_decision_download() on the @policy_decision
     * object.
     *
     * Return value: %TRUE if a decision was made, %FALSE to have the
     * default behavior apply
     *
     * Since: 1.0.3
     */
    webkit_web_view_signals[MIME_TYPE_POLICY_DECISION_REQUESTED] = g_signal_new("mime-type-policy-decision-requested",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)G_SIGNAL_RUN_LAST,
            0,
            g_signal_accumulator_true_handled,
            NULL,
            webkit_marshal_BOOLEAN__OBJECT_OBJECT_STRING_OBJECT,
            G_TYPE_BOOLEAN, 4,
            WEBKIT_TYPE_WEB_FRAME,
            WEBKIT_TYPE_NETWORK_REQUEST,
            G_TYPE_STRING,
            WEBKIT_TYPE_WEB_POLICY_DECISION);

    /**
     * WebKitWebView::window-object-cleared:
     * @web_view: the object on which the signal is emitted
     * @frame: the #WebKitWebFrame to which @window_object belongs
     * @context: the #JSGlobalContextRef holding the global object and other
     * execution state; equivalent to the return value of
     * webkit_web_frame_get_global_context(@frame)
     * @window_object: the #JSObjectRef representing the frame's JavaScript
     * window object
     *
     * Emitted when the JavaScript window object in a #WebKitWebFrame has been
     * cleared in preparation for a new load. This is the preferred place to
     * set custom properties on the window object using the JavaScriptCore API.
     */
    webkit_web_view_signals[WINDOW_OBJECT_CLEARED] = g_signal_new("window-object-cleared",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (WebKitWebViewClass, window_object_cleared),
            NULL,
            NULL,
            webkit_marshal_VOID__OBJECT_POINTER_POINTER,
            G_TYPE_NONE, 3,
            WEBKIT_TYPE_WEB_FRAME,
            G_TYPE_POINTER,
            G_TYPE_POINTER);

    /**
     * WebKitWebView::download-requested:
     * @web_view: the object on which the signal is emitted
     * @download: a #WebKitDownload object that lets you control the
     * download process
     *
     * A new Download is being requested. By default, if the signal is
     * not handled, the download is cancelled. If you handle the download
     * and call webkit_download_set_destination_uri(), it will be
     * started for you. If you need to set the destination asynchronously
     * you are responsible for starting or cancelling it yourself.
     *
     * If you intend to handle downloads yourself rather than using
     * the #WebKitDownload helper object you must handle this signal,
     * and return %FALSE.
     *
     * Also, keep in mind that the default policy for WebKitGTK+ is to
     * ignore files with a MIME type that it does not know how to
     * handle, which means this signal won't be emitted in the default
     * setup. One way to trigger downloads is to connect to
     * WebKitWebView::mime-type-policy-decision-requested and call
     * webkit_web_policy_decision_download() on the
     * #WebKitWebPolicyDecision in the parameter list for the kind of
     * files you want your application to download (a common solution
     * is to download anything that WebKit can't handle, which you can
     * figure out by using webkit_web_view_can_show_mime_type()).
     *
     * Return value: TRUE if the download should be performed, %FALSE to
     * cancel it
     *
     * Since: 1.1.2
     */
    webkit_web_view_signals[DOWNLOAD_REQUESTED] = g_signal_new("download-requested",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)G_SIGNAL_RUN_LAST,
            0,
            g_signal_accumulator_true_handled,
            NULL,
            webkit_marshal_BOOLEAN__OBJECT,
            G_TYPE_BOOLEAN, 1,
            G_TYPE_OBJECT);

    /**
     * WebKitWebView::load-started:
     * @web_view: the object on which the signal is emitted
     * @frame: the frame going to do the load
     *
     * When a #WebKitWebFrame begins to load this signal is emitted.
     *
     * Deprecated: Use the "load-status" property instead.
     */
    webkit_web_view_signals[LOAD_STARTED] = g_signal_new("load-started",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)G_SIGNAL_RUN_LAST,
            0,
            NULL,
            NULL,
            g_cclosure_marshal_VOID__OBJECT,
            G_TYPE_NONE, 1,
            WEBKIT_TYPE_WEB_FRAME);

    /**
     * WebKitWebView::load-committed:
     * @web_view: the object on which the signal is emitted
     * @frame: the main frame that received the first data
     *
     * When a #WebKitWebFrame loaded the first data this signal is emitted.
     *
     * Deprecated: Use the "load-status" property instead.
     */
    webkit_web_view_signals[LOAD_COMMITTED] = g_signal_new("load-committed",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            G_STRUCT_OFFSET(WebKitWebViewClass, load_committed),
            NULL,
            NULL,
            g_cclosure_marshal_VOID__OBJECT,
            G_TYPE_NONE, 1,
            WEBKIT_TYPE_WEB_FRAME);


    /**
     * WebKitWebView::load-progress-changed:
     * @web_view: the #WebKitWebView
     * @progress: the global progress
     *
     * Deprecated: Use the "progress" property instead.
     */
    webkit_web_view_signals[LOAD_PROGRESS_CHANGED] = g_signal_new("load-progress-changed",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)G_SIGNAL_RUN_LAST,
            0,
            NULL,
            NULL,
            g_cclosure_marshal_VOID__INT,
            G_TYPE_NONE, 1,
            G_TYPE_INT);

    /**
     * WebKitWebView::load-error
     * @web_view: the object on which the signal is emitted
     * @web_frame: the #WebKitWebFrame
     * @uri: the URI that triggered the error
     * @web_error: the #GError that was triggered
     *
     * An error occurred while loading. By default, if the signal is not
     * handled, the @web_view will display a stock error page. You need to
     * handle the signal if you want to provide your own error page.
     *
     * Since: 1.1.6
     *
     * Return value: %TRUE to stop other handlers from being invoked for the
     * event. %FALSE to propagate the event further.
     */
    webkit_web_view_signals[LOAD_ERROR] = g_signal_new("load-error",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST),
            0,
            g_signal_accumulator_true_handled,
            NULL,
            webkit_marshal_BOOLEAN__OBJECT_STRING_POINTER,
            G_TYPE_BOOLEAN, 3,
            WEBKIT_TYPE_WEB_FRAME,
            G_TYPE_STRING,
            G_TYPE_POINTER);

    /**
     * WebKitWebView::load-finished:
     * @web_view: the #WebKitWebView
     * @frame: the #WebKitWebFrame
     *
     * Deprecated: Use the "load-status" property instead.
     */
    webkit_web_view_signals[LOAD_FINISHED] = g_signal_new("webkit-load-finished",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)G_SIGNAL_RUN_LAST,
            0,
            NULL,
            NULL,
            g_cclosure_marshal_VOID__OBJECT,
            G_TYPE_NONE, 1,
            WEBKIT_TYPE_WEB_FRAME);

    /**
     * WebKitWebView::onload-event:
     * @web_view: the object on which the signal is emitted
     * @frame: the frame
     *
     * When a #WebKitWebFrame receives an onload event this signal is emitted.
     */
    webkit_web_view_signals[LOAD_STARTED] = g_signal_new("onload-event",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)G_SIGNAL_RUN_LAST,
            0,
            NULL,
            NULL,
            g_cclosure_marshal_VOID__OBJECT,
            G_TYPE_NONE, 1,
            WEBKIT_TYPE_WEB_FRAME);

    /**
     * WebKitWebView::title-changed:
     * @web_view: the object on which the signal is emitted
     * @frame: the main frame
     * @title: the new title
     *
     * When a #WebKitWebFrame changes the document title this signal is emitted.
     *
     * Deprecated: 1.1.4: Use "notify::title" instead.
     */
    webkit_web_view_signals[TITLE_CHANGED] = g_signal_new("title-changed",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)G_SIGNAL_RUN_LAST,
            0,
            NULL,
            NULL,
            webkit_marshal_VOID__OBJECT_STRING,
            G_TYPE_NONE, 2,
            WEBKIT_TYPE_WEB_FRAME,
            G_TYPE_STRING);

    /**
     * WebKitWebView::hovering-over-link:
     * @web_view: the object on which the signal is emitted
     * @title: the link's title
     * @uri: the URI the link points to
     *
     * When the cursor is over a link, this signal is emitted.
     */
    webkit_web_view_signals[HOVERING_OVER_LINK] = g_signal_new("hovering-over-link",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)G_SIGNAL_RUN_LAST,
            0,
            NULL,
            NULL,
            webkit_marshal_VOID__STRING_STRING,
            G_TYPE_NONE, 2,
            G_TYPE_STRING,
            G_TYPE_STRING);

    /**
     * WebKitWebView::populate-popup:
     * @web_view: the object on which the signal is emitted
     * @menu: the context menu
     *
     * When a context menu is about to be displayed this signal is emitted.
     *
     * Add menu items to #menu to extend the context menu.
     */
    webkit_web_view_signals[POPULATE_POPUP] = g_signal_new("populate-popup",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)G_SIGNAL_RUN_LAST,
            0,
            NULL,
            NULL,
            g_cclosure_marshal_VOID__OBJECT,
            G_TYPE_NONE, 1,
            G_TYPE_POINTER);

    /**
     * WebKitWebView::print-requested
     * @web_view: the object in which the signal is emitted
     * @web_frame: the frame that is requesting to be printed
     *
     * Emitted when printing is requested by the frame, usually
     * because of a javascript call. When handling this signal you
     * should call webkit_web_frame_print_full() or
     * webkit_web_frame_print() to do the actual printing.
     *
     * The default handler will present a print dialog and carry a
     * print operation. Notice that this means that if you intend to
     * ignore a print request you must connect to this signal, and
     * return %TRUE.
     *
     * Return value: %TRUE if the print request has been handled, %FALSE if
     * the default handler should run
     *
     * Since: 1.1.5
     */
    webkit_web_view_signals[PRINT_REQUESTED] = g_signal_new("print-requested",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)G_SIGNAL_RUN_LAST,
            0,
            g_signal_accumulator_true_handled,
            NULL,
            webkit_marshal_BOOLEAN__OBJECT,
            G_TYPE_BOOLEAN, 1,
            WEBKIT_TYPE_WEB_FRAME);

    webkit_web_view_signals[STATUS_BAR_TEXT_CHANGED] = g_signal_new("status-bar-text-changed",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)G_SIGNAL_RUN_LAST,
            0,
            NULL,
            NULL,
            g_cclosure_marshal_VOID__STRING,
            G_TYPE_NONE, 1,
            G_TYPE_STRING);

    /**
     * WebKitWebView::icon-loaded:
     * @web_view: the object on which the signal is emitted
     * @icon_uri: the URI for the icon
     *
     * This signal is emitted when the main frame has got a favicon.
     *
     * Since: 1.1.18
     */
    webkit_web_view_signals[ICON_LOADED] = g_signal_new("icon-loaded",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)G_SIGNAL_RUN_LAST,
            0,
            NULL,
            NULL,
            g_cclosure_marshal_VOID__STRING,
            G_TYPE_NONE, 1,
            G_TYPE_STRING);

    webkit_web_view_signals[SELECTION_CHANGED] = g_signal_new("selection-changed",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)G_SIGNAL_RUN_LAST,
            0,
            NULL,
            NULL,
            g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * WebKitWebView::console-message:
     * @web_view: the object on which the signal is emitted
     * @message: the message text
     * @line: the line where the error occured
     * @source_id: the source id
     *
     * A JavaScript console message was created.
     *
     * Return value: %TRUE to stop other handlers from being invoked for the
     * event. %FALSE to propagate the event further.
     */
    webkit_web_view_signals[CONSOLE_MESSAGE] = g_signal_new("console-message",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET(WebKitWebViewClass, console_message),
            g_signal_accumulator_true_handled,
            NULL,
            webkit_marshal_BOOLEAN__STRING_INT_STRING,
            G_TYPE_BOOLEAN, 3,
            G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING);

    /**
     * WebKitWebView::script-alert:
     * @web_view: the object on which the signal is emitted
     * @frame: the relevant frame
     * @message: the message text
     *
     * A JavaScript alert dialog was created.
     *
     * Return value: %TRUE to stop other handlers from being invoked for the
     * event. %FALSE to propagate the event further.
     */
    webkit_web_view_signals[SCRIPT_ALERT] = g_signal_new("script-alert",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET(WebKitWebViewClass, script_alert),
            g_signal_accumulator_true_handled,
            NULL,
            webkit_marshal_BOOLEAN__OBJECT_STRING,
            G_TYPE_BOOLEAN, 2,
            WEBKIT_TYPE_WEB_FRAME, G_TYPE_STRING);

    /**
     * WebKitWebView::script-confirm:
     * @web_view: the object on which the signal is emitted
     * @frame: the relevant frame
     * @message: the message text
     * @confirmed: whether the dialog has been confirmed
     *
     * A JavaScript confirm dialog was created, providing Yes and No buttons.
     *
     * Return value: %TRUE to stop other handlers from being invoked for the
     * event. %FALSE to propagate the event further.
     */
    webkit_web_view_signals[SCRIPT_CONFIRM] = g_signal_new("script-confirm",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET(WebKitWebViewClass, script_confirm),
            g_signal_accumulator_true_handled,
            NULL,
            webkit_marshal_BOOLEAN__OBJECT_STRING_POINTER,
            G_TYPE_BOOLEAN, 3,
            WEBKIT_TYPE_WEB_FRAME, G_TYPE_STRING, G_TYPE_POINTER);

    /**
     * WebKitWebView::script-prompt:
     * @web_view: the object on which the signal is emitted
     * @frame: the relevant frame
     * @message: the message text
     * @default: the default value
     * @text: To be filled with the return value or NULL if the dialog was cancelled.
     *
     * A JavaScript prompt dialog was created, providing an entry to input text.
     *
     * Return value: %TRUE to stop other handlers from being invoked for the
     * event. %FALSE to propagate the event further.
     */
    webkit_web_view_signals[SCRIPT_PROMPT] = g_signal_new("script-prompt",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET(WebKitWebViewClass, script_prompt),
            g_signal_accumulator_true_handled,
            NULL,
            webkit_marshal_BOOLEAN__OBJECT_STRING_STRING_STRING,
            G_TYPE_BOOLEAN, 4,
            WEBKIT_TYPE_WEB_FRAME, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);

    /**
     * WebKitWebView::select-all:
     * @web_view: the object which received the signal
     *
     * The #WebKitWebView::select-all signal is a keybinding signal which gets emitted to
     * select the complete contents of the text view.
     *
     * The default bindings for this signal is Ctrl-a.
     */
    webkit_web_view_signals[SELECT_ALL] = g_signal_new("select-all",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            G_STRUCT_OFFSET(WebKitWebViewClass, select_all),
            NULL, NULL,
            g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * WebKitWebView::cut-clipboard:
     * @web_view: the object which received the signal
     *
     * The #WebKitWebView::cut-clipboard signal is a keybinding signal which gets emitted to
     * cut the selection to the clipboard.
     *
     * The default bindings for this signal are Ctrl-x and Shift-Delete.
     */
    webkit_web_view_signals[CUT_CLIPBOARD] = g_signal_new("cut-clipboard",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            G_STRUCT_OFFSET(WebKitWebViewClass, cut_clipboard),
            NULL, NULL,
            g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * WebKitWebView::copy-clipboard:
     * @web_view: the object which received the signal
     *
     * The #WebKitWebView::copy-clipboard signal is a keybinding signal which gets emitted to
     * copy the selection to the clipboard.
     *
     * The default bindings for this signal are Ctrl-c and Ctrl-Insert.
     */
    webkit_web_view_signals[COPY_CLIPBOARD] = g_signal_new("copy-clipboard",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            G_STRUCT_OFFSET(WebKitWebViewClass, copy_clipboard),
            NULL, NULL,
            g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * WebKitWebView::paste-clipboard:
     * @web_view: the object which received the signal
     *
     * The #WebKitWebView::paste-clipboard signal is a keybinding signal which gets emitted to
     * paste the contents of the clipboard into the Web view.
     *
     * The default bindings for this signal are Ctrl-v and Shift-Insert.
     */
    webkit_web_view_signals[PASTE_CLIPBOARD] = g_signal_new("paste-clipboard",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            G_STRUCT_OFFSET(WebKitWebViewClass, paste_clipboard),
            NULL, NULL,
            g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * WebKitWebView::undo
     * @web_view: the object which received the signal
     *
     * The #WebKitWebView::undo signal is a keybinding signal which gets emitted to
     * undo the last editing command.
     *
     * The default binding for this signal is Ctrl-z
     *
     * Since: 1.1.14
     */
    webkit_web_view_signals[UNDO] = g_signal_new("undo",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            G_STRUCT_OFFSET(WebKitWebViewClass, undo),
            NULL, NULL,
            g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * WebKitWebView::redo
     * @web_view: the object which received the signal
     *
     * The #WebKitWebView::redo signal is a keybinding signal which gets emitted to
     * redo the last editing command.
     *
     * The default binding for this signal is Ctrl-Shift-z
     *
     * Since: 1.1.14
     */
    webkit_web_view_signals[REDO] = g_signal_new("redo",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            G_STRUCT_OFFSET(WebKitWebViewClass, redo),
            NULL, NULL,
            g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /**
     * WebKitWebView::move-cursor:
     * @web_view: the object which received the signal
     * @step: the type of movement, one of #GtkMovementStep
     * @count: an integer indicating the subtype of movement. Currently
     *         the permitted values are '1' = forward, '-1' = backwards.
     *
     * The #WebKitWebView::move-cursor will be emitted to apply the
     * cursor movement described by its parameters to the @view.
     *
     * Return value: %TRUE or %FALSE
     * 
     * Since: 1.1.4
     */
    // webkit_web_view_signals[MOVE_CURSOR] = g_signal_new("move-cursor",
    //         G_TYPE_FROM_CLASS(webViewClass),
    //         (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
    //         G_STRUCT_OFFSET(WebKitWebViewClass, move_cursor),
    //         NULL, NULL,
    //         webkit_marshal_BOOLEAN__ENUM_INT,
    //         G_TYPE_BOOLEAN, 2,
    //         GTK_TYPE_MOVEMENT_STEP,
    //         G_TYPE_INT);

    /**
     * WebKitWebView::create-plugin-widget:
     * @web_view: the object which received the signal
     * @mime_type: the mimetype of the requested object
     * @uri: the URI to load
     * @param: a #GHashTable with additional attributes (strings)
     *
     * The #WebKitWebView::create-plugin-widget signal will be emitted to
     * create a plugin widget for embed or object HTML tags. This
     * allows to embed a GtkWidget as a plugin into HTML content. In
     * case of a textual selection of the GtkWidget WebCore will attempt
     * to set the property value of "webkit-widget-is-selected". This can
     * be used to draw a visual indicator of the selection.
     *
     * Return value: (transfer full): a new #GtkWidget, or %NULL
     *
     * Since: 1.1.8
     */
    // webkit_web_view_signals[PLUGIN_WIDGET] = g_signal_new("create-plugin-widget",
    //         G_TYPE_FROM_CLASS(webViewClass),
    //         (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
    //         0,
    //         webkit_signal_accumulator_object_handled,
    //         NULL,
    //         webkit_marshal_OBJECT__STRING_STRING_POINTER,
    //         GTK_TYPE_WIDGET, 3,
    //         G_TYPE_STRING, G_TYPE_STRING, G_TYPE_HASH_TABLE);

    /**
     * WebKitWebView::database-quota-exceeded
     * @web_view: the object which received the signal
     * @frame: the relevant frame
     * @database: the #WebKitWebDatabase which exceeded the quota of its #WebKitSecurityOrigin
     *
     * The #WebKitWebView::database-quota-exceeded signal will be emitted when
     * a Web Database exceeds the quota of its security origin. This signal
     * may be used to increase the size of the quota before the originating
     * operation fails.
     *
     * Since: 1.1.14
     */
    webkit_web_view_signals[DATABASE_QUOTA_EXCEEDED] = g_signal_new("database-quota-exceeded",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            0,
            NULL, NULL,
            webkit_marshal_VOID__OBJECT_OBJECT,
            G_TYPE_NONE, 2,
            G_TYPE_OBJECT, G_TYPE_OBJECT);

    /**
     * WebKitWebView::resource-request-starting:
     * @web_view: the object which received the signal
     * @web_frame: the #WebKitWebFrame whose load dispatched this request
     * @web_resource: an empty #WebKitWebResource object
     * @request: the #WebKitNetworkRequest that will be dispatched
     * @response: the #WebKitNetworkResponse representing the redirect
     * response, if any
     *
     * Emitted when a request is about to be sent. You can modify the
     * request while handling this signal. You can set the URI in the
     * #WebKitNetworkRequest object itself, and add/remove/replace
     * headers using the #SoupMessage object it carries, if it is
     * present. See webkit_network_request_get_message(). Setting the
     * request URI to "about:blank" will effectively cause the request
     * to load nothing, and can be used to disable the loading of
     * specific resources.
     *
     * Notice that information about an eventual redirect is available
     * in @response's #SoupMessage, not in the #SoupMessage carried by
     * the @request. If @response is %NULL, then this is not a
     * redirected request.
     *
     * The #WebKitWebResource object will be the same throughout all
     * the lifetime of the resource, but the contents may change from
     * inbetween signal emissions.
     *
     * Since: 1.1.14
     */
    webkit_web_view_signals[RESOURCE_REQUEST_STARTING] = g_signal_new("resource-request-starting",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            0,
            NULL, NULL,
            webkit_marshal_VOID__OBJECT_OBJECT_OBJECT_OBJECT,
            G_TYPE_NONE, 4,
            WEBKIT_TYPE_WEB_FRAME,
            WEBKIT_TYPE_WEB_RESOURCE,
            WEBKIT_TYPE_NETWORK_REQUEST,
            WEBKIT_TYPE_NETWORK_RESPONSE);

    /**
     * WebKitWebView::geolocation-policy-decision-requested:
     * @web_view: the object on which the signal is emitted
     * @frame: the frame that requests permission
     * @policy_decision: a WebKitGeolocationPolicyDecision
     *
     * This signal is emitted when a @frame wants to obtain the user's
     * location. The decision can be made asynchronously, but you must
     * call g_object_ref() the @policy_decision, and return %TRUE if
     * you are going to handle the request. To actually make the
     * decision you need to call webkit_geolocation_policy_allow() or
     * webkit_geolocation_policy_deny() on @policy_decision.
     *
     * Since: 1.1.23
     */
    webkit_web_view_signals[GEOLOCATION_POLICY_DECISION_REQUESTED] = g_signal_new("geolocation-policy-decision-requested",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST),
            0,
            NULL, NULL,
            webkit_marshal_BOOLEAN__OBJECT_OBJECT,
            G_TYPE_BOOLEAN, 2,
            WEBKIT_TYPE_WEB_FRAME,
            WEBKIT_TYPE_GEOLOCATION_POLICY_DECISION);

    /**
     * WebKitWebView::geolocation-policy-decision-cancelled:
     * @web_view: the object on which the signal is emitted
     * @frame: the frame that cancels geolocation request.
     *
     * When a @frame wants to cancel geolocation permission it had requested
     * before.
     *
     * Since: 1.1.23
     */
    webkit_web_view_signals[GEOLOCATION_POLICY_DECISION_CANCELLED] = g_signal_new("geolocation-policy-decision-cancelled",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST),
            0,
            NULL, NULL,
            g_cclosure_marshal_VOID__OBJECT,
            G_TYPE_NONE, 1,
            WEBKIT_TYPE_WEB_FRAME);

    /*
     * DOM-related signals. These signals are experimental, for now,
     * and may change API and ABI. Their comments lack one * on
     * purpose, to make them not be catched by gtk-doc.
     */

    /*
     * WebKitWebView::document-load-finished
     * @web_view: the object which received the signal
     * @web_frame: the #WebKitWebFrame whose load dispatched this request
     *
     * Emitted when the DOM document object load is finished for the
     * given frame.
     */
    webkit_web_view_signals[DOCUMENT_LOAD_FINISHED] = g_signal_new("document-load-finished",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            0,
            NULL, NULL,
            g_cclosure_marshal_VOID__OBJECT,
            G_TYPE_NONE, 1,
            WEBKIT_TYPE_WEB_FRAME);

    /*
     * WebKitWebView::frame-created
     * @web_view: the object which received the signal
     * @web_frame: the #WebKitWebFrame which was just created.
     *
     * Emitted when a WebKitWebView has created a new frame. This signal will
     * be emitted for all sub-frames created during page load. It will not be
     * emitted for the main frame, which originates in the WebKitWebView constructor
     * and may be accessed at any time using webkit_web_view_get_main_frame.
     *
     * Since: 1.3.4
     */
    webkit_web_view_signals[FRAME_CREATED] = g_signal_new("frame-created",
            G_TYPE_FROM_CLASS(webViewClass),
            (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            0,
            NULL, NULL,
            g_cclosure_marshal_VOID__OBJECT,
            G_TYPE_NONE, 1,
            WEBKIT_TYPE_WEB_FRAME);

    /*
     * implementations of virtual methods
     */
    webViewClass->create_web_view = webkit_web_view_real_create_web_view;
    
    webViewClass->can_take_focus = webkit_web_view_real_can_take_focus;
    webViewClass->get_page_rect = webkit_web_view_real_get_page_rect;
    webViewClass->request_focus = webkit_web_view_real_request_focus;
    webViewClass->lose_focus = webkit_web_view_real_lose_focus;
    
    webViewClass->web_view_ready = webkit_web_view_real_web_view_ready;
    webViewClass->close_web_view = webkit_web_view_real_close_web_view;
    webViewClass->navigation_requested = webkit_web_view_real_navigation_requested;
    webViewClass->window_object_cleared = webkit_web_view_real_window_object_cleared;
    webViewClass->choose_file = webkit_web_view_real_choose_file;
    webViewClass->load_committed = webkit_web_view_real_load_committed;
    webViewClass->script_alert = webkit_web_view_real_script_alert;
    webViewClass->script_confirm = webkit_web_view_real_script_confirm;
    webViewClass->script_prompt = webkit_web_view_real_script_prompt;
    webViewClass->console_message = webkit_web_view_real_console_message;
    webViewClass->select_all = webkit_web_view_real_select_all;
    webViewClass->cut_clipboard = webkit_web_view_real_cut_clipboard;
    webViewClass->copy_clipboard = webkit_web_view_real_copy_clipboard;
    webViewClass->paste_clipboard = webkit_web_view_real_paste_clipboard;
    webViewClass->undo = webkit_web_view_real_undo;
    webViewClass->redo = webkit_web_view_real_redo;
//    webViewClass->move_cursor = webkit_web_view_real_move_cursor;

    GObjectClass* objectClass = G_OBJECT_CLASS(webViewClass);
    objectClass->dispose = webkit_web_view_dispose;
    objectClass->finalize = webkit_web_view_finalize;
    objectClass->get_property = webkit_web_view_get_property;
    objectClass->set_property = webkit_web_view_set_property;

    

    WebkitActorClass *webkitClass = WEBKIT_ACTOR_CLASS(webViewClass);
    webkitClass->expose = webkit_web_view_expose;
    webkitClass->focus = webkit_web_view_focus;

    ClutterActorClass *actorClass = CLUTTER_ACTOR_CLASS(webViewClass);
    actorClass->button_press_event = webkit_web_view_button_press_event;
    actorClass->button_release_event = webkit_web_view_button_release_event;
    actorClass->scroll_event = webkit_web_view_scroll_event;
    actorClass->motion_event = webkit_web_view_motion_event;
    actorClass->key_press_event = webkit_web_view_key_press_event;
    actorClass->key_release_event = webkit_web_view_key_release_event;
    actorClass->enter_event = webkit_web_view_enter_event;
    actorClass->leave_event = webkit_web_view_leave_event;
    actorClass->paint = webkit_web_view_paint;
//    actorClass->pick = webkit_web_view_pick;
    actorClass->allocate = webkit_web_view_allocate;

    /*
     * make us scrollable (e.g. addable to a GtkScrolledWindow)
     */
    // webViewClass->set_scroll_adjustments = webkit_web_view_set_scroll_adjustments;
    // GTK_WIDGET_CLASS(webViewClass)->set_scroll_adjustments_signal = g_signal_new("set-scroll-adjustments",
    //         G_TYPE_FROM_CLASS(webViewClass),
    //         (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
    //         G_STRUCT_OFFSET(WebKitWebViewClass, set_scroll_adjustments),
    //         NULL, NULL,
    //         webkit_marshal_VOID__OBJECT_OBJECT,
    //         G_TYPE_NONE, 2,
    //         GTK_TYPE_ADJUSTMENT, GTK_TYPE_ADJUSTMENT);

    // /*
    //  * Key bindings
    //  */
    // 
    // binding_set = gtk_binding_set_by_class(webViewClass);
    // 
    // gtk_binding_entry_add_signal(binding_set, GDK_a, GDK_CONTROL_MASK,
    //                              "select_all", 0);
    // 
    // /* Cut/copy/paste */
    // 
    // gtk_binding_entry_add_signal(binding_set, GDK_x, GDK_CONTROL_MASK,
    //                              "cut_clipboard", 0);
    // gtk_binding_entry_add_signal(binding_set, GDK_c, GDK_CONTROL_MASK,
    //                              "copy_clipboard", 0);
    // gtk_binding_entry_add_signal(binding_set, GDK_v, GDK_CONTROL_MASK,
    //                              "paste_clipboard", 0);
    // gtk_binding_entry_add_signal(binding_set, GDK_z, GDK_CONTROL_MASK,
    //                              "undo", 0);
    // gtk_binding_entry_add_signal(binding_set, GDK_z, static_cast<GdkModifierType>(GDK_CONTROL_MASK | GDK_SHIFT_MASK),
    //                              "redo", 0);
    // 
    // gtk_binding_entry_add_signal(binding_set, GDK_Delete, GDK_SHIFT_MASK,
    //                              "cut_clipboard", 0);
    // gtk_binding_entry_add_signal(binding_set, GDK_Insert, GDK_CONTROL_MASK,
    //                              "copy_clipboard", 0);
    // gtk_binding_entry_add_signal(binding_set, GDK_Insert, GDK_SHIFT_MASK,
    //                              "paste_clipboard", 0);
    // 
    // /* Movement */
    // 
    // gtk_binding_entry_add_signal(binding_set, GDK_Down, static_cast<GdkModifierType>(0),
    //                              "move-cursor", 2,
    //                              G_TYPE_ENUM, GTK_MOVEMENT_DISPLAY_LINES,
    //                              G_TYPE_INT, 1);
    // gtk_binding_entry_add_signal(binding_set, GDK_Up, static_cast<GdkModifierType>(0),
    //                              "move-cursor", 2,
    //                              G_TYPE_ENUM, GTK_MOVEMENT_DISPLAY_LINES,
    //                              G_TYPE_INT, -1);
    // gtk_binding_entry_add_signal(binding_set, GDK_Right, static_cast<GdkModifierType>(0),
    //                              "move-cursor", 2,
    //                              G_TYPE_ENUM, GTK_MOVEMENT_VISUAL_POSITIONS,
    //                              G_TYPE_INT, 1);
    // gtk_binding_entry_add_signal(binding_set, GDK_Left, static_cast<GdkModifierType>(0),
    //                              "move-cursor", 2,
    //                              G_TYPE_ENUM, GTK_MOVEMENT_VISUAL_POSITIONS,
    //                              G_TYPE_INT, -1);
    // gtk_binding_entry_add_signal(binding_set, GDK_space, static_cast<GdkModifierType>(0),
    //                              "move-cursor", 2,
    //                              G_TYPE_ENUM, GTK_MOVEMENT_PAGES,
    //                              G_TYPE_INT, 1);
    // gtk_binding_entry_add_signal(binding_set, GDK_space, GDK_SHIFT_MASK,
    //                              "move-cursor", 2,
    //                              G_TYPE_ENUM, GTK_MOVEMENT_PAGES,
    //                              G_TYPE_INT, -1);
    // gtk_binding_entry_add_signal(binding_set, GDK_Page_Down, static_cast<GdkModifierType>(0),
    //                              "move-cursor", 2,
    //                              G_TYPE_ENUM, GTK_MOVEMENT_PAGES,
    //                              G_TYPE_INT, 1);
    // gtk_binding_entry_add_signal(binding_set, GDK_Page_Up, static_cast<GdkModifierType>(0),
    //                              "move-cursor", 2,
    //                              G_TYPE_ENUM, GTK_MOVEMENT_PAGES,
    //                              G_TYPE_INT, -1);
    // gtk_binding_entry_add_signal(binding_set, GDK_End, static_cast<GdkModifierType>(0),
    //                              "move-cursor", 2,
    //                              G_TYPE_ENUM, GTK_MOVEMENT_BUFFER_ENDS,
    //                              G_TYPE_INT, 1);
    // gtk_binding_entry_add_signal(binding_set, GDK_Home, static_cast<GdkModifierType>(0),
    //                              "move-cursor", 2,
    //                              G_TYPE_ENUM, GTK_MOVEMENT_BUFFER_ENDS,
    //                              G_TYPE_INT, -1);

    /*
     * properties
     */

    /**
    * WebKitWebView:title:
    *
    * Returns the @web_view's document title.
    *
    * Since: 1.1.4
    */
    g_object_class_install_property(objectClass, PROP_TITLE,
                                    g_param_spec_string("title",
                                                        _("Title"),
                                                        _("Returns the @web_view's document title"),
                                                        NULL,
                                                        WEBKIT_PARAM_READABLE));

    /**
    * WebKitWebView:uri:
    *
    * Returns the current URI of the contents displayed by the @web_view.
    *
    * Since: 1.1.4
    */
    g_object_class_install_property(objectClass, PROP_URI,
                                    g_param_spec_string("uri",
                                                        _("URI"),
                                                        _("Returns the current URI of the contents displayed by the @web_view"),
                                                        NULL,
                                                        WEBKIT_PARAM_READABLE));

    // /**
    // * WebKitWebView:copy-target-list:
    // *
    // * The list of targets this web view supports for clipboard copying.
    // *
    // * Since: 1.0.2
    // */
    // g_object_class_install_property(objectClass, PROP_COPY_TARGET_LIST,
    //                                 g_param_spec_boxed("copy-target-list",
    //                                                    _("Copy target list"),
    //                                                    _("The list of targets this web view supports for clipboard copying"),
    //                                                    GTK_TYPE_TARGET_LIST,
    //                                                    WEBKIT_PARAM_READABLE));
    // 
    // /**
    // * WebKitWebView:paste-target-list:
    // *
    // * The list of targets this web view supports for clipboard pasting.
    // *
    // * Since: 1.0.2
    // */
    // g_object_class_install_property(objectClass, PROP_PASTE_TARGET_LIST,
    //                                 g_param_spec_boxed("paste-target-list",
    //                                                    _("Paste target list"),
    //                                                    _("The list of targets this web view supports for clipboard pasting"),
    //                                                    GTK_TYPE_TARGET_LIST,
    //                                                    WEBKIT_PARAM_READABLE));

    g_object_class_install_property(objectClass, PROP_SETTINGS,
                                    g_param_spec_object("settings",
                                                        _("Settings"),
                                                        _("An associated WebKitWebSettings instance"),
                                                        WEBKIT_TYPE_WEB_SETTINGS,
                                                        WEBKIT_PARAM_READWRITE));

    /**
    * WebKitWebView:web-inspector:
    *
    * The associated WebKitWebInspector instance.
    *
    * Since: 1.0.3
    */
    g_object_class_install_property(objectClass, PROP_WEB_INSPECTOR,
                                    g_param_spec_object("web-inspector",
                                                        _("Web Inspector"),
                                                        _("The associated WebKitWebInspector instance"),
                                                        WEBKIT_TYPE_WEB_INSPECTOR,
                                                        WEBKIT_PARAM_READABLE));

    /**
    * WebKitWebView:window-features:
    *
    * An associated WebKitWebWindowFeatures instance.
    *
    * Since: 1.0.3
    */
    g_object_class_install_property(objectClass, PROP_WINDOW_FEATURES,
                                    g_param_spec_object("window-features",
                                                        "Window Features",
                                                        "An associated WebKitWebWindowFeatures instance",
                                                        WEBKIT_TYPE_WEB_WINDOW_FEATURES,
                                                        WEBKIT_PARAM_READWRITE));

    g_object_class_install_property(objectClass, PROP_EDITABLE,
                                    g_param_spec_boolean("editable",
                                                         _("Editable"),
                                                         _("Whether content can be modified by the user"),
                                                         FALSE,
                                                         WEBKIT_PARAM_READWRITE));

    g_object_class_install_property(objectClass, PROP_TRANSPARENT,
                                    g_param_spec_boolean("transparent",
                                                         _("Transparent"),
                                                         _("Whether content has a transparent background"),
                                                         FALSE,
                                                         WEBKIT_PARAM_READWRITE));

    /**
    * WebKitWebView:zoom-level:
    *
    * The level of zoom of the content.
    *
    * Since: 1.0.1
    */
    g_object_class_install_property(objectClass, PROP_ZOOM_LEVEL,
                                    g_param_spec_float("zoom-level",
                                                       _("Zoom level"),
                                                       _("The level of zoom of the content"),
                                                       G_MINFLOAT,
                                                       G_MAXFLOAT,
                                                       1.0f,
                                                       WEBKIT_PARAM_READWRITE));

    /**
    * WebKitWebView:full-content-zoom:
    *
    * Whether the full content is scaled when zooming.
    *
    * Since: 1.0.1
    */
    g_object_class_install_property(objectClass, PROP_FULL_CONTENT_ZOOM,
                                    g_param_spec_boolean("full-content-zoom",
                                                         _("Full content zoom"),
                                                         _("Whether the full content is scaled when zooming"),
                                                         FALSE,
                                                         WEBKIT_PARAM_READWRITE));

    /**
     * WebKitWebView:encoding:
     *
     * The default encoding of the web view.
     *
     * Since: 1.1.2
     */
    g_object_class_install_property(objectClass, PROP_ENCODING,
                                    g_param_spec_string("encoding",
                                                        _("Encoding"),
                                                        _("The default encoding of the web view"),
                                                        NULL,
                                                        WEBKIT_PARAM_READABLE));

    /**
     * WebKitWebView:custom-encoding:
     *
     * The custom encoding of the web view.
     *
     * Since: 1.1.2
     */
    g_object_class_install_property(objectClass, PROP_CUSTOM_ENCODING,
                                    g_param_spec_string("custom-encoding",
                                                        _("Custom Encoding"),
                                                        _("The custom encoding of the web view"),
                                                        NULL,
                                                        WEBKIT_PARAM_READWRITE));

    /**
    * WebKitWebView:load-status:
    *
    * Determines the current status of the load.
    *
    * Connect to "notify::load-status" to monitor loading.
    *
    * Some versions of WebKitGTK+ emitted this signal for the default
    * error page, while loading it. This behavior was considered bad,
    * because it was essentially exposing an implementation
    * detail. From 1.1.19 onwards this signal is no longer emitted for
    * the default error pages, but keep in mind that if you override
    * the error pages by using webkit_web_frame_load_alternate_string()
    * the signals will be emitted.
    *
    * Since: 1.1.7
    */
    g_object_class_install_property(objectClass, PROP_LOAD_STATUS,
                                    g_param_spec_enum("load-status",
                                                      "Load Status",
                                                      "Determines the current status of the load",
                                                      WEBKIT_TYPE_LOAD_STATUS,
                                                      WEBKIT_LOAD_FINISHED,
                                                      WEBKIT_PARAM_READABLE));

    /**
    * WebKitWebView:progress:
    *
    * Determines the current progress of the load.
    *
    * Since: 1.1.7
    */
    g_object_class_install_property(objectClass, PROP_PROGRESS,
                                    g_param_spec_double("progress",
                                                        "Progress",
                                                        "Determines the current progress of the load",
                                                        0.0, 1.0, 1.0,
                                                        WEBKIT_PARAM_READABLE));

    /**
     * WebKitWebView:icon-uri:
     *
     * The URI for the favicon for the #WebKitWebView.
     *
     * Since: 1.1.18
     */
    g_object_class_install_property(objectClass, PROP_ICON_URI,
                                    g_param_spec_string("icon-uri",
                                                        _("Icon URI"),
                                                        _("The URI for the favicon for the #WebKitWebView."),
                                                        NULL,
                                                        WEBKIT_PARAM_READABLE));
    /**
    * WebKitWebView:im-context:
    *
    * The GtkIMMulticontext for the #WebKitWebView.
    *
    * This is the input method context used for all text entry widgets inside
    * the #WebKitWebView. It can be used to generate context menu items for
    * controlling the active input method.
    *
    * Since: 1.1.20
    */
    // g_object_class_install_property(objectClass, PROP_IM_CONTEXT,
    //                                 g_param_spec_object("im-context",
    //                                                     "IM Context",
    //                                                     "The GtkIMMultiContext for the #WebKitWebView.",
    //                                                     GTK_TYPE_IM_CONTEXT,
    //                                                     WEBKIT_PARAM_READABLE));

    /**
    * WebKitWebView:view-mode:
    *
    * The "view-mode" media feature for the #WebKitWebView.
    *
    * The "view-mode" media feature is additional information for web
    * applications about how the application is running, when it comes
    * to user experience. Whether the application is running inside a
    * regular browser window, in a dedicated window, fullscreen, for
    * instance.
    *
    * This property stores a %WebKitWebViewViewMode value that matches
    * the "view-mode" media feature the web application will see.
    *
    * See http://www.w3.org/TR/view-mode/ for more information.
    *
    * Since: 1.3.4
    */
    g_object_class_install_property(objectClass, PROP_VIEW_MODE,
                                    g_param_spec_enum("view-mode",
                                                      "View Mode",
                                                      "The view-mode media feature for the #WebKitWebView.",
                                                      WEBKIT_TYPE_WEB_VIEW_VIEW_MODE,
                                                      WEBKIT_WEB_VIEW_VIEW_MODE_WINDOWED,
                                                      WEBKIT_PARAM_READWRITE));

    g_object_class_install_property(objectClass, PROP_ZOOM_PADDING,
                                    g_param_spec_int("zoom-padding",
                                                     "Element-zoom padding",
                                                     "Padding, in pixels, to frame zoomed elements with",
                                                     0, G_MAXINT, 16,
                                                     WEBKIT_PARAM_READWRITE));

    g_object_class_install_property(objectClass, PROP_TRANSITION_TIME,
                                    g_param_spec_uint("transition-time",
                                                     "Transition time",
                                                     "The time to spend between transitions, in milliseconds",
                                                     0, G_MAXUINT, 150,
                                                     WEBKIT_PARAM_READWRITE));
                                                     
    g_type_class_add_private(webViewClass, sizeof(WebKitWebViewPrivate));
}

static void webkit_web_view_real_add(ClutterContainer* container, ClutterActor* actor)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(container);
    WebKitWebViewPrivate* priv = webView->priv;

    g_object_ref (actor);
    priv->children.add(actor);
    clutter_actor_set_parent(actor, CLUTTER_ACTOR(container));
    g_object_unref (actor);
}

static void webkit_web_view_real_remove(ClutterContainer* container, ClutterActor* actor)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(container);
    WebKitWebViewPrivate* priv = webView->priv;

    g_object_ref(actor);
    
    priv->children.remove(actor);
    if(CLUTTER_ACTOR_IS_VISIBLE(CLUTTER_ACTOR(container))) {
	clutter_actor_queue_redraw(CLUTTER_ACTOR(container));
    }

    g_object_unref(actor);
}

static void webkit_web_view_real_foreach(ClutterContainer* container, ClutterCallback callback, gpointer callbackData)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(container);
    WebKitWebViewPrivate* priv = webView->priv;

    HashSet<ClutterActor*> children = priv->children;
    HashSet<ClutterActor*>::const_iterator end = children.end();
    for (HashSet<ClutterActor*>::const_iterator current = children.begin(); current != end; ++current) {
	(*callback)(*current, callbackData);
    }
}

static void webkit_web_view_real_raise(ClutterContainer* container, ClutterActor* actor, ClutterActor* sibling)
{
}

static void webkit_web_view_real_lower(ClutterContainer* container, ClutterActor* actor, ClutterActor* sibling)
{
}

static void webkit_web_view_real_sort_depth_order(ClutterContainer* container)
{
}

static void webkit_web_view_container_iface_init (ClutterContainerIface *iface)
{
    iface->add = webkit_web_view_real_add;
    iface->remove = webkit_web_view_real_remove;
    iface->foreach = webkit_web_view_real_foreach;
    iface->raise = webkit_web_view_real_raise;
    iface->lower = webkit_web_view_real_lower;
    iface->sort_depth_order = webkit_web_view_real_sort_depth_order;
}

static void webkit_web_view_update_settings(WebKitWebView* webView)
{
    WebKitWebViewPrivate* priv = webView->priv;
    WebKitWebSettings* webSettings = priv->webSettings.get();
    Settings* settings = core(webView)->settings();

    gchar* defaultEncoding, *cursiveFontFamily, *defaultFontFamily, *fantasyFontFamily, *monospaceFontFamily, *sansSerifFontFamily, *serifFontFamily, *userStylesheetUri;
    gboolean autoLoadImages, autoShrinkImages, printBackgrounds,
        enableScripts, enablePlugins, enableDeveloperExtras, resizableTextAreas,
        enablePrivateBrowsing, enableCaretBrowsing, enableHTML5Database, enableHTML5LocalStorage,
        enableXSSAuditor, enableSpatialNavigation, javascriptCanOpenWindows,
        javaScriptCanAccessClipboard, enableOfflineWebAppCache,
        enableUniversalAccessFromFileURI, enableFileAccessFromFileURI,
        enableDOMPaste, tabKeyCyclesThroughElements,
        enableSiteSpecificQuirks, usePageCache, enableJavaApplet, enableHyperlinkAuditing;

    WebKitEditingBehavior editingBehavior;

    g_object_get(webSettings,
                 "default-encoding", &defaultEncoding,
                 "cursive-font-family", &cursiveFontFamily,
                 "default-font-family", &defaultFontFamily,
                 "fantasy-font-family", &fantasyFontFamily,
                 "monospace-font-family", &monospaceFontFamily,
                 "sans-serif-font-family", &sansSerifFontFamily,
                 "serif-font-family", &serifFontFamily,
                 "auto-load-images", &autoLoadImages,
                 "auto-shrink-images", &autoShrinkImages,
                 "print-backgrounds", &printBackgrounds,
                 "enable-scripts", &enableScripts,
                 "enable-plugins", &enablePlugins,
                 "resizable-text-areas", &resizableTextAreas,
                 "user-stylesheet-uri", &userStylesheetUri,
                 "enable-developer-extras", &enableDeveloperExtras,
                 "enable-private-browsing", &enablePrivateBrowsing,
                 "enable-caret-browsing", &enableCaretBrowsing,
                 "enable-html5-database", &enableHTML5Database,
                 "enable-html5-local-storage", &enableHTML5LocalStorage,
                 "enable-xss-auditor", &enableXSSAuditor,
                 "enable-spatial-navigation", &enableSpatialNavigation,
                 "javascript-can-open-windows-automatically", &javascriptCanOpenWindows,
                 "javascript-can-access-clipboard", &javaScriptCanAccessClipboard,
                 "enable-offline-web-application-cache", &enableOfflineWebAppCache,
                 "editing-behavior", &editingBehavior,
                 "enable-universal-access-from-file-uris", &enableUniversalAccessFromFileURI,
                 "enable-file-access-from-file-uris", &enableFileAccessFromFileURI,
                 "enable-dom-paste", &enableDOMPaste,
                 "tab-key-cycles-through-elements", &tabKeyCyclesThroughElements,
                 "enable-site-specific-quirks", &enableSiteSpecificQuirks,
                 "enable-page-cache", &usePageCache,
                 "enable-java-applet", &enableJavaApplet,
                 "enable-hyperlink-auditing", &enableHyperlinkAuditing,
                 NULL);

    settings->setDefaultTextEncodingName(defaultEncoding);
    settings->setCursiveFontFamily(cursiveFontFamily);
    settings->setStandardFontFamily(defaultFontFamily);
    settings->setFantasyFontFamily(fantasyFontFamily);
    settings->setFixedFontFamily(monospaceFontFamily);
    settings->setSansSerifFontFamily(sansSerifFontFamily);
    settings->setSerifFontFamily(serifFontFamily);
    settings->setLoadsImagesAutomatically(autoLoadImages);
    settings->setShrinksStandaloneImagesToFit(autoShrinkImages);
    settings->setShouldPrintBackgrounds(printBackgrounds);
    settings->setJavaScriptEnabled(enableScripts);
    settings->setPluginsEnabled(enablePlugins);
    settings->setTextAreasAreResizable(resizableTextAreas);
    settings->setUserStyleSheetLocation(KURL(KURL(), userStylesheetUri));
    settings->setDeveloperExtrasEnabled(enableDeveloperExtras);
    settings->setPrivateBrowsingEnabled(enablePrivateBrowsing);
    settings->setCaretBrowsingEnabled(enableCaretBrowsing);
#if ENABLE(DATABASE)
    AbstractDatabase::setIsAvailable(enableHTML5Database);
#endif
    settings->setLocalStorageEnabled(enableHTML5LocalStorage);
    settings->setXSSAuditorEnabled(enableXSSAuditor);
    settings->setSpatialNavigationEnabled(enableSpatialNavigation);
    settings->setJavaScriptCanOpenWindowsAutomatically(javascriptCanOpenWindows);
    settings->setJavaScriptCanAccessClipboard(javaScriptCanAccessClipboard);
    settings->setOfflineWebApplicationCacheEnabled(enableOfflineWebAppCache);
    settings->setEditingBehaviorType(core(editingBehavior));
    settings->setAllowUniversalAccessFromFileURLs(enableUniversalAccessFromFileURI);
    settings->setAllowFileAccessFromFileURLs(enableFileAccessFromFileURI);
    settings->setDOMPasteAllowed(enableDOMPaste);
    settings->setNeedsSiteSpecificQuirks(enableSiteSpecificQuirks);
    settings->setUsesPageCache(usePageCache);
    settings->setJavaEnabled(enableJavaApplet);
    settings->setHyperlinkAuditingEnabled(enableHyperlinkAuditing);

    Page* page = core(webView);
    if (page)
        page->setTabKeyCyclesThroughElements(tabKeyCyclesThroughElements);

    g_free(defaultEncoding);
    g_free(cursiveFontFamily);
    g_free(defaultFontFamily);
    g_free(fantasyFontFamily);
    g_free(monospaceFontFamily);
    g_free(sansSerifFontFamily);
    g_free(serifFontFamily);
    g_free(userStylesheetUri);

    webkit_web_view_screen_changed(CLUTTER_ACTOR(webView));
}

static inline gint pixelsFromSize(WebKitWebView* webView, gint size)
{
    gdouble DPI = webViewGetDPI(webView);
    return size / 72.0 * DPI;
}

static void webkit_web_view_settings_notify(WebKitWebSettings* webSettings, GParamSpec* pspec, WebKitWebView* webView)
{
    Settings* settings = core(webView)->settings();

    const gchar* name = g_intern_string(pspec->name);
    GValue value = { 0, { { 0 } } };
    g_value_init(&value, pspec->value_type);
    g_object_get_property(G_OBJECT(webSettings), name, &value);

    if (name == g_intern_string("default-encoding"))
        settings->setDefaultTextEncodingName(g_value_get_string(&value));
    else if (name == g_intern_string("cursive-font-family"))
        settings->setCursiveFontFamily(g_value_get_string(&value));
    else if (name == g_intern_string("default-font-family"))
        settings->setStandardFontFamily(g_value_get_string(&value));
    else if (name == g_intern_string("fantasy-font-family"))
        settings->setFantasyFontFamily(g_value_get_string(&value));
    else if (name == g_intern_string("monospace-font-family"))
        settings->setFixedFontFamily(g_value_get_string(&value));
    else if (name == g_intern_string("sans-serif-font-family"))
        settings->setSansSerifFontFamily(g_value_get_string(&value));
    else if (name == g_intern_string("serif-font-family"))
        settings->setSerifFontFamily(g_value_get_string(&value));
    else if (name == g_intern_string("default-font-size"))
        settings->setDefaultFontSize(pixelsFromSize(webView, g_value_get_int(&value)));
    else if (name == g_intern_string("default-monospace-font-size"))
        settings->setDefaultFixedFontSize(pixelsFromSize(webView, g_value_get_int(&value)));
    else if (name == g_intern_string("minimum-font-size"))
        settings->setMinimumFontSize(pixelsFromSize(webView, g_value_get_int(&value)));
    else if (name == g_intern_string("minimum-logical-font-size"))
        settings->setMinimumLogicalFontSize(pixelsFromSize(webView, g_value_get_int(&value)));
    else if (name == g_intern_string("enforce-96-dpi"))
        webkit_web_view_screen_changed(CLUTTER_ACTOR(webView));
    else if (name == g_intern_string("auto-load-images"))
        settings->setLoadsImagesAutomatically(g_value_get_boolean(&value));
    else if (name == g_intern_string("auto-shrink-images"))
        settings->setShrinksStandaloneImagesToFit(g_value_get_boolean(&value));
    else if (name == g_intern_string("print-backgrounds"))
        settings->setShouldPrintBackgrounds(g_value_get_boolean(&value));
    else if (name == g_intern_string("enable-scripts"))
        settings->setJavaScriptEnabled(g_value_get_boolean(&value));
    else if (name == g_intern_string("enable-plugins"))
        settings->setPluginsEnabled(g_value_get_boolean(&value));
    else if (name == g_intern_string("resizable-text-areas"))
        settings->setTextAreasAreResizable(g_value_get_boolean(&value));
    else if (name == g_intern_string("user-stylesheet-uri"))
        settings->setUserStyleSheetLocation(KURL(KURL(), g_value_get_string(&value)));
    else if (name == g_intern_string("enable-developer-extras"))
        settings->setDeveloperExtrasEnabled(g_value_get_boolean(&value));
    else if (name == g_intern_string("enable-private-browsing"))
        settings->setPrivateBrowsingEnabled(g_value_get_boolean(&value));
    else if (name == g_intern_string("enable-caret-browsing"))
        settings->setCaretBrowsingEnabled(g_value_get_boolean(&value));
#if ENABLE(DATABASE)
    else if (name == g_intern_string("enable-html5-database")) {
        AbstractDatabase::setIsAvailable(g_value_get_boolean(&value));
    }
#endif
    else if (name == g_intern_string("enable-html5-local-storage"))
        settings->setLocalStorageEnabled(g_value_get_boolean(&value));
    else if (name == g_intern_string("enable-xss-auditor"))
        settings->setXSSAuditorEnabled(g_value_get_boolean(&value));
    else if (name == g_intern_string("enable-spatial-navigation"))
        settings->setSpatialNavigationEnabled(g_value_get_boolean(&value));
    else if (name == g_intern_string("javascript-can-open-windows-automatically"))
        settings->setJavaScriptCanOpenWindowsAutomatically(g_value_get_boolean(&value));
    else if (name == g_intern_string("javascript-can-access-clipboard"))
        settings->setJavaScriptCanAccessClipboard(g_value_get_boolean(&value));
    else if (name == g_intern_string("enable-offline-web-application-cache"))
        settings->setOfflineWebApplicationCacheEnabled(g_value_get_boolean(&value));
    else if (name == g_intern_string("editing-behavior"))
        settings->setEditingBehaviorType(core(static_cast<WebKitEditingBehavior>(g_value_get_enum(&value))));
    else if (name == g_intern_string("enable-universal-access-from-file-uris"))
        settings->setAllowUniversalAccessFromFileURLs(g_value_get_boolean(&value));
    else if (name == g_intern_string("enable-file-access-from-file-uris"))
        settings->setAllowFileAccessFromFileURLs(g_value_get_boolean(&value));
    else if (name == g_intern_string("enable-dom-paste"))
        settings->setDOMPasteAllowed(g_value_get_boolean(&value));
    else if (name == g_intern_string("tab-key-cycles-through-elements")) {
        Page* page = core(webView);
        if (page)
            page->setTabKeyCyclesThroughElements(g_value_get_boolean(&value));
    } else if (name == g_intern_string("enable-site-specific-quirks"))
        settings->setNeedsSiteSpecificQuirks(g_value_get_boolean(&value));
    else if (name == g_intern_string("enable-page-cache"))
        settings->setUsesPageCache(g_value_get_boolean(&value));
    else if (name == g_intern_string("enable-java-applet"))
        settings->setJavaEnabled(g_value_get_boolean(&value));
    else if (name == g_intern_string("enable-hyperlink-auditing"))
        settings->setHyperlinkAuditingEnabled(g_value_get_boolean(&value));
    else if (!g_object_class_find_property(G_OBJECT_GET_CLASS(webSettings), name))
        g_warning("Unexpected setting '%s'", name);
    g_value_unset(&value);
}

static void webkit_web_view_init(WebKitWebView* webView)
{
    WebKitWebViewPrivate* priv = WEBKIT_WEB_VIEW_GET_PRIVATE(webView);
    webView->priv = priv;
    // This is the placement new syntax: http://www.parashift.com/c++-faq-lite/dtors.html#faq-11.10
    // It allows us to call a constructor on manually allocated locations in memory. We must use it
    // in this case, because GLib manages the memory for the private data section, but we wish it
    // to contain C++ object members. The use of placement new calls the constructor on all C++ data
    // members, which ensures they are initialized properly.
    new (priv) WebKitWebViewPrivate();

//    priv->imContext = adoptPlatformRef(gtk_im_multicontext_new());

    Page::PageClients pageClients;
    pageClients.chromeClient = new WebKit::ChromeClient(webView);
    pageClients.contextMenuClient = new WebKit::ContextMenuClient(webView);
    pageClients.editorClient = new WebKit::EditorClient(webView);
    pageClients.dragClient = new WebKit::DragClient(webView);
    pageClients.inspectorClient = new WebKit::InspectorClient(webView);
    priv->corePage = new Page(pageClients);

    // We also add a simple wrapper class to provide the public
    // interface for the Web Inspector.
    priv->webInspector = adoptPlatformRef(WEBKIT_WEB_INSPECTOR(g_object_new(WEBKIT_TYPE_WEB_INSPECTOR, NULL)));
    webkit_web_inspector_set_inspector_client(priv->webInspector.get(), priv->corePage);

    // The smart pointer will call g_object_ref_sink on these adjustments.
//    priv->horizontalAdjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
//    priv->verticalAdjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0));

//    gtk_widget_set_can_focus(GTK_WIDGET(webView), TRUE);
    priv->mainFrame = WEBKIT_WEB_FRAME(webkit_web_frame_new(webView));
    priv->lastPopupXPosition = priv->lastPopupYPosition = -1;
    priv->editable = false;

    priv->backForwardList = adoptPlatformRef(webkit_web_back_forward_list_new_with_web_view(webView));

    priv->zoomFullContent = FALSE;

    priv->webSettings = adoptPlatformRef(webkit_web_settings_new());
    webkit_web_view_update_settings(webView);
    g_signal_connect(priv->webSettings.get(), "notify", G_CALLBACK(webkit_web_view_settings_notify), webView);

    priv->webWindowFeatures = adoptPlatformRef(webkit_web_window_features_new());

    priv->subResources = adoptPlatformRef(g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref));

    priv->currentClickCount = 0;
    priv->previousClickButton = 0;
    priv->previousClickTime = 0;
    
    priv->transition_time = 150;
    
    priv->zoom_factor = 1.0;
    priv->zoom_pad = 16;
//    gtk_drag_dest_set(GTK_WIDGET(webView), static_cast<GtkDestDefaults>(0), 0, 0, static_cast<GdkDragAction>(GDK_ACTION_COPY | GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK | GDK_ACTION_PRIVATE));
//    gtk_drag_dest_set_target_list(GTK_WIDGET(webView), pasteboardHelperInstance()->targetList());
}

ClutterActor* webkit_web_view_new(guint width, guint height)
{
    WebKitWebView* webView = WEBKIT_WEB_VIEW(g_object_new(WEBKIT_TYPE_WEB_VIEW,
                                                          "surface-width", width,
                                                          "surface-height", height, NULL));
    clutter_actor_set_size (CLUTTER_ACTOR (webView), width, height);
    
    return CLUTTER_ACTOR(webView);
}

// for internal use only
void webkit_web_view_notify_ready(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    gboolean isHandled = FALSE;
    g_signal_emit(webView, webkit_web_view_signals[WEB_VIEW_READY], 0, &isHandled);
}

void webkit_web_view_request_download(WebKitWebView* webView, WebKitNetworkRequest* request, const ResourceResponse& response, ResourceHandle* handle)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    WebKitDownload* download;

    if (handle)
        download = webkit_download_new_with_handle(request, handle, response);
    else
        download = webkit_download_new(request);

    gboolean handled;
    g_signal_emit(webView, webkit_web_view_signals[DOWNLOAD_REQUESTED], 0, download, &handled);

    if (!handled) {
        webkit_download_cancel(download);
        g_object_unref(download);
        return;
    }

    /* Start the download now if it has a destination URI, otherwise it
        may be handled asynchronously by the application. */
    if (webkit_download_get_destination_uri(download))
        webkit_download_start(download);
}

bool webkit_web_view_use_primary_for_paste(WebKitWebView* webView)
{
    return webView->priv->usePrimaryForPaste;
}

/**
 * webkit_web_view_set_settings:
 * @web_view: a #WebKitWebView
 * @settings: (transfer none): the #WebKitWebSettings to be set
 *
 * Replaces the #WebKitWebSettings instance that is currently attached
 * to @web_view with @settings. The reference held by the @web_view on
 * the old #WebKitWebSettings instance is dropped, and the reference
 * count of @settings is inscreased.
 *
 * The settings are automatically applied to @web_view.
 */
void webkit_web_view_set_settings(WebKitWebView* webView, WebKitWebSettings* webSettings)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));
    g_return_if_fail(WEBKIT_IS_WEB_SETTINGS(webSettings));

    WebKitWebViewPrivate* priv = webView->priv;
    g_signal_handlers_disconnect_by_func(priv->webSettings.get(), (gpointer)webkit_web_view_settings_notify, webView);
    priv->webSettings = webSettings;
    webkit_web_view_update_settings(webView);
    g_signal_connect(webSettings, "notify", G_CALLBACK(webkit_web_view_settings_notify), webView);
    g_object_notify(G_OBJECT(webView), "settings");
}

/**
 * webkit_web_view_get_settings:
 * @web_view: a #WebKitWebView
 *
 * Obtains the #WebKitWebSettings associated with the
 * #WebKitWebView. The #WebKitWebView always has an associated
 * instance of #WebKitWebSettings. The reference that is returned by
 * this call is owned by the #WebKitWebView. You may need to increase
 * its reference count if you intend to keep it alive for longer than
 * the #WebKitWebView.
 *
 * Return value: (transfer none): the #WebKitWebSettings instance
 */
WebKitWebSettings* webkit_web_view_get_settings(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), 0);
    return webView->priv->webSettings.get();
}

/**
 * webkit_web_view_get_inspector:
 * @web_view: a #WebKitWebView
 *
 * Obtains the #WebKitWebInspector associated with the
 * #WebKitWebView. Every #WebKitWebView object has a
 * #WebKitWebInspector object attached to it as soon as it is created,
 * so this function will only return NULL if the argument is not a
 * valid #WebKitWebView.
 *
 * Return value: (transfer none): the #WebKitWebInspector instance.
 *
 * Since: 1.0.3
 */
WebKitWebInspector* webkit_web_view_get_inspector(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), 0);
    return webView->priv->webInspector.get();
}

// internal
static void webkit_web_view_set_window_features(WebKitWebView* webView, WebKitWebWindowFeatures* webWindowFeatures)
{
    if (!webWindowFeatures)
      return;
    if (webkit_web_window_features_equal(webView->priv->webWindowFeatures.get(), webWindowFeatures))
      return;
    webView->priv->webWindowFeatures = webWindowFeatures;
}

/**
 * webkit_web_view_get_window_features:
 * @web_view: a #WebKitWebView
 *
 * Returns the instance of #WebKitWebWindowFeatures held by the given
 * #WebKitWebView.
 *
 * Return value: (transfer none): the #WebKitWebWindowFeatures
 *
 * Since: 1.0.3
 */
WebKitWebWindowFeatures* webkit_web_view_get_window_features(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), 0);
    return webView->priv->webWindowFeatures.get();
}

/**
 * webkit_web_view_get_title:
 * @web_view: a #WebKitWebView
 *
 * Returns the @web_view's document title
 *
 * Since: 1.1.4
 *
 * Return value: the title of @web_view
 */
G_CONST_RETURN gchar* webkit_web_view_get_title(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), NULL);

    WebKitWebViewPrivate* priv = webView->priv;
    return priv->mainFrame->priv->title;
}

/**
 * webkit_web_view_get_uri:
 * @web_view: a #WebKitWebView
 *
 * Returns the current URI of the contents displayed by the @web_view
 *
 * Since: 1.1.4
 *
 * Return value: the URI of @web_view
 */
G_CONST_RETURN gchar* webkit_web_view_get_uri(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), NULL);

    WebKitWebViewPrivate* priv = webView->priv;
    return priv->mainFrame->priv->uri;
}

/**
 * webkit_web_view_set_maintains_back_forward_list:
 * @web_view: a #WebKitWebView
 * @flag: to tell the view to maintain a back or forward list
 *
 * Set the view to maintain a back or forward list of history items.
 */
void webkit_web_view_set_maintains_back_forward_list(WebKitWebView* webView, gboolean flag)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    core(webView)->backForwardList()->setEnabled(flag);
}

/**
 * webkit_web_view_get_back_forward_list:
 * @web_view: a #WebKitWebView
 *
 * Obtains the #WebKitWebBackForwardList associated with the given #WebKitWebView. The
 * #WebKitWebBackForwardList is owned by the #WebKitWebView.
 *
 * Return value: (transfer none): the #WebKitWebBackForwardList
 */
WebKitWebBackForwardList* webkit_web_view_get_back_forward_list(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), 0);
    if (!core(webView) || !core(webView)->backForwardList()->enabled())
        return 0;
    return webView->priv->backForwardList.get();
}

/**
 * webkit_web_view_go_to_back_forward_item:
 * @web_view: a #WebKitWebView
 * @item: a #WebKitWebHistoryItem*
 *
 * Go to the specified #WebKitWebHistoryItem
 *
 * Return value: %TRUE if loading of item is successful, %FALSE if not
 */
gboolean webkit_web_view_go_to_back_forward_item(WebKitWebView* webView, WebKitWebHistoryItem* item)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);
    g_return_val_if_fail(WEBKIT_IS_WEB_HISTORY_ITEM(item), FALSE);

    WebKitWebBackForwardList* backForwardList = webkit_web_view_get_back_forward_list(webView);
    if (!webkit_web_back_forward_list_contains_item(backForwardList, item))
        return FALSE;

    core(webView)->goToItem(core(item), FrameLoadTypeIndexedBackForward);
    return TRUE;
}

/**
 * webkit_web_view_go_back:
 * @web_view: a #WebKitWebView
 *
 * Loads the previous history item.
 */
void webkit_web_view_go_back(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    core(webView)->goBack();
}

/**
 * webkit_web_view_go_back_or_forward:
 * @web_view: a #WebKitWebView
 * @steps: the number of steps
 *
 * Loads the history item that is the number of @steps away from the current
 * item. Negative values represent steps backward while positive values
 * represent steps forward.
 */
void webkit_web_view_go_back_or_forward(WebKitWebView* webView, gint steps)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    core(webView)->goBackOrForward(steps);
}

/**
 * webkit_web_view_go_forward:
 * @web_view: a #WebKitWebView
 *
 * Loads the next history item.
 */
void webkit_web_view_go_forward(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    core(webView)->goForward();
}

/**
 * webkit_web_view_can_go_back:
 * @web_view: a #WebKitWebView
 *
 * Determines whether #web_view has a previous history item.
 *
 * Return value: %TRUE if able to move back, %FALSE otherwise
 */
gboolean webkit_web_view_can_go_back(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    if (!core(webView) || !core(webView)->backForwardList()->backItem())
        return FALSE;

    return TRUE;
}

/**
 * webkit_web_view_can_go_back_or_forward:
 * @web_view: a #WebKitWebView
 * @steps: the number of steps
 *
 * Determines whether #web_view has a history item of @steps. Negative values
 * represent steps backward while positive values represent steps forward.
 *
 * Return value: %TRUE if able to move back or forward the given number of
 * steps, %FALSE otherwise
 */
gboolean webkit_web_view_can_go_back_or_forward(WebKitWebView* webView, gint steps)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    return core(webView)->canGoBackOrForward(steps);
}

/**
 * webkit_web_view_can_go_forward:
 * @web_view: a #WebKitWebView
 *
 * Determines whether #web_view has a next history item.
 *
 * Return value: %TRUE if able to move forward, %FALSE otherwise
 */
gboolean webkit_web_view_can_go_forward(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    Page* page = core(webView);

    if (!page)
        return FALSE;

    if (!page->backForwardList()->forwardItem())
        return FALSE;

    return TRUE;
}

/**
 * webkit_web_view_open:
 * @web_view: a #WebKitWebView
 * @uri: an URI
 *
 * Requests loading of the specified URI string.
 *
 * Deprecated: 1.1.1: Use webkit_web_view_load_uri() instead.
  */
void webkit_web_view_open(WebKitWebView* webView, const gchar* uri)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));
    g_return_if_fail(uri);

    // We used to support local paths, unlike the newer
    // function webkit_web_view_load_uri
    if (g_path_is_absolute(uri)) {
        gchar* fileUri = g_filename_to_uri(uri, NULL, NULL);
        webkit_web_view_load_uri(webView, fileUri);
        g_free(fileUri);
    }
    else
        webkit_web_view_load_uri(webView, uri);
}

void webkit_web_view_reload(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    core(webView)->mainFrame()->loader()->reload();
}

/**
 * webkit_web_view_reload_bypass_cache:
 * @web_view: a #WebKitWebView
 *
 * Reloads the @web_view without using any cached data.
 *
 * Since: 1.0.3
 */
void webkit_web_view_reload_bypass_cache(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    core(webView)->mainFrame()->loader()->reload(true);
}

/**
 * webkit_web_view_load_uri:
 * @web_view: a #WebKitWebView
 * @uri: an URI string
 *
 * Requests loading of the specified URI string.
 *
 * Since: 1.1.1
 */
void webkit_web_view_load_uri(WebKitWebView* webView, const gchar* uri)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));
    g_return_if_fail(uri);

    WebKitWebFrame* frame = webView->priv->mainFrame;
    webkit_web_frame_load_uri(frame, uri);
}

/**
+  * webkit_web_view_load_string:
+  * @web_view: a #WebKitWebView
+  * @content: an URI string
+  * @mime_type: the MIME type, or %NULL
+  * @encoding: the encoding, or %NULL
+  * @base_uri: the base URI for relative locations
+  *
+  * Requests loading of the given @content with the specified @mime_type,
+  * @encoding and @base_uri.
+  *
+  * If @mime_type is %NULL, "text/html" is assumed.
+  *
+  * If @encoding is %NULL, "UTF-8" is assumed.
+  */
void webkit_web_view_load_string(WebKitWebView* webView, const gchar* content, const gchar* mimeType, const gchar* encoding, const gchar* baseUri)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));
    g_return_if_fail(content);

    WebKitWebFrame* frame = webView->priv->mainFrame;
    webkit_web_frame_load_string(frame, content, mimeType, encoding, baseUri);
}
/**
 * webkit_web_view_load_html_string:
 * @web_view: a #WebKitWebView
 * @content: an URI string
 * @base_uri: the base URI for relative locations
 *
 * Requests loading of the given @content with the specified @base_uri.
 *
 * Deprecated: 1.1.1: Use webkit_web_view_load_string() instead.
 */
void webkit_web_view_load_html_string(WebKitWebView* webView, const gchar* content, const gchar* baseUri)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));
    g_return_if_fail(content);

    webkit_web_view_load_string(webView, content, NULL, NULL, baseUri);
}

/**
 * webkit_web_view_load_request:
 * @web_view: a #WebKitWebView
 * @request: a #WebKitNetworkRequest
 *
 * Requests loading of the specified asynchronous client request.
 *
 * Creates a provisional data source that will transition to a committed data
 * source once any data has been received. Use webkit_web_view_stop_loading() to
 * stop the load.
 *
 * Since: 1.1.1
 */
void webkit_web_view_load_request(WebKitWebView* webView, WebKitNetworkRequest* request)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));
    g_return_if_fail(WEBKIT_IS_NETWORK_REQUEST(request));

    WebKitWebFrame* frame = webView->priv->mainFrame;
    webkit_web_frame_load_request(frame, request);
}

/**
 * webkit_web_view_stop_loading:
 * @webView: a #WebKitWebView
 * 
 * Stops any ongoing load in the @webView.
 **/
void webkit_web_view_stop_loading(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    Frame* frame = core(webView)->mainFrame();

    if (FrameLoader* loader = frame->loader())
        loader->stopForUserCancel();
}

/**
 * webkit_web_view_search_text:
 * @web_view: a #WebKitWebView
 * @text: a string to look for
 * @forward: whether to find forward or not
 * @case_sensitive: whether to respect the case of text
 * @wrap: whether to continue looking at the beginning after reaching the end
 *
 * Looks for a specified string inside #web_view.
 *
 * Return value: %TRUE on success or %FALSE on failure
 */
gboolean webkit_web_view_search_text(WebKitWebView* webView, const gchar* string, gboolean caseSensitive, gboolean forward, gboolean shouldWrap)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);
    g_return_val_if_fail(string, FALSE);

    TextCaseSensitivity caseSensitivity = caseSensitive ? TextCaseSensitive : TextCaseInsensitive;
    FindDirection direction = forward ? FindDirectionForward : FindDirectionBackward;

    return core(webView)->findString(String::fromUTF8(string), caseSensitivity, direction, shouldWrap);
}

/**
 * webkit_web_view_mark_text_matches:
 * @web_view: a #WebKitWebView
 * @string: a string to look for
 * @case_sensitive: whether to respect the case of text
 * @limit: the maximum number of strings to look for or 0 for all
 *
 * Attempts to highlight all occurances of #string inside #web_view.
 *
 * Return value: the number of strings highlighted
 */
guint webkit_web_view_mark_text_matches(WebKitWebView* webView, const gchar* string, gboolean caseSensitive, guint limit)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), 0);
    g_return_val_if_fail(string, 0);

    TextCaseSensitivity caseSensitivity = caseSensitive ? TextCaseSensitive : TextCaseInsensitive;

    return core(webView)->markAllMatchesForText(String::fromUTF8(string), caseSensitivity, false, limit);
}

/**
 * webkit_web_view_set_highlight_text_matches:
 * @web_view: a #WebKitWebView
 * @highlight: whether to highlight text matches
 *
 * Highlights text matches previously marked by webkit_web_view_mark_text_matches.
 */
void webkit_web_view_set_highlight_text_matches(WebKitWebView* webView, gboolean shouldHighlight)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    Frame *frame = core(webView)->mainFrame();
    do {
        frame->editor()->setMarkedTextMatchesAreHighlighted(shouldHighlight);
        frame = frame->tree()->traverseNextWithWrap(false);
    } while (frame);
}

/**
 * webkit_web_view_unmark_text_matches:
 * @web_view: a #WebKitWebView
 *
 * Removes highlighting previously set by webkit_web_view_mark_text_matches.
 */
void webkit_web_view_unmark_text_matches(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    return core(webView)->unmarkAllTextMatches();
}

/**
 * webkit_web_view_get_main_frame:
 * @webView: a #WebKitWebView
 *
 * Returns the main frame for the @webView.
 *
 * Return value: (transfer none): the main #WebKitWebFrame for @webView
 */
WebKitWebFrame* webkit_web_view_get_main_frame(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), NULL);

    return webView->priv->mainFrame;
}

/**
 * webkit_web_view_get_focused_frame:
 * @web_view: a #WebKitWebView
 *
 * Returns the frame that has focus or an active text selection.
 *
 * Return value: (transfer none): The focused #WebKitWebFrame or %NULL if no frame is focused
 */
WebKitWebFrame* webkit_web_view_get_focused_frame(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), NULL);

    Frame* focusedFrame = core(webView)->focusController()->focusedFrame();
    return kit(focusedFrame);
}

void webkit_web_view_execute_script(WebKitWebView* webView, const gchar* script)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));
    g_return_if_fail(script);

    core(webView)->mainFrame()->script()->executeScript(String::fromUTF8(script), true);
}

/**
 * webkit_web_view_cut_clipboard:
 * @web_view: a #WebKitWebView
 *
 * Determines whether or not it is currently possible to cut to the clipboard.
 *
 * Return value: %TRUE if a selection can be cut, %FALSE if not
 */
gboolean webkit_web_view_can_cut_clipboard(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    return frame->editor()->canCut() || frame->editor()->canDHTMLCut();
}

/**
 * webkit_web_view_copy_clipboard:
 * @web_view: a #WebKitWebView
 *
 * Determines whether or not it is currently possible to copy to the clipboard.
 *
 * Return value: %TRUE if a selection can be copied, %FALSE if not
 */
gboolean webkit_web_view_can_copy_clipboard(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    return frame->editor()->canCopy() || frame->editor()->canDHTMLCopy();
}

/**
 * webkit_web_view_paste_clipboard:
 * @web_view: a #WebKitWebView
 *
 * Determines whether or not it is currently possible to paste from the clipboard.
 *
 * Return value: %TRUE if a selection can be pasted, %FALSE if not
 */
gboolean webkit_web_view_can_paste_clipboard(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    return frame->editor()->canPaste() || frame->editor()->canDHTMLPaste();
}

/**
 * webkit_web_view_cut_clipboard:
 * @web_view: a #WebKitWebView
 *
 * Cuts the current selection inside the @web_view to the clipboard.
 */
void webkit_web_view_cut_clipboard(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    if (webkit_web_view_can_cut_clipboard(webView))
        g_signal_emit(webView, webkit_web_view_signals[CUT_CLIPBOARD], 0);
}

/**
 * webkit_web_view_copy_clipboard:
 * @web_view: a #WebKitWebView
 *
 * Copies the current selection inside the @web_view to the clipboard.
 */
void webkit_web_view_copy_clipboard(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    if (webkit_web_view_can_copy_clipboard(webView))
        g_signal_emit(webView, webkit_web_view_signals[COPY_CLIPBOARD], 0);
}

/**
 * webkit_web_view_paste_clipboard:
 * @web_view: a #WebKitWebView
 *
 * Pastes the current contents of the clipboard to the @web_view.
 */
void webkit_web_view_paste_clipboard(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    if (webkit_web_view_can_paste_clipboard(webView))
        g_signal_emit(webView, webkit_web_view_signals[PASTE_CLIPBOARD], 0);
}

/**
 * webkit_web_view_delete_selection:
 * @web_view: a #WebKitWebView
 *
 * Deletes the current selection inside the @web_view.
 */
void webkit_web_view_delete_selection(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    frame->editor()->performDelete();
}

/**
 * webkit_web_view_has_selection:
 * @web_view: a #WebKitWebView
 *
 * Determines whether text was selected.
 *
 * Return value: %TRUE if there is selected text, %FALSE if not
 */
gboolean webkit_web_view_has_selection(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    return !core(webView)->selection().isNone();
}

/**
 * webkit_web_view_get_selected_text:
 * @web_view: a #WebKitWebView
 *
 * Retrieves the selected text if any.
 *
 * Return value: a newly allocated string with the selection or %NULL
 */
gchar* webkit_web_view_get_selected_text(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), 0);

    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    return g_strdup(frame->editor()->selectedText().utf8().data());
}

/**
 * webkit_web_view_select_all:
 * @web_view: a #WebKitWebView
 *
 * Attempts to select everything inside the @web_view.
 */
void webkit_web_view_select_all(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    g_signal_emit(webView, webkit_web_view_signals[SELECT_ALL], 0);
}

/**
 * webkit_web_view_get_editable:
 * @web_view: a #WebKitWebView
 *
 * Returns whether the user is allowed to edit the document.
 *
 * Returns %TRUE if @web_view allows the user to edit the HTML document, %FALSE if
 * it doesn't. You can change @web_view's document programmatically regardless of
 * this setting.
 *
 * Return value: a #gboolean indicating the editable state
 */
gboolean webkit_web_view_get_editable(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    WebKitWebViewPrivate* priv = webView->priv;

    return priv->editable;
}

/**
 * webkit_web_view_set_editable:
 * @web_view: a #WebKitWebView
 * @flag: a #gboolean indicating the editable state
 *
 * Sets whether @web_view allows the user to edit its HTML document.
 *
 * If @flag is %TRUE, @web_view allows the user to edit the document. If @flag is
 * %FALSE, an element in @web_view's document can only be edited if the
 * CONTENTEDITABLE attribute has been set on the element or one of its parent
 * elements. You can change @web_view's document programmatically regardless of
 * this setting. By default a #WebKitWebView is not editable.

 * Normally, an HTML document is not editable unless the elements within the
 * document are editable. This function provides a low-level way to make the
 * contents of a #WebKitWebView editable without altering the document or DOM
 * structure.
 */
void webkit_web_view_set_editable(WebKitWebView* webView, gboolean flag)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    WebKitWebViewPrivate* priv = webView->priv;

    Frame* frame = core(webView)->mainFrame();
    g_return_if_fail(frame);

    // TODO: What happens when the frame is replaced?
    flag = flag != FALSE;
    if (flag == priv->editable)
        return;

    priv->editable = flag;

    if (flag) {
        frame->editor()->applyEditingStyleToBodyElement();
        // TODO: If the WebKitWebView is made editable and the selection is empty, set it to something.
        //if (!webkit_web_view_get_selected_dom_range(webView))
        //    mainFrame->setSelectionFromNone();
    }
    g_object_notify(G_OBJECT(webView), "editable");
}

/**
 * webkit_web_view_get_copy_target_list:
 * @web_view: a #WebKitWebView
 *
 * This function returns the list of targets this #WebKitWebView can
 * provide for clipboard copying and as DND source. The targets in the list are
 * added with values from the #WebKitWebViewTargetInfo enum,
 * using gtk_target_list_add() and
 * gtk_target_list_add_text_targets().
 *
 * Return value: the #GtkTargetList
 **/
//GtkTargetList* webkit_web_view_get_copy_target_list(WebKitWebView* webView)
//{
//    return pasteboardHelperInstance()->targetList();
//}

/**
 * webkit_web_view_get_paste_target_list:
 * @web_view: a #WebKitWebView
 *
 * This function returns the list of targets this #WebKitWebView can
 * provide for clipboard pasting and as DND destination. The targets in the list are
 * added with values from the #WebKitWebViewTargetInfo enum,
 * using gtk_target_list_add() and
 * gtk_target_list_add_text_targets().
 *
 * Return value: the #GtkTargetList
 **/
//GtkTargetList* webkit_web_view_get_paste_target_list(WebKitWebView* webView)
//{
//    return pasteboardHelperInstance()->targetList();
//}

/**
 * webkit_web_view_can_show_mime_type:
 * @web_view: a #WebKitWebView
 * @mime_type: a MIME type
 *
 * This functions returns whether or not a MIME type can be displayed using this view.
 *
 * Return value: a #gboolean indicating if the MIME type can be displayed
 *
 * Since: 1.0.3
 **/

gboolean webkit_web_view_can_show_mime_type(WebKitWebView* webView, const gchar* mimeType)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    Frame* frame = core(webkit_web_view_get_main_frame(webView));
    if (FrameLoader* loader = frame->loader())
        return loader->canShowMIMEType(String::fromUTF8(mimeType));
    else
        return FALSE;
}

/**
 * webkit_web_view_get_transparent:
 * @web_view: a #WebKitWebView
 *
 * Returns whether the #WebKitWebView has a transparent background.
 *
 * Return value: %FALSE when the #WebKitWebView draws a solid background
 * (the default), otherwise %TRUE.
 */
gboolean webkit_web_view_get_transparent(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    WebKitWebViewPrivate* priv = webView->priv;
    return priv->transparent;
}

/**
 * webkit_web_view_set_transparent:
 * @web_view: a #WebKitWebView
 *
 * Sets whether the #WebKitWebView has a transparent background.
 *
 * Pass %FALSE to have the #WebKitWebView draw a solid background
 * (the default), otherwise %TRUE.
 */
void webkit_web_view_set_transparent(WebKitWebView* webView, gboolean flag)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    WebKitWebViewPrivate* priv = webView->priv;
    priv->transparent = flag;

    // TODO: This needs to be made persistent or it could become a problem when
    // the main frame is replaced.
    Frame* frame = core(webView)->mainFrame();
    g_return_if_fail(frame);
    frame->view()->setTransparent(flag);
    g_object_notify(G_OBJECT(webView), "transparent");
}

/**
 * webkit_web_view_get_zoom_level:
 * @web_view: a #WebKitWebView
 *
 * Returns the zoom level of @web_view, i.e. the factor by which elements in
 * the page are scaled with respect to their original size.
 * If the "full-content-zoom" property is set to %FALSE (the default)
 * the zoom level changes the text size, or if %TRUE, scales all
 * elements in the page.
 *
 * Return value: the zoom level of @web_view
 *
 * Since: 1.0.1
 */
gfloat webkit_web_view_get_zoom_level(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), 1.0f);

    Frame* frame = core(webView)->mainFrame();
    if (!frame)
        return 1.0f;

    WebKitWebViewPrivate* priv = webView->priv;
    return priv->zoomFullContent ? frame->pageZoomFactor() : frame->textZoomFactor();
}

static void webkit_web_view_apply_zoom_level(WebKitWebView* webView, gfloat zoomLevel)
{
    Frame* frame = core(webView)->mainFrame();
    if (!frame)
        return;

    WebKitWebViewPrivate* priv = webView->priv;
    if (priv->zoomFullContent)
        frame->setPageZoomFactor(zoomLevel);
    else
        frame->setTextZoomFactor(zoomLevel);        
}

/**
 * webkit_web_view_set_zoom_level:
 * @web_view: a #WebKitWebView
 * @zoom_level: the new zoom level
 *
 * Sets the zoom level of @web_view, i.e. the factor by which elements in
 * the page are scaled with respect to their original size.
 * If the "full-content-zoom" property is set to %FALSE (the default)
 * the zoom level changes the text size, or if %TRUE, scales all
 * elements in the page.
 *
 * Since: 1.0.1
 */
void webkit_web_view_set_zoom_level(WebKitWebView* webView, gfloat zoomLevel)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    webkit_web_view_apply_zoom_level(webView, zoomLevel);
    g_object_notify(G_OBJECT(webView), "zoom-level");
}

/**
 * webkit_web_view_zoom_in:
 * @web_view: a #WebKitWebView
 *
 * Increases the zoom level of @web_view. The current zoom
 * level is incremented by the value of the "zoom-step"
 * property of the #WebKitWebSettings associated with @web_view.
 *
 * Since: 1.0.1
 */
void webkit_web_view_zoom_in(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    WebKitWebViewPrivate* priv = webView->priv;
    gfloat zoomMultiplierRatio;
    g_object_get(priv->webSettings.get(), "zoom-step", &zoomMultiplierRatio, NULL);

    webkit_web_view_set_zoom_level(webView, webkit_web_view_get_zoom_level(webView) + zoomMultiplierRatio);
}

/**
 * webkit_web_view_zoom_out:
 * @web_view: a #WebKitWebView
 *
 * Decreases the zoom level of @web_view. The current zoom
 * level is decremented by the value of the "zoom-step"
 * property of the #WebKitWebSettings associated with @web_view.
 *
 * Since: 1.0.1
 */
void webkit_web_view_zoom_out(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    WebKitWebViewPrivate* priv = webView->priv;
    gfloat zoomMultiplierRatio;
    g_object_get(priv->webSettings.get(), "zoom-step", &zoomMultiplierRatio, NULL);

    webkit_web_view_set_zoom_level(webView, webkit_web_view_get_zoom_level(webView) - zoomMultiplierRatio);
}

/**
 * webkit_web_view_get_full_content_zoom:
 * @web_view: a #WebKitWebView
 *
 * Returns whether the zoom level affects only text or all elements.
 *
 * Return value: %FALSE if only text should be scaled (the default),
 * %TRUE if the full content of the view should be scaled.
 *
 * Since: 1.0.1
 */
gboolean webkit_web_view_get_full_content_zoom(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    WebKitWebViewPrivate* priv = webView->priv;
    return priv->zoomFullContent;
}

/**
 * webkit_web_view_set_full_content_zoom:
 * @web_view: a #WebKitWebView
 * @full_content_zoom: %FALSE if only text should be scaled (the default),
 * %TRUE if the full content of the view should be scaled.
 *
 * Sets whether the zoom level affects only text or all elements.
 *
 * Since: 1.0.1
 */
void webkit_web_view_set_full_content_zoom(WebKitWebView* webView, gboolean zoomFullContent)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    WebKitWebViewPrivate* priv = webView->priv;
    if (priv->zoomFullContent == zoomFullContent)
      return;

    Frame* frame = core(webView)->mainFrame();
    if (!frame)
      return;

    gfloat zoomLevel = priv->zoomFullContent ? frame->pageZoomFactor() : frame->textZoomFactor();

    priv->zoomFullContent = zoomFullContent;
    if (priv->zoomFullContent)
        frame->setPageAndTextZoomFactors(zoomLevel, 1);
    else
        frame->setPageAndTextZoomFactors(1, zoomLevel);

    g_object_notify(G_OBJECT(webView), "full-content-zoom");
}

/**
 * webkit_web_view_get_load_status:
 * @web_view: a #WebKitWebView
 *
 * Determines the current status of the load.
 *
 * Since: 1.1.7
 */
WebKitLoadStatus webkit_web_view_get_load_status(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), WEBKIT_LOAD_FINISHED);

    WebKitWebViewPrivate* priv = webView->priv;
    return priv->loadStatus;
}

/**
 * webkit_web_view_get_progress:
 * @web_view: a #WebKitWebView
 *
 * Determines the current progress of the load.
 *
 * Since: 1.1.7
 */
gdouble webkit_web_view_get_progress(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), 1.0);

    return core(webView)->progress()->estimatedProgress();
}

/**
 * webkit_web_view_get_encoding:
 * @web_view: a #WebKitWebView
 *
 * Returns the default encoding of the #WebKitWebView.
 *
 * Return value: the default encoding
 *
 * Since: 1.1.1
 */
const gchar* webkit_web_view_get_encoding(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), NULL);
    String encoding = core(webView)->mainFrame()->loader()->writer()->encoding();
    if (encoding.isEmpty())
        return 0;
    webView->priv->encoding = encoding.utf8();
    return webView->priv->encoding.data();
}

/**
 * webkit_web_view_set_custom_encoding:
 * @web_view: a #WebKitWebView
 * @encoding: the new encoding, or %NULL to restore the default encoding
 *
 * Sets the current #WebKitWebView encoding, without modifying the default one,
 * and reloads the page.
 *
 * Since: 1.1.1
 */
void webkit_web_view_set_custom_encoding(WebKitWebView* webView, const char* encoding)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    core(webView)->mainFrame()->loader()->reloadWithOverrideEncoding(String::fromUTF8(encoding));
}

/**
 * webkit_web_view_get_custom_encoding:
 * @web_view: a #WebKitWebView
 *
 * Returns the current encoding of the #WebKitWebView, not the default-encoding
 * of WebKitWebSettings.
 *
 * Return value: a string containing the current custom encoding for @web_view, or %NULL if there's none set.
 *
 * Since: 1.1.1
 */
const char* webkit_web_view_get_custom_encoding(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), NULL);
    String overrideEncoding = core(webView)->mainFrame()->loader()->documentLoader()->overrideEncoding();
    if (overrideEncoding.isEmpty())
        return 0;
    webView->priv->customEncoding = overrideEncoding.utf8();
    return webView->priv->customEncoding.data();
}

/**
 * webkit_web_view_set_view_mode:
 * @web_view: the #WebKitWebView that will have its view mode set
 * @mode: the %WebKitWebViewViewMode to be set
 *
 * Sets the view-mode property of the #WebKitWebView. Check the
 * property's documentation for more information.
 *
 * Since: 1.3.4
 */
void webkit_web_view_set_view_mode(WebKitWebView* webView, WebKitWebViewViewMode mode)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    Page* page = core(webView);

    switch (mode) {
    case WEBKIT_WEB_VIEW_VIEW_MODE_FLOATING:
        page->setViewMode(Page::ViewModeFloating);
        break;
    case WEBKIT_WEB_VIEW_VIEW_MODE_FULLSCREEN:
        page->setViewMode(Page::ViewModeFullscreen);
        break;
    case WEBKIT_WEB_VIEW_VIEW_MODE_MAXIMIZED:
        page->setViewMode(Page::ViewModeMaximized);
        break;
    case WEBKIT_WEB_VIEW_VIEW_MODE_MINIMIZED:
        page->setViewMode(Page::ViewModeMinimized);
        break;
    default:
        page->setViewMode(Page::ViewModeWindowed);
        break;
    }
}

/**
 * webkit_web_view_get_view_mode:
 * @web_view: the #WebKitWebView to obtain the view mode from
 *
 * Gets the value of the view-mode property of the
 * #WebKitWebView. Check the property's documentation for more
 * information.
 *
 * Return value: the %WebKitWebViewViewMode currently set for the
 * #WebKitWebView.
 *
 * Since: 1.3.4
 */
WebKitWebViewViewMode webkit_web_view_get_view_mode(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), WEBKIT_WEB_VIEW_VIEW_MODE_WINDOWED);

    Page* page = core(webView);
    Page::ViewMode mode = page->viewMode();

    if (mode == Page::ViewModeFloating)
        return WEBKIT_WEB_VIEW_VIEW_MODE_FLOATING;

    if (mode == Page::ViewModeFullscreen)
        return WEBKIT_WEB_VIEW_VIEW_MODE_FULLSCREEN;

    if (mode == Page::ViewModeMaximized)
        return WEBKIT_WEB_VIEW_VIEW_MODE_MAXIMIZED;

    if (mode == Page::ViewModeMinimized)
        return WEBKIT_WEB_VIEW_VIEW_MODE_MINIMIZED;

    return WEBKIT_WEB_VIEW_VIEW_MODE_WINDOWED;
}

/**
 * webkit_web_view_move_cursor:
 * @web_view: a #WebKitWebView
 * @step: a #GtkMovementStep
 * @count: integer describing the direction of the movement. 1 for forward, -1 for backwards.
 *
 * Move the cursor in @view as described by @step and @count.
 *
 * Since: 1.1.4
 */
// void webkit_web_view_move_cursor(WebKitWebView* webView, GtkMovementStep step, gint count)
// {
//     g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));
//     g_return_if_fail(step == GTK_MOVEMENT_VISUAL_POSITIONS ||
//                      step == GTK_MOVEMENT_DISPLAY_LINES ||
//                      step == GTK_MOVEMENT_PAGES ||
//                      step == GTK_MOVEMENT_BUFFER_ENDS);
//     g_return_if_fail(count == 1 || count == -1);
// 
//     gboolean handled;
//     g_signal_emit(webView, webkit_web_view_signals[MOVE_CURSOR], 0, step, count, &handled);
// }

void webkit_web_view_set_group_name(WebKitWebView* webView, const gchar* groupName)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    WebKitWebViewPrivate* priv = webView->priv;

    if (!priv->corePage)
        return;

    priv->corePage->setGroupName(String::fromUTF8(groupName));
}

/**
 * webkit_web_view_can_undo:
 * @web_view: a #WebKitWebView
 *
 * Determines whether or not it is currently possible to undo the last
 * editing command in the view.
 *
 * Return value: %TRUE if a undo can be done, %FALSE if not
 *
 * Since: 1.1.14
 */
gboolean webkit_web_view_can_undo(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    return frame->editor()->canUndo();
}

/**
 * webkit_web_view_undo:
 * @web_view: a #WebKitWebView
 *
 * Undoes the last editing command in the view, if possible.
 *
 * Since: 1.1.14
 */
void webkit_web_view_undo(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    if (webkit_web_view_can_undo(webView))
        g_signal_emit(webView, webkit_web_view_signals[UNDO], 0);
}

/**
 * webkit_web_view_can_redo:
 * @web_view: a #WebKitWebView
 *
 * Determines whether or not it is currently possible to redo the last
 * editing command in the view.
 *
 * Return value: %TRUE if a redo can be done, %FALSE if not
 *
 * Since: 1.1.14
 */
gboolean webkit_web_view_can_redo(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    return frame->editor()->canRedo();
}

/**
 * webkit_web_view_redo:
 * @web_view: a #WebKitWebView
 *
 * Redoes the last editing command in the view, if possible.
 *
 * Since: 1.1.14
 */
void webkit_web_view_redo(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    if (webkit_web_view_can_redo(webView))
        g_signal_emit(webView, webkit_web_view_signals[REDO], 0);
}


/**
 * webkit_web_view_set_view_source_mode:
 * @web_view: a #WebKitWebView
 * @view_source_mode: the mode to turn on or off view source mode
 *
 * Set whether the view should be in view source mode. Setting this mode to
 * %TRUE before loading a URI will display the source of the web page in a
 * nice and readable format.
 *
 * Since: 1.1.14
 */
void webkit_web_view_set_view_source_mode (WebKitWebView* webView, gboolean mode)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    if (Frame* mainFrame = core(webView)->mainFrame())
        mainFrame->setInViewSourceMode(mode);
}

/**
 * webkit_web_view_get_view_source_mode:
 * @web_view: a #WebKitWebView
 *
 * Return value: %TRUE if @web_view is in view source mode, %FALSE otherwise.
 *
 * Since: 1.1.14
 */
gboolean webkit_web_view_get_view_source_mode (WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);

    if (Frame* mainFrame = core(webView)->mainFrame())
        return mainFrame->inViewSourceMode();

    return FALSE;
}

// Internal subresource management
void webkit_web_view_add_resource(WebKitWebView* webView, const char* identifier, WebKitWebResource* webResource)
{
    WebKitWebViewPrivate* priv = webView->priv;

    if (!priv->mainResource) {
        priv->mainResource = adoptPlatformRef(webResource);
        priv->mainResourceIdentifier = identifier;
        return;
    }

    g_hash_table_insert(priv->subResources.get(), g_strdup(identifier), webResource);
}

WebKitWebResource* webkit_web_view_get_resource(WebKitWebView* webView, char* identifier)
{
    WebKitWebViewPrivate* priv = webView->priv;
    gpointer webResource = 0;
    gboolean resourceFound = g_hash_table_lookup_extended(priv->subResources.get(), identifier, NULL, &webResource);

    // The only resource we do not store in this hash table is the
    // main!  If we did not find a request, it probably means the load
    // has been interrupted while while a resource was still being
    // loaded.
    if (!resourceFound && !g_str_equal(identifier, priv->mainResourceIdentifier.data()))
        return 0;

    if (!webResource)
        return webkit_web_view_get_main_resource(webView);

    return WEBKIT_WEB_RESOURCE(webResource);
}

WebKitWebResource* webkit_web_view_get_main_resource(WebKitWebView* webView)
{
    return webView->priv->mainResource.get();
}

void webkit_web_view_clear_resources(WebKitWebView* webView)
{
    WebKitWebViewPrivate* priv = webView->priv;
    priv->mainResourceIdentifier = "";
    priv->mainResource = 0;

    if (priv->subResources)
        g_hash_table_remove_all(priv->subResources.get());
}

GList* webkit_web_view_get_subresources(WebKitWebView* webView)
{
    WebKitWebViewPrivate* priv = webView->priv;
    GList* subResources = g_hash_table_get_values(priv->subResources.get());
    return g_list_remove(subResources, priv->mainResource.get());
}

/* From EventHandler.cpp */
static IntPoint documentPointForWindowPoint(Frame* frame, const IntPoint& windowPoint)
{
    FrameView* view = frame->view();
    // FIXME: Is it really OK to use the wrong coordinates here when view is 0?
    // Historically the code would just crash; this is clearly no worse than that.
    return view ? view->windowToContents(windowPoint) : windowPoint;
}

void webkit_web_view_set_tooltip_text(WebKitWebView* webView, const char* tooltip)
{
    notImplemented();
}

/**
 * webkit_web_view_get_hit_test_result:
 * @webView: a #WebKitWebView
 * @event: a #GdkEventButton
 * 
 * Does a 'hit test' in the coordinates specified by @event to figure
 * out context information about that position in the @webView.
 * 
 * Returns: (transfer none): a newly created #WebKitHitTestResult with the context of the
 * specified position.
 *
 * Since: 1.1.15
 **/
WebKitHitTestResult* webkit_web_view_get_hit_test_result(WebKitWebView* webView, ClutterButtonEvent* event)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), NULL);
    g_return_val_if_fail(event, NULL);

    PlatformMouseEvent mouseEvent = PlatformMouseEvent(event);
    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    HitTestRequest request(HitTestRequest::Active);
    IntPoint documentPoint = documentPointForWindowPoint(frame, mouseEvent.pos());
    MouseEventWithHitTestResults mev = frame->document()->prepareMouseEvent(request, documentPoint, mouseEvent);

    return kit(mev.hitTestResult());
}

/**
 * webkit_web_view_get_icon_uri:
 * @web_view: the #WebKitWebView object
 *
 * Obtains the URI for the favicon for the given #WebKitWebView, or
 * %NULL if there is none.
 *
 * Return value: the URI for the favicon, or %NULL
 *
 * Since: 1.1.18
 */
G_CONST_RETURN gchar* webkit_web_view_get_icon_uri(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), 0);
    String iconURL = iconDatabase()->iconURLForPageURL(core(webView)->mainFrame()->loader()->url().prettyURL());
    webView->priv->iconURI = iconURL.utf8();
    return webView->priv->iconURI.data();
}

/**
 * webkit_web_view_get_dom_document:
 * @webView: a #WebKitWebView
 * 
 * Returns: (transfer none): the #WebKitDOMDocument currently loaded in the @webView
 *
 * Since: 1.3.1
 **/
WebKitDOMDocument*
webkit_web_view_get_dom_document(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), 0);

    Frame* coreFrame = core(webView)->mainFrame();
    if (!coreFrame)
        return 0;

    Document* doc = coreFrame->document();
    if (!doc)
        return 0;

    return static_cast<WebKitDOMDocument*>(kit(doc));
}

/**
 * SECTION:webkit
 * @short_description: Global functions controlling WebKit
 *
 * WebKit manages many resources which are not related to specific
 * views. These functions relate to cross-view limits, such as cache
 * sizes, database quotas, and the HTTP session management.
 */

/**
 * webkit_get_default_session:
 *
 * Retrieves the default #SoupSession used by all web views.
 * Note that the session features are added by WebKit on demand,
 * so if you insert your own #SoupCookieJar before any network
 * traffic occurs, WebKit will use it instead of the default.
 *
 * Return value: the default #SoupSession
 *
 * Since: 1.1.1
 */
SoupSession* webkit_get_default_session ()
{
    webkit_init();
    return ResourceHandle::defaultSession();
}

/**
 * webkit_set_cache_model:
 * @cache_model: a #WebKitCacheModel
 *
 * Specifies a usage model for WebViews, which WebKit will use to
 * determine its caching behavior. All web views follow the cache
 * model. This cache model determines the RAM and disk space to use
 * for caching previously viewed content .
 *
 * Research indicates that users tend to browse within clusters of
 * documents that hold resources in common, and to revisit previously
 * visited documents. WebKit and the frameworks below it include
 * built-in caches that take advantage of these patterns,
 * substantially improving document load speed in browsing
 * situations. The WebKit cache model controls the behaviors of all of
 * these caches, including various WebCore caches.
 *
 * Browsers can improve document load speed substantially by
 * specifying WEBKIT_CACHE_MODEL_WEB_BROWSER. Applications without a
 * browsing interface can reduce memory usage substantially by
 * specifying WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER. Default value is
 * WEBKIT_CACHE_MODEL_WEB_BROWSER.
 *
 * Since: 1.1.18
 */
void webkit_set_cache_model(WebKitCacheModel model)
{
    webkit_init();

    if (cacheModel == model)
        return;

    // FIXME: Add disk cache handling when soup has the API
    guint cacheTotalCapacity;
    guint cacheMinDeadCapacity;
    guint cacheMaxDeadCapacity;
    gdouble deadDecodedDataDeletionInterval;
    guint pageCacheCapacity;

    switch (model) {
    case WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER:
        pageCacheCapacity = 0;
        cacheTotalCapacity = 0;
        cacheMinDeadCapacity = 0;
        cacheMaxDeadCapacity = 0;
        deadDecodedDataDeletionInterval = 0;
        break;
    case WEBKIT_CACHE_MODEL_WEB_BROWSER:
        pageCacheCapacity = 3;
        cacheTotalCapacity = 32 * 1024 * 1024;
        cacheMinDeadCapacity = cacheTotalCapacity / 4;
        cacheMaxDeadCapacity = cacheTotalCapacity / 2;
        deadDecodedDataDeletionInterval = 60;
        break;
    default:
        g_return_if_reached();
    }

    cache()->setCapacities(cacheMinDeadCapacity, cacheMaxDeadCapacity, cacheTotalCapacity);
    cache()->setDeadDecodedDataDeletionInterval(deadDecodedDataDeletionInterval);
    pageCache()->setCapacity(pageCacheCapacity);
    cacheModel = model;
}

/**
 * webkit_get_cache_model:
 *
 * Returns the current cache model. For more information about this
 * value check the documentation of the function
 * webkit_set_cache_model().
 *
 * Return value: the current #WebKitCacheModel
 *
 * Since: 1.1.18
 */
WebKitCacheModel webkit_get_cache_model()
{
    webkit_init();
    return cacheModel;
}

void webkit_web_view_execute_core_command_by_name(WebKitWebView* webView, const gchar* name, const gchar* value)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));
    g_return_if_fail(name);
    g_return_if_fail(value);

    core(webView)->focusController()->focusedOrMainFrame()->editor()->command(name).execute(value);
}

gboolean webkit_web_view_is_command_enabled(WebKitWebView* webView, const gchar* name)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);
    g_return_val_if_fail(name, FALSE);

    return core(webView)->focusController()->focusedOrMainFrame()->editor()->command(name).isEnabled();
}

GList* webkit_web_view_get_context_menu(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), 0);

#if ENABLE(CONTEXT_MENUS)
    ContextMenu* menu = core(webView)->contextMenuController()->contextMenu();
    if (!menu)
        return 0;
    return menu->platformDescription();
#else
    return 0;
#endif
}

gint
webkit_web_view_get_zoom_padding (WebKitWebView *webView)
{
    WebKitWebViewPrivate *priv = webView->priv;
    return priv->zoom_pad;
}

void
webkit_web_view_set_zoom_padding (WebKitWebView *webView, gint padding)
{
    WebKitWebViewPrivate *priv = webView->priv;
    priv->zoom_pad = padding;
}

guint
webkit_web_view_get_transition_time (WebKitWebView *webView)
{
    WebKitWebViewPrivate *priv = webView->priv;
    return priv->transition_time;
}

void
webkit_web_view_set_transition_time (WebKitWebView *webView, guint time)
{
    WebKitWebViewPrivate *priv = webView->priv;
    priv->transition_time = time;
}

void
webkit_web_view_zoom_to_selected_node (WebKitWebView* webView)
{
    WebKitWebViewPrivate* priv = webView->priv;
    Frame* frame = core(webView)->mainFrame();
    g_return_if_fail(frame);

    Document* document = frame->document();
    Node* node = document->focusedNode();

    zoom_on_node(webView, node);
}

void
webkit_web_view_zoom_to_default (WebKitWebView* webView)
{
    zoom_on_node(webView, 0);
}
