/*
 * Copyright (C) 2012, 2013 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef PagePopup_h
#define PagePopup_h

#include "IntRect.h"
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

typedef const struct OpaqueJSContext* JSContextRef;
typedef struct OpaqueJSValue* JSObjectRef;
typedef const struct OpaqueJSValue* JSValueRef;

namespace WebCore {
class DocumentWriter;
class Frame;
}

namespace BlackBerry {
namespace WebKit {
class WebPage;
class WebPagePrivate;
class PagePopupClient;

class PagePopup : public RefCounted<PagePopup> {
public:
    static PassRefPtr<PagePopup> create(WebPagePrivate* webPagePrivate, PagePopupClient* client)
    {
        return adoptRef(new PagePopup(webPagePrivate, client));
    }
    ~PagePopup();

    void initialize(WebPage*);
    void close();

private:
    PagePopup(WebPagePrivate*, PagePopupClient*);

    void writeDocument(WebCore::DocumentWriter*);
    void installDOMFunction(WebCore::Frame*);

    static JSValueRef setValueAndClosePopupCallback(JSContextRef, JSObjectRef, JSObjectRef, size_t argumentCount, const JSValueRef arguments[], JSValueRef*);

    WebPagePrivate* m_webPagePrivate;
    OwnPtr<PagePopupClient> m_client;
};

}
}

#endif // PagePopup_h
