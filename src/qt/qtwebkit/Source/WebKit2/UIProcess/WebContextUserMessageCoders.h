/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#include "UserMessageCoders.h"
#include "WebContext.h"
#include "WebFrameProxy.h"
#include "WebPageGroup.h"
#include "WebPageGroupData.h"
#include "WebPageProxy.h"

#if PLATFORM(MAC)
#include "ObjCObjectGraphCoders.h"
#endif

namespace WebKit {

// Adds
// - Page -> BundlePage
// - Frame -> BundleFrame
// - PageGroup -> BundlePageGroup

class WebContextUserMessageEncoder : public UserMessageEncoder<WebContextUserMessageEncoder> {
public:
    typedef UserMessageEncoder<WebContextUserMessageEncoder> Base;

    explicit WebContextUserMessageEncoder(APIObject* root) 
        : Base(root)
    {
    }

    void encode(CoreIPC::ArgumentEncoder& encoder) const
    {
        APIObject::Type type = APIObject::TypeNull;
        if (baseEncode(encoder, type))
            return;

        switch (type) {
        case APIObject::TypePage: {
            WebPageProxy* page = static_cast<WebPageProxy*>(m_root);
            encoder << page->pageID();
            break;
        }
        case APIObject::TypeFrame: {
            WebFrameProxy* frame = static_cast<WebFrameProxy*>(m_root);
            encoder << frame->frameID();
            break;
        }
        case APIObject::TypePageGroup: {
            WebPageGroup* pageGroup = static_cast<WebPageGroup*>(m_root);
            encoder << pageGroup->data();
            break;
        }
#if PLATFORM(MAC)
        case APIObject::TypeObjCObjectGraph: {
            ObjCObjectGraph* objectGraph = static_cast<ObjCObjectGraph*>(m_root);
            encoder << WebContextObjCObjectGraphEncoder(objectGraph);
            break;
        }
#endif
        default:
            ASSERT_NOT_REACHED();
            break;
        }
    }
};

// Adds
//   - Page -> BundlePage
//   - Frame -> BundleFrame
//   - PageGroup -> BundlePageGroup

class WebContextUserMessageDecoder : public UserMessageDecoder<WebContextUserMessageDecoder> {
public:
    typedef UserMessageDecoder<WebContextUserMessageDecoder> Base;

    WebContextUserMessageDecoder(RefPtr<APIObject>& root, WebProcessProxy* process)
        : Base(root)
        , m_process(process)
    {
    }

    WebContextUserMessageDecoder(WebContextUserMessageDecoder& userMessageDecoder, RefPtr<APIObject>& root)
        : Base(root)
        , m_process(userMessageDecoder.m_process)
    {
    }

    static bool decode(CoreIPC::ArgumentDecoder& decoder, WebContextUserMessageDecoder& coder)
    {
        APIObject::Type type = APIObject::TypeNull;
        if (!Base::baseDecode(decoder, coder, type))
            return false;

        if (coder.m_root || type == APIObject::TypeNull)
            return true;

        switch (type) {
        case APIObject::TypeBundlePage: {
            uint64_t pageID;
            if (!decoder.decode(pageID))
                return false;
            coder.m_root = coder.m_process->webPage(pageID);
            break;
        }
        case APIObject::TypeBundleFrame: {
            uint64_t frameID;
            if (!decoder.decode(frameID))
                return false;
            coder.m_root = coder.m_process->webFrame(frameID);
            break;
        }
        case APIObject::TypeBundlePageGroup: {
            uint64_t pageGroupID;
            if (!decoder.decode(pageGroupID))
                return false;
            coder.m_root = WebPageGroup::get(pageGroupID);
            break;
        }
#if PLATFORM(MAC)
        case APIObject::TypeObjCObjectGraph: {
            RefPtr<ObjCObjectGraph> objectGraph;
            WebContextObjCObjectGraphDecoder objectGraphDecoder(objectGraph, coder.m_process);
            if (!decoder.decode(objectGraphDecoder))
                return false;
            coder.m_root = objectGraph.get();
            break;
        }
#endif
        default:
            return false;
        }

        return true;
    }

private:
    WebProcessProxy* m_process;
};

} // namespace WebKit
