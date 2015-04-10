/*
 * Copyright (C) 2008, 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "LegacyProfiler.h"

#include "CallFrame.h"
#include "CodeBlock.h"
#include "CommonIdentifiers.h"
#include "InternalFunction.h"
#include "JSFunction.h"
#include "JSGlobalObject.h"
#include "Nodes.h"
#include "Operations.h"
#include "Profile.h"
#include "ProfileGenerator.h"
#include "ProfileNode.h"
#include <stdio.h>

namespace JSC {

static const char* GlobalCodeExecution = "(program)";
static const char* AnonymousFunction = "(anonymous function)";
static unsigned ProfilesUID = 0;

static CallIdentifier createCallIdentifierFromFunctionImp(ExecState*, JSObject*, const String& defaultSourceURL, int defaultLineNumber);

LegacyProfiler* LegacyProfiler::s_sharedLegacyProfiler = 0;

LegacyProfiler* LegacyProfiler::profiler()
{
    if (!s_sharedLegacyProfiler)
        s_sharedLegacyProfiler = new LegacyProfiler();
    return s_sharedLegacyProfiler;
}   

void LegacyProfiler::startProfiling(ExecState* exec, const String& title)
{
    ASSERT_ARG(title, !title.isNull());

    if (!exec)
        return;

    // Check if we currently have a Profile for this global ExecState and title.
    // If so return early and don't create a new Profile.
    JSGlobalObject* origin = exec->lexicalGlobalObject();

    for (size_t i = 0; i < m_currentProfiles.size(); ++i) {
        ProfileGenerator* profileGenerator = m_currentProfiles[i].get();
        if (profileGenerator->origin() == origin && profileGenerator->title() == title)
            return;
    }

    exec->vm().m_enabledProfiler = this;
    RefPtr<ProfileGenerator> profileGenerator = ProfileGenerator::create(exec, title, ++ProfilesUID);
    m_currentProfiles.append(profileGenerator);
}

PassRefPtr<Profile> LegacyProfiler::stopProfiling(ExecState* exec, const String& title)
{
    if (!exec)
        return 0;

    JSGlobalObject* origin = exec->lexicalGlobalObject();
    for (ptrdiff_t i = m_currentProfiles.size() - 1; i >= 0; --i) {
        ProfileGenerator* profileGenerator = m_currentProfiles[i].get();
        if (profileGenerator->origin() == origin && (title.isNull() || profileGenerator->title() == title)) {
            profileGenerator->stopProfiling();
            RefPtr<Profile> returnProfile = profileGenerator->profile();

            m_currentProfiles.remove(i);
            if (!m_currentProfiles.size())
                exec->vm().m_enabledProfiler = 0;
            
            return returnProfile;
        }
    }

    return 0;
}

void LegacyProfiler::stopProfiling(JSGlobalObject* origin)
{
    for (ptrdiff_t i = m_currentProfiles.size() - 1; i >= 0; --i) {
        ProfileGenerator* profileGenerator = m_currentProfiles[i].get();
        if (profileGenerator->origin() == origin) {
            profileGenerator->stopProfiling();
            m_currentProfiles.remove(i);
            if (!m_currentProfiles.size())
                origin->vm().m_enabledProfiler = 0;
        }
    }
}

static inline void dispatchFunctionToProfiles(ExecState* callerOrHandlerCallFrame, const Vector<RefPtr<ProfileGenerator> >& profiles, ProfileGenerator::ProfileFunction function, const CallIdentifier& callIdentifier, unsigned currentProfileTargetGroup)
{
    for (size_t i = 0; i < profiles.size(); ++i) {
        if (profiles[i]->profileGroup() == currentProfileTargetGroup || !profiles[i]->origin())
            (profiles[i].get()->*function)(callerOrHandlerCallFrame, callIdentifier);
    }
}

void LegacyProfiler::willExecute(ExecState* callerCallFrame, JSValue function)
{
    ASSERT(!m_currentProfiles.isEmpty());

    dispatchFunctionToProfiles(callerCallFrame, m_currentProfiles, &ProfileGenerator::willExecute, createCallIdentifier(callerCallFrame, function, "", 0), callerCallFrame->lexicalGlobalObject()->profileGroup());
}

void LegacyProfiler::willExecute(ExecState* callerCallFrame, const String& sourceURL, int startingLineNumber)
{
    ASSERT(!m_currentProfiles.isEmpty());

    CallIdentifier callIdentifier = createCallIdentifier(callerCallFrame, JSValue(), sourceURL, startingLineNumber);

    dispatchFunctionToProfiles(callerCallFrame, m_currentProfiles, &ProfileGenerator::willExecute, callIdentifier, callerCallFrame->lexicalGlobalObject()->profileGroup());
}

void LegacyProfiler::didExecute(ExecState* callerCallFrame, JSValue function)
{
    ASSERT(!m_currentProfiles.isEmpty());

    dispatchFunctionToProfiles(callerCallFrame, m_currentProfiles, &ProfileGenerator::didExecute, createCallIdentifier(callerCallFrame, function, "", 0), callerCallFrame->lexicalGlobalObject()->profileGroup());
}

void LegacyProfiler::didExecute(ExecState* callerCallFrame, const String& sourceURL, int startingLineNumber)
{
    ASSERT(!m_currentProfiles.isEmpty());

    dispatchFunctionToProfiles(callerCallFrame, m_currentProfiles, &ProfileGenerator::didExecute, createCallIdentifier(callerCallFrame, JSValue(), sourceURL, startingLineNumber), callerCallFrame->lexicalGlobalObject()->profileGroup());
}

void LegacyProfiler::exceptionUnwind(ExecState* handlerCallFrame)
{
    ASSERT(!m_currentProfiles.isEmpty());

    dispatchFunctionToProfiles(handlerCallFrame, m_currentProfiles, &ProfileGenerator::exceptionUnwind, createCallIdentifier(handlerCallFrame, JSValue(), "", 0), handlerCallFrame->lexicalGlobalObject()->profileGroup());
}

CallIdentifier LegacyProfiler::createCallIdentifier(ExecState* exec, JSValue functionValue, const String& defaultSourceURL, int defaultLineNumber)
{
    if (!functionValue)
        return CallIdentifier(GlobalCodeExecution, defaultSourceURL, defaultLineNumber);
    if (!functionValue.isObject())
        return CallIdentifier("(unknown)", defaultSourceURL, defaultLineNumber);
    if (asObject(functionValue)->inherits(&JSFunction::s_info) || asObject(functionValue)->inherits(&InternalFunction::s_info))
        return createCallIdentifierFromFunctionImp(exec, asObject(functionValue), defaultSourceURL, defaultLineNumber);
    return CallIdentifier(makeString("(", asObject(functionValue)->methodTable()->className(asObject(functionValue)), " object)"), defaultSourceURL, defaultLineNumber);
}

CallIdentifier createCallIdentifierFromFunctionImp(ExecState* exec, JSObject* function, const String& defaultSourceURL, int defaultLineNumber)
{
    const String& name = getCalculatedDisplayName(exec, function);
    JSFunction* jsFunction = jsDynamicCast<JSFunction*>(function);
    if (jsFunction && !jsFunction->isHostFunction())
        return CallIdentifier(name.isEmpty() ? AnonymousFunction : name, jsFunction->jsExecutable()->sourceURL(), jsFunction->jsExecutable()->lineNo());
    return CallIdentifier(name.isEmpty() ? AnonymousFunction : name, defaultSourceURL, defaultLineNumber);
}

} // namespace JSC
