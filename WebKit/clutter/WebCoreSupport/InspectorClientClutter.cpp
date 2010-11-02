/*
 * Copyright (C) 2008 Gustavo Noronha Silva
 * Copyright (C) 2010 Collabora Ltd.
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
#include "InspectorClientClutter.h"

#include "Frame.h"
#include "webkitwebview.h"
#include "webkitwebinspector.h"
#include "webkitprivate.h"
#include "webkitversion.h"
#include "InspectorController.h"
#include "NotImplemented.h"
#include "PlatformString.h"
#include <wtf/text/CString.h>

using namespace WebCore;

namespace WebKit {

static void notifyWebViewDestroyed(WebKitWebView* webView, InspectorFrontendClient* inspectorFrontendClient)
{
    inspectorFrontendClient->destroyInspectorWindow(true);
}

InspectorClient::InspectorClient(WebKitWebView* webView)
    : m_inspectedWebView(webView)
    , m_frontendPage(0)
    , m_frontendClient(0)
{}

InspectorClient::~InspectorClient()
{
    if (m_frontendClient) {
        m_frontendClient->disconnectInspectorClient();
        m_frontendClient = 0;
    }
}

void InspectorClient::inspectorDestroyed()
{
    delete this;
}

void InspectorClient::openInspectorFrontend(InspectorController* controller)
{
    // This g_object_get will ref the inspector. We're not doing an
    // unref if this method succeeds because the inspector object must
    // be alive even after the inspected WebView is destroyed - the
    // close-window and destroy signals still need to be
    // emitted.
    WebKitWebInspector* webInspector = 0;
    g_object_get(m_inspectedWebView, "web-inspector", &webInspector, NULL);
    ASSERT(webInspector);

    WebKitWebView* inspectorWebView = 0;
    g_signal_emit_by_name(webInspector, "inspect-web-view", m_inspectedWebView, &inspectorWebView);

    if (!inspectorWebView) {
        g_object_unref(webInspector);
        return;
    }

    webkit_web_inspector_set_web_view(webInspector, inspectorWebView);

    GOwnPtr<gchar> inspectorPath(g_build_filename(inspectorFilesPath(), "inspector.html", NULL));
    GOwnPtr<gchar> inspectorURI(g_filename_to_uri(inspectorPath.get(), 0, 0));
    webkit_web_view_load_uri(inspectorWebView, inspectorURI.get());

//    gtk_widget_show(GTK_WIDGET(inspectorWebView));

    m_frontendPage = core(inspectorWebView);
    m_frontendClient = new InspectorFrontendClient(m_inspectedWebView, inspectorWebView, webInspector, m_frontendPage, this);
    m_frontendPage->inspectorController()->setInspectorFrontendClient(m_frontendClient);
}

void InspectorClient::releaseFrontendPage()
{
    m_frontendPage = 0;
}

void InspectorClient::highlight(Node*)
{
    hideHighlight();
}

void InspectorClient::hideHighlight()
{
    // FIXME: we should be able to only invalidate the actual rects of
    // the new and old nodes. We need to track the nodes, and take the
    // actual highlight size into account when calculating the damage
    // rect.
//    gtk_widget_queue_draw(GTK_WIDGET(m_inspectedWebView));
}

#ifdef HAVE_GSETTINGS
static String toGSettingName(String inspectorSettingName)
{
    if (inspectorSettingName == "resourceTrackingEnabled")
        return String("resource-tracking-enabled");

    if (inspectorSettingName == "xhrMonitor")
        return String("xhr-monitor-enabled");

    if (inspectorSettingName == "frontendSettings")
        return String("frontend-settings");

    if (inspectorSettingName == "debuggerEnabled")
        return String("debugger-enabled");

    if (inspectorSettingName == "profilerEnabled")
        return String("profiler-enabled");

    return inspectorSettingName;
}

static String truthStringFromVariant(GVariant* variant)
{
    if (g_variant_get_boolean(variant))
        return String("true");

    return String("false");
}

static GVariant* variantFromTruthString(const String& truth)
{
    if (truth == "true")
        return g_variant_new_boolean(TRUE);

    return g_variant_new_boolean(FALSE);
}

static bool shouldIgnoreSetting(const String& key)
{
    // Ignore this setting for now, it doesn't seem to be used for
    // anything right now.
    if (key == "lastActivePanel")
        return true;

    // GSettings considers trying to fetch or set a setting that is
    // not backed by a schema as programmer error, and aborts the
    // program's execution. We check here to avoid having an unhandled
    // setting as a fatal error.
    if (key == "resourceTrackingEnabled" || key == "xhrMonitor"
        || key == "frontendSettings" || key == "debuggerEnabled"
        || key == "profilerEnabled")
        return false;

    LOG_VERBOSE(NotYetImplemented, "Unknown key ignored: %s", key.ascii().data());
    return true;
}

void InspectorClient::populateSetting(const String& key, String* value)
{
    if (shouldIgnoreSetting(key))
        return;

    GSettings* settings = inspectorGSettings();
    if (!settings)
        return;

    PlatformRefPtr<GVariant> variant = adoptPlatformRef(g_settings_get_value(settings, toGSettingName(key).utf8().data()));

    if (key == "resourceTrackingEnabled" || key == "xhrMonitor"
        || key == "debuggerEnabled" || key == "profilerEnabled")
        *value = truthStringFromVariant(variant.get());
    else if (key == "frontendSettings")
        *value = String(g_variant_get_string(variant.get(), 0));
}

void InspectorClient::storeSetting(const String& key, const String& value)
{
    if (shouldIgnoreSetting(key))
        return;

    GSettings* settings = inspectorGSettings();
    if (!settings)
        return;

    PlatformRefPtr<GVariant> variant(0);

    // Set the key with the appropriate type, and also avoid setting
    // unknown keys to avoid aborting the execution.
    if (key == "resourceTrackingEnabled" || key == "xhrMonitor"
        || key == "debuggerEnabled" || key == "profilerEnabled")
        variant = adoptPlatformRef(variantFromTruthString(value));
    else if (key == "frontendSettings")
        variant = adoptPlatformRef(g_variant_new_string(value.utf8().data()));

    if (!variant)
        return;

    g_settings_set_value(settings, toGSettingName(key).utf8().data(), variant.get());
}
#else
void InspectorClient::populateSetting(const String&, String*)
{
    notImplemented();
}

void InspectorClient::storeSetting(const String&, const String&)
{
    notImplemented();
}
#endif // HAVE_GSETTINGS

bool InspectorClient::sendMessageToFrontend(const String& message)
{
    if (!m_frontendPage)
        return false;

    Frame* frame = m_frontendPage->mainFrame();
    if (!frame)
        return false;

    ScriptController* scriptController = frame->script();
    if (!scriptController)
        return false;

    String dispatchToFrontend("WebInspector.dispatchMessageFromBackend(");
    dispatchToFrontend += message;
    dispatchToFrontend += ");";
    scriptController->executeScript(dispatchToFrontend);
    return true;
}

const char* InspectorClient::inspectorFilesPath()
{
    if (m_inspectorFilesPath)
        m_inspectorFilesPath.get();

    const char* environmentPath = getenv("WEBKIT_INSPECTOR_PATH");
    if (environmentPath && g_file_test(environmentPath, G_FILE_TEST_IS_DIR))
        m_inspectorFilesPath.set(g_strdup(environmentPath));
    else
        m_inspectorFilesPath.set(g_build_filename(DATA_DIR, "webkit-clutter-"WEBKITCLUTTER_API_VERSION_STRING, "webinspector", NULL));

    return m_inspectorFilesPath.get();
}

InspectorFrontendClient::InspectorFrontendClient(WebKitWebView* inspectedWebView, WebKitWebView* inspectorWebView, WebKitWebInspector* webInspector, Page* inspectorPage, InspectorClient* inspectorClient)
    : InspectorFrontendClientLocal(core(inspectedWebView)->inspectorController(), inspectorPage)
    , m_inspectorWebView(inspectorWebView)
    , m_inspectedWebView(inspectedWebView)
    , m_webInspector(webInspector)
    , m_inspectorClient(inspectorClient)
{
    g_signal_connect(m_inspectorWebView, "destroy",
                     G_CALLBACK(notifyWebViewDestroyed), (gpointer)this);
}

InspectorFrontendClient::~InspectorFrontendClient()
{
    if (m_inspectorClient) {
        m_inspectorClient->disconnectFrontendClient();
        m_inspectorClient = 0;
    }
    ASSERT(!m_webInspector);
}

void InspectorFrontendClient::destroyInspectorWindow(bool notifyInspectorController)
{
    if (!m_webInspector)
        return;
    WebKitWebInspector* webInspector = m_webInspector;
    m_webInspector = 0;

    g_signal_handlers_disconnect_by_func(m_inspectorWebView, (gpointer)notifyWebViewDestroyed, (gpointer)this);
    m_inspectorWebView = 0;

    if (notifyInspectorController)
        core(m_inspectedWebView)->inspectorController()->disconnectFrontend();

    if (m_inspectorClient)
        m_inspectorClient->releaseFrontendPage();

    gboolean handled = FALSE;
    g_signal_emit_by_name(webInspector, "close-window", &handled);
    ASSERT(handled);

    // Please do not use member variables here because InspectorFrontendClient object pointed by 'this'
    // has been implicitly deleted by "close-window" function.

    /* we should now dispose our own reference */
    g_object_unref(webInspector);
}

String InspectorFrontendClient::localizedStringsURL()
{
    GOwnPtr<gchar> stringsPath(g_build_filename(m_inspectorClient->inspectorFilesPath(), "localizedStrings.js", NULL));
    GOwnPtr<gchar> stringsURI(g_filename_to_uri(stringsPath.get(), 0, 0));

    // FIXME: support l10n of localizedStrings.js
    return String::fromUTF8(stringsURI.get());
}

String InspectorFrontendClient::hiddenPanels()
{
    notImplemented();
    return String();
}

void InspectorFrontendClient::bringToFront()
{
    if (!m_inspectorWebView)
        return;

    gboolean handled = FALSE;
    g_signal_emit_by_name(m_webInspector, "show-window", &handled);
}

void InspectorFrontendClient::closeWindow()
{
    destroyInspectorWindow(true);
}

void InspectorFrontendClient::disconnectFromBackend()
{
    destroyInspectorWindow(false);
}

void InspectorFrontendClient::attachWindow()
{
    if (!m_inspectorWebView)
        return;

    gboolean handled = FALSE;
    g_signal_emit_by_name(m_webInspector, "attach-window", &handled);
}

void InspectorFrontendClient::detachWindow()
{
    if (!m_inspectorWebView)
        return;

    gboolean handled = FALSE;
    g_signal_emit_by_name(m_webInspector, "detach-window", &handled);
}

void InspectorFrontendClient::setAttachedWindowHeight(unsigned height)
{
    notImplemented();
}

void InspectorFrontendClient::inspectedURLChanged(const String& newURL)
{
    if (!m_inspectorWebView)
        return;

    webkit_web_inspector_set_inspected_uri(m_webInspector, newURL.utf8().data());
}

}

