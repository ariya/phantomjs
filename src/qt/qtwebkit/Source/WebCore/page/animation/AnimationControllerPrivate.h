/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#ifndef AnimationControllerPrivate_h
#define AnimationControllerPrivate_h

#include "CSSPropertyNames.h"
#include "Timer.h"
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/AtomicString.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class AnimationBase;
class CompositeAnimation;
class Document;
class Element;
class Frame;
class Node;
class RenderObject;
class RenderStyle;

enum SetChanged {
    DoNotCallSetChanged = 0,
    CallSetChanged = 1
};

class AnimationControllerPrivate {
    WTF_MAKE_NONCOPYABLE(AnimationControllerPrivate); WTF_MAKE_FAST_ALLOCATED;
public:
    AnimationControllerPrivate(Frame*);
    ~AnimationControllerPrivate();

    // Returns the time until the next animation needs to be serviced, or -1 if there are none.
    double updateAnimations(SetChanged callSetChanged = DoNotCallSetChanged);
    void updateAnimationTimer(SetChanged callSetChanged = DoNotCallSetChanged);

    CompositeAnimation* ensureCompositeAnimation(RenderObject*);
    bool clear(RenderObject*);

    void updateStyleIfNeededDispatcherFired(Timer<AnimationControllerPrivate>*);
    void startUpdateStyleIfNeededDispatcher();
    void addEventToDispatch(PassRefPtr<Element> element, const AtomicString& eventType, const String& name, double elapsedTime);
    void addNodeChangeToDispatch(PassRefPtr<Node>);

    bool hasAnimations() const { return !m_compositeAnimations.isEmpty(); }

    bool isSuspended() const { return m_isSuspended; }
    void suspendAnimations();
    void resumeAnimations();
#if ENABLE(REQUEST_ANIMATION_FRAME)
    void animationFrameCallbackFired();
#endif

    void suspendAnimationsForDocument(Document*);
    void resumeAnimationsForDocument(Document*);
    void startAnimationsIfNotSuspended(Document*);

    bool isRunningAnimationOnRenderer(RenderObject*, CSSPropertyID, bool isRunningNow) const;
    bool isRunningAcceleratedAnimationOnRenderer(RenderObject*, CSSPropertyID, bool isRunningNow) const;

    bool pauseAnimationAtTime(RenderObject*, const AtomicString& name, double t);
    bool pauseTransitionAtTime(RenderObject*, const String& property, double t);
    unsigned numberOfActiveAnimations(Document*) const;

    PassRefPtr<RenderStyle> getAnimatedStyleForRenderer(RenderObject* renderer);

    double beginAnimationUpdateTime();
    void setBeginAnimationUpdateTime(double t) { m_beginAnimationUpdateTime = t; }
    void endAnimationUpdate();
    void receivedStartTimeResponse(double);
    
    void addToAnimationsWaitingForStyle(AnimationBase*);
    void removeFromAnimationsWaitingForStyle(AnimationBase*);

    void addToAnimationsWaitingForStartTimeResponse(AnimationBase*, bool willGetResponse);
    void removeFromAnimationsWaitingForStartTimeResponse(AnimationBase*);

    void animationWillBeRemoved(AnimationBase*);

    void updateAnimationTimerForRenderer(RenderObject*);
    
private:
    void animationTimerFired(Timer<AnimationControllerPrivate>*);

    void styleAvailable();
    void fireEventsAndUpdateStyle();
    void startTimeResponse(double t);

    typedef HashMap<RenderObject*, RefPtr<CompositeAnimation> > RenderObjectAnimationMap;

    RenderObjectAnimationMap m_compositeAnimations;
    Timer<AnimationControllerPrivate> m_animationTimer;
    Timer<AnimationControllerPrivate> m_updateStyleIfNeededDispatcher;
    Frame* m_frame;
    
    class EventToDispatch {
    public:
        RefPtr<Element> element;
        AtomicString eventType;
        String name;
        double elapsedTime;
    };
    
    Vector<EventToDispatch> m_eventsToDispatch;
    Vector<RefPtr<Node> > m_nodeChangesToDispatch;
    
    double m_beginAnimationUpdateTime;

    typedef HashSet<RefPtr<AnimationBase> > WaitingAnimationsSet;
    WaitingAnimationsSet m_animationsWaitingForStyle;
    WaitingAnimationsSet m_animationsWaitingForStartTimeResponse;
    bool m_waitingForAsyncStartNotification;
    bool m_isSuspended;
};

} // namespace WebCore

#endif // AnimationControllerPrivate_h
