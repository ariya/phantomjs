/*
 * Copyright (C) 2011 ProFUSION Embedded Systems
 * Copyright (C) 2011 Samsung Electronics
 * Copyright (C) 2012 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND ITS CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DumpRenderTreeChrome_h
#define DumpRenderTreeChrome_h

#include "GCController.h"

#include <Eina.h>
#include <Evas.h>
#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>

#if HAVE(ACCESSIBILITY)
#include "AccessibilityController.h"
#endif

class DumpRenderTreeChrome {
public:
    ~DumpRenderTreeChrome();

    static PassOwnPtr<DumpRenderTreeChrome> create(Evas*);

    Evas_Object* createNewWindow();
    void removeWindow(Evas_Object*);

    Evas_Object* createInspectorView();
    void removeInspectorView();
    void waitInspectorLoadFinished();

    const Vector<Evas_Object*>& extraViews() const;
    void clearExtraViews();

    Evas_Object* mainFrame() const;
    Evas_Object* mainView() const;

    void resetDefaultsToConsistentValues();

private:
    DumpRenderTreeChrome(Evas*);

    Evas_Object* createView() const;
    bool initialize();
#if HAVE(ACCESSIBILITY)
    AccessibilityController* accessibilityController() const;
    OwnPtr<AccessibilityController> m_axController;
#endif
    Evas_Object* m_mainFrame;
    Evas_Object* m_mainView;
    Evas* m_evas;
    OwnPtr<GCController> m_gcController;
    Vector<Evas_Object*> m_extraViews;
    static HashMap<unsigned long, CString> m_dumpAssignedUrls;
    static Evas_Object* m_provisionalLoadFailedFrame;

    // Smart callbacks
    static void onWindowObjectCleared(void*, Evas_Object*, void*);
    static void onLoadStarted(void*, Evas_Object*, void*);

    static Eina_Bool processWork(void*);

    static void topLoadingFrameLoadFinished();

    static void onStatusbarTextSet(void*, Evas_Object*, void*);

    static void onFrameTitleChanged(void*, Evas_Object*, void*);

    static void onDocumentLoadFinished(void*, Evas_Object*, void*);

    static void onWillSendRequest(void*, Evas_Object*, void*);

    static void onWebViewOnloadEvent(void*, Evas_Object*, void*);

    static void onWebViewNavigatedWithData(void*, Evas_Object*, void*);

    static void onWebViewServerRedirect(void*, Evas_Object*, void*);

    static void onWebViewClientRedirect(void*, Evas_Object*, void*);

    static void onWebViewPopulateVisitedLinks(void*, Evas_Object*, void*);

    static void onInsecureContentRun(void*, Evas_Object*, void*);

    static void onInsecureContentDisplayed(void*, Evas_Object*, void*);

    static void onFrameCreated(void*, Evas_Object*, void*);

    static void onInspectorViewCreate(void*, Evas_Object*, void*);

    static void onInspectorViewClose(void*, Evas_Object*, void*);

    static void onInspectorFrameLoadFinished(void*, Evas_Object*, void*);

    static void onFrameIconChanged(void*, Evas_Object*, void*);

    static void onFrameProvisionalLoad(void*, Evas_Object*, void*);
    static void onFrameProvisionalLoadFailed(void*, Evas_Object*, void*);

    static void onFrameLoadCommitted(void*, Evas_Object*, void*);

    static void onFrameLoadFinished(void*, Evas_Object*, void*);

    static void onFrameRedirectCancelled(void*, Evas_Object*, void*);
    static void onFrameRedirectForProvisionalLoad(void*, Evas_Object*, void*);
    static void onFrameRedirectRequested(void*, Evas_Object*, void*);

    static void onFrameLoadError(void*, Evas_Object*, void*);
    static void onDidDetectXSS(void*, Evas_Object*, void*);

    static void onResponseReceived(void*, Evas_Object*, void*);

    static void onResourceLoadFinished(void*, Evas_Object*, void*);

    static void onResourceLoadFailed(void*, Evas_Object*, void*);

    static void onNewResourceRequest(void*, Evas_Object*, void*);

    static void onDownloadRequest(void*, Evas_Object*, void*);
};

#endif // DumpRenderTreeChrome_h
