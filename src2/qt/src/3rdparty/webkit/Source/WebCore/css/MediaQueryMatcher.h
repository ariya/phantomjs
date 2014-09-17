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

#ifndef MediaQueryMatcher_h
#define MediaQueryMatcher_h

#include "ScriptState.h"
#include <wtf/Forward.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

namespace WebCore {

class Document;
class MediaList;
class MediaQueryList;
class MediaQueryListListener;
class MediaQueryEvaluator;

// MediaQueryMatcher class is responsible for keeping a vector of pairs
// MediaQueryList x MediaQueryListListener. It is responsible for evaluating the queries
// whenever it is needed and to call the listeners if the corresponding query has changed.
// The listeners must be called in the very same order in which they have been added.

class MediaQueryMatcher : public RefCounted<MediaQueryMatcher> {
public:
    static PassRefPtr<MediaQueryMatcher> create(Document* document) { return adoptRef(new MediaQueryMatcher(document)); }
    ~MediaQueryMatcher();
    void documentDestroyed();

    void addListener(PassRefPtr<MediaQueryListListener>, PassRefPtr<MediaQueryList>);
    void removeListener(MediaQueryListListener*, MediaQueryList*);

    PassRefPtr<MediaQueryList> matchMedia(const String&);

    unsigned evaluationRound() const { return m_evaluationRound; }
    void styleSelectorChanged();
    bool evaluate(MediaList*);

private:
    class Listener {
    public:
        Listener(PassRefPtr<MediaQueryListListener>, PassRefPtr<MediaQueryList>);
        ~Listener();

        void evaluate(ScriptState*, MediaQueryEvaluator*);

        MediaQueryListListener* listener() { return m_listener.get(); }
        MediaQueryList* query() { return m_query.get(); }

    private:
        RefPtr<MediaQueryListListener> m_listener;
        RefPtr<MediaQueryList> m_query;
    };

    MediaQueryMatcher(Document*);
    PassOwnPtr<MediaQueryEvaluator> prepareEvaluator() const;
    String mediaType() const;

    Document* m_document;
    Vector<OwnPtr<Listener> > m_listeners;

    // This value is incremented at style selector changes.
    // It is used to avoid evaluating queries more then once and to make sure
    // that a media query result change is notified exactly once.
    unsigned m_evaluationRound;
};

}

#endif // MediaQueryMatcher_h
