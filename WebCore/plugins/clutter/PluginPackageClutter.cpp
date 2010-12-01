/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2008 Collabora Ltd. All rights reserved.
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (C) 2008 Novell Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "PluginPackage.h"

#include <gio/gio.h>
#include <stdio.h>

#include "GOwnPtr.h"
#include "MIMETypeRegistry.h"
#include "NotImplemented.h"
#include "npruntime_impl.h"
#include "PluginDebug.h"
#include <wtf/text/CString.h>

namespace WebCore {

bool PluginPackage::fetchInfo()
{
#if defined(XP_UNIX)
    if (!load())
        return false;

    NP_GetMIMEDescriptionFuncPtr NP_GetMIMEDescription = 0;
    NPP_GetValueProcPtr NPP_GetValue = 0;

    g_module_symbol(m_module, "NP_GetMIMEDescription", (void**)&NP_GetMIMEDescription);
    g_module_symbol(m_module, "NP_GetValue", (void**)&NPP_GetValue);

    if (!NP_GetMIMEDescription || !NPP_GetValue)
        return false;

    char* buffer = 0;
    NPError err = NPP_GetValue(0, NPPVpluginNameString, &buffer);
    if (err == NPERR_NO_ERROR)
        m_name = buffer;

    buffer = 0;
    err = NPP_GetValue(0, NPPVpluginDescriptionString, &buffer);
    if (err == NPERR_NO_ERROR) {
        m_description = buffer;
        determineModuleVersionFromDescription();
    }

    const gchar* types = NP_GetMIMEDescription();
    if (!types)
        return true;

    gchar** mimeDescs = g_strsplit(types, ";", -1);
    for (int i = 0; mimeDescs[i] && mimeDescs[i][0]; i++) {
        gchar** mimeData = g_strsplit(mimeDescs[i], ":", 3);
        if (g_strv_length(mimeData) < 3) {
            g_strfreev(mimeData);
            continue;
        }

        String description = String::fromUTF8(mimeData[2]);
        gchar** extensions = g_strsplit(mimeData[1], ",", -1);

        Vector<String> extVector;
        for (int j = 0; extensions[j]; j++)
            extVector.append(String::fromUTF8(extensions[j]));

        determineQuirks(mimeData[0]);
        m_mimeToExtensions.add(mimeData[0], extVector);
        m_mimeToDescriptions.add(mimeData[0], description);

        g_strfreev(extensions);
        g_strfreev(mimeData);
    }
    g_strfreev(mimeDescs);

    return true;
#else
    notImplemented();
    return false;
#endif
}

static NPError staticPluginQuirkRequiresGtkToolKit_NPN_GetValue(NPP instance, NPNVariable variable, void* value)
{
    if (variable == NPNVToolkit) {
        *static_cast<uint32_t*>(value) = 2;
        return NPERR_NO_ERROR;
    }

    return NPN_GetValue(instance, variable, value);
}

static void initializeGtk(GModule* module = 0)
{
    // Ensures missing Gtk initialization in some versions of Adobe's flash player
    // plugin do not cause crashes. See BR# 40567, 44324, and 44405 for details.  
    if (module) {
        typedef void *(*gtk_init_ptr)(int*, char***);
        gtk_init_ptr gtkInit;
        g_module_symbol(module, "gtk_init", (gpointer*)&gtkInit);
        if (gtkInit) {
            // Prevent gtk_init() from replacing the X error handlers, since the Gtk
            // handlers abort when they receive an X error, thus killing the viewer.
            // FIXME: this may require including X headers
            int (*old_error_handler)(Display*, XErrorEvent*) = XSetErrorHandler(0);
            int (*old_io_error_handler)(Display*) = XSetIOErrorHandler(0);

            gtkInit(0, 0);

            XSetErrorHandler(old_error_handler);
            XSetIOErrorHandler(old_io_error_handler);
            
            return;
        }
    }

    // FIXME: probably should use g_module_build_path here
    GModule *library = g_module_open("libgtk-x11-2.0.so.0", G_MODULE_BIND_LOCAL);
    if (library) {
        typedef void *(*gtk_init_check_ptr)(int*, char***);
        gtk_init_check_ptr gtkInitCheck;
        g_module_symbol(library, "gtk_init_check", (gpointer*)&gtkInitCheck);
        // NOTE: We're using gtk_init_check() since gtk_init() calls exit() on failure.
        if (gtkInitCheck)
            (void) gtkInitCheck(0, 0);
    }
}

bool PluginPackage::load()
{
    if (m_isLoaded) {
        m_loadCount++;
        return true;
    }

    GOwnPtr<gchar> finalPath(g_strdup(m_path.utf8().data()));
    while (g_file_test(finalPath.get(), G_FILE_TEST_IS_SYMLINK)) {
        GOwnPtr<GFile> file(g_file_new_for_path(finalPath.get()));
        GOwnPtr<GFile> dir(g_file_get_parent(file.get()));
        GOwnPtr<gchar> linkPath(g_file_read_link(finalPath.get(), 0));
        GOwnPtr<GFile> resolvedFile(g_file_resolve_relative_path(dir.get(), linkPath.get()));
        finalPath.set(g_file_get_path(resolvedFile.get()));
    }

    // No joke. If there is a netscape component in the path, go back
    // to the symlink, as flash breaks otherwise.
    // See http://src.chromium.org/viewvc/chrome/trunk/src/webkit/glue/plugins/plugin_list_posix.cc
    GOwnPtr<gchar> baseName(g_path_get_basename(finalPath.get()));
    if (!g_strcmp0(baseName.get(), "libflashplayer.so")
        && g_strstr_len(finalPath.get(), -1, "/netscape/"))
        finalPath.set(g_strdup(m_path.utf8().data()));

    m_module = g_module_open(finalPath.get(), G_MODULE_BIND_LOCAL);

    if (!m_module) {
        LOG(Plugins,"Module Load Failed :%s, Error:%s\n", (m_path.utf8()).data(), g_module_error());
        return false;
    }

    m_isLoaded = true;

    NP_InitializeFuncPtr NP_Initialize = 0;
    m_NPP_Shutdown = 0;

    NPError npErr;

    g_module_symbol(m_module, "NP_Initialize", (void**)&NP_Initialize);
    g_module_symbol(m_module, "NP_Shutdown", (void**)&m_NPP_Shutdown);

    if (!NP_Initialize || !m_NPP_Shutdown)
        goto abort;

    memset(&m_pluginFuncs, 0, sizeof(m_pluginFuncs));
    m_pluginFuncs.size = sizeof(m_pluginFuncs);

    initializeBrowserFuncs();
    
    if (m_path.contains("npwrapper.")) {
        // nspluginwrapper relies on the toolkit value to know if glib is available
        // It does so in NP_Initialize with a null instance, therefore it is done this way:
        m_browserFuncs.getvalue = staticPluginQuirkRequiresGtkToolKit_NPN_GetValue;
        // Workaround Adobe's failure to properly initialize Gtk in some versions
        // of their flash player plugin.
        initializeGtk();
    } else if (m_path.contains("flashplayer")) {
        // Workaround Adobe's failure to properly initialize Gtk in some versions
        // of their flash player plugin.
        initializeGtk(m_module);
    }
    
#if defined(XP_UNIX)
    npErr = NP_Initialize(&m_browserFuncs, &m_pluginFuncs);
#else
    npErr = NP_Initialize(&m_browserFuncs);
#endif
    if (npErr != NPERR_NO_ERROR)
        goto abort;

    m_loadCount++;
    return true;

abort:
    unloadWithoutShutdown();
    return false;
}

uint16_t PluginPackage::NPVersion() const
{
    return NPVERS_HAS_PLUGIN_THREAD_ASYNC_CALL;
}
}
