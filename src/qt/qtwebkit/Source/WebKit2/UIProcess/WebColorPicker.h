/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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

#ifndef WebColorPicker_h
#define WebColorPicker_h

#if ENABLE(INPUT_TYPE_COLOR)

#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {
class Color;
}

namespace WebKit {

class WebPageProxy;

class WebColorPicker : public RefCounted<WebColorPicker> {
public:
    class Client {
    protected:
        virtual ~Client() { }

    public:
        virtual void didChooseColor(const WebCore::Color&) = 0;
        virtual void didEndColorChooser() = 0;
    };

    static PassRefPtr<WebColorPicker> create(Client* client)
    {
        return adoptRef(new WebColorPicker(client));
    }

    virtual ~WebColorPicker();

    void invalidate() { m_client = 0; }

    virtual void endChooser();
    virtual void setSelectedColor(const WebCore::Color&);

protected:
    explicit WebColorPicker(Client*);

    Client* m_client;
};

} // namespace WebKit

#endif // ENABLE(INPUT_TYPE_COLOR)

#endif // WebColorPicker_h
