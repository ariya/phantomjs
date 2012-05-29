/*
 * Copyright (C) 2004, 2006, 2010, 2011 Apple Computer, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#import "config.h"
#import "PlatformWheelEvent.h"

#import "PlatformMouseEvent.h"
#import "Scrollbar.h"
#import "WebCoreSystemInterface.h"
#import <wtf/UnusedParam.h>

namespace WebCore {

static PlatformWheelEventPhase momentumPhaseForEvent(NSEvent *event)
{
    uint32_t phase = PlatformWheelEventPhaseNone;

#if !defined(BUILDING_ON_LEOPARD) && !defined(BUILDING_ON_SNOW_LEOPARD)
    if ([event momentumPhase] & NSEventPhaseBegan)
        phase |= PlatformWheelEventPhaseBegan;
    if ([event momentumPhase] & NSEventPhaseStationary)
        phase |= PlatformWheelEventPhaseStationary;
    if ([event momentumPhase] & NSEventPhaseChanged)
        phase |= PlatformWheelEventPhaseChanged;
    if ([event momentumPhase] & NSEventPhaseEnded)
        phase |= PlatformWheelEventPhaseEnded;
    if ([event momentumPhase] & NSEventPhaseCancelled)
        phase |= PlatformWheelEventPhaseCancelled;
#else
    switch (wkGetNSEventMomentumPhase(event)) {
    case wkEventPhaseNone:
        phase = PlatformWheelEventPhaseNone;
        break;
    case wkEventPhaseBegan:
        phase = PlatformWheelEventPhaseBegan;
        break;
    case wkEventPhaseChanged:
        phase = PlatformWheelEventPhaseChanged;
        break;
    case wkEventPhaseEnded:
        phase = PlatformWheelEventPhaseEnded;
        break;
    }
#endif

    return static_cast<PlatformWheelEventPhase>(phase);
}

static PlatformWheelEventPhase phaseForEvent(NSEvent *event)
{
#if !defined(BUILDING_ON_LEOPARD) && !defined(BUILDING_ON_SNOW_LEOPARD)
    uint32_t phase = PlatformWheelEventPhaseNone; 
    if ([event phase] & NSEventPhaseBegan)
        phase |= PlatformWheelEventPhaseBegan;
    if ([event phase] & NSEventPhaseStationary)
        phase |= PlatformWheelEventPhaseStationary;
    if ([event phase] & NSEventPhaseChanged)
        phase |= PlatformWheelEventPhaseChanged;
    if ([event phase] & NSEventPhaseEnded)
        phase |= PlatformWheelEventPhaseEnded;
    if ([event phase] & NSEventPhaseCancelled)
        phase |= PlatformWheelEventPhaseCancelled;
    return static_cast<PlatformWheelEventPhase>(phase);
#else
    UNUSED_PARAM(event);
    return PlatformWheelEventPhaseNone;
#endif
}

PlatformWheelEvent::PlatformWheelEvent(NSEvent* event, NSView *windowView)
    : m_position(pointForEvent(event, windowView))
    , m_globalPosition(globalPointForEvent(event))
    , m_granularity(ScrollByPixelWheelEvent)
    , m_isAccepted(false)
    , m_shiftKey([event modifierFlags] & NSShiftKeyMask)
    , m_ctrlKey([event modifierFlags] & NSControlKeyMask)
    , m_altKey([event modifierFlags] & NSAlternateKeyMask)
    , m_metaKey([event modifierFlags] & NSCommandKeyMask)
    , m_phase(phaseForEvent(event))
    , m_momentumPhase(momentumPhaseForEvent(event))
    , m_timestamp([event timestamp])
{
    BOOL continuous;
    wkGetWheelEventDeltas(event, &m_deltaX, &m_deltaY, &continuous);
    if (continuous) {
        m_wheelTicksX = m_deltaX / static_cast<float>(Scrollbar::pixelsPerLineStep());
        m_wheelTicksY = m_deltaY / static_cast<float>(Scrollbar::pixelsPerLineStep());
        m_hasPreciseScrollingDeltas = true;
    } else {
        m_wheelTicksX = m_deltaX;
        m_wheelTicksY = m_deltaY;
        m_deltaX *= static_cast<float>(Scrollbar::pixelsPerLineStep());
        m_deltaY *= static_cast<float>(Scrollbar::pixelsPerLineStep());
        m_hasPreciseScrollingDeltas = false;
    }
}

} // namespace WebCore
