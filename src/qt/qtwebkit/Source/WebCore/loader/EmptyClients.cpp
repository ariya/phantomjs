/*
 * Copyright (C) 2006 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008, 2009, 2012 Apple Inc. All rights reserved.
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
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
#include "EmptyClients.h"

#include "DateTimeChooser.h"
#include "DocumentLoader.h"
#include "FileChooser.h"
#include "FormState.h"
#include "Frame.h"
#include "FrameNetworkingContext.h"
#include "HTMLFormElement.h"

#if ENABLE(INPUT_TYPE_COLOR)
#include "ColorChooser.h"
#endif

namespace WebCore {

void fillWithEmptyClients(Page::PageClients& pageClients)
{
    static ChromeClient* dummyChromeClient = adoptPtr(new EmptyChromeClient).leakPtr();
    pageClients.chromeClient = dummyChromeClient;

#if ENABLE(CONTEXT_MENUS)
    static ContextMenuClient* dummyContextMenuClient = adoptPtr(new EmptyContextMenuClient).leakPtr();
    pageClients.contextMenuClient = dummyContextMenuClient;
#endif

#if ENABLE(DRAG_SUPPORT)
    static DragClient* dummyDragClient = adoptPtr(new EmptyDragClient).leakPtr();
    pageClients.dragClient = dummyDragClient;
#endif

    static EditorClient* dummyEditorClient = adoptPtr(new EmptyEditorClient).leakPtr();
    pageClients.editorClient = dummyEditorClient;

    static InspectorClient* dummyInspectorClient = adoptPtr(new EmptyInspectorClient).leakPtr();
    pageClients.inspectorClient = dummyInspectorClient;
}

class EmptyPopupMenu : public PopupMenu {
public:
    virtual void show(const IntRect&, FrameView*, int) { }
    virtual void hide() { }
    virtual void updateFromElement() { }
    virtual void disconnectClient() { }
};

class EmptySearchPopupMenu : public SearchPopupMenu {
public:
    virtual PopupMenu* popupMenu() { return m_popup.get(); }
    virtual void saveRecentSearches(const AtomicString&, const Vector<String>&) { }
    virtual void loadRecentSearches(const AtomicString&, Vector<String>&) { }
    virtual bool enabled() { return false; }

private:
    RefPtr<EmptyPopupMenu> m_popup;
};

PassRefPtr<PopupMenu> EmptyChromeClient::createPopupMenu(PopupMenuClient*) const
{
    return adoptRef(new EmptyPopupMenu());
}

PassRefPtr<SearchPopupMenu> EmptyChromeClient::createSearchPopupMenu(PopupMenuClient*) const
{
    return adoptRef(new EmptySearchPopupMenu());
}

#if ENABLE(INPUT_TYPE_COLOR)
PassOwnPtr<ColorChooser> EmptyChromeClient::createColorChooser(ColorChooserClient*, const Color&)
{
    return nullptr;
}
#endif

#if ENABLE(DATE_AND_TIME_INPUT_TYPES)
PassRefPtr<DateTimeChooser> EmptyChromeClient::openDateTimeChooser(DateTimeChooserClient*, const DateTimeChooserParameters&)
{
    return PassRefPtr<DateTimeChooser>();
}
#endif

void EmptyChromeClient::runOpenPanel(Frame*, PassRefPtr<FileChooser>)
{
}

void EmptyFrameLoaderClient::dispatchDecidePolicyForNewWindowAction(FramePolicyFunction, const NavigationAction&, const ResourceRequest&, PassRefPtr<FormState>, const String&)
{
}

void EmptyFrameLoaderClient::dispatchDecidePolicyForNavigationAction(FramePolicyFunction, const NavigationAction&, const ResourceRequest&, PassRefPtr<FormState>)
{
}

void EmptyFrameLoaderClient::dispatchWillSendSubmitEvent(PassRefPtr<FormState>)
{
}

void EmptyFrameLoaderClient::dispatchWillSubmitForm(FramePolicyFunction, PassRefPtr<FormState>)
{
}

PassRefPtr<DocumentLoader> EmptyFrameLoaderClient::createDocumentLoader(const ResourceRequest& request, const SubstituteData& substituteData)
{
    return DocumentLoader::create(request, substituteData);
}

PassRefPtr<Frame> EmptyFrameLoaderClient::createFrame(const KURL&, const String&, HTMLFrameOwnerElement*, const String&, bool, int, int)
{
    return 0;
}

PassRefPtr<Widget> EmptyFrameLoaderClient::createPlugin(const IntSize&, HTMLPlugInElement*, const KURL&, const Vector<String>&, const Vector<String>&, const String&, bool)
{
    return 0;
}

void EmptyFrameLoaderClient::recreatePlugin(Widget*)
{
}

PassRefPtr<Widget> EmptyFrameLoaderClient::createJavaAppletWidget(const IntSize&, HTMLAppletElement*, const KURL&, const Vector<String>&, const Vector<String>&)
{
    return 0;
}

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
PassRefPtr<Widget> EmptyFrameLoaderClient::createMediaPlayerProxyPlugin(const IntSize&, HTMLMediaElement*, const KURL&, const Vector<String>&, const Vector<String>&, const String&)
{
    return 0;
}
#endif

PassRefPtr<FrameNetworkingContext> EmptyFrameLoaderClient::createNetworkingContext()
{
    return PassRefPtr<FrameNetworkingContext>();
}

void EmptyTextCheckerClient::requestCheckingOfString(PassRefPtr<TextCheckingRequest>)
{
}

void EmptyEditorClient::registerUndoStep(PassRefPtr<UndoStep>)
{
}

void EmptyEditorClient::registerRedoStep(PassRefPtr<UndoStep>)
{
}

#if ENABLE(CONTEXT_MENUS)
#if USE(CROSS_PLATFORM_CONTEXT_MENUS)
PassOwnPtr<ContextMenu> EmptyContextMenuClient::customizeMenu(PassOwnPtr<ContextMenu>)
{
    return nullptr;
}
#endif
#endif

}
