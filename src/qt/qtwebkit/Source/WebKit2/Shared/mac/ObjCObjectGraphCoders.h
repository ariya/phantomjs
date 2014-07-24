/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#ifndef ObjCObjectGraphCoders_h
#define ObjCObjectGraphCoders_h

#include "ArgumentDecoder.h"
#include "ArgumentEncoder.h"
#include "ObjCObjectGraph.h"
#include <wtf/RefPtr.h>

namespace WebKit {

class WebProcess;
class WebProcessProxy;

class WebContextObjCObjectGraphEncoder {
public:
    explicit WebContextObjCObjectGraphEncoder(ObjCObjectGraph*);
    void encode(CoreIPC::ArgumentEncoder&) const;

private:
    ObjCObjectGraph* m_objectGraph;
};

class WebContextObjCObjectGraphDecoder {
public:
    explicit WebContextObjCObjectGraphDecoder(RefPtr<ObjCObjectGraph>&, WebProcessProxy*);
    static bool decode(CoreIPC::ArgumentDecoder&, WebContextObjCObjectGraphDecoder&);

private:
    RefPtr<ObjCObjectGraph>& m_objectGraph;
    WebProcessProxy* m_process;
};


class InjectedBundleObjCObjectGraphEncoder {
public:
    explicit InjectedBundleObjCObjectGraphEncoder(ObjCObjectGraph*);
    void encode(CoreIPC::ArgumentEncoder&) const;

private:
    ObjCObjectGraph* m_objectGraph;
};

class InjectedBundleObjCObjectGraphDecoder {
public:
    explicit InjectedBundleObjCObjectGraphDecoder(RefPtr<ObjCObjectGraph>&, WebProcess*);
    static bool decode(CoreIPC::ArgumentDecoder&, InjectedBundleObjCObjectGraphDecoder&);

private:
    RefPtr<ObjCObjectGraph>& m_objectGraph;
    WebProcess* m_process;
};

} // namespace WebKit

#endif // ObjCObjectGraphCoders_h
