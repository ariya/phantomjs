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

#include "config.h"
#include "WebPageAccessibilityObject.h"

#if HAVE(ACCESSIBILITY)

#include "WebPage.h"
#include <WebCore/AXObjectCache.h>
#include <WebCore/Frame.h>
#include <WebCore/Page.h>

using namespace WebKit;
using namespace WebCore;

G_DEFINE_TYPE(WebPageAccessibilityObject, web_page_accessibility_object, ATK_TYPE_PLUG)

static AtkObject* accessibilityRootObjectWrapper(AtkObject* atkObject)
{
    if (!AXObjectCache::accessibilityEnabled())
        AXObjectCache::enableAccessibility();

    WebPageAccessibilityObject* accessible = WEB_PAGE_ACCESSIBILITY_OBJECT(atkObject);
    if (!accessible->m_page)
        return 0;

    Page* corePage = accessible->m_page->corePage();
    if (!corePage)
        return 0;

    Frame* coreFrame = corePage->mainFrame();
    if (!coreFrame || !coreFrame->document())
        return 0;

    AccessibilityObject* coreRootObject = coreFrame->document()->axObjectCache()->rootObject();
    if (!coreRootObject)
        return 0;

    AtkObject* rootObject = coreRootObject->wrapper();
    if (!rootObject || !ATK_IS_OBJECT(rootObject))
        return 0;

    return rootObject;
}

static void webPageAccessibilityObjectInitialize(AtkObject* atkObject, gpointer data)
{
    if (ATK_OBJECT_CLASS(web_page_accessibility_object_parent_class)->initialize)
        ATK_OBJECT_CLASS(web_page_accessibility_object_parent_class)->initialize(atkObject, data);

    WEB_PAGE_ACCESSIBILITY_OBJECT(atkObject)->m_page = reinterpret_cast<WebPage*>(data);
    atk_object_set_role(atkObject, ATK_ROLE_FILLER);
}

static gint webPageAccessibilityObjectGetIndexInParent(AtkObject*)
{
    // An AtkPlug is the only child an AtkSocket can have.
    return 0;
}

static gint webPageAccessibilityObjectGetNChildren(AtkObject* atkObject)
{
    AtkObject* rootObject = accessibilityRootObjectWrapper(atkObject);
    if (!rootObject)
        return 0;

    return 1;
}

static AtkObject* webPageAccessibilityObjectRefChild(AtkObject* atkObject, gint index)
{
    // It's supposed to have either one child or zero.
    if (index && index != 1)
        return 0;

    AtkObject* rootObject = accessibilityRootObjectWrapper(atkObject);
    if (!rootObject)
        return 0;

    atk_object_set_parent(rootObject, atkObject);
    g_object_ref(rootObject);

    return rootObject;
}

static void web_page_accessibility_object_init(WebPageAccessibilityObject*)
{
}

static void web_page_accessibility_object_class_init(WebPageAccessibilityObjectClass* klass)
{
    AtkObjectClass* atkObjectClass = ATK_OBJECT_CLASS(klass);

    // No need to implement get_parent() here since this is a subclass
    // of AtkPlug and all the logic related to that function will be
    // implemented by the ATK bridge.
    atkObjectClass->initialize = webPageAccessibilityObjectInitialize;
    atkObjectClass->get_index_in_parent = webPageAccessibilityObjectGetIndexInParent;
    atkObjectClass->get_n_children = webPageAccessibilityObjectGetNChildren;
    atkObjectClass->ref_child = webPageAccessibilityObjectRefChild;
}

WebPageAccessibilityObject* webPageAccessibilityObjectNew(WebPage* page)
{
    AtkObject* object = ATK_OBJECT(g_object_new(WEB_TYPE_PAGE_ACCESSIBILITY_OBJECT, NULL));
    atk_object_initialize(object, page);
    return WEB_PAGE_ACCESSIBILITY_OBJECT(object);
}

void webPageAccessibilityObjectRefresh(WebPageAccessibilityObject* accessible)
{
    // We just need to ensure that there's a connection in the ATK
    // world between this accessibility object and the AtkObject of
    // the accessibility object for the root of the DOM tree.
    AtkObject* rootObject = accessibilityRootObjectWrapper(ATK_OBJECT(accessible));
    if (!rootObject)
        return;
    atk_object_set_parent(rootObject, ATK_OBJECT(accessible));
}

#endif
