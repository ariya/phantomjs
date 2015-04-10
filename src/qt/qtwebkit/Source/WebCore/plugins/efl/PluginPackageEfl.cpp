/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2008 Collabora Ltd. All rights reserved.
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (C) 2008 INdT - Instituto Nokia de Tecnologia
 * Copyright (C) 2009-2010 ProFUSION embedded systems
 * Copyright (C) 2009-2011 Samsung Electronics
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

#include "MIMETypeRegistry.h"
#include "NotImplemented.h"
#include "PluginDatabase.h"
#include "PluginDebug.h"
#include "npruntime_impl.h"

#include <Eina.h>
#include <dlfcn.h>
#include <wtf/text/CString.h>

namespace WebCore {

bool PluginPackage::fetchInfo()
{
    const char *errmsg;

    if (!load())
        return false;

    NPP_GetValueProcPtr getValue = 0;
    NP_GetMIMEDescriptionFuncPtr getMIMEDescription = 0;

    getValue = reinterpret_cast<NPP_GetValueProcPtr>(eina_module_symbol_get(m_module, "NP_GetValue"));
    if ((errmsg = dlerror())) {
        EINA_LOG_ERR("Could not get symbol NP_GetValue: %s", errmsg);
        return false;
    }

    getMIMEDescription = reinterpret_cast<NP_GetMIMEDescriptionFuncPtr>(eina_module_symbol_get(m_module, "NP_GetMIMEDescription"));
    if ((errmsg = dlerror())) {
        EINA_LOG_ERR("Could not get symbol NP_GetMIMEDescription: %s", errmsg);
        return false;
    }

    char* buffer = 0;
    NPError err = getValue(0, NPPVpluginNameString, static_cast<void*>(&buffer));
    if (err != NPERR_NO_ERROR)
        return false;
    m_name = String::fromUTF8(buffer);

    buffer = 0;
    err = getValue(0, NPPVpluginDescriptionString, static_cast<void*>(&buffer));
    if (err != NPERR_NO_ERROR)
        return false;
    m_description = String::fromUTF8(buffer);
    determineModuleVersionFromDescription();

    String description = String::fromUTF8(getMIMEDescription());

    Vector<String> types;
    description.split(UChar(';'), false, types);

    for (size_t i = 0; i < types.size(); ++i) {
        Vector<String> mime;
        types[i].split(UChar(':'), true, mime);

        if (!mime.isEmpty() > 0) {
            Vector<String> exts;

            if (mime.size() > 1)
                mime[1].split(UChar(','), false, exts);

            determineQuirks(mime[0]);
            m_mimeToExtensions.add(mime[0], exts);

            if (mime.size() > 2)
                m_mimeToDescriptions.add(mime[0], mime[2]);
        }
    }

    return true;
}

uint16_t PluginPackage::NPVersion() const
{
    return NPVERS_HAS_PLUGIN_THREAD_ASYNC_CALL;
}

bool PluginPackage::load()
{
    if (m_isLoaded) {
        ++m_loadCount;
        return true;
    }

    m_module = eina_module_new(m_path.utf8().data());
    if (!m_module) {
        EINA_LOG_WARN("%s not loaded: eina_module_new() failed", m_path.utf8().data());
        return false;
    }
    if (!eina_module_load(m_module)) {
        const char* errorMessage = eina_error_msg_get(eina_error_get());
        EINA_LOG_WARN("%s not loaded: %s", m_path.utf8().data(), errorMessage ? errorMessage : "None");
        return false;
    }

    m_isLoaded = true;

    NPError err;

    NP_InitializeFuncPtr initialize = reinterpret_cast<NP_InitializeFuncPtr>(eina_module_symbol_get(m_module, "NP_Initialize"));
    if (!initialize) {
        EINA_LOG_ERR("Could not get symbol NP_Initialize");
        goto abort;
    }

    m_NPP_Shutdown = reinterpret_cast<NPP_ShutdownProcPtr>(eina_module_symbol_get(m_module, "NP_Shutdown"));
    if (!m_NPP_Shutdown) {
        EINA_LOG_ERR("Could not get symbol NP_Shutdown");
        goto abort;
    }

    memset(&m_pluginFuncs, 0, sizeof(m_pluginFuncs));
    m_pluginFuncs.size = sizeof(m_pluginFuncs);

    initializeBrowserFuncs();

#if defined(XP_UNIX)
    err = initialize(&m_browserFuncs, &m_pluginFuncs);
#else
    err = initialize(&m_browserFuncs);
#endif
    if (err != NPERR_NO_ERROR)
        goto abort;

    ++m_loadCount;
    return true;

abort:
    EINA_LOG_DBG("failed to load plugin, unload it without shutting it down.");
    unloadWithoutShutdown();
    return false;
}

}
