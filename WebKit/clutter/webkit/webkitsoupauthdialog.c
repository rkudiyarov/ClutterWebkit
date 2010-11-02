/*
 * Copyright (C) 2009 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#define LIBSOUP_I_HAVE_READ_BUG_594377_AND_KNOW_SOUP_PASSWORD_MANAGER_MIGHT_GO_AWAY

#include <glib/gi18n-lib.h>
#include <libsoup/soup.h>

#include "webkitmarshal.h"
#include "webkitsoupauthdialog.h"

/**
 * SECTION:webkitsoupauthdialog
 * @short_description: A #SoupSessionFeature to provide a simple
 * authentication dialog for HTTP basic auth support.
 *
 * #WebKitSoupAuthDialog is a #SoupSessionFeature that you can attach to your
 * #SoupSession to provide a simple authentication dialog while
 * handling HTTP basic auth. It is built as a simple C-only module
 * to ease reuse.
 */

static void webkit_soup_auth_dialog_session_feature_init(SoupSessionFeatureInterface* feature_interface, gpointer interface_data);
static void attach(SoupSessionFeature* manager, SoupSession* session);
static void detach(SoupSessionFeature* manager, SoupSession* session);

enum {
    CURRENT_TOPLEVEL,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_CODE(WebKitSoupAuthDialog, webkit_soup_auth_dialog, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(SOUP_TYPE_SESSION_FEATURE,
                                              webkit_soup_auth_dialog_session_feature_init))

static void webkit_soup_auth_dialog_class_init(WebKitSoupAuthDialogClass* klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS(klass);

    signals[CURRENT_TOPLEVEL] =
      g_signal_new("current-toplevel",
                   G_OBJECT_CLASS_TYPE(object_class),
                   G_SIGNAL_RUN_LAST,
                   G_STRUCT_OFFSET(WebKitSoupAuthDialogClass, current_toplevel),
                   NULL, NULL,
                   webkit_marshal_OBJECT__OBJECT,
                   CLUTTER_TYPE_ACTOR, 1,
                   SOUP_TYPE_MESSAGE);
}

static void webkit_soup_auth_dialog_init(WebKitSoupAuthDialog* instance)
{
}

static void webkit_soup_auth_dialog_session_feature_init(SoupSessionFeatureInterface *feature_interface,
                                                         gpointer interface_data)
{
    feature_interface->attach = attach;
    feature_interface->detach = detach;
}

typedef struct _WebKitAuthData {
    SoupMessage* msg;
    SoupAuth* auth;
    SoupSession* session;
    SoupSessionFeature* manager;
    char *username;
    char *password;
} WebKitAuthData;

static void free_authData(WebKitAuthData* authData)
{
    g_object_unref(authData->msg);
    g_free(authData->username);
    g_free(authData->password);
    g_slice_free(WebKitAuthData, authData);
}

#ifdef SOUP_TYPE_PASSWORD_MANAGER
static void save_password_callback(SoupMessage* msg, WebKitAuthData* authData)
{
    /* Anything but 401 and 5xx means the password was accepted */
    if (msg->status_code != 401 && msg->status_code < 500)
        soup_auth_save_password(authData->auth, authData->username, authData->password);

    /* Disconnect the callback. If the authentication succeeded we are
     * done, and if it failed we'll create a new authData and we'll
     * connect to 'got-headers' again in response_callback */
    g_signal_handlers_disconnect_by_func(msg, save_password_callback, authData);

    free_authData(authData);
}
#endif

static gboolean session_can_save_passwords(SoupSession* session)
{
#ifdef SOUP_TYPE_PASSWORD_MANAGER
    return soup_session_get_feature(session, SOUP_TYPE_PASSWORD_MANAGER) != NULL;
#else
    return FALSE;
#endif
}

static void show_auth_dialog(WebKitAuthData* authData, const char* login, const char* password)
{
}

static void session_authenticate(SoupSession* session, SoupMessage* msg, SoupAuth* auth, gboolean retrying, gpointer user_data)
{
    SoupURI* uri;
    WebKitAuthData* authData;
    SoupSessionFeature* manager = (SoupSessionFeature*)user_data;
#ifdef SOUP_TYPE_PASSWORD_MANAGER
    GSList* users;
#endif
    const char *login, *password;

    soup_session_pause_message(session, msg);
    /* We need to make sure the message sticks around when pausing it */
    g_object_ref(msg);

    uri = soup_message_get_uri(msg);
    authData = g_slice_new0(WebKitAuthData);
    authData->msg = msg;
    authData->auth = auth;
    authData->session = session;
    authData->manager = manager;

    login = password = NULL;

#ifdef SOUP_TYPE_PASSWORD_MANAGER
    users = soup_auth_get_saved_users(auth);
    if (users) {
        login = users->data;
        password = soup_auth_get_saved_password(auth, login);
        g_slist_free(users);
    }
#endif

    show_auth_dialog(authData, login, password);
}

static void attach(SoupSessionFeature* manager, SoupSession* session)
{
    g_signal_connect(session, "authenticate", G_CALLBACK(session_authenticate), manager);
}

static void detach(SoupSessionFeature* manager, SoupSession* session)
{
    g_signal_handlers_disconnect_by_func(session, session_authenticate, manager);
}


