/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef MediaQueryList_h
#define MediaQueryList_h

#include <wtf/Forward.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class MediaList;
class MediaQueryListListener;
class MediaQueryEvaluator;
class MediaQueryMatcher;

// MediaQueryList interface is specified at http://dev.w3.org/csswg/cssom-view/#the-mediaquerylist-interface
// The objects of this class are returned by window.matchMedia. They may be used to
// retrieve the current value of the given media query and to add/remove listeners that
// will be called whenever the value of the query changes.

class MediaQueryList : public RefCounted<MediaQueryList> {
public:
    static PassRefPtr<MediaQueryList> create(PassRefPtr<MediaQueryMatcher>, PassRefPtr<MediaList>, bool);
    ~MediaQueryList();

    String media() const;
    bool matches();

    void addListener(PassRefPtr<MediaQueryListListener>);
    void removeListener(PassRefPtr<MediaQueryListListener>);

    void evaluate(MediaQueryEvaluator*, bool& notificationNeeded);

private:
    MediaQueryList(PassRefPtr<MediaQueryMatcher>, PassRefPtr<MediaList>, bool matches);
    void setMatches(bool);

    RefPtr<MediaQueryMatcher> m_matcher;
    RefPtr<MediaList> m_media;
    unsigned m_evaluationRound; // Indicates if the query has been evaluated after the last style selector change.
    unsigned m_changeRound; // Used to know if the query has changed in the last style selector change.
    bool m_matches;
};

}

#endif // MediaQueryList_h
