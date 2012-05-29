/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef InspectorFrontendClientLocal_h
#define InspectorFrontendClientLocal_h

#include "InspectorFrontendClient.h"
#include "ScriptState.h"
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>

namespace WebCore {

class InspectorController;
class InspectorFrontendHost;
class Page;

class InspectorFrontendClientLocal : public InspectorFrontendClient {
    WTF_MAKE_NONCOPYABLE(InspectorFrontendClientLocal); WTF_MAKE_FAST_ALLOCATED;
public:
    class Settings {
    public:
        Settings() { }
        virtual ~Settings() { }
        virtual String getProperty(const String& name);
        virtual void setProperty(const String& name, const String& value);
    };

    InspectorFrontendClientLocal(InspectorController*, Page*, PassOwnPtr<Settings>);
    virtual ~InspectorFrontendClientLocal();
    
    virtual void windowObjectCleared();
    virtual void frontendLoaded();

    virtual void moveWindowBy(float x, float y);

    virtual void requestAttachWindow();
    virtual void requestDetachWindow();
    virtual void changeAttachedWindowHeight(unsigned);
    virtual void saveAs(const String&, const String&) { }

    virtual void attachWindow() = 0;
    virtual void detachWindow() = 0;
    bool canAttachWindow();

    virtual void sendMessageToBackend(const String& message);

protected:
    virtual void setAttachedWindowHeight(unsigned) = 0;
    void setAttachedWindow(bool);
    void restoreAttachedWindowHeight();

private:
    static unsigned constrainedAttachedWindowHeight(unsigned preferredHeight, unsigned totalWindowHeight);

    friend class FrontendMenuProvider;
    InspectorController* m_inspectorController;
    Page* m_frontendPage;
    ScriptState* m_frontendScriptState;
    // TODO(yurys): this ref shouldn't be needed.
    RefPtr<InspectorFrontendHost> m_frontendHost;
    OwnPtr<InspectorFrontendClientLocal::Settings> m_settings;
};

} // namespace WebCore

#endif
