/*
 * Copyright (C) 2007, 2008 Holger Hans Peter Freyther
 * Copyright (C) 2007, 2008 Christian Dywan <christian@imendio.com>
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (C) 2008 Alp Toker <alp@atoker.com>
 * Copyright (C) 2008 Gustavo Noronha Silva <gns@gnome.org>
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include "ChromeClientClutter.h"

#include "Console.h"
#include "FileSystem.h"
#include "FileChooser.h"
#include "FloatRect.h"
#include "FrameLoadRequest.h"
#include "HTMLNames.h"
#include "IntRect.h"
#include "HitTestResult.h"
#include "Icon.h"
#include "KURL.h"
#include "PlatformString.h"
#include "PopupMenuClient.h"
#include "PopupMenuClutter.h"
#include "SearchPopupMenuClutter.h"
#include "SecurityOrigin.h"
#include "webkitactor.h"
#include "webkitgeolocationpolicydecision.h"
#include "webkitwebview.h"
#include "webkitnetworkrequest.h"
#include "webkitprivate.h"
#include "NotImplemented.h"
#include "WindowFeatures.h"
#if ENABLE(DATABASE)
#include "DatabaseTracker.h"
#endif
#include <wtf/text/CString.h>

#include <glib.h>
#include <glib/gi18n-lib.h>

#include <cairo.h>

using namespace WebCore;

namespace WebKit {

ChromeClient::ChromeClient(WebKitWebView* webView)
    : m_webView(webView)
{
    ASSERT(m_webView);
}

void ChromeClient::chromeDestroyed()
{
    delete this;
}

FloatRect ChromeClient::windowRect()
{
    notImplemented();
    return FloatRect();
}

void ChromeClient::setWindowRect(const FloatRect& rect)
{
    notImplemented();
}

FloatRect ChromeClient::pageRect()
{
    if (!m_webView)
        return FloatRect();

    int x, y, width, height;
    WEBKIT_WEB_VIEW_GET_CLASS(m_webView)->get_page_rect(m_webView, &x, &y, 
							&width, &height);

    return IntRect(x, y, width, height);
}

float ChromeClient::scaleFactor()
{
    // Not implementable
    return 1.0;
}

void ChromeClient::focus()
{
    if (!m_webView)
        return;
    WEBKIT_WEB_VIEW_GET_CLASS(m_webView)->request_focus(m_webView);
}

void ChromeClient::unfocus()
{
    if (!m_webView)
        return;
    WEBKIT_WEB_VIEW_GET_CLASS(m_webView)->lose_focus(m_webView);
}

Page* ChromeClient::createWindow(Frame* frame, const FrameLoadRequest& frameLoadRequest, const WindowFeatures& coreFeatures)
{
    WebKitWebView* webView = 0;

    g_signal_emit_by_name(m_webView, "create-web-view", kit(frame), &webView);

    if (!webView)
        return 0;

    WebKitWebWindowFeatures* webWindowFeatures = webkit_web_window_features_new_from_core_features(coreFeatures);
    g_object_set(webView, "window-features", webWindowFeatures, NULL);
    g_object_unref(webWindowFeatures);

    if (!frameLoadRequest.isEmpty())
        webkit_web_view_open(webView, frameLoadRequest.resourceRequest().url().string().utf8().data());

    return core(webView);
}

void ChromeClient::show()
{
    webkit_web_view_notify_ready(m_webView);
}

bool ChromeClient::canRunModal()
{
    notImplemented();
    return false;
}

void ChromeClient::runModal()
{
    notImplemented();
}

void ChromeClient::setToolbarsVisible(bool visible)
{
    WebKitWebWindowFeatures* webWindowFeatures = webkit_web_view_get_window_features(m_webView);

    g_object_set(webWindowFeatures, "toolbar-visible", visible, NULL);
}

bool ChromeClient::toolbarsVisible()
{
    WebKitWebWindowFeatures* webWindowFeatures = webkit_web_view_get_window_features(m_webView);
    gboolean visible;

    g_object_get(webWindowFeatures, "toolbar-visible", &visible, NULL);
    return visible;
}

void ChromeClient::setStatusbarVisible(bool visible)
{
    WebKitWebWindowFeatures* webWindowFeatures = webkit_web_view_get_window_features(m_webView);

    g_object_set(webWindowFeatures, "statusbar-visible", visible, NULL);
}

bool ChromeClient::statusbarVisible()
{
    WebKitWebWindowFeatures* webWindowFeatures = webkit_web_view_get_window_features(m_webView);
    gboolean visible;

    g_object_get(webWindowFeatures, "statusbar-visible", &visible, NULL);
    return visible;
}

void ChromeClient::setScrollbarsVisible(bool visible)
{
    WebKitWebWindowFeatures* webWindowFeatures = webkit_web_view_get_window_features(m_webView);

    g_object_set(webWindowFeatures, "scrollbar-visible", visible, NULL);
}

bool ChromeClient::scrollbarsVisible() {
    WebKitWebWindowFeatures* webWindowFeatures = webkit_web_view_get_window_features(m_webView);
    gboolean visible;

    g_object_get(webWindowFeatures, "scrollbar-visible", &visible, NULL);
    return visible;
}

void ChromeClient::setMenubarVisible(bool visible)
{
    WebKitWebWindowFeatures* webWindowFeatures = webkit_web_view_get_window_features(m_webView);

    g_object_set(webWindowFeatures, "menubar-visible", visible, NULL);
}

bool ChromeClient::menubarVisible()
{
    WebKitWebWindowFeatures* webWindowFeatures = webkit_web_view_get_window_features(m_webView);
    gboolean visible;

    g_object_get(webWindowFeatures, "menubar-visible", &visible, NULL);
    return visible;
}

void ChromeClient::setResizable(bool)
{
    // Ignored for now
}

void ChromeClient::closeWindowSoon()
{
    // We may not have a WebView as create-web-view can return NULL.
    if (!m_webView)
        return;

    webkit_web_view_stop_loading(m_webView);

    gboolean isHandled = false;
    g_signal_emit_by_name(m_webView, "close-web-view", &isHandled);

    if (isHandled)
        return;

    // FIXME: should we clear the frame group name here explicitly? Mac does it.
    // But this gets cleared in Page's destructor anyway.
    // webkit_web_view_set_group_name(m_webView, "");
}

bool ChromeClient::canTakeFocus(FocusDirection)
{
    if (!m_webView)
        return false;
    return WEBKIT_WEB_VIEW_GET_CLASS(m_webView)->can_take_focus(m_webView);
}

void ChromeClient::takeFocus(FocusDirection)
{
    unfocus();
}

void ChromeClient::focusedNodeChanged(Node*)
{
}

bool ChromeClient::canRunBeforeUnloadConfirmPanel()
{
    return true;
}

bool ChromeClient::runBeforeUnloadConfirmPanel(const WTF::String& message, WebCore::Frame* frame)
{
    return runJavaScriptConfirm(frame, message);
}

void ChromeClient::addMessageToConsole(WebCore::MessageSource source, WebCore::MessageType type, WebCore::MessageLevel level, const WTF::String& message, unsigned int lineNumber, const WTF::String& sourceId)
{
    gboolean retval;
    g_signal_emit_by_name(m_webView, "console-message", message.utf8().data(), lineNumber, sourceId.utf8().data(), &retval);
}

void ChromeClient::runJavaScriptAlert(Frame* frame, const String& message)
{
    gboolean retval;
    g_signal_emit_by_name(m_webView, "script-alert", kit(frame), message.utf8().data(), &retval);
}

bool ChromeClient::runJavaScriptConfirm(Frame* frame, const String& message)
{
    gboolean retval;
    gboolean didConfirm;
    g_signal_emit_by_name(m_webView, "script-confirm", kit(frame), message.utf8().data(), &didConfirm, &retval);
    return didConfirm == TRUE;
}

bool ChromeClient::runJavaScriptPrompt(Frame* frame, const String& message, const String& defaultValue, String& result)
{
    gboolean retval;
    gchar* value = 0;
    g_signal_emit_by_name(m_webView, "script-prompt", kit(frame), message.utf8().data(), defaultValue.utf8().data(), &value, &retval);
    if (value) {
        result = String::fromUTF8(value);
        g_free(value);
        return true;
    }
    return false;
}

void ChromeClient::setStatusbarText(const String& string)
{
    CString stringMessage = string.utf8();
    g_signal_emit_by_name(m_webView, "status-bar-text-changed", stringMessage.data());
}

bool ChromeClient::shouldInterruptJavaScript()
{
    notImplemented();
    return false;
}

bool ChromeClient::tabsToLinks() const
{
    return true;
}

IntRect ChromeClient::windowResizerRect() const
{
    notImplemented();
    return IntRect();
}

void ChromeClient::invalidateWindow(const IntRect&, bool)
{
    notImplemented();
}

void ChromeClient::invalidateContentsAndWindow(const IntRect& updateRect, bool immediate)
{
    WebkitActorRectangle uRect = updateRect;
    webkit_actor_queue_expose(WEBKIT_ACTOR(m_webView), &uRect);
}

void ChromeClient::invalidateContentsForSlowScroll(const IntRect& updateRect, bool immediate)
{
    invalidateContentsAndWindow(updateRect, immediate);
}

void ChromeClient::scroll(const IntSize& delta, const IntRect& rectToScroll, const IntRect& clipRect)
{
    IntRect area = clipRect;
    IntRect moveRect;

    IntRect sourceRect = area;
    sourceRect.move(-delta.width(), -delta.height());
    
    cairo_rectangle_int_t rect;
    rect.x = area.x();
    rect.y = area.y();
    rect.width = area.width();
    rect.height = area.height();
    
    cairo_region_t* invalidRegion = cairo_region_create_rectangle(&rect);
    
    moveRect = area;
    if (moveRect.intersects(sourceRect)) {
        moveRect.intersect(sourceRect);
        
        rect.x = moveRect.x();
        rect.y = moveRect.y();
        rect.width = moveRect.width();
        rect.height = moveRect.height();
        
        cairo_region_t* moveRegion = cairo_region_create_rectangle(&rect);
        cairo_region_t* tmpRegion = cairo_region_copy(moveRegion);
        cairo_region_translate(moveRegion, delta.width(), delta.height());
        cairo_region_subtract(tmpRegion, moveRegion);
        
        for (int i = 0; i < cairo_region_num_rectangles(tmpRegion); ++i) {
            cairo_region_get_rectangle(tmpRegion, i, &rect);
            WebkitActorRectangle dirty;
            dirty.x = rect.x;
            dirty.y = rect.y;
            dirty.width = rect.width;
            dirty.height = rect.height; 
            webkit_actor_queue_expose(WEBKIT_ACTOR(m_webView), &dirty);
        }
        
        cairo_region_subtract(invalidRegion, moveRegion);
        cairo_region_destroy(tmpRegion);
        cairo_region_destroy(moveRegion);
    }

    for (int i = 0; i < cairo_region_num_rectangles(invalidRegion); ++i) {
        cairo_region_get_rectangle(invalidRegion, i, &rect);
        WebkitActorRectangle dirty;
        dirty.x = rect.x;
        dirty.y = rect.y;
        dirty.width = rect.width;
        dirty.height = rect.height;
        webkit_actor_queue_expose(WEBKIT_ACTOR(m_webView), &dirty);
    }
    
    cairo_region_destroy(invalidRegion);
}

static IntPoint actorScreenPosition(ClutterActor* actor)
{
    gfloat actorX = 0.0, actorY = 0.0;
    clutter_actor_get_transformed_position(actor, &actorX, &actorY);
    return IntPoint((int)actorX, (int)actorY);
}

IntRect ChromeClient::windowToScreen(const IntRect& rect) const
{
    IntRect result(rect);
    IntPoint screenPosition = actorScreenPosition(CLUTTER_ACTOR(m_webView));
    result.move(screenPosition.x(), screenPosition.y());

    return result;
}

IntPoint ChromeClient::screenToWindow(const IntPoint& point) const
{
    IntPoint result(point);
    IntPoint screenPosition = actorScreenPosition(CLUTTER_ACTOR(m_webView));
    result.move(-screenPosition.x(), -screenPosition.y());

    return result;
}

PlatformPageClient ChromeClient::platformPageClient() const
{
    return CLUTTER_ACTOR(m_webView);
}

void ChromeClient::contentsSizeChanged(Frame* frame, const IntSize& size) const
{
    notImplemented();
}

void ChromeClient::scrollbarsModeDidChange() const
{
    notImplemented();
}

void ChromeClient::mouseDidMoveOverElement(const HitTestResult& hit, unsigned modifierFlags)
{
    // check if the element is a link...
    bool isLink = hit.isLiveLink();
    if (isLink) {
        KURL url = hit.absoluteLinkURL();
        if (!url.isEmpty() && url != m_hoveredLinkURL) {
            TextDirection dir;
            CString titleString = hit.title(dir).utf8();
            CString urlString = url.prettyURL().utf8();
            g_signal_emit_by_name(m_webView, "hovering-over-link", titleString.data(), urlString.data());
            m_hoveredLinkURL = url;
        }
    } else if (!isLink && !m_hoveredLinkURL.isEmpty()) {
        g_signal_emit_by_name(m_webView, "hovering-over-link", 0, 0);
        m_hoveredLinkURL = KURL();
    }
}

void ChromeClient::setToolTip(const String& toolTip, TextDirection)
{
    notImplemented();
}

void ChromeClient::print(Frame* frame)
{
    notImplemented();
}

#if ENABLE(DATABASE)
void ChromeClient::exceededDatabaseQuota(Frame* frame, const String& databaseName)
{
    guint64 defaultQuota = webkit_get_default_web_database_quota();
    DatabaseTracker::tracker().setQuota(frame->document()->securityOrigin(), defaultQuota);

    WebKitWebFrame* webFrame = kit(frame);
    WebKitWebView* webView = getViewFromFrame(webFrame);

    WebKitSecurityOrigin* origin = webkit_web_frame_get_security_origin(webFrame);
    WebKitWebDatabase* webDatabase = webkit_security_origin_get_web_database(origin, databaseName.utf8().data());
    g_signal_emit_by_name(webView, "database-quota-exceeded", webFrame, webDatabase);
}
#endif

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
void ChromeClient::reachedMaxAppCacheSize(int64_t spaceNeeded)
{
    // FIXME: Free some space.
    notImplemented();
}

void ChromeClient::reachedApplicationCacheOriginQuota(SecurityOrigin*)
{
    notImplemented();
}
#endif

void ChromeClient::runOpenPanel(Frame*, PassRefPtr<FileChooser> prpFileChooser)
{
    RefPtr<FileChooser> chooser = prpFileChooser;

    notImplemented();
}

void ChromeClient::chooseIconForFiles(const Vector<WTF::String>& filenames, WebCore::FileChooser* chooser)
{
    chooser->iconLoaded(Icon::createIconForFiles(filenames));
}

void ChromeClient::setCursor(const Cursor&)
{
    notImplemented();
}

void ChromeClient::requestGeolocationPermissionForFrame(Frame* frame, Geolocation* geolocation)
{
    WebKitWebFrame* webFrame = kit(frame);
    WebKitWebView* webView = getViewFromFrame(webFrame);

    PlatformRefPtr<WebKitGeolocationPolicyDecision> policyDecision(adoptPlatformRef(webkit_geolocation_policy_decision_new(webFrame, geolocation)));

    gboolean isHandled = FALSE;
    g_signal_emit_by_name(webView, "geolocation-policy-decision-requested", webFrame, policyDecision.get(), &isHandled);
    if (!isHandled)
        webkit_geolocation_policy_deny(policyDecision.get());
}

void ChromeClient::cancelGeolocationPermissionRequestForFrame(WebCore::Frame* frame, WebCore::Geolocation*)
{
    WebKitWebFrame* webFrame = kit(frame);
    WebKitWebView* webView = getViewFromFrame(webFrame);
    g_signal_emit_by_name(webView, "geolocation-policy-decision-cancelled", webFrame);
}

bool ChromeClient::selectItemWritingDirectionIsNatural()
{
    return true;
}

PassRefPtr<WebCore::PopupMenu> ChromeClient::createPopupMenu(WebCore::PopupMenuClient* client) const
{
    return adoptRef(new PopupMenuClutter(client));
}

PassRefPtr<WebCore::SearchPopupMenu> ChromeClient::createSearchPopupMenu(WebCore::PopupMenuClient* client) const
{
    return adoptRef(new SearchPopupMenuClutter(client));
}

#if ENABLE(VIDEO)

bool ChromeClient::supportsFullscreenForNode(const Node* node)
{
    return node->hasTagName(HTMLNames::videoTag);
}

void ChromeClient::enterFullscreenForNode(Node* node)
{
    WebCore::Frame* frame = node->document()->frame();
    WebKitWebFrame* webFrame = kit(frame);
    WebKitWebView* webView = getViewFromFrame(webFrame);
    webkitWebViewEnterFullscreen(webView, node);
}

void ChromeClient::exitFullscreenForNode(Node* node)
{
    WebCore::Frame* frame = node->document()->frame();
    WebKitWebFrame* webFrame = kit(frame);
    WebKitWebView* webView = getViewFromFrame(webFrame);
    webkitWebViewExitFullscreen(webView);
}
#endif

}
