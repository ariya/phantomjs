/*
 * Copyright (C) 2011 Samsung Electronics
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "InjectedBundle.h"

#include "WKBundleAPICast.h"
#include "WKBundleInitialize.h"
#include <wtf/text/CString.h>

using namespace WebCore;

namespace WebKit {

bool InjectedBundle::load(APIObject* initializationUserData)
{
    m_platformBundle = eina_module_new(m_path.utf8().data());
    if (!m_platformBundle) {
        EINA_LOG_CRIT("Error loading the injected bundle: eina_module_new() failed");
        return false;
    }
    if (!eina_module_load(m_platformBundle)) {
        EINA_LOG_CRIT("Error loading the injected bundle from %s: %s", m_path.utf8().data(), eina_error_msg_get(eina_error_get()));
        eina_module_free(m_platformBundle);
        m_platformBundle = 0;
        return false;
    }

    WKBundleInitializeFunctionPtr initializeFunction = reinterpret_cast<WKBundleInitializeFunctionPtr>(eina_module_symbol_get(m_platformBundle, "WKBundleInitialize"));
    if (!initializeFunction) {
        EINA_LOG_CRIT("Error loading WKBundleInitialize symbol from injected bundle.");
        return false;
    }

    initializeFunction(toAPI(this), toAPI(initializationUserData));
    return true;
}

void InjectedBundle::activateMacFontAscentHack()
{
}

} // namespace WebKit
