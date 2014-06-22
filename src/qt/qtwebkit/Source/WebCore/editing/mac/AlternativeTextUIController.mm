/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "AlternativeTextUIController.h"

#if USE(DICTATION_ALTERNATIVES)
#import <AppKit/NSSpellChecker.h>
#import <AppKit/NSTextAlternatives.h>
#import <AppKit/NSView.h>

namespace WebCore {

uint64_t AlternativeTextUIController::AlernativeTextContextController::addAlternatives(const RetainPtr<NSTextAlternatives>& alternatives)
{
    uint64_t context = reinterpret_cast<uint64_t>(alternatives.get());
    if (!context)
        return invalidContext;
    if (alternativesForContext(context))
        return context;

    HashMapType::AddResult result = m_alternativesObjectMap.add(context, alternatives);
    return result.isNewEntry ? context : invalidContext;
}

NSTextAlternatives* AlternativeTextUIController::AlernativeTextContextController::alternativesForContext(uint64_t context)
{
    HashMapType::const_iterator itr = m_alternativesObjectMap.find(context);
    if (itr == m_alternativesObjectMap.end())
        return NULL;
    return itr->value.get();
}

void AlternativeTextUIController::AlernativeTextContextController::removeAlternativesForContext(uint64_t context)
{
    m_alternativesObjectMap.remove(context);
}

void AlternativeTextUIController::AlernativeTextContextController::clear()
{
    m_alternativesObjectMap.clear();
}

uint64_t AlternativeTextUIController::addAlternatives(const RetainPtr<NSTextAlternatives>& alternatives)
{
    return m_contextController.addAlternatives(alternatives);
}

Vector<String> AlternativeTextUIController::alternativesForContext(uint64_t context)
{
    NSTextAlternatives* textAlternatives = m_contextController.alternativesForContext(context);
    Vector<String> alternativeStrings;
    for (NSString* string in textAlternatives.alternativeStrings)
        alternativeStrings.append(string);
    return alternativeStrings;
}

void AlternativeTextUIController::clear()
{
    return m_contextController.clear();
}

void AlternativeTextUIController::showAlternatives(NSView* view, const FloatRect& boundingBoxOfPrimaryString, uint64_t context, void(^acceptanceHandler)(NSString*))
{
    dismissAlternatives();
    if (!view)
        return;

    m_view = view;
    NSTextAlternatives* alternatives = m_contextController.alternativesForContext(context);
    if (!alternatives)
        return;

    [[NSSpellChecker sharedSpellChecker] showCorrectionIndicatorOfType:NSCorrectionIndicatorTypeGuesses primaryString:alternatives.primaryString alternativeStrings:alternatives.alternativeStrings forStringInRect:boundingBoxOfPrimaryString view:m_view.get() completionHandler:^(NSString* acceptedString) {
        if (acceptedString) {
            handleAcceptedAlternative(acceptedString, context, alternatives);
            acceptanceHandler(acceptedString);
        }
    }];
}

void AlternativeTextUIController::handleAcceptedAlternative(NSString* acceptedAlternative, uint64_t context, NSTextAlternatives* alternatives)
{
    [alternatives noteSelectedAlternativeString:acceptedAlternative];
    m_contextController.removeAlternativesForContext(context);
    m_view.clear();
}

void AlternativeTextUIController::dismissAlternatives()
{
    if (!m_view)
        return;
    [[NSSpellChecker sharedSpellChecker] dismissCorrectionIndicatorForView:m_view.get()];
}

void AlternativeTextUIController::removeAlternatives(uint64_t context)
{
    m_contextController.removeAlternativesForContext(context);
}
}
#endif // USE(DICTATION_ALTERNATIVES)
