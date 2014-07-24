/*
 * Copyright (C) 2008, 2009, 2010, 2013 Apple Inc. All Rights Reserved.
 * Copyright (C) 2012 Serotek Corporation. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */


#include "config.h"
#include "AXObjectCache.h"

#include "AccessibilityObject.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "Document.h"
#include "Page.h"
#include "RenderObject.h"

// Provided by IAccessibleEventID.idl
#define IA2_EVENT_DOCUMENT_LOAD_COMPLETE    261

using namespace std;

namespace WebCore {

void AXObjectCache::detachWrapper(AccessibilityObject* obj)
{
    // On Windows, AccessibilityObjects are created when get_accChildCount is
    // called, but they are not wrapped until get_accChild is called, so this
    // object may not have a wrapper.
    if (AccessibilityObjectWrapper* wrapper = obj->wrapper())
        wrapper->detach();
}

void AXObjectCache::attachWrapper(AccessibilityObject*)
{
    // On Windows, AccessibilityObjects are wrapped when the accessibility
    // software requests them via get_accChild.
}

void AXObjectCache::handleScrolledToAnchor(const Node* anchorNode)
{
    // The anchor node may not be accessible. Post the notification for the
    // first accessible object.
    postPlatformNotification(AccessibilityObject::firstAccessibleObjectFromNode(anchorNode), AXScrolledToAnchor);
}

void AXObjectCache::postPlatformNotification(AccessibilityObject* obj, AXNotification notification)
{
    if (!obj)
        return;

    Document* document = obj->document();
    if (!document)
        return;

    Page* page = document->page();
    if (!page || !page->chrome().platformPageClient())
        return;

    DWORD msaaEvent;
    switch (notification) {
        case AXCheckedStateChanged:
            msaaEvent = EVENT_OBJECT_STATECHANGE;
            break;

        case AXFocusedUIElementChanged:
        case AXActiveDescendantChanged:
            msaaEvent = EVENT_OBJECT_FOCUS;
            break;

        case AXScrolledToAnchor:
            msaaEvent = EVENT_SYSTEM_SCROLLINGSTART;
            break;

        case AXLayoutComplete:
            msaaEvent = EVENT_OBJECT_REORDER;
            break;

        case AXLoadComplete:
            msaaEvent = IA2_EVENT_DOCUMENT_LOAD_COMPLETE;
            break;

        case AXValueChanged:
        case AXMenuListValueChanged:
            msaaEvent = EVENT_OBJECT_VALUECHANGE;
            break;

        case AXMenuListItemSelected:
            msaaEvent = EVENT_OBJECT_SELECTION;
            break;

        default:
            return;
    }

    // Windows will end up calling get_accChild() on the root accessible
    // object for the WebView, passing the child ID that we specify below. We
    // negate the AXID so we know that the caller is passing the ID of an
    // element, not the index of a child element.

    ASSERT(obj->axObjectID() >= 1);
    ASSERT(obj->axObjectID() <= numeric_limits<LONG>::max());

    NotifyWinEvent(msaaEvent, page->chrome().platformPageClient(), OBJID_CLIENT, -static_cast<LONG>(obj->axObjectID()));
}

void AXObjectCache::nodeTextChangePlatformNotification(AccessibilityObject*, AXTextChange, unsigned, const String&)
{
}

void AXObjectCache::frameLoadingEventPlatformNotification(AccessibilityObject* obj, AXLoadingEvent notification)
{
    if (!obj)
        return;
    
    Document* document = obj->document();
    if (!document)
        return;

    Page* page = document->page();
    if (!page)
        return;

    if (notification == AXLoadingStarted)
        page->chrome().client()->AXStartFrameLoad();
    else if (notification == AXLoadingFinished)
        page->chrome().client()->AXFinishFrameLoad();
}

AXID AXObjectCache::platformGenerateAXID() const
{
    static AXID lastUsedID = 0;

    // Generate a new ID. Windows accessibility relies on a positive AXID,
    // ranging from 1 to LONG_MAX.
    AXID objID = lastUsedID;
    do {
        ++objID;
        objID %= std::numeric_limits<LONG>::max();
    } while (objID == 0 || HashTraits<AXID>::isDeletedValue(objID) || m_idsInUse.contains(objID));

    ASSERT(objID >= 1 && objID <= std::numeric_limits<LONG>::max());

    lastUsedID = objID;

    return objID;
}

void AXObjectCache::handleFocusedUIElementChanged(Node*, Node* newFocusedNode)
{
    if (!newFocusedNode)
        return;

    Page* page = newFocusedNode->document()->page();
    if (!page || !page->chrome().platformPageClient())
        return;

    AccessibilityObject* focusedObject = focusedUIElementForPage(page);
    if (!focusedObject)
        return;

    ASSERT(!focusedObject->accessibilityIsIgnored());

    postPlatformNotification(focusedObject, AXFocusedUIElementChanged);
}

} // namespace WebCore
