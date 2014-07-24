/*
 * Copyright (C) 2011, 2012 Google Inc. All rights reserved.
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

#ifndef InspectorFileSystemAgent_h
#define InspectorFileSystemAgent_h

#if ENABLE(INSPECTOR) && ENABLE(FILE_SYSTEM)

#include "InspectorBaseAgent.h"

#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>

namespace WebCore {

class InspectorFrontend;
class InspectorPageAgent;
class ScriptExecutionContext;
class SecurityOrigin;

class InspectorFileSystemAgent : public InspectorBaseAgent<InspectorFileSystemAgent>, public InspectorBackendDispatcher::FileSystemCommandHandler {
public:
    static PassOwnPtr<InspectorFileSystemAgent> create(InstrumentingAgents*, InspectorPageAgent*, InspectorCompositeState*);
    virtual ~InspectorFileSystemAgent();

    virtual void enable(ErrorString*) OVERRIDE;
    virtual void disable(ErrorString*) OVERRIDE;

    virtual void requestFileSystemRoot(ErrorString*, const String& origin, const String& typeString, PassRefPtr<RequestFileSystemRootCallback>) OVERRIDE;
    virtual void requestDirectoryContent(ErrorString*, const String& url, PassRefPtr<RequestDirectoryContentCallback>) OVERRIDE;
    virtual void requestMetadata(ErrorString*, const String& url, PassRefPtr<RequestMetadataCallback>) OVERRIDE;
    virtual void requestFileContent(ErrorString*, const String& url, bool readAsText, const int* start, const int* end, const String* charset, PassRefPtr<RequestFileContentCallback>) OVERRIDE;
    virtual void deleteEntry(ErrorString*, const String& url, PassRefPtr<DeleteEntryCallback>) OVERRIDE;

    virtual void clearFrontend() OVERRIDE;
    virtual void restore() OVERRIDE;

private:
    InspectorFileSystemAgent(InstrumentingAgents*, InspectorPageAgent*, InspectorCompositeState*);
    bool assertEnabled(ErrorString*);
    ScriptExecutionContext* assertScriptExecutionContextForOrigin(ErrorString*, SecurityOrigin*);

    InspectorPageAgent* m_pageAgent;
    bool m_enabled;
};

} // namespace WebCore

#endif // ENABLE(INSPECTOR) && ENABLE(FILE_SYSTEM)
#endif // InspectorFileSystemAgent_h
