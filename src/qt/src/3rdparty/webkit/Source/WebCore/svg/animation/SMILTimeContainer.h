/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef SMILTimeContainer_h
#define SMILTimeContainer_h

#if ENABLE(SVG)

#include "QualifiedName.h"
#include "PlatformString.h"
#include "SMILTime.h"
#include "Timer.h"
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/StringHash.h>

namespace WebCore {
    
class SVGElement;
class SVGSMILElement;
class SVGSVGElement;

class SMILTimeContainer : public RefCounted<SMILTimeContainer>  {
public:
    static PassRefPtr<SMILTimeContainer> create(SVGSVGElement* owner) { return adoptRef(new SMILTimeContainer(owner)); } 

    void schedule(SVGSMILElement*);
    void unschedule(SVGSMILElement*);
    
    SMILTime elapsed() const;

    bool isActive() const;
    bool isPaused() const;
    
    void begin();
    void pause();
    void resume();
    
    void setDocumentOrderIndexesDirty() { m_documentOrderIndexesDirty = true; }

    // Move to a specific time. Only used for DRT testing purposes.
    void sampleAnimationAtTime(const String& elementId, double seconds);

private:
    SMILTimeContainer(SVGSVGElement* owner);
    
    void timerFired(Timer<SMILTimeContainer>*);
    void startTimer(SMILTime fireTime, SMILTime minimumDelay = 0);
    void updateAnimations(SMILTime elapsed);
    
    void updateDocumentOrderIndexes();
    void sortByPriority(Vector<SVGSMILElement*>& smilElements, SMILTime elapsed);
    
    typedef pair<SVGElement*, QualifiedName> ElementAttributePair;
    String baseValueFor(ElementAttributePair);
    
    double m_beginTime;
    double m_pauseTime;
    double m_accumulatedPauseTime;
    double m_nextManualSampleTime;
    String m_nextSamplingTarget;

    bool m_documentOrderIndexesDirty;
    
    Timer<SMILTimeContainer> m_timer;

    typedef HashSet<SVGSMILElement*> TimingElementSet;
    TimingElementSet m_scheduledAnimations;
    
    typedef HashMap<ElementAttributePair, String> BaseValueMap;
    BaseValueMap m_savedBaseValues;

    SVGSVGElement* m_ownerSVGElement;
};
}

#endif // ENABLE(SVG)
#endif // SMILTimeContainer_h
