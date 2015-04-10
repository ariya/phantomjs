/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
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

#ifndef InPageSearchManager_h
#define InPageSearchManager_h

#include "FindOptions.h"
#include <wtf/text/WTFString.h>

namespace WebCore {
class Frame;
class Range;
class VisibleSelection;
}

namespace BlackBerry {

namespace WebKit {

class WebPagePrivate;

class InPageSearchManager {
public:
    InPageSearchManager(WebPagePrivate*);
    ~InPageSearchManager();

    bool findNextString(const String&, WebCore::FindOptions, bool wrap, bool highlightAllMatches, bool selectActiveMatchOnClear);
    void frameUnloaded(const WebCore::Frame*);

private:
    class DeferredScopeStringMatches;
    friend class DeferredScopeStringMatches;

    void clearTextMatches(bool selectActiveMatchOnClear = false);
    void setActiveMatchAndMarker(PassRefPtr<WebCore::Range>);
    bool findAndMarkText(const String&, WebCore::Range*, WebCore::Frame*, const WebCore::FindOptions&, bool /* isNewSearch */, bool /* startFromSelection */);
    bool shouldSearchForText(const String&);
    void scopeStringMatches(const String& text, bool reset, bool locateActiveMatchOnly, WebCore::Frame* scopingFrame = 0);
    void scopeStringMatchesSoon(WebCore::Frame* scopingFrame, const String& text, bool reset, bool locateActiveMatchOnly);
    void callScopeStringMatches(DeferredScopeStringMatches* caller, WebCore::Frame* scopingFrame, const String& text, bool reset, bool locateActiveMatchOnly);
    void cancelPendingScopingEffort();

    Vector<DeferredScopeStringMatches*> m_deferredScopingWork;
    WebPagePrivate* m_webPage;
    RefPtr<WebCore::Range> m_activeMatch;
    RefPtr<WebCore::Range> m_resumeScopingFromRange;
    String m_activeSearchString;
    int m_activeMatchCount;
    bool m_scopingComplete;
    bool m_scopingCaseInsensitive;
    bool m_locatingActiveMatch;
    bool m_highlightAllMatches;
    int m_activeMatchIndex;
};

}
}

#endif // InPageSearchManager_h
