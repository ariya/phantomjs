/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#include "PluginTest.h"

#include "PluginObject.h"

using namespace std;

// NPP_SetWindow should be called with a null window handle as destruction begins on non-Mac platforms.

class NPPSetWindowCalledDuringDestruction : public PluginTest {
public:
    NPPSetWindowCalledDuringDestruction(NPP, const string& identifier);

    void setWillBeDestroyed() { m_willBeDestroyed = true; }

private:
    struct ScriptObject : Object<ScriptObject> {
        bool hasMethod(NPIdentifier);
        bool invoke(NPIdentifier, const NPVariant*, uint32_t, NPVariant*);
    };

    virtual NPError NPP_GetValue(NPPVariable, void*);
    virtual NPError NPP_SetWindow(NPWindow*);
    virtual NPError NPP_Destroy(NPSavedData**);

    bool m_willBeDestroyed;
    bool m_setWindowCalledBeforeDestruction;
    bool m_setWindowCalledDuringDestruction;
};

static PluginTest::Register<NPPSetWindowCalledDuringDestruction> registrar("npp-set-window-called-during-destruction");

NPPSetWindowCalledDuringDestruction::NPPSetWindowCalledDuringDestruction(NPP npp, const string& identifier)
    : PluginTest(npp, identifier)
    , m_willBeDestroyed(false)
    , m_setWindowCalledBeforeDestruction(false)
    , m_setWindowCalledDuringDestruction(false)
{
}

NPError NPPSetWindowCalledDuringDestruction::NPP_GetValue(NPPVariable variable, void* value)
{
    if (variable != NPPVpluginScriptableNPObject)
        return NPERR_GENERIC_ERROR;

    *static_cast<NPObject**>(value) = ScriptObject::create(this);

    return NPERR_NO_ERROR;
}

NPError NPPSetWindowCalledDuringDestruction::NPP_SetWindow(NPWindow* window)
{
    if (m_willBeDestroyed) {
        m_setWindowCalledDuringDestruction = true;
        if (!m_setWindowCalledBeforeDestruction) {
            log("Fail: setWillBeDestroyed() was called before the initial NPP_SetWindow call");
            return NPERR_NO_ERROR;
        }
#ifndef XP_MACOSX
        if (window->window)
            log("Fail: NPP_SetWindow passed a non-null window during plugin destruction");
#endif
        return NPERR_NO_ERROR;
    }

    if (m_setWindowCalledBeforeDestruction) {
        log("Fail: NPP_SetWindow called more than once before plugin destruction");
        return NPERR_NO_ERROR;
    }

    m_setWindowCalledBeforeDestruction = true;
    return NPERR_NO_ERROR;
}

NPError NPPSetWindowCalledDuringDestruction::NPP_Destroy(NPSavedData**)
{
#ifdef XP_MACOSX
    bool shouldHaveBeenCalledDuringDestruction = false;
#else
    bool shouldHaveBeenCalledDuringDestruction = true;
#endif

    if (m_setWindowCalledDuringDestruction == shouldHaveBeenCalledDuringDestruction)
        log("Success: NPP_SetWindow %s called during plugin destruction", shouldHaveBeenCalledDuringDestruction ? "was" : "was not");
    else
        log("Fail: NPP_SetWindow %s called during plugin destruction", shouldHaveBeenCalledDuringDestruction ? "was not" : "was");

    return NPERR_NO_ERROR;
}

bool NPPSetWindowCalledDuringDestruction::ScriptObject::hasMethod(NPIdentifier methodName)
{
    return methodName == pluginTest()->NPN_GetStringIdentifier("setWillBeDestroyed");
}

bool NPPSetWindowCalledDuringDestruction::ScriptObject::invoke(NPIdentifier identifier, const NPVariant*, uint32_t, NPVariant*)
{
    assert(identifier == pluginTest()->NPN_GetStringIdentifier("setWillBeDestroyed"));
    static_cast<NPPSetWindowCalledDuringDestruction*>(pluginTest())->setWillBeDestroyed();
    return true;
}
