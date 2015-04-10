/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(INSPECTOR)

#include "InspectorCSSAgent.h"

#include "CSSComputedStyleDeclaration.h"
#include "CSSImportRule.h"
#include "CSSPropertyNames.h"
#include "CSSPropertySourceData.h"
#include "CSSRule.h"
#include "CSSRuleList.h"
#include "CSSStyleRule.h"
#include "CSSStyleSheet.h"
#include "ContentSecurityPolicy.h"
#include "DOMWindow.h"
#include "ExceptionCodePlaceholder.h"
#include "HTMLHeadElement.h"
#include "HTMLStyleElement.h"
#include "InspectorDOMAgent.h"
#include "InspectorHistory.h"
#include "InspectorState.h"
#include "InspectorTypeBuilder.h"
#include "InspectorValues.h"
#include "InstrumentingAgents.h"
#include "NamedFlowCollection.h"
#include "Node.h"
#include "NodeList.h"
#include "RenderRegion.h"
#include "SVGStyleElement.h"
#include "StylePropertySet.h"
#include "StylePropertyShorthand.h"
#include "StyleResolver.h"
#include "StyleRule.h"
#include "StyleSheetList.h"
#include "WebKitNamedFlow.h"

#include <wtf/CurrentTime.h>
#include <wtf/HashSet.h>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringConcatenate.h>

namespace CSSAgentState {
static const char cssAgentEnabled[] = "cssAgentEnabled";
static const char isSelectorProfiling[] = "isSelectorProfiling";
}

namespace WebCore {

enum ForcePseudoClassFlags {
    PseudoNone = 0,
    PseudoHover = 1 << 0,
    PseudoFocus = 1 << 1,
    PseudoActive = 1 << 2,
    PseudoVisited = 1 << 3
};

struct RuleMatchData {
    String selector;
    String url;
    unsigned lineNumber;
    double startTime;
};

struct RuleMatchingStats {
    RuleMatchingStats()
        : lineNumber(0), totalTime(0.0), hits(0), matches(0)
    {
    }
    RuleMatchingStats(const RuleMatchData& data, double totalTime, unsigned hits, unsigned matches)
        : selector(data.selector), url(data.url), lineNumber(data.lineNumber), totalTime(totalTime), hits(hits), matches(matches)
    {
    }

    String selector;
    String url;
    unsigned lineNumber;
    double totalTime;
    unsigned hits;
    unsigned matches;
};

class SelectorProfile {
    WTF_MAKE_FAST_ALLOCATED;
public:
    SelectorProfile()
        : m_totalMatchingTimeMs(0.0)
    {
    }
    virtual ~SelectorProfile()
    {
    }

    double totalMatchingTimeMs() const { return m_totalMatchingTimeMs; }

    String makeKey();
    void startSelector(const CSSStyleRule*);
    void commitSelector(bool);
    void commitSelectorTime();
    PassRefPtr<TypeBuilder::CSS::SelectorProfile> toInspectorObject() const;

private:

    // Key is "selector?url:line".
    typedef HashMap<String, RuleMatchingStats> RuleMatchingStatsMap;

    double m_totalMatchingTimeMs;
    RuleMatchingStatsMap m_ruleMatchingStats;
    RuleMatchData m_currentMatchData;
};


static unsigned computePseudoClassMask(InspectorArray* pseudoClassArray)
{
    DEFINE_STATIC_LOCAL(String, active, (ASCIILiteral("active")));
    DEFINE_STATIC_LOCAL(String, hover, (ASCIILiteral("hover")));
    DEFINE_STATIC_LOCAL(String, focus, (ASCIILiteral("focus")));
    DEFINE_STATIC_LOCAL(String, visited, (ASCIILiteral("visited")));
    if (!pseudoClassArray || !pseudoClassArray->length())
        return PseudoNone;

    unsigned result = PseudoNone;
    for (size_t i = 0; i < pseudoClassArray->length(); ++i) {
        RefPtr<InspectorValue> pseudoClassValue = pseudoClassArray->get(i);
        String pseudoClass;
        bool success = pseudoClassValue->asString(&pseudoClass);
        if (!success)
            continue;
        if (pseudoClass == active)
            result |= PseudoActive;
        else if (pseudoClass == hover)
            result |= PseudoHover;
        else if (pseudoClass == focus)
            result |= PseudoFocus;
        else if (pseudoClass == visited)
            result |= PseudoVisited;
    }

    return result;
}

inline String SelectorProfile::makeKey()
{
    return makeString(m_currentMatchData.selector, "?", m_currentMatchData.url, ":", String::number(m_currentMatchData.lineNumber));
}

inline void SelectorProfile::startSelector(const CSSStyleRule* rule)
{
    m_currentMatchData.selector = rule->selectorText();
    CSSStyleSheet* styleSheet = rule->parentStyleSheet();
    String url = emptyString();
    if (styleSheet) {
        url = InspectorStyleSheet::styleSheetURL(styleSheet);
        if (url.isEmpty())
            url = InspectorDOMAgent::documentURLString(styleSheet->ownerDocument());
    }
    m_currentMatchData.url = url;
    m_currentMatchData.lineNumber = rule->styleRule()->sourceLine();
    m_currentMatchData.startTime = WTF::currentTimeMS();
}

inline void SelectorProfile::commitSelector(bool matched)
{
    double matchTimeMs = WTF::currentTimeMS() - m_currentMatchData.startTime;
    m_totalMatchingTimeMs += matchTimeMs;

    RuleMatchingStatsMap::AddResult result = m_ruleMatchingStats.add(makeKey(), RuleMatchingStats(m_currentMatchData, matchTimeMs, 1, matched ? 1 : 0));
    if (!result.isNewEntry) {
        result.iterator->value.totalTime += matchTimeMs;
        result.iterator->value.hits += 1;
        if (matched)
            result.iterator->value.matches += 1;
    }
}

inline void SelectorProfile::commitSelectorTime()
{
    double processingTimeMs = WTF::currentTimeMS() - m_currentMatchData.startTime;
    m_totalMatchingTimeMs += processingTimeMs;

    RuleMatchingStatsMap::iterator it = m_ruleMatchingStats.find(makeKey());
    if (it == m_ruleMatchingStats.end())
        return;

    it->value.totalTime += processingTimeMs;
}

PassRefPtr<TypeBuilder::CSS::SelectorProfile> SelectorProfile::toInspectorObject() const
{
    RefPtr<TypeBuilder::Array<TypeBuilder::CSS::SelectorProfileEntry> > selectorProfileData = TypeBuilder::Array<TypeBuilder::CSS::SelectorProfileEntry>::create();
    for (RuleMatchingStatsMap::const_iterator it = m_ruleMatchingStats.begin(); it != m_ruleMatchingStats.end(); ++it) {
        RefPtr<TypeBuilder::CSS::SelectorProfileEntry> entry = TypeBuilder::CSS::SelectorProfileEntry::create()
            .setSelector(it->value.selector)
            .setUrl(it->value.url)
            .setLineNumber(it->value.lineNumber)
            .setTime(it->value.totalTime)
            .setHitCount(it->value.hits)
            .setMatchCount(it->value.matches);
        selectorProfileData->addItem(entry.release());
    }

    RefPtr<TypeBuilder::CSS::SelectorProfile> result = TypeBuilder::CSS::SelectorProfile::create()
        .setTotalTime(totalMatchingTimeMs())
        .setData(selectorProfileData);
    return result.release();
}

class UpdateRegionLayoutTask {
public:
    UpdateRegionLayoutTask(InspectorCSSAgent*);
    void scheduleFor(WebKitNamedFlow*, int documentNodeId);
    void unschedule(WebKitNamedFlow*);
    void reset();
    void onTimer(Timer<UpdateRegionLayoutTask>*);

private:
    InspectorCSSAgent* m_cssAgent;
    Timer<UpdateRegionLayoutTask> m_timer;
    HashMap<WebKitNamedFlow*, int> m_namedFlows;
};

UpdateRegionLayoutTask::UpdateRegionLayoutTask(InspectorCSSAgent* cssAgent)
    : m_cssAgent(cssAgent)
    , m_timer(this, &UpdateRegionLayoutTask::onTimer)
{
}

void UpdateRegionLayoutTask::scheduleFor(WebKitNamedFlow* namedFlow, int documentNodeId)
{
    m_namedFlows.add(namedFlow, documentNodeId);

    if (!m_timer.isActive())
        m_timer.startOneShot(0);
}

void UpdateRegionLayoutTask::unschedule(WebKitNamedFlow* namedFlow)
{
    m_namedFlows.remove(namedFlow);
}

void UpdateRegionLayoutTask::reset()
{
    m_timer.stop();
    m_namedFlows.clear();
}

void UpdateRegionLayoutTask::onTimer(Timer<UpdateRegionLayoutTask>*)
{
    // The timer is stopped on m_cssAgent destruction, so this method will never be called after m_cssAgent has been destroyed.
    Vector<std::pair<WebKitNamedFlow*, int> > namedFlows;

    for (HashMap<WebKitNamedFlow*, int>::iterator it = m_namedFlows.begin(), end = m_namedFlows.end(); it != end; ++it)
        namedFlows.append(std::make_pair(it->key, it->value));

    for (unsigned i = 0, size = namedFlows.size(); i < size; ++i) {
        WebKitNamedFlow* namedFlow = namedFlows.at(i).first;
        int documentNodeId = namedFlows.at(i).second;

        if (m_namedFlows.contains(namedFlow)) {
            m_cssAgent->regionLayoutUpdated(namedFlow, documentNodeId);
            m_namedFlows.remove(namedFlow);
        }
    }

    if (!m_namedFlows.isEmpty() && !m_timer.isActive())
        m_timer.startOneShot(0);
}
    
class ChangeRegionOversetTask {
public:
    ChangeRegionOversetTask(InspectorCSSAgent*);
    void scheduleFor(WebKitNamedFlow*, int documentNodeId);
    void unschedule(WebKitNamedFlow*);
    void reset();
    void onTimer(Timer<ChangeRegionOversetTask>*);
    
private:
    InspectorCSSAgent* m_cssAgent;
    Timer<ChangeRegionOversetTask> m_timer;
    HashMap<WebKitNamedFlow*, int> m_namedFlows;
};

ChangeRegionOversetTask::ChangeRegionOversetTask(InspectorCSSAgent* cssAgent)
    : m_cssAgent(cssAgent)
    , m_timer(this, &ChangeRegionOversetTask::onTimer)
{
}

void ChangeRegionOversetTask::scheduleFor(WebKitNamedFlow* namedFlow, int documentNodeId)
{
    m_namedFlows.add(namedFlow, documentNodeId);
    
    if (!m_timer.isActive())
        m_timer.startOneShot(0);
}

void ChangeRegionOversetTask::unschedule(WebKitNamedFlow* namedFlow)
{
    m_namedFlows.remove(namedFlow);
}

void ChangeRegionOversetTask::reset()
{
    m_timer.stop();
    m_namedFlows.clear();
}

void ChangeRegionOversetTask::onTimer(Timer<ChangeRegionOversetTask>*)
{
    // The timer is stopped on m_cssAgent destruction, so this method will never be called after m_cssAgent has been destroyed.
    for (HashMap<WebKitNamedFlow*, int>::iterator it = m_namedFlows.begin(), end = m_namedFlows.end(); it != end; ++it)
        m_cssAgent->regionOversetChanged(it->key, it->value);

    m_namedFlows.clear();
}

class InspectorCSSAgent::StyleSheetAction : public InspectorHistory::Action {
    WTF_MAKE_NONCOPYABLE(StyleSheetAction);
public:
    StyleSheetAction(const String& name, InspectorStyleSheet* styleSheet)
        : InspectorHistory::Action(name)
        , m_styleSheet(styleSheet)
    {
    }

protected:
    RefPtr<InspectorStyleSheet> m_styleSheet;
};

class InspectorCSSAgent::SetStyleSheetTextAction : public InspectorCSSAgent::StyleSheetAction {
    WTF_MAKE_NONCOPYABLE(SetStyleSheetTextAction);
public:
    SetStyleSheetTextAction(InspectorStyleSheet* styleSheet, const String& text)
        : InspectorCSSAgent::StyleSheetAction("SetStyleSheetText", styleSheet)
        , m_text(text)
    {
    }

    virtual bool perform(ExceptionCode& ec)
    {
        if (!m_styleSheet->getText(&m_oldText))
            return false;
        return redo(ec);
    }

    virtual bool undo(ExceptionCode& ec)
    {
        if (m_styleSheet->setText(m_oldText, ec)) {
            m_styleSheet->reparseStyleSheet(m_oldText);
            return true;
        }
        return false;
    }

    virtual bool redo(ExceptionCode& ec)
    {
        if (m_styleSheet->setText(m_text, ec)) {
            m_styleSheet->reparseStyleSheet(m_text);
            return true;
        }
        return false;
    }

    virtual String mergeId()
    {
        return String::format("SetStyleSheetText %s", m_styleSheet->id().utf8().data());
    }

    virtual void merge(PassOwnPtr<Action> action)
    {
        ASSERT(action->mergeId() == mergeId());

        SetStyleSheetTextAction* other = static_cast<SetStyleSheetTextAction*>(action.get());
        m_text = other->m_text;
    }

private:
    String m_text;
    String m_oldText;
};

class InspectorCSSAgent::SetStyleTextAction : public InspectorCSSAgent::StyleSheetAction {
    WTF_MAKE_NONCOPYABLE(SetStyleTextAction);
public:
    SetStyleTextAction(InspectorStyleSheet* styleSheet, const InspectorCSSId& cssId, const String& text)
        : InspectorCSSAgent::StyleSheetAction(ASCIILiteral("SetStyleText"), styleSheet)
        , m_cssId(cssId)
        , m_text(text)
    {
    }

    virtual bool perform(ExceptionCode& ec)
    {
        return redo(ec);
    }

    virtual bool undo(ExceptionCode& ec)
    {
        return m_styleSheet->setStyleText(m_cssId, m_oldText, 0, ec);
    }

    virtual bool redo(ExceptionCode& ec)
    {
        return m_styleSheet->setStyleText(m_cssId, m_text, &m_oldText, ec);
    }

    virtual String mergeId()
    {
        ASSERT(m_styleSheet->id() == m_cssId.styleSheetId());
        return String::format("SetStyleText %s:%u", m_styleSheet->id().utf8().data(), m_cssId.ordinal());
    }

    virtual void merge(PassOwnPtr<Action> action)
    {
        ASSERT(action->mergeId() == mergeId());

        SetStyleTextAction* other = static_cast<SetStyleTextAction*>(action.get());
        m_text = other->m_text;
    }

private:
    InspectorCSSId m_cssId;
    String m_text;
    String m_oldText;
};

class InspectorCSSAgent::SetPropertyTextAction : public InspectorCSSAgent::StyleSheetAction {
    WTF_MAKE_NONCOPYABLE(SetPropertyTextAction);
public:
    SetPropertyTextAction(InspectorStyleSheet* styleSheet, const InspectorCSSId& cssId, unsigned propertyIndex, const String& text, bool overwrite)
        : InspectorCSSAgent::StyleSheetAction("SetPropertyText", styleSheet)
        , m_cssId(cssId)
        , m_propertyIndex(propertyIndex)
        , m_text(text)
        , m_overwrite(overwrite)
    {
    }

    virtual String toString()
    {
        return mergeId() + ": " + m_oldText + " -> " + m_text;
    }

    virtual bool perform(ExceptionCode& ec)
    {
        return redo(ec);
    }

    virtual bool undo(ExceptionCode& ec)
    {
        String placeholder;
        return m_styleSheet->setPropertyText(m_cssId, m_propertyIndex, m_overwrite ? m_oldText : "", true, &placeholder, ec);
    }

    virtual bool redo(ExceptionCode& ec)
    {
        String oldText;
        bool result = m_styleSheet->setPropertyText(m_cssId, m_propertyIndex, m_text, m_overwrite, &oldText, ec);
        m_oldText = oldText.stripWhiteSpace();
        // FIXME: remove this once the model handles this case.
        if (!m_oldText.endsWith(';'))
            m_oldText.append(';');

        return result;
    }

    virtual String mergeId()
    {
        return String::format("SetPropertyText %s:%u:%s", m_styleSheet->id().utf8().data(), m_propertyIndex, m_overwrite ? "true" : "false");
    }

    virtual void merge(PassOwnPtr<Action> action)
    {
        ASSERT(action->mergeId() == mergeId());

        SetPropertyTextAction* other = static_cast<SetPropertyTextAction*>(action.get());
        m_text = other->m_text;
    }

private:
    InspectorCSSId m_cssId;
    unsigned m_propertyIndex;
    String m_text;
    String m_oldText;
    bool m_overwrite;
};

class InspectorCSSAgent::TogglePropertyAction : public InspectorCSSAgent::StyleSheetAction {
    WTF_MAKE_NONCOPYABLE(TogglePropertyAction);
public:
    TogglePropertyAction(InspectorStyleSheet* styleSheet, const InspectorCSSId& cssId, unsigned propertyIndex, bool disable)
        : InspectorCSSAgent::StyleSheetAction("ToggleProperty", styleSheet)
        , m_cssId(cssId)
        , m_propertyIndex(propertyIndex)
        , m_disable(disable)
    {
    }

    virtual bool perform(ExceptionCode& ec)
    {
        return redo(ec);
    }

    virtual bool undo(ExceptionCode& ec)
    {
        return m_styleSheet->toggleProperty(m_cssId, m_propertyIndex, !m_disable, ec);
    }

    virtual bool redo(ExceptionCode& ec)
    {
        return m_styleSheet->toggleProperty(m_cssId, m_propertyIndex, m_disable, ec);
    }

private:
    InspectorCSSId m_cssId;
    unsigned m_propertyIndex;
    bool m_disable;
};

class InspectorCSSAgent::SetRuleSelectorAction : public InspectorCSSAgent::StyleSheetAction {
    WTF_MAKE_NONCOPYABLE(SetRuleSelectorAction);
public:
    SetRuleSelectorAction(InspectorStyleSheet* styleSheet, const InspectorCSSId& cssId, const String& selector)
        : InspectorCSSAgent::StyleSheetAction("SetRuleSelector", styleSheet)
        , m_cssId(cssId)
        , m_selector(selector)
    {
    }

    virtual bool perform(ExceptionCode& ec)
    {
        m_oldSelector = m_styleSheet->ruleSelector(m_cssId, ec);
        if (ec)
            return false;
        return redo(ec);
    }

    virtual bool undo(ExceptionCode& ec)
    {
        return m_styleSheet->setRuleSelector(m_cssId, m_oldSelector, ec);
    }

    virtual bool redo(ExceptionCode& ec)
    {
        return m_styleSheet->setRuleSelector(m_cssId, m_selector, ec);
    }

private:
    InspectorCSSId m_cssId;
    String m_selector;
    String m_oldSelector;
};

class InspectorCSSAgent::AddRuleAction : public InspectorCSSAgent::StyleSheetAction {
    WTF_MAKE_NONCOPYABLE(AddRuleAction);
public:
    AddRuleAction(InspectorStyleSheet* styleSheet, const String& selector)
        : InspectorCSSAgent::StyleSheetAction("AddRule", styleSheet)
        , m_selector(selector)
    {
    }

    virtual bool perform(ExceptionCode& ec)
    {
        return redo(ec);
    }

    virtual bool undo(ExceptionCode& ec)
    {
        return m_styleSheet->deleteRule(m_newId, ec);
    }

    virtual bool redo(ExceptionCode& ec)
    {
        CSSStyleRule* cssStyleRule = m_styleSheet->addRule(m_selector, ec);
        if (ec)
            return false;
        m_newId = m_styleSheet->ruleId(cssStyleRule);
        return true;
    }

    InspectorCSSId newRuleId() { return m_newId; }

private:
    InspectorCSSId m_newId;
    String m_selector;
    String m_oldSelector;
};

// static
CSSStyleRule* InspectorCSSAgent::asCSSStyleRule(CSSRule* rule)
{
    if (rule->type() != CSSRule::STYLE_RULE)
        return 0;
    return static_cast<CSSStyleRule*>(rule);
}

InspectorCSSAgent::InspectorCSSAgent(InstrumentingAgents* instrumentingAgents, InspectorCompositeState* state, InspectorDOMAgent* domAgent)
    : InspectorBaseAgent<InspectorCSSAgent>("CSS", instrumentingAgents, state)
    , m_frontend(0)
    , m_domAgent(domAgent)
    , m_lastStyleSheetId(1)
{
    m_domAgent->setDOMListener(this);
}

InspectorCSSAgent::~InspectorCSSAgent()
{
    ASSERT(!m_domAgent);
    reset();
}

void InspectorCSSAgent::setFrontend(InspectorFrontend* frontend)
{
    ASSERT(!m_frontend);
    m_frontend = frontend->css();
}

void InspectorCSSAgent::clearFrontend()
{
    ASSERT(m_frontend);
    m_frontend = 0;
    resetNonPersistentData();
    String errorString;
    stopSelectorProfilerImpl(&errorString, false);
}

void InspectorCSSAgent::discardAgent()
{
    m_domAgent->setDOMListener(0);
    m_domAgent = 0;
}

void InspectorCSSAgent::restore()
{
    if (m_state->getBoolean(CSSAgentState::cssAgentEnabled)) {
        ErrorString error;
        enable(&error);
    }
    if (m_state->getBoolean(CSSAgentState::isSelectorProfiling)) {
        String errorString;
        startSelectorProfiler(&errorString);
    }
}

void InspectorCSSAgent::reset()
{
    m_idToInspectorStyleSheet.clear();
    m_cssStyleSheetToInspectorStyleSheet.clear();
    m_nodeToInspectorStyleSheet.clear();
    m_documentToInspectorStyleSheet.clear();
    resetNonPersistentData();
}

void InspectorCSSAgent::resetNonPersistentData()
{
    m_namedFlowCollectionsRequested.clear();
    if (m_updateRegionLayoutTask)
        m_updateRegionLayoutTask->reset();
    if (m_changeRegionOversetTask)
        m_changeRegionOversetTask->reset();
    resetPseudoStates();
}

void InspectorCSSAgent::enable(ErrorString*)
{
    m_state->setBoolean(CSSAgentState::cssAgentEnabled, true);
    m_instrumentingAgents->setInspectorCSSAgent(this);
}

void InspectorCSSAgent::disable(ErrorString*)
{
    m_instrumentingAgents->setInspectorCSSAgent(0);
    m_state->setBoolean(CSSAgentState::cssAgentEnabled, false);
}

void InspectorCSSAgent::mediaQueryResultChanged()
{
    if (m_frontend)
        m_frontend->mediaQueryResultChanged();
}

void InspectorCSSAgent::didCreateNamedFlow(Document* document, WebKitNamedFlow* namedFlow)
{
    int documentNodeId = documentNodeWithRequestedFlowsId(document);
    if (!documentNodeId)
        return;

    ErrorString errorString;
    m_frontend->namedFlowCreated(buildObjectForNamedFlow(&errorString, namedFlow, documentNodeId));
}

void InspectorCSSAgent::willRemoveNamedFlow(Document* document, WebKitNamedFlow* namedFlow)
{
    int documentNodeId = documentNodeWithRequestedFlowsId(document);
    if (!documentNodeId)
        return;

    if (m_updateRegionLayoutTask)
        m_updateRegionLayoutTask->unschedule(namedFlow);
    
    if (m_changeRegionOversetTask)
        m_changeRegionOversetTask->unschedule(namedFlow);

    m_frontend->namedFlowRemoved(documentNodeId, namedFlow->name().string());
}

void InspectorCSSAgent::didUpdateRegionLayout(Document* document, WebKitNamedFlow* namedFlow)
{
    int documentNodeId = documentNodeWithRequestedFlowsId(document);
    if (!documentNodeId)
        return;

    if (!m_updateRegionLayoutTask)
        m_updateRegionLayoutTask = adoptPtr(new UpdateRegionLayoutTask(this));
    m_updateRegionLayoutTask->scheduleFor(namedFlow, documentNodeId);
}

void InspectorCSSAgent::regionLayoutUpdated(WebKitNamedFlow* namedFlow, int documentNodeId)
{
    if (namedFlow->flowState() == WebKitNamedFlow::FlowStateNull)
        return;

    ErrorString errorString;
    RefPtr<WebKitNamedFlow> protector(namedFlow);

    m_frontend->regionLayoutUpdated(buildObjectForNamedFlow(&errorString, namedFlow, documentNodeId));
}

void InspectorCSSAgent::didChangeRegionOverset(Document* document, WebKitNamedFlow* namedFlow)
{
    int documentNodeId = documentNodeWithRequestedFlowsId(document);
    if (!documentNodeId)
        return;
    
    if (!m_changeRegionOversetTask)
        m_changeRegionOversetTask = adoptPtr(new ChangeRegionOversetTask(this));
    m_changeRegionOversetTask->scheduleFor(namedFlow, documentNodeId);
}

void InspectorCSSAgent::regionOversetChanged(WebKitNamedFlow* namedFlow, int documentNodeId)
{
    if (namedFlow->flowState() == WebKitNamedFlow::FlowStateNull)
        return;
    
    ErrorString errorString;
    RefPtr<WebKitNamedFlow> protector(namedFlow);
    
    m_frontend->regionOversetChanged(buildObjectForNamedFlow(&errorString, namedFlow, documentNodeId));
}

bool InspectorCSSAgent::forcePseudoState(Element* element, CSSSelector::PseudoType pseudoType)
{
    if (m_nodeIdToForcedPseudoState.isEmpty())
        return false;

    int nodeId = m_domAgent->boundNodeId(element);
    if (!nodeId)
        return false;

    NodeIdToForcedPseudoState::iterator it = m_nodeIdToForcedPseudoState.find(nodeId);
    if (it == m_nodeIdToForcedPseudoState.end())
        return false;

    unsigned forcedPseudoState = it->value;
    switch (pseudoType) {
    case CSSSelector::PseudoActive:
        return forcedPseudoState & PseudoActive;
    case CSSSelector::PseudoFocus:
        return forcedPseudoState & PseudoFocus;
    case CSSSelector::PseudoHover:
        return forcedPseudoState & PseudoHover;
    case CSSSelector::PseudoVisited:
        return forcedPseudoState & PseudoVisited;
    default:
        return false;
    }
}

void InspectorCSSAgent::getMatchedStylesForNode(ErrorString* errorString, int nodeId, const bool* includePseudo, const bool* includeInherited, RefPtr<TypeBuilder::Array<TypeBuilder::CSS::RuleMatch> >& matchedCSSRules, RefPtr<TypeBuilder::Array<TypeBuilder::CSS::PseudoIdMatches> >& pseudoIdMatches, RefPtr<TypeBuilder::Array<TypeBuilder::CSS::InheritedStyleEntry> >& inheritedEntries)
{
    Element* element = elementForId(errorString, nodeId);
    if (!element)
        return;

    // Matched rules.
    StyleResolver* styleResolver = element->ownerDocument()->ensureStyleResolver();
    Vector<RefPtr<StyleRuleBase> > matchedRules = styleResolver->styleRulesForElement(element, StyleResolver::AllCSSRules);
    matchedCSSRules = buildArrayForMatchedRuleList(matchedRules, styleResolver, element);

    // Pseudo elements.
    if (!includePseudo || *includePseudo) {
        RefPtr<TypeBuilder::Array<TypeBuilder::CSS::PseudoIdMatches> > pseudoElements = TypeBuilder::Array<TypeBuilder::CSS::PseudoIdMatches>::create();
        for (PseudoId pseudoId = FIRST_PUBLIC_PSEUDOID; pseudoId < AFTER_LAST_INTERNAL_PSEUDOID; pseudoId = static_cast<PseudoId>(pseudoId + 1)) {
            Vector<RefPtr<StyleRuleBase> > matchedRules = styleResolver->pseudoStyleRulesForElement(element, pseudoId, StyleResolver::AllCSSRules);
            if (!matchedRules.isEmpty()) {
                RefPtr<TypeBuilder::CSS::PseudoIdMatches> matches = TypeBuilder::CSS::PseudoIdMatches::create()
                    .setPseudoId(static_cast<int>(pseudoId))
                    .setMatches(buildArrayForMatchedRuleList(matchedRules, styleResolver, element));
                pseudoElements->addItem(matches.release());
            }
        }

        pseudoIdMatches = pseudoElements.release();
    }

    // Inherited styles.
    if (!includeInherited || *includeInherited) {
        RefPtr<TypeBuilder::Array<TypeBuilder::CSS::InheritedStyleEntry> > entries = TypeBuilder::Array<TypeBuilder::CSS::InheritedStyleEntry>::create();
        Element* parentElement = element->parentElement();
        while (parentElement) {
            StyleResolver* parentStyleResolver = parentElement->ownerDocument()->ensureStyleResolver();
            Vector<RefPtr<StyleRuleBase> > parentMatchedRules = parentStyleResolver->styleRulesForElement(parentElement, StyleResolver::AllCSSRules);
            RefPtr<TypeBuilder::CSS::InheritedStyleEntry> entry = TypeBuilder::CSS::InheritedStyleEntry::create()
                .setMatchedCSSRules(buildArrayForMatchedRuleList(parentMatchedRules, styleResolver, parentElement));
            if (parentElement->style() && parentElement->style()->length()) {
                InspectorStyleSheetForInlineStyle* styleSheet = asInspectorStyleSheet(parentElement);
                if (styleSheet)
                    entry->setInlineStyle(styleSheet->buildObjectForStyle(styleSheet->styleForId(InspectorCSSId(styleSheet->id(), 0))));
            }

            entries->addItem(entry.release());
            parentElement = parentElement->parentElement();
        }

        inheritedEntries = entries.release();
    }
}

void InspectorCSSAgent::getInlineStylesForNode(ErrorString* errorString, int nodeId, RefPtr<TypeBuilder::CSS::CSSStyle>& inlineStyle, RefPtr<TypeBuilder::CSS::CSSStyle>& attributesStyle)
{
    Element* element = elementForId(errorString, nodeId);
    if (!element)
        return;

    InspectorStyleSheetForInlineStyle* styleSheet = asInspectorStyleSheet(element);
    if (!styleSheet)
        return;

    inlineStyle = styleSheet->buildObjectForStyle(element->style());
    RefPtr<TypeBuilder::CSS::CSSStyle> attributes = buildObjectForAttributesStyle(element);
    attributesStyle = attributes ? attributes.release() : 0;
}

void InspectorCSSAgent::getComputedStyleForNode(ErrorString* errorString, int nodeId, RefPtr<TypeBuilder::Array<TypeBuilder::CSS::CSSComputedStyleProperty> >& style)
{
    Element* element = elementForId(errorString, nodeId);
    if (!element)
        return;

    RefPtr<CSSComputedStyleDeclaration> computedStyleInfo = CSSComputedStyleDeclaration::create(element, true);
    RefPtr<InspectorStyle> inspectorStyle = InspectorStyle::create(InspectorCSSId(), computedStyleInfo, 0);
    style = inspectorStyle->buildArrayForComputedStyle();
}

void InspectorCSSAgent::getAllStyleSheets(ErrorString*, RefPtr<TypeBuilder::Array<TypeBuilder::CSS::CSSStyleSheetHeader> >& styleInfos)
{
    styleInfos = TypeBuilder::Array<TypeBuilder::CSS::CSSStyleSheetHeader>::create();
    Vector<Document*> documents = m_domAgent->documents();
    for (Vector<Document*>::iterator it = documents.begin(); it != documents.end(); ++it) {
        StyleSheetList* list = (*it)->styleSheets();
        for (unsigned i = 0; i < list->length(); ++i) {
            StyleSheet* styleSheet = list->item(i);
            if (styleSheet->isCSSStyleSheet())
                collectStyleSheets(static_cast<CSSStyleSheet*>(styleSheet), styleInfos.get());
        }
    }
}

void InspectorCSSAgent::getStyleSheet(ErrorString* errorString, const String& styleSheetId, RefPtr<TypeBuilder::CSS::CSSStyleSheetBody>& styleSheetObject)
{
    InspectorStyleSheet* inspectorStyleSheet = assertStyleSheetForId(errorString, styleSheetId);
    if (!inspectorStyleSheet)
        return;

    styleSheetObject = inspectorStyleSheet->buildObjectForStyleSheet();
}

void InspectorCSSAgent::getStyleSheetText(ErrorString* errorString, const String& styleSheetId, String* result)
{
    InspectorStyleSheet* inspectorStyleSheet = assertStyleSheetForId(errorString, styleSheetId);
    if (!inspectorStyleSheet)
        return;

    inspectorStyleSheet->getText(result);
}

void InspectorCSSAgent::setStyleSheetText(ErrorString* errorString, const String& styleSheetId, const String& text)
{
    InspectorStyleSheet* inspectorStyleSheet = assertStyleSheetForId(errorString, styleSheetId);
    if (!inspectorStyleSheet)
        return;

    ExceptionCode ec = 0;
    m_domAgent->history()->perform(adoptPtr(new SetStyleSheetTextAction(inspectorStyleSheet, text)), ec);
    *errorString = InspectorDOMAgent::toErrorString(ec);
}

void InspectorCSSAgent::setStyleText(ErrorString* errorString, const RefPtr<InspectorObject>& fullStyleId, const String& text, RefPtr<TypeBuilder::CSS::CSSStyle>& result)
{
    InspectorCSSId compoundId(fullStyleId);
    ASSERT(!compoundId.isEmpty());

    InspectorStyleSheet* inspectorStyleSheet = assertStyleSheetForId(errorString, compoundId.styleSheetId());
    if (!inspectorStyleSheet)
        return;

    ExceptionCode ec = 0;
    bool success = m_domAgent->history()->perform(adoptPtr(new SetStyleTextAction(inspectorStyleSheet, compoundId, text)), ec);
    if (success)
        result = inspectorStyleSheet->buildObjectForStyle(inspectorStyleSheet->styleForId(compoundId));
    *errorString = InspectorDOMAgent::toErrorString(ec);
}

void InspectorCSSAgent::setPropertyText(ErrorString* errorString, const RefPtr<InspectorObject>& fullStyleId, int propertyIndex, const String& text, bool overwrite, RefPtr<TypeBuilder::CSS::CSSStyle>& result)
{
    InspectorCSSId compoundId(fullStyleId);
    ASSERT(!compoundId.isEmpty());

    InspectorStyleSheet* inspectorStyleSheet = assertStyleSheetForId(errorString, compoundId.styleSheetId());
    if (!inspectorStyleSheet)
        return;

    ExceptionCode ec = 0;
    bool success = m_domAgent->history()->perform(adoptPtr(new SetPropertyTextAction(inspectorStyleSheet, compoundId, propertyIndex, text, overwrite)), ec);
    if (success)
        result = inspectorStyleSheet->buildObjectForStyle(inspectorStyleSheet->styleForId(compoundId));
    *errorString = InspectorDOMAgent::toErrorString(ec);
}

void InspectorCSSAgent::toggleProperty(ErrorString* errorString, const RefPtr<InspectorObject>& fullStyleId, int propertyIndex, bool disable, RefPtr<TypeBuilder::CSS::CSSStyle>& result)
{
    InspectorCSSId compoundId(fullStyleId);
    ASSERT(!compoundId.isEmpty());

    InspectorStyleSheet* inspectorStyleSheet = assertStyleSheetForId(errorString, compoundId.styleSheetId());
    if (!inspectorStyleSheet)
        return;

    ExceptionCode ec = 0;
    bool success = m_domAgent->history()->perform(adoptPtr(new TogglePropertyAction(inspectorStyleSheet, compoundId, propertyIndex, disable)), ec);
    if (success)
        result = inspectorStyleSheet->buildObjectForStyle(inspectorStyleSheet->styleForId(compoundId));
    *errorString = InspectorDOMAgent::toErrorString(ec);
}

void InspectorCSSAgent::setRuleSelector(ErrorString* errorString, const RefPtr<InspectorObject>& fullRuleId, const String& selector, RefPtr<TypeBuilder::CSS::CSSRule>& result)
{
    InspectorCSSId compoundId(fullRuleId);
    ASSERT(!compoundId.isEmpty());

    InspectorStyleSheet* inspectorStyleSheet = assertStyleSheetForId(errorString, compoundId.styleSheetId());
    if (!inspectorStyleSheet)
        return;

    ExceptionCode ec = 0;
    bool success = m_domAgent->history()->perform(adoptPtr(new SetRuleSelectorAction(inspectorStyleSheet, compoundId, selector)), ec);

    if (success)
        result = inspectorStyleSheet->buildObjectForRule(inspectorStyleSheet->ruleForId(compoundId));
    *errorString = InspectorDOMAgent::toErrorString(ec);
}

void InspectorCSSAgent::addRule(ErrorString* errorString, const int contextNodeId, const String& selector, RefPtr<TypeBuilder::CSS::CSSRule>& result)
{
    Node* node = m_domAgent->assertNode(errorString, contextNodeId);
    if (!node)
        return;

    InspectorStyleSheet* inspectorStyleSheet = viaInspectorStyleSheet(node->document(), true);
    if (!inspectorStyleSheet) {
        *errorString = "No target stylesheet found";
        return;
    }

    ExceptionCode ec = 0;
    OwnPtr<AddRuleAction> action = adoptPtr(new AddRuleAction(inspectorStyleSheet, selector));
    AddRuleAction* rawAction = action.get();
    bool success = m_domAgent->history()->perform(action.release(), ec);
    if (!success) {
        *errorString = InspectorDOMAgent::toErrorString(ec);
        return;
    }

    InspectorCSSId ruleId = rawAction->newRuleId();
    CSSStyleRule* rule = inspectorStyleSheet->ruleForId(ruleId);
    result = inspectorStyleSheet->buildObjectForRule(rule);
}

void InspectorCSSAgent::getSupportedCSSProperties(ErrorString*, RefPtr<TypeBuilder::Array<TypeBuilder::CSS::CSSPropertyInfo> >& cssProperties)
{
    RefPtr<TypeBuilder::Array<TypeBuilder::CSS::CSSPropertyInfo> > properties = TypeBuilder::Array<TypeBuilder::CSS::CSSPropertyInfo>::create();
    for (int i = firstCSSProperty; i <= lastCSSProperty; ++i) {
        CSSPropertyID id = convertToCSSPropertyID(i);
        RefPtr<TypeBuilder::CSS::CSSPropertyInfo> property = TypeBuilder::CSS::CSSPropertyInfo::create()
            .setName(getPropertyNameString(id));

        const StylePropertyShorthand& shorthand = shorthandForProperty(id);
        if (!shorthand.length()) {
            properties->addItem(property.release());
            continue;
        }
        RefPtr<TypeBuilder::Array<String> > longhands = TypeBuilder::Array<String>::create();
        for (unsigned j = 0; j < shorthand.length(); ++j) {
            CSSPropertyID longhandID = shorthand.properties()[j];
            longhands->addItem(getPropertyNameString(longhandID));
        }
        property->setLonghands(longhands);
        properties->addItem(property.release());
    }
    cssProperties = properties.release();
}

void InspectorCSSAgent::forcePseudoState(ErrorString* errorString, int nodeId, const RefPtr<InspectorArray>& forcedPseudoClasses)
{
    Element* element = m_domAgent->assertElement(errorString, nodeId);
    if (!element)
        return;

    unsigned forcedPseudoState = computePseudoClassMask(forcedPseudoClasses.get());
    NodeIdToForcedPseudoState::iterator it = m_nodeIdToForcedPseudoState.find(nodeId);
    unsigned currentForcedPseudoState = it == m_nodeIdToForcedPseudoState.end() ? 0 : it->value;
    bool needStyleRecalc = forcedPseudoState != currentForcedPseudoState;
    if (!needStyleRecalc)
        return;

    if (forcedPseudoState)
        m_nodeIdToForcedPseudoState.set(nodeId, forcedPseudoState);
    else
        m_nodeIdToForcedPseudoState.remove(nodeId);
    element->ownerDocument()->styleResolverChanged(RecalcStyleImmediately);
}

void InspectorCSSAgent::getNamedFlowCollection(ErrorString* errorString, int documentNodeId, RefPtr<TypeBuilder::Array<TypeBuilder::CSS::NamedFlow> >& result)
{
    Document* document = m_domAgent->assertDocument(errorString, documentNodeId);
    if (!document)
        return;

    m_namedFlowCollectionsRequested.add(documentNodeId);

    Vector<RefPtr<WebKitNamedFlow> > namedFlowsVector = document->namedFlows()->namedFlows();
    RefPtr<TypeBuilder::Array<TypeBuilder::CSS::NamedFlow> > namedFlows = TypeBuilder::Array<TypeBuilder::CSS::NamedFlow>::create();

    for (Vector<RefPtr<WebKitNamedFlow> >::iterator it = namedFlowsVector.begin(); it != namedFlowsVector.end(); ++it)
        namedFlows->addItem(buildObjectForNamedFlow(errorString, it->get(), documentNodeId));

    result = namedFlows.release();
}

void InspectorCSSAgent::startSelectorProfiler(ErrorString*)
{
    m_currentSelectorProfile = adoptPtr(new SelectorProfile());
    m_state->setBoolean(CSSAgentState::isSelectorProfiling, true);
}

void InspectorCSSAgent::stopSelectorProfiler(ErrorString* errorString, RefPtr<TypeBuilder::CSS::SelectorProfile>& result)
{
    result = stopSelectorProfilerImpl(errorString, true);
}

PassRefPtr<TypeBuilder::CSS::SelectorProfile> InspectorCSSAgent::stopSelectorProfilerImpl(ErrorString*, bool needProfile)
{
    if (!m_state->getBoolean(CSSAgentState::isSelectorProfiling))
        return 0;
    m_state->setBoolean(CSSAgentState::isSelectorProfiling, false);
    RefPtr<TypeBuilder::CSS::SelectorProfile> result;
    if (m_frontend && needProfile)
        result = m_currentSelectorProfile->toInspectorObject();
    m_currentSelectorProfile.clear();
    return result.release();
}

void InspectorCSSAgent::willMatchRule(StyleRule* rule, InspectorCSSOMWrappers& inspectorCSSOMWrappers, DocumentStyleSheetCollection* styleSheetCollection)
{
//    printf("InspectorCSSAgent::willMatchRule %s\n", rule->selectorList().selectorsText().utf8().data());
    if (m_currentSelectorProfile)
        m_currentSelectorProfile->startSelector(inspectorCSSOMWrappers.getWrapperForRuleInSheets(rule, styleSheetCollection));
}

void InspectorCSSAgent::didMatchRule(bool matched)
{
    if (m_currentSelectorProfile)
        m_currentSelectorProfile->commitSelector(matched);
}

void InspectorCSSAgent::willProcessRule(StyleRule* rule, StyleResolver* styleResolver)
{
    if (m_currentSelectorProfile)
        m_currentSelectorProfile->startSelector(styleResolver->inspectorCSSOMWrappers().getWrapperForRuleInSheets(rule, styleResolver->document()->styleSheetCollection()));
}

void InspectorCSSAgent::didProcessRule()
{
    if (m_currentSelectorProfile)
        m_currentSelectorProfile->commitSelectorTime();
}

InspectorStyleSheetForInlineStyle* InspectorCSSAgent::asInspectorStyleSheet(Element* element)
{
    NodeToInspectorStyleSheet::iterator it = m_nodeToInspectorStyleSheet.find(element);
    if (it == m_nodeToInspectorStyleSheet.end()) {
        CSSStyleDeclaration* style = element->isStyledElement() ? element->style() : 0;
        if (!style)
            return 0;

        String newStyleSheetId = String::number(m_lastStyleSheetId++);
        RefPtr<InspectorStyleSheetForInlineStyle> inspectorStyleSheet = InspectorStyleSheetForInlineStyle::create(m_domAgent->pageAgent(), newStyleSheetId, element, TypeBuilder::CSS::StyleSheetOrigin::Regular, this);
        m_idToInspectorStyleSheet.set(newStyleSheetId, inspectorStyleSheet);
        m_nodeToInspectorStyleSheet.set(element, inspectorStyleSheet);
        return inspectorStyleSheet.get();
    }

    return it->value.get();
}

Element* InspectorCSSAgent::elementForId(ErrorString* errorString, int nodeId)
{
    Node* node = m_domAgent->nodeForId(nodeId);
    if (!node) {
        *errorString = "No node with given id found";
        return 0;
    }
    if (node->nodeType() != Node::ELEMENT_NODE) {
        *errorString = "Not an element node";
        return 0;
    }
    return toElement(node);
}

int InspectorCSSAgent::documentNodeWithRequestedFlowsId(Document* document)
{
    int documentNodeId = m_domAgent->boundNodeId(document);
    if (!documentNodeId || !m_namedFlowCollectionsRequested.contains(documentNodeId))
        return 0;

    return documentNodeId;
}

void InspectorCSSAgent::collectStyleSheets(CSSStyleSheet* styleSheet, TypeBuilder::Array<TypeBuilder::CSS::CSSStyleSheetHeader>* result)
{
    InspectorStyleSheet* inspectorStyleSheet = bindStyleSheet(static_cast<CSSStyleSheet*>(styleSheet));
    result->addItem(inspectorStyleSheet->buildObjectForStyleSheetInfo());
    for (unsigned i = 0, size = styleSheet->length(); i < size; ++i) {
        CSSRule* rule = styleSheet->item(i);
        if (rule->type() == CSSRule::IMPORT_RULE) {
            CSSStyleSheet* importedStyleSheet = static_cast<CSSImportRule*>(rule)->styleSheet();
            if (importedStyleSheet)
                collectStyleSheets(importedStyleSheet, result);
        }
    }
}

InspectorStyleSheet* InspectorCSSAgent::bindStyleSheet(CSSStyleSheet* styleSheet)
{
    RefPtr<InspectorStyleSheet> inspectorStyleSheet = m_cssStyleSheetToInspectorStyleSheet.get(styleSheet);
    if (!inspectorStyleSheet) {
        String id = String::number(m_lastStyleSheetId++);
        Document* document = styleSheet->ownerDocument();
        inspectorStyleSheet = InspectorStyleSheet::create(m_domAgent->pageAgent(), id, styleSheet, detectOrigin(styleSheet, document), InspectorDOMAgent::documentURLString(document), this);
        m_idToInspectorStyleSheet.set(id, inspectorStyleSheet);
        m_cssStyleSheetToInspectorStyleSheet.set(styleSheet, inspectorStyleSheet);
    }
    return inspectorStyleSheet.get();
}

InspectorStyleSheet* InspectorCSSAgent::viaInspectorStyleSheet(Document* document, bool createIfAbsent)
{
    if (!document) {
        ASSERT(!createIfAbsent);
        return 0;
    }

    if (!document->isHTMLDocument() && !document->isSVGDocument())
        return 0;

    RefPtr<InspectorStyleSheet> inspectorStyleSheet = m_documentToInspectorStyleSheet.get(document);
    if (inspectorStyleSheet || !createIfAbsent)
        return inspectorStyleSheet.get();

    ExceptionCode ec = 0;
    RefPtr<Element> styleElement = document->createElement("style", ec);
    if (!ec)
        styleElement->setAttribute("type", "text/css", ec);
    if (!ec) {
        ContainerNode* targetNode;
        // HEAD is absent in ImageDocuments, for example.
        if (document->head())
            targetNode = document->head();
        else if (document->body())
            targetNode = document->body();
        else
            return 0;

        InlineStyleOverrideScope overrideScope(document);
        targetNode->appendChild(styleElement, ec);
    }
    if (ec)
        return 0;

    CSSStyleSheet* cssStyleSheet = 0;
    if (styleElement->isHTMLElement())
        cssStyleSheet = toHTMLStyleElement(styleElement.get())->sheet();
#if ENABLE(SVG)
    else if (styleElement->isSVGElement())
        cssStyleSheet = static_cast<SVGStyleElement*>(styleElement.get())->sheet();
#endif

    if (!cssStyleSheet)
        return 0;

    String id = String::number(m_lastStyleSheetId++);
    inspectorStyleSheet = InspectorStyleSheet::create(m_domAgent->pageAgent(), id, cssStyleSheet, TypeBuilder::CSS::StyleSheetOrigin::Inspector, InspectorDOMAgent::documentURLString(document), this);
    m_idToInspectorStyleSheet.set(id, inspectorStyleSheet);
    m_cssStyleSheetToInspectorStyleSheet.set(cssStyleSheet, inspectorStyleSheet);
    m_documentToInspectorStyleSheet.set(document, inspectorStyleSheet);
    return inspectorStyleSheet.get();
}

InspectorStyleSheet* InspectorCSSAgent::assertStyleSheetForId(ErrorString* errorString, const String& styleSheetId)
{
    IdToInspectorStyleSheet::iterator it = m_idToInspectorStyleSheet.find(styleSheetId);
    if (it == m_idToInspectorStyleSheet.end()) {
        *errorString = "No style sheet with given id found";
        return 0;
    }
    return it->value.get();
}

TypeBuilder::CSS::StyleSheetOrigin::Enum InspectorCSSAgent::detectOrigin(CSSStyleSheet* pageStyleSheet, Document* ownerDocument)
{
    TypeBuilder::CSS::StyleSheetOrigin::Enum origin = TypeBuilder::CSS::StyleSheetOrigin::Regular;
    if (pageStyleSheet && !pageStyleSheet->ownerNode() && pageStyleSheet->href().isEmpty())
        origin = TypeBuilder::CSS::StyleSheetOrigin::User_agent;
    else if (pageStyleSheet && pageStyleSheet->ownerNode() && pageStyleSheet->ownerNode()->nodeName() == "#document")
        origin = TypeBuilder::CSS::StyleSheetOrigin::User;
    else {
        InspectorStyleSheet* viaInspectorStyleSheetForOwner = viaInspectorStyleSheet(ownerDocument, false);
        if (viaInspectorStyleSheetForOwner && pageStyleSheet == viaInspectorStyleSheetForOwner->pageStyleSheet())
            origin = TypeBuilder::CSS::StyleSheetOrigin::Inspector;
    }
    return origin;
}

PassRefPtr<TypeBuilder::CSS::CSSRule> InspectorCSSAgent::buildObjectForRule(StyleRule* styleRule, StyleResolver* styleResolver)
{
    if (!styleRule)
        return 0;

    // StyleRules returned by StyleResolver::styleRulesForElement lack parent pointers since that infomation is not cheaply available.
    // Since the inspector wants to walk the parent chain, we construct the full wrappers here.
    CSSStyleRule* cssomWrapper = styleResolver->inspectorCSSOMWrappers().getWrapperForRuleInSheets(styleRule, styleResolver->document()->styleSheetCollection());
    if (!cssomWrapper)
        return 0;
    InspectorStyleSheet* inspectorStyleSheet = bindStyleSheet(cssomWrapper->parentStyleSheet());
    return inspectorStyleSheet ? inspectorStyleSheet->buildObjectForRule(cssomWrapper) : 0;
}

PassRefPtr<TypeBuilder::CSS::CSSRule> InspectorCSSAgent::buildObjectForRule(CSSStyleRule* rule)
{
    if (!rule)
        return 0;

    ASSERT(rule->parentStyleSheet());
    InspectorStyleSheet* inspectorStyleSheet = bindStyleSheet(rule->parentStyleSheet());
    return inspectorStyleSheet ? inspectorStyleSheet->buildObjectForRule(rule) : 0;
}

PassRefPtr<TypeBuilder::Array<TypeBuilder::CSS::CSSRule> > InspectorCSSAgent::buildArrayForRuleList(CSSRuleList* ruleList)
{
    RefPtr<TypeBuilder::Array<TypeBuilder::CSS::CSSRule> > result = TypeBuilder::Array<TypeBuilder::CSS::CSSRule>::create();
    if (!ruleList)
        return result.release();

    for (unsigned i = 0, size = ruleList->length(); i < size; ++i) {
        CSSStyleRule* rule = asCSSStyleRule(ruleList->item(i));
        RefPtr<TypeBuilder::CSS::CSSRule> ruleObject = buildObjectForRule(rule);
        if (!ruleObject)
            continue;
        result->addItem(ruleObject);
    }
    return result.release();
}

PassRefPtr<TypeBuilder::Array<TypeBuilder::CSS::RuleMatch> > InspectorCSSAgent::buildArrayForMatchedRuleList(const Vector<RefPtr<StyleRuleBase> >& matchedRules, StyleResolver* styleResolver, Element* element)
{
    RefPtr<TypeBuilder::Array<TypeBuilder::CSS::RuleMatch> > result = TypeBuilder::Array<TypeBuilder::CSS::RuleMatch>::create();

    for (unsigned i = 0; i < matchedRules.size(); ++i) {
        if (!matchedRules[i]->isStyleRule())
            continue;
        StyleRule* matchedStyleRule = static_cast<StyleRule*>(matchedRules[i].get());
        RefPtr<TypeBuilder::CSS::CSSRule> ruleObject = buildObjectForRule(matchedStyleRule, styleResolver);
        if (!ruleObject)
            continue;
        RefPtr<TypeBuilder::Array<int> > matchingSelectors = TypeBuilder::Array<int>::create();
        const CSSSelectorList& selectorList = matchedStyleRule->selectorList();
        long index = 0;
        for (const CSSSelector* selector = selectorList.first(); selector; selector = CSSSelectorList::next(selector)) {
            bool matched = element->webkitMatchesSelector(selector->selectorText(), IGNORE_EXCEPTION);
            if (matched)
                matchingSelectors->addItem(index);
            ++index;
        }
        RefPtr<TypeBuilder::CSS::RuleMatch> match = TypeBuilder::CSS::RuleMatch::create()
            .setRule(ruleObject)
            .setMatchingSelectors(matchingSelectors);
        result->addItem(match);
    }

    return result.release();
}

PassRefPtr<TypeBuilder::CSS::CSSStyle> InspectorCSSAgent::buildObjectForAttributesStyle(Element* element)
{
    if (!element->isStyledElement())
        return 0;

    // FIXME: Ugliness below.
    StylePropertySet* attributeStyle = const_cast<StylePropertySet*>(static_cast<StyledElement*>(element)->presentationAttributeStyle());
    if (!attributeStyle)
        return 0;

    ASSERT(attributeStyle->isMutable());
    MutableStylePropertySet* mutableAttributeStyle = static_cast<MutableStylePropertySet*>(attributeStyle);

    RefPtr<InspectorStyle> inspectorStyle = InspectorStyle::create(InspectorCSSId(), mutableAttributeStyle->ensureCSSStyleDeclaration(), 0);
    return inspectorStyle->buildObjectForStyle();
}

PassRefPtr<TypeBuilder::Array<TypeBuilder::CSS::Region> > InspectorCSSAgent::buildArrayForRegions(ErrorString* errorString, PassRefPtr<NodeList> regionList, int documentNodeId)
{
    RefPtr<TypeBuilder::Array<TypeBuilder::CSS::Region> > regions = TypeBuilder::Array<TypeBuilder::CSS::Region>::create();

    for (unsigned i = 0; i < regionList->length(); ++i) {
        TypeBuilder::CSS::Region::RegionOverset::Enum regionOverset;

        switch (toElement(regionList->item(i))->renderRegion()->regionOversetState()) {
        case RegionFit:
            regionOverset = TypeBuilder::CSS::Region::RegionOverset::Fit;
            break;
        case RegionEmpty:
            regionOverset = TypeBuilder::CSS::Region::RegionOverset::Empty;
            break;
        case RegionOverset:
            regionOverset = TypeBuilder::CSS::Region::RegionOverset::Overset;
            break;
        case RegionUndefined:
            continue;
        default:
            ASSERT_NOT_REACHED();
            continue;
        }

        RefPtr<TypeBuilder::CSS::Region> region = TypeBuilder::CSS::Region::create()
            .setRegionOverset(regionOverset)
            // documentNodeId was previously asserted
            .setNodeId(m_domAgent->pushNodeToFrontend(errorString, documentNodeId, regionList->item(i)));

        regions->addItem(region);
    }

    return regions.release();
}

PassRefPtr<TypeBuilder::CSS::NamedFlow> InspectorCSSAgent::buildObjectForNamedFlow(ErrorString* errorString, WebKitNamedFlow* webkitNamedFlow, int documentNodeId)
{
    RefPtr<NodeList> contentList = webkitNamedFlow->getContent();
    RefPtr<TypeBuilder::Array<int> > content = TypeBuilder::Array<int>::create();

    for (unsigned i = 0; i < contentList->length(); ++i) {
        // documentNodeId was previously asserted
        content->addItem(m_domAgent->pushNodeToFrontend(errorString, documentNodeId, contentList->item(i)));
    }

    RefPtr<TypeBuilder::CSS::NamedFlow> namedFlow = TypeBuilder::CSS::NamedFlow::create()
        .setDocumentNodeId(documentNodeId)
        .setName(webkitNamedFlow->name().string())
        .setOverset(webkitNamedFlow->overset())
        .setContent(content)
        .setRegions(buildArrayForRegions(errorString, webkitNamedFlow->getRegions(), documentNodeId));

    return namedFlow.release();
}

void InspectorCSSAgent::didRemoveDocument(Document* document)
{
    if (document)
        m_documentToInspectorStyleSheet.remove(document);
}

void InspectorCSSAgent::didRemoveDOMNode(Node* node)
{
    if (!node)
        return;

    int nodeId = m_domAgent->boundNodeId(node);
    if (nodeId)
        m_nodeIdToForcedPseudoState.remove(nodeId);

    NodeToInspectorStyleSheet::iterator it = m_nodeToInspectorStyleSheet.find(node);
    if (it == m_nodeToInspectorStyleSheet.end())
        return;

    m_idToInspectorStyleSheet.remove(it->value->id());
    m_nodeToInspectorStyleSheet.remove(node);
}

void InspectorCSSAgent::didModifyDOMAttr(Element* element)
{
    if (!element)
        return;

    NodeToInspectorStyleSheet::iterator it = m_nodeToInspectorStyleSheet.find(element);
    if (it == m_nodeToInspectorStyleSheet.end())
        return;

    it->value->didModifyElementAttribute();
}

void InspectorCSSAgent::styleSheetChanged(InspectorStyleSheet* styleSheet)
{
    if (m_frontend)
        m_frontend->styleSheetChanged(styleSheet->id());
}

void InspectorCSSAgent::resetPseudoStates()
{
    HashSet<Document*> documentsToChange;
    for (NodeIdToForcedPseudoState::iterator it = m_nodeIdToForcedPseudoState.begin(), end = m_nodeIdToForcedPseudoState.end(); it != end; ++it) {
        Element* element = toElement(m_domAgent->nodeForId(it->key));
        if (element && element->ownerDocument())
            documentsToChange.add(element->ownerDocument());
    }

    m_nodeIdToForcedPseudoState.clear();
    for (HashSet<Document*>::iterator it = documentsToChange.begin(), end = documentsToChange.end(); it != end; ++it)
        (*it)->styleResolverChanged(RecalcStyleImmediately);
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
