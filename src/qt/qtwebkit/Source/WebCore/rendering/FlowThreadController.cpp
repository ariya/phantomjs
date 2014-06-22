/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"

#include "FlowThreadController.h"

#include "NamedFlowCollection.h"
#include "RenderFlowThread.h"
#include "RenderNamedFlowThread.h"
#include "StyleInheritedData.h"
#include "WebKitNamedFlow.h"
#include <wtf/text/AtomicString.h>

namespace WebCore {

PassOwnPtr<FlowThreadController> FlowThreadController::create(RenderView* view)
{
    return adoptPtr(new FlowThreadController(view));
}

FlowThreadController::FlowThreadController(RenderView* view)
    : m_view(view)
    , m_currentRenderFlowThread(0)
    , m_isRenderNamedFlowThreadOrderDirty(false)
    , m_flowThreadsWithAutoLogicalHeightRegions(0)
{
}

FlowThreadController::~FlowThreadController()
{
}

RenderNamedFlowThread* FlowThreadController::ensureRenderFlowThreadWithName(const AtomicString& name)
{
    if (!m_renderNamedFlowThreadList)
        m_renderNamedFlowThreadList = adoptPtr(new RenderNamedFlowThreadList());
    else {
        for (RenderNamedFlowThreadList::iterator iter = m_renderNamedFlowThreadList->begin(); iter != m_renderNamedFlowThreadList->end(); ++iter) {
            RenderNamedFlowThread* flowRenderer = *iter;
            if (flowRenderer->flowThreadName() == name)
                return flowRenderer;
        }
    }

    NamedFlowCollection* namedFlows = m_view->document()->namedFlows();

    // Sanity check for the absence of a named flow in the "CREATED" state with the same name.
    ASSERT(!namedFlows->flowByName(name));

    RenderNamedFlowThread* flowRenderer = RenderNamedFlowThread::createAnonymous(m_view->document(), namedFlows->ensureFlowWithName(name));
    flowRenderer->setStyle(RenderFlowThread::createFlowThreadStyle(m_view->style()));
    m_renderNamedFlowThreadList->add(flowRenderer);

    // Keep the flow renderer as a child of RenderView.
    m_view->addChild(flowRenderer);

    setIsRenderNamedFlowThreadOrderDirty(true);

    return flowRenderer;
}

void FlowThreadController::styleDidChange()
{
    RenderStyle* viewStyle = m_view->style();
    for (RenderNamedFlowThreadList::iterator iter = m_renderNamedFlowThreadList->begin(); iter != m_renderNamedFlowThreadList->end(); ++iter) {
        RenderNamedFlowThread* flowRenderer = *iter;
        flowRenderer->setStyle(RenderFlowThread::createFlowThreadStyle(viewStyle));
    }
}

void FlowThreadController::layoutRenderNamedFlowThreads()
{
    updateFlowThreadsChainIfNecessary();

    for (RenderNamedFlowThreadList::iterator iter = m_renderNamedFlowThreadList->begin(); iter != m_renderNamedFlowThreadList->end(); ++iter) {
        RenderNamedFlowThread* flowRenderer = *iter;
        flowRenderer->layoutIfNeeded();
    }
}

void FlowThreadController::registerNamedFlowContentNode(Node* contentNode, RenderNamedFlowThread* namedFlow)
{
    ASSERT(contentNode && contentNode->isElementNode());
    ASSERT(namedFlow);
    ASSERT(!m_mapNamedFlowContentNodes.contains(contentNode));
    ASSERT(!namedFlow->hasContentNode(contentNode));
    m_mapNamedFlowContentNodes.add(contentNode, namedFlow);
    namedFlow->registerNamedFlowContentNode(contentNode);
}

void FlowThreadController::unregisterNamedFlowContentNode(Node* contentNode)
{
    ASSERT(contentNode && contentNode->isElementNode());
    HashMap<const Node*, RenderNamedFlowThread*>::iterator it = m_mapNamedFlowContentNodes.find(contentNode);
    ASSERT(it != m_mapNamedFlowContentNodes.end());
    ASSERT(it->value);
    ASSERT(it->value->hasContentNode(contentNode));
    it->value->unregisterNamedFlowContentNode(contentNode);
    m_mapNamedFlowContentNodes.remove(contentNode);
}

void FlowThreadController::updateFlowThreadsChainIfNecessary()
{
    ASSERT(m_renderNamedFlowThreadList);
    ASSERT(isAutoLogicalHeightRegionsCountConsistent());

    // Remove the left-over flow threads.
    RenderNamedFlowThreadList toRemoveList;
    for (RenderNamedFlowThreadList::iterator iter = m_renderNamedFlowThreadList->begin(); iter != m_renderNamedFlowThreadList->end(); ++iter) {
        RenderNamedFlowThread* flowRenderer = *iter;
        if (flowRenderer->isMarkedForDestruction())
            toRemoveList.add(flowRenderer);
    }

    if (toRemoveList.size() > 0)
        setIsRenderNamedFlowThreadOrderDirty(true);

    for (RenderNamedFlowThreadList::iterator iter = toRemoveList.begin(); iter != toRemoveList.end(); ++iter) {
        RenderNamedFlowThread* flowRenderer = *iter;
        m_renderNamedFlowThreadList->remove(flowRenderer);
        flowRenderer->destroy();
    }

    if (isRenderNamedFlowThreadOrderDirty()) {
        // Arrange the thread list according to dependencies.
        RenderNamedFlowThreadList sortedList;
        for (RenderNamedFlowThreadList::iterator iter = m_renderNamedFlowThreadList->begin(); iter != m_renderNamedFlowThreadList->end(); ++iter) {
            RenderNamedFlowThread* flowRenderer = *iter;
            if (sortedList.contains(flowRenderer))
                continue;
            flowRenderer->pushDependencies(sortedList);
            sortedList.add(flowRenderer);
        }
        m_renderNamedFlowThreadList->swap(sortedList);
        setIsRenderNamedFlowThreadOrderDirty(false);
    }
}

bool FlowThreadController::updateFlowThreadsNeedingLayout()
{
    bool needsTwoPassLayout = false;

    for (RenderNamedFlowThreadList::iterator iter = m_renderNamedFlowThreadList->begin(); iter != m_renderNamedFlowThreadList->end(); ++iter) {
        RenderNamedFlowThread* flowRenderer = *iter;
        ASSERT(!flowRenderer->needsTwoPhasesLayout());
        flowRenderer->setInConstrainedLayoutPhase(false);
        if (flowRenderer->needsLayout() && flowRenderer->hasAutoLogicalHeightRegions())
            needsTwoPassLayout = true;
    }

    if (needsTwoPassLayout)
        resetFlowThreadsWithAutoHeightRegions();

    return needsTwoPassLayout;
}

bool FlowThreadController::updateFlowThreadsNeedingTwoStepLayout()
{
    bool needsTwoPassLayout = false;

    for (RenderNamedFlowThreadList::iterator iter = m_renderNamedFlowThreadList->begin(); iter != m_renderNamedFlowThreadList->end(); ++iter) {
        RenderNamedFlowThread* flowRenderer = *iter;
        if (flowRenderer->needsTwoPhasesLayout()) {
            needsTwoPassLayout = true;
            break;
        }
    }

    if (needsTwoPassLayout)
        resetFlowThreadsWithAutoHeightRegions();

    return needsTwoPassLayout;
}

void FlowThreadController::resetFlowThreadsWithAutoHeightRegions()
{
    for (RenderNamedFlowThreadList::iterator iter = m_renderNamedFlowThreadList->begin(); iter != m_renderNamedFlowThreadList->end(); ++iter) {
        RenderNamedFlowThread* flowRenderer = *iter;
        if (flowRenderer->hasAutoLogicalHeightRegions()) {
            flowRenderer->markAutoLogicalHeightRegionsForLayout();
            flowRenderer->invalidateRegions();
        }
    }
}

void FlowThreadController::updateFlowThreadsIntoConstrainedPhase()
{
    // Walk the flow chain in reverse order to update the auto-height regions and compute correct sizes for the containing regions. Only after this we can
    // set the flow in the constrained layout phase.
    for (RenderNamedFlowThreadList::reverse_iterator iter = m_renderNamedFlowThreadList->rbegin(); iter != m_renderNamedFlowThreadList->rend(); ++iter) {
        RenderNamedFlowThread* flowRenderer = *iter;
        ASSERT(!flowRenderer->hasRegions() || flowRenderer->hasValidRegionInfo());
        flowRenderer->layoutIfNeeded();
        if (flowRenderer->hasAutoLogicalHeightRegions()) {
            ASSERT(flowRenderer->needsTwoPhasesLayout());
            flowRenderer->markAutoLogicalHeightRegionsForLayout();
        }
        flowRenderer->setInConstrainedLayoutPhase(true);
        flowRenderer->clearNeedsTwoPhasesLayout();
    }
}

bool FlowThreadController::isContentNodeRegisteredWithAnyNamedFlow(const Node* contentNode) const
{
    return m_mapNamedFlowContentNodes.contains(contentNode);
}

#ifndef NDEBUG
bool FlowThreadController::isAutoLogicalHeightRegionsCountConsistent() const
{
    if (!hasRenderNamedFlowThreads())
        return !hasFlowThreadsWithAutoLogicalHeightRegions();

    for (RenderNamedFlowThreadList::iterator iter = m_renderNamedFlowThreadList->begin(); iter != m_renderNamedFlowThreadList->end(); ++iter) {
        if (!(*iter)->isAutoLogicalHeightRegionsCountConsistent())
            return false;
    }

    return true;
}
#endif

} // namespace WebCore
