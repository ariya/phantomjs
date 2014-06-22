/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef InjectedScript_h
#define InjectedScript_h

#include "InjectedScriptBase.h"
#include "InjectedScriptManager.h"
#include "InspectorTypeBuilder.h"
#include "ScriptArguments.h"
#include "ScriptObject.h"
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class InjectedScriptModule;
class Node;
class SerializedScriptValue;

#if ENABLE(INSPECTOR)

class InjectedScript : public InjectedScriptBase {
public:
    InjectedScript();
    ~InjectedScript() { }

    void evaluate(ErrorString*,
                  const String& expression,
                  const String& objectGroup,
                  bool includeCommandLineAPI,
                  bool returnByValue,
                  bool generatePreview,
                  RefPtr<TypeBuilder::Runtime::RemoteObject>* result,
                  TypeBuilder::OptOutput<bool>* wasThrown);
    void callFunctionOn(ErrorString*,
                        const String& objectId,
                        const String& expression,
                        const String& arguments,
                        bool returnByValue,
                        bool generatePreview,
                        RefPtr<TypeBuilder::Runtime::RemoteObject>* result,
                        TypeBuilder::OptOutput<bool>* wasThrown);
    void evaluateOnCallFrame(ErrorString*,
                             const ScriptValue& callFrames,
                             const String& callFrameId,
                             const String& expression,
                             const String& objectGroup,
                             bool includeCommandLineAPI,
                             bool returnByValue,
                             bool generatePreview,
                             RefPtr<TypeBuilder::Runtime::RemoteObject>* result,
                             TypeBuilder::OptOutput<bool>* wasThrown);
    void restartFrame(ErrorString*, const ScriptValue& callFrames, const String& callFrameId, RefPtr<InspectorObject>* result);
    void setVariableValue(ErrorString*, const ScriptValue& callFrames, const String* callFrameIdOpt, const String* functionObjectIdOpt, int scopeNumber, const String& variableName, const String& newValueStr);
    void getFunctionDetails(ErrorString*, const String& functionId, RefPtr<TypeBuilder::Debugger::FunctionDetails>* result);
    void getProperties(ErrorString*, const String& objectId, bool ownProperties, RefPtr<TypeBuilder::Array<TypeBuilder::Runtime::PropertyDescriptor> >* result);
    void getInternalProperties(ErrorString*, const String& objectId, RefPtr<TypeBuilder::Array<TypeBuilder::Runtime::InternalPropertyDescriptor> >* result);
    Node* nodeForObjectId(const String& objectId);
    void releaseObject(const String& objectId);

#if ENABLE(JAVASCRIPT_DEBUGGER)
    PassRefPtr<TypeBuilder::Array<TypeBuilder::Debugger::CallFrame> > wrapCallFrames(const ScriptValue&);
#endif

    PassRefPtr<TypeBuilder::Runtime::RemoteObject> wrapObject(const ScriptValue&, const String& groupName, bool generatePreview = false) const;
    PassRefPtr<TypeBuilder::Runtime::RemoteObject> wrapTable(const ScriptValue& table, const ScriptValue& columns) const;
    PassRefPtr<TypeBuilder::Runtime::RemoteObject> wrapNode(Node*, const String& groupName);
    ScriptValue findObjectById(const String& objectId) const;

    void inspectNode(Node*);
    void releaseObjectGroup(const String&);

private:
    friend class InjectedScriptModule;
    friend InjectedScript InjectedScriptManager::injectedScriptFor(ScriptState*);
    InjectedScript(ScriptObject, InspectedStateAccessCheck);

    ScriptValue nodeAsScriptValue(Node*);
};

#endif

} // namespace WebCore

#endif
