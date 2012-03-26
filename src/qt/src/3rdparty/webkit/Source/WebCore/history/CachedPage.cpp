/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "CachedPage.h"

#include "CSSStyleSelector.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameView.h"
#include "Page.h"
#include <wtf/CurrentTime.h>
#include <wtf/RefCountedLeakCounter.h>

using namespace JSC;

namespace WebCore {

#ifndef NDEBUG
static WTF::RefCountedLeakCounter cachedPageCounter("CachedPage");
#endif

PassRefPtr<CachedPage> CachedPage::create(Page* page)
{
    return adoptRef(new CachedPage(page));
}

CachedPage::CachedPage(Page* page)
    : m_timeStamp(currentTime())
    , m_cachedMainFrame(CachedFrame::create(page->mainFrame()))
    , m_needStyleRecalcForVisitedLinks(false)
{
#ifndef NDEBUG
    cachedPageCounter.increment();
#endif
}

CachedPage::~CachedPage()
{
#ifndef NDEBUG
    cachedPageCounter.decrement();
#endif

    destroy();
    ASSERT(!m_cachedMainFrame);
}

void CachedPage::restore(Page* page)
{
    ASSERT(m_cachedMainFrame);
    ASSERT(page && page->mainFrame() && page->mainFrame() == m_cachedMainFrame->view()->frame());
    ASSERT(!page->frameCount());

    m_cachedMainFrame->open();
    
    // Restore the focus appearance for the focused element.
    // FIXME: Right now we don't support pages w/ frames in the b/f cache.  This may need to be tweaked when we add support for that.
    Document* focusedDocument = page->focusController()->focusedOrMainFrame()->document();
    if (Node* node = focusedDocument->focusedNode()) {
        if (node->isElementNode())
            static_cast<Element*>(node)->updateFocusAppearance(true);
    }

    if (m_needStyleRecalcForVisitedLinks) {
        for (Frame* frame = page->mainFrame(); frame; frame = frame->tree()->traverseNext()) {
            if (CSSStyleSelector* styleSelector = frame->document()->styleSelector())
                styleSelector->allVisitedStateChanged();
        }
    }

    clear();
}

void CachedPage::clear()
{
    ASSERT(m_cachedMainFrame);
    m_cachedMainFrame->clear();
    m_cachedMainFrame = 0;
    m_needStyleRecalcForVisitedLinks = false;
}

void CachedPage::destroy()
{
    if (m_cachedMainFrame)
        m_cachedMainFrame->destroy();

    m_cachedMainFrame = 0;
}

} // namespace WebCore
