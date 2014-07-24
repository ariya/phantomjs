/*
 * Copyright (C) 2012 Igalia S.L.
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

#ifndef WebPageAccessibilityObject_h
#define WebPageAccessibilityObject_h

#if HAVE(ACCESSIBILITY)

#include <atk/atk.h>

namespace WebKit {
class WebPage;
}

G_BEGIN_DECLS

#define WEB_TYPE_PAGE_ACCESSIBILITY_OBJECT              (web_page_accessibility_object_get_type())
#define WEB_PAGE_ACCESSIBILITY_OBJECT(object)           (G_TYPE_CHECK_INSTANCE_CAST((object), WEB_TYPE_PAGE_ACCESSIBILITY_OBJECT, WebPageAccessibilityObject))
#define WEB_PAGE_ACCESSIBILITY_OBJECT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), WEB_TYPE_PAGE_ACCESSIBILITY_OBJECT, WebPageAccessibilityObjectClass))
#define WEB_IS_PAGE_ACCESSIBILITY_OBJECT(object)        (G_TYPE_CHECK_INSTANCE_TYPE((object), WEB_TYPE_PAGE_ACCESSIBILITY_OBJECT))
#define WEB_IS_PAGE_ACCESSIBILITY_OBJECT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), WEB_TYPE_PAGE_ACCESSIBILITY_OBJECT))
#define WEB_PAGE_ACCESSIBILITY_OBJECT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS((object), WEB_TYPE_PAGE_ACCESSIBILITY_OBJECT, WebPageAccessibilityObjectClass))

typedef struct _WebPageAccessibilityObject WebPageAccessibilityObject;
typedef struct _WebPageAccessibilityObjectClass WebPageAccessibilityObjectClass;

struct _WebPageAccessibilityObject {
    AtkPlug parent;
    WebKit::WebPage* m_page;
};

struct _WebPageAccessibilityObjectClass {
    AtkPlugClass parentClass;
};

GType web_page_accessibility_object_get_type();

WebPageAccessibilityObject* webPageAccessibilityObjectNew(WebKit::WebPage*);

void webPageAccessibilityObjectRefresh(WebPageAccessibilityObject*);

G_END_DECLS

#endif
#endif // WebPageAccessibilityObject_h
