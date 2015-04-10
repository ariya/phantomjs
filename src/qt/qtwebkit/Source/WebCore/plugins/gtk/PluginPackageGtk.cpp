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

#include "GOwnPtrGtk.h"
#include "GRefPtrGtk.h"
#include "MIMETypeRegistry.h"
#include "NotImplemented.h"
#include "npruntime_impl.h"
#include "PluginDebug.h"
#include <gio/gio.h>
#include <wtf/text/CString.h>

namespace WebCore {

bool PluginPackage::fetchInfo()
{
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
        m_name = String::fromUTF8(buffer);

    buffer = 0;
    err = NPP_GetValue(0, NPPVpluginDescriptionString, &buffer);
    if (err == NPERR_NO_ERROR) {
        m_description = String::fromUTF8(buffer);
        determineModuleVersionFromDescription();
    }

    const gchar* types = NP_GetMIMEDescription();
    if (!types)
        return true;

    gchar** mimeDescs = g_strsplit(types, ";", -1);
    for (int i = 0; mimeDescs[i] && mimeDescs[i][0]; i++) {
        GOwnPtr<char> mime(g_utf8_strdown(mimeDescs[i], -1));
        gchar** mimeData = g_strsplit(mime.get(), ":", 3);
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
}

static int webkitgtkXError(Display* xdisplay, XErrorEvent* error)
{
    gchar errorMessage[64];
    XGetErrorText(xdisplay, error->error_code, errorMessage, 63);
    g_warning("The program '%s' received an X Window System error.\n"
              "This probably reflects a bug in the Adobe Flash plugin.\n"
              "The error was '%s'.\n"
              "  (Details: serial %ld error_code %d request_code %d minor_code %d)\n",
              g_get_prgname(), errorMessage,
              error->serial, error->error_code,
              error->request_code, error->minor_code);
    return 0;
}

static bool moduleMixesGtkSymbols(GModule* module)
{
    void* symbol;
#ifdef GTK_API_VERSION_2
    return g_module_symbol(module, "gtk_application_get_type", &symbol);
#else
    return g_module_symbol(module, "gtk_object_get_type", &symbol);
#endif
}

bool PluginPackage::load()
{
    if (m_isLoaded) {
        m_loadCount++;
        return true;
    }

    GOwnPtr<gchar> finalPath(g_strdup(m_path.utf8().data()));
    while (g_file_test(finalPath.get(), G_FILE_TEST_IS_SYMLINK)) {
        GRefPtr<GFile> file = adoptGRef(g_file_new_for_path(finalPath.get()));
        GRefPtr<GFile> dir = adoptGRef(g_file_get_parent(file.get()));
        GOwnPtr<gchar> linkPath(g_file_read_link(finalPath.get(), 0));
        GRefPtr<GFile> resolvedFile = adoptGRef(g_file_resolve_relative_path(dir.get(), linkPath.get()));
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

    if (moduleMixesGtkSymbols(m_module)) {
        LOG(Plugins, "Ignoring module '%s' to avoid mixing GTK+ 2 and GTK+ 3 symbols.\n", m_path.utf8().data());
        return false;
    }

    m_isLoaded = true;

    if (!g_strcmp0(baseName.get(), "libflashplayer.so")) {
        // Flash plugin can produce X errors that are handled by the GDK X error handler, which
        // exits the process. Since we don't want to crash due to flash bugs, we install a
        // custom error handler to show a warning when a X error happens without aborting.
        XSetErrorHandler(webkitgtkXError);
    }

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

    npErr = NP_Initialize(&m_browserFuncs, &m_pluginFuncs);
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
