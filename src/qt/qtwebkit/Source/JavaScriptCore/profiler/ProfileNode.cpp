/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ProfileNode.h"

#include "LegacyProfiler.h"
#include <stdio.h>
#include <wtf/DateMath.h>
#include <wtf/DataLog.h>
#include <wtf/text/StringHash.h>

#if OS(WINDOWS)
#include <windows.h>
#endif

using namespace WTF;

namespace JSC {

static double getCount()
{
#if OS(WINDOWS)
    static LARGE_INTEGER frequency;
    if (!frequency.QuadPart)
        QueryPerformanceFrequency(&frequency);
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return static_cast<double>(counter.QuadPart) / frequency.QuadPart;
#else
    return currentTimeMS();
#endif
}

ProfileNode::ProfileNode(ExecState* callerCallFrame, const CallIdentifier& callIdentifier, ProfileNode* headNode, ProfileNode* parentNode)
    : m_callerCallFrame(callerCallFrame)
    , m_callIdentifier(callIdentifier)
    , m_head(headNode)
    , m_parent(parentNode)
    , m_nextSibling(0)
    , m_startTime(0.0)
    , m_actualTotalTime(0.0)
    , m_visibleTotalTime(0.0)
    , m_actualSelfTime(0.0)
    , m_visibleSelfTime(0.0)
    , m_numberOfCalls(0)
    , m_visible(true)
{
    startTimer();
}

ProfileNode::ProfileNode(ExecState* callerCallFrame, ProfileNode* headNode, ProfileNode* nodeToCopy)
    : m_callerCallFrame(callerCallFrame)
    , m_callIdentifier(nodeToCopy->callIdentifier())
    , m_head(headNode)
    , m_parent(nodeToCopy->parent())
    , m_nextSibling(0)
    , m_startTime(0.0)
    , m_actualTotalTime(nodeToCopy->actualTotalTime())
    , m_visibleTotalTime(nodeToCopy->totalTime())
    , m_actualSelfTime(nodeToCopy->actualSelfTime())
    , m_visibleSelfTime(nodeToCopy->selfTime())
    , m_numberOfCalls(nodeToCopy->numberOfCalls())
    , m_visible(nodeToCopy->visible())
{
}

ProfileNode* ProfileNode::willExecute(ExecState* callerCallFrame, const CallIdentifier& callIdentifier)
{
    for (StackIterator currentChild = m_children.begin(); currentChild != m_children.end(); ++currentChild) {
        if ((*currentChild)->callIdentifier() == callIdentifier) {
            (*currentChild)->startTimer();
            return (*currentChild).get();
        }
    }

    RefPtr<ProfileNode> newChild = ProfileNode::create(callerCallFrame, callIdentifier, m_head ? m_head : this, this); // If this ProfileNode has no head it is the head.
    if (m_children.size())
        m_children.last()->setNextSibling(newChild.get());
    m_children.append(newChild.release());
    return m_children.last().get();
}

ProfileNode* ProfileNode::didExecute()
{
    endAndRecordCall();
    return m_parent;
}

void ProfileNode::addChild(PassRefPtr<ProfileNode> prpChild)
{
    RefPtr<ProfileNode> child = prpChild;
    child->setParent(this);
    if (m_children.size())
        m_children.last()->setNextSibling(child.get());
    m_children.append(child.release());
}

ProfileNode* ProfileNode::findChild(ProfileNode* node) const
{
    if (!node)
        return 0;

    for (size_t i = 0; i < m_children.size(); ++i) {
        if (*node == m_children[i].get())
            return m_children[i].get();
    }

    return 0;
}

void ProfileNode::removeChild(ProfileNode* node)
{
    if (!node)
        return;

    for (size_t i = 0; i < m_children.size(); ++i) {
        if (*node == m_children[i].get()) {
            m_children.remove(i);
            break;
        }
    }
    
    resetChildrensSiblings();
}

void ProfileNode::insertNode(PassRefPtr<ProfileNode> prpNode)
{
    RefPtr<ProfileNode> node = prpNode;

    for (unsigned i = 0; i < m_children.size(); ++i)
        node->addChild(m_children[i].release());

    m_children.clear();
    m_children.append(node.release());
}

void ProfileNode::stopProfiling()
{
    if (m_startTime)
        endAndRecordCall();

    m_visibleTotalTime = m_actualTotalTime;

    ASSERT(m_actualSelfTime == 0.0 && m_startTime == 0.0);

    // Because we iterate in post order all of our children have been stopped before us.
    for (unsigned i = 0; i < m_children.size(); ++i)
        m_actualSelfTime += m_children[i]->totalTime();

    ASSERT(m_actualSelfTime <= m_actualTotalTime);
    m_actualSelfTime = m_actualTotalTime - m_actualSelfTime;
    m_visibleSelfTime = m_actualSelfTime;
}

ProfileNode* ProfileNode::traverseNextNodePostOrder() const
{
    ProfileNode* next = m_nextSibling;
    if (!next)
        return m_parent;
    while (ProfileNode* firstChild = next->firstChild())
        next = firstChild;
    return next;
}

ProfileNode* ProfileNode::traverseNextNodePreOrder(bool processChildren) const
{
    if (processChildren && m_children.size())
        return m_children[0].get();

    if (m_nextSibling)
        return m_nextSibling;

    ProfileNode* nextParent = m_parent;
    if (!nextParent)
        return 0;

    ProfileNode* next;
    for (next = m_parent->nextSibling(); !next; next = nextParent->nextSibling()) {
        nextParent = nextParent->parent();
        if (!nextParent)
            return 0;
    }

    return next;
}

void ProfileNode::setTreeVisible(ProfileNode* node, bool visible)
{
    ProfileNode* nodeParent = node->parent();
    ProfileNode* nodeSibling = node->nextSibling();
    node->setParent(0);
    node->setNextSibling(0);

    for (ProfileNode* currentNode = node; currentNode; currentNode = currentNode->traverseNextNodePreOrder())
        currentNode->setVisible(visible);

    node->setParent(nodeParent);
    node->setNextSibling(nodeSibling);
}

void ProfileNode::calculateVisibleTotalTime()
{
    double sumOfVisibleChildrensTime = 0.0;

    for (unsigned i = 0; i < m_children.size(); ++i) {
        if (m_children[i]->visible())
            sumOfVisibleChildrensTime += m_children[i]->totalTime();
    }

    m_visibleTotalTime = m_visibleSelfTime + sumOfVisibleChildrensTime;
}

bool ProfileNode::focus(const CallIdentifier& callIdentifier)
{
    if (!m_visible)
        return false;

    if (m_callIdentifier != callIdentifier) {
        m_visible = false;
        return true;
    }

    for (ProfileNode* currentParent = m_parent; currentParent; currentParent = currentParent->parent())
        currentParent->setVisible(true);

    return false;
}

void ProfileNode::exclude(const CallIdentifier& callIdentifier)
{
    if (m_visible && m_callIdentifier == callIdentifier) {
        setTreeVisible(this, false);

        m_parent->setVisibleSelfTime(m_parent->selfTime() + m_visibleTotalTime);
    }
}

void ProfileNode::restore()
{
    m_visibleTotalTime = m_actualTotalTime;
    m_visibleSelfTime = m_actualSelfTime;
    m_visible = true;
}

void ProfileNode::endAndRecordCall()
{
    m_actualTotalTime += m_startTime ? getCount() - m_startTime : 0.0;
    m_startTime = 0.0;

    ++m_numberOfCalls;
}

void ProfileNode::startTimer()
{
    if (!m_startTime)
        m_startTime = getCount();
}

void ProfileNode::resetChildrensSiblings()
{
    unsigned size = m_children.size();
    for (unsigned i = 0; i < size; ++i)
        m_children[i]->setNextSibling(i + 1 == size ? 0 : m_children[i + 1].get());
}

#ifndef NDEBUG
void ProfileNode::debugPrintData(int indentLevel) const
{
    // Print function names
    for (int i = 0; i < indentLevel; ++i)
        dataLogF("  ");

    dataLogF("Function Name %s %d SelfTime %.3fms/%.3f%% TotalTime %.3fms/%.3f%% VSelf %.3fms VTotal %.3fms Visible %s Next Sibling %s\n",
        functionName().utf8().data(), 
        m_numberOfCalls, m_actualSelfTime, selfPercent(), m_actualTotalTime, totalPercent(),
        m_visibleSelfTime, m_visibleTotalTime, 
        (m_visible ? "True" : "False"),
        m_nextSibling ? m_nextSibling->functionName().utf8().data() : "");

    ++indentLevel;

    // Print children's names and information
    for (StackIterator currentChild = m_children.begin(); currentChild != m_children.end(); ++currentChild)
        (*currentChild)->debugPrintData(indentLevel);
}

// print the profiled data in a format that matches the tool sample's output.
double ProfileNode::debugPrintDataSampleStyle(int indentLevel, FunctionCallHashCount& countedFunctions) const
{
    dataLogF("    ");

    // Print function names
    const char* name = functionName().utf8().data();
    double sampleCount = m_actualTotalTime * 1000;
    if (indentLevel) {
        for (int i = 0; i < indentLevel; ++i)
            dataLogF("  ");

         countedFunctions.add(functionName().impl());

        dataLogF("%.0f %s\n", sampleCount ? sampleCount : 1, name);
    } else
        dataLogF("%s\n", name);

    ++indentLevel;

    // Print children's names and information
    double sumOfChildrensCount = 0.0;
    for (StackIterator currentChild = m_children.begin(); currentChild != m_children.end(); ++currentChild)
        sumOfChildrensCount += (*currentChild)->debugPrintDataSampleStyle(indentLevel, countedFunctions);

    sumOfChildrensCount *= 1000;    //
    // Print remainder of samples to match sample's output
    if (sumOfChildrensCount < sampleCount) {
        dataLogF("    ");
        while (indentLevel--)
            dataLogF("  ");

        dataLogF("%.0f %s\n", sampleCount - sumOfChildrensCount, functionName().utf8().data());
    }

    return m_actualTotalTime;
}
#endif

} // namespace JSC
