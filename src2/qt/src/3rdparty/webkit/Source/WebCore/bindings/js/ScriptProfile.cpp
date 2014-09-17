/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
#include "ScriptProfile.h"

#if ENABLE(JAVASCRIPT_DEBUGGER)
#include "InspectorValues.h"
#include "JSDOMBinding.h"
#include <profiler/Profile.h>
#include <profiler/ProfileNode.h>

namespace WebCore {

PassRefPtr<ScriptProfile> ScriptProfile::create(PassRefPtr<JSC::Profile> profile)
{
    if (!profile)
        return 0;
    return adoptRef(new ScriptProfile(profile));
}

ScriptProfile::ScriptProfile(PassRefPtr<JSC::Profile> profile)
    : m_profile(profile)
{
}

ScriptProfile::~ScriptProfile()
{
}

String ScriptProfile::title() const
{
    return ustringToString(m_profile->title());
}

unsigned int ScriptProfile::uid() const
{
    return m_profile->uid();
}

ScriptProfileNode* ScriptProfile::head() const
{
    return m_profile->head();
}

#if ENABLE(INSPECTOR)
static PassRefPtr<InspectorObject> buildInspectorObjectFor(const JSC::ProfileNode* node)
{
    RefPtr<InspectorObject> result = InspectorObject::create();

    result->setString("functionName", ustringToString(node->functionName()));
    result->setString("url", ustringToString(node->url()));
    result->setNumber("lineNumber", node->lineNumber());
    result->setNumber("totalTime", node->totalTime());
    result->setNumber("selfTime", node->selfTime());
    result->setNumber("numberOfCalls", node->numberOfCalls());
    result->setBoolean("visible", node->visible());
    result->setNumber("callUID", node->callIdentifier().hash());

    RefPtr<InspectorArray> childrenArray = InspectorArray::create();
    typedef Vector<RefPtr<JSC::ProfileNode> > ProfileNodesList;
    const ProfileNodesList& children = node->children();
    ProfileNodesList::const_iterator end = children.end();
    for (ProfileNodesList::const_iterator iter = children.begin(); iter != end; ++iter)
        childrenArray->pushObject(buildInspectorObjectFor(iter->get()));
    result->setArray("children", childrenArray);

    return result;
}

PassRefPtr<InspectorObject> ScriptProfile::buildInspectorObjectForHead() const
{
    return buildInspectorObjectFor(m_profile->head());
}
#endif

} // namespace WebCore

#endif // ENABLE(JAVASCRIPT_DEBUGGER)
