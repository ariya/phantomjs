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

#include "config.h"
#include "MediaQueryMatcher.h"

#include "CSSStyleSelector.h"
#include "Document.h"
#include "Element.h"
#include "FrameView.h"
#include "MediaList.h"
#include "MediaQueryEvaluator.h"
#include "MediaQueryList.h"
#include "MediaQueryListListener.h"

namespace WebCore {

MediaQueryMatcher::Listener::Listener(PassRefPtr<MediaQueryListListener> listener, PassRefPtr<MediaQueryList> query)
    : m_listener(listener)
    , m_query(query)
{
}

MediaQueryMatcher::Listener::~Listener()
{
}

void MediaQueryMatcher::Listener::evaluate(ScriptState* state, MediaQueryEvaluator* evaluator)
{
    bool notify;
    m_query->evaluate(evaluator, notify);
    if (notify)
        m_listener->queryChanged(state, m_query.get());
}

MediaQueryMatcher::MediaQueryMatcher(Document* document)
    : m_document(document)
    , m_evaluationRound(1)
{
    ASSERT(m_document);
}

MediaQueryMatcher::~MediaQueryMatcher()
{
}

void MediaQueryMatcher::documentDestroyed()
{
    m_listeners.clear();
    m_document = 0;
}

String MediaQueryMatcher::mediaType() const
{
    if (!m_document || !m_document->frame() || !m_document->frame()->view())
        return String();

    return m_document->frame()->view()->mediaType();
}

PassOwnPtr<MediaQueryEvaluator> MediaQueryMatcher::prepareEvaluator() const
{
    if (!m_document || !m_document->frame())
        return nullptr;

    Element* documentElement = m_document->documentElement();
    if (!documentElement)
        return nullptr;

    CSSStyleSelector* styleSelector = m_document->styleSelector();
    if (!styleSelector)
        return nullptr;

    RefPtr<RenderStyle> rootStyle = styleSelector->styleForElement(documentElement, 0 /*defaultParent*/, false /*allowSharing*/, true /*resolveForRootDefault*/);

    return adoptPtr(new MediaQueryEvaluator(mediaType(), m_document->frame(), rootStyle.get()));
}

bool MediaQueryMatcher::evaluate(MediaList* media)
{
    if (!media)
        return false;

    OwnPtr<MediaQueryEvaluator> evaluator(prepareEvaluator());
    return evaluator && evaluator->eval(media);
}

PassRefPtr<MediaQueryList> MediaQueryMatcher::matchMedia(const String& query)
{
    if (!m_document)
        return 0;

    RefPtr<MediaList> media = MediaList::create(query, false);
    return MediaQueryList::create(this, media, evaluate(media.get()));
}

void MediaQueryMatcher::addListener(PassRefPtr<MediaQueryListListener> listener, PassRefPtr<MediaQueryList> query)
{
    if (!m_document)
        return;

    for (size_t i = 0; i < m_listeners.size(); ++i) {
        if (*m_listeners[i]->listener() == *listener && m_listeners[i]->query() == query)
            return;
    }

    m_listeners.append(adoptPtr(new Listener(listener, query)));
}

void MediaQueryMatcher::removeListener(MediaQueryListListener* listener, MediaQueryList* query)
{
    if (!m_document)
        return;

    for (size_t i = 0; i < m_listeners.size(); ++i) {
        if (*m_listeners[i]->listener() == *listener && m_listeners[i]->query() == query) {
            m_listeners.remove(i);
            return;
        }
    }
}

void MediaQueryMatcher::styleSelectorChanged()
{
    ASSERT(m_document);

    ScriptState* scriptState = mainWorldScriptState(m_document->frame());
    if (!scriptState)
        return;

    ++m_evaluationRound;
    OwnPtr<MediaQueryEvaluator> evaluator = prepareEvaluator();
    if (!evaluator)
        return;

    for (size_t i = 0; i < m_listeners.size(); ++i)
        m_listeners[i]->evaluate(scriptState, evaluator.get());
}

}
