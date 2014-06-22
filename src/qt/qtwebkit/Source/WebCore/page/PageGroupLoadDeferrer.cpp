/*
 * Copyright (C) 2006, 2007, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "PageGroupLoadDeferrer.h"

#include "Document.h"
#include "DocumentParser.h"
#include "Frame.h"
#include "Page.h"
#include "PageGroup.h"
#include "ScriptRunner.h"
#include <wtf/HashSet.h>

namespace WebCore {

using namespace std;

PageGroupLoadDeferrer::PageGroupLoadDeferrer(Page* page, bool deferSelf)
{
    const HashSet<Page*>& pages = page->group().pages();

    HashSet<Page*>::const_iterator end = pages.end();
    for (HashSet<Page*>::const_iterator it = pages.begin(); it != end; ++it) {
        Page* otherPage = *it;
        if ((deferSelf || otherPage != page)) {
            if (!otherPage->defersLoading()) {
                m_deferredFrames.append(otherPage->mainFrame());

                // This code is not logically part of load deferring, but we do not want JS code executed beneath modal
                // windows or sheets, which is exactly when PageGroupLoadDeferrer is used.
                for (Frame* frame = otherPage->mainFrame(); frame; frame = frame->tree()->traverseNext())
                    frame->document()->suspendScheduledTasks(ActiveDOMObject::WillDeferLoading);
            }
        }
    }

    size_t count = m_deferredFrames.size();
    for (size_t i = 0; i < count; ++i)
        if (Page* page = m_deferredFrames[i]->page())
            page->setDefersLoading(true);
}

PageGroupLoadDeferrer::~PageGroupLoadDeferrer()
{
    for (size_t i = 0; i < m_deferredFrames.size(); ++i) {
        if (Page* page = m_deferredFrames[i]->page()) {
            page->setDefersLoading(false);

            for (Frame* frame = page->mainFrame(); frame; frame = frame->tree()->traverseNext())
                frame->document()->resumeScheduledTasks(ActiveDOMObject::WillDeferLoading);
        }
    }
}


} // namespace WebCore
