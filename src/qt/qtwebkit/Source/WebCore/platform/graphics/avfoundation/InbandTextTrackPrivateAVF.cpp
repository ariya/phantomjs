/*
 * Copyright (C) 2012, 2013 Apple Inc. All rights reserved.
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

#include "config.h"

#if ENABLE(VIDEO) && (USE(AVFOUNDATION) || PLATFORM(IOS))

#include "InbandTextTrackPrivateAVF.h"

#include "InbandTextTrackPrivateClient.h"
#include "Logging.h"
#include "SoftLinking.h"
#include <CoreMedia/CoreMedia.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>
#include <wtf/unicode/CharacterNames.h>

#if !PLATFORM(WIN)
#define SOFT_LINK_AVF_FRAMEWORK(Lib) SOFT_LINK_FRAMEWORK_OPTIONAL(Lib)
#define SOFT_LINK_AVF_POINTER(Lib, Name, Type) SOFT_LINK_POINTER_OPTIONAL(Lib, Name, Type)
#else
#define SOFT_LINK_AVF_FRAMEWORK(Lib) SOFT_LINK_LIBRARY(Lib)
#define SOFT_LINK_AVF_POINTER(Lib, Name, Type) SOFT_LINK_VARIABLE_DLL_IMPORT_OPTIONAL(Lib, Name, Type)
#endif

SOFT_LINK_AVF_FRAMEWORK(CoreMedia)

SOFT_LINK_AVF_POINTER(CoreMedia, kCMTextMarkupAttribute_Alignment, CFStringRef)
SOFT_LINK_AVF_POINTER(CoreMedia, kCMTextMarkupAlignmentType_Start, CFStringRef)
SOFT_LINK_AVF_POINTER(CoreMedia, kCMTextMarkupAlignmentType_Middle, CFStringRef)
SOFT_LINK_AVF_POINTER(CoreMedia, kCMTextMarkupAlignmentType_End, CFStringRef)
SOFT_LINK_AVF_POINTER(CoreMedia, kCMTextMarkupAttribute_BoldStyle, CFStringRef)
SOFT_LINK_AVF_POINTER(CoreMedia, kCMTextMarkupAttribute_ItalicStyle, CFStringRef)
SOFT_LINK_AVF_POINTER(CoreMedia, kCMTextMarkupAttribute_UnderlineStyle, CFStringRef)
SOFT_LINK_AVF_POINTER(CoreMedia, kCMTextMarkupAttribute_TextPositionPercentageRelativeToWritingDirection, CFStringRef)
SOFT_LINK_AVF_POINTER(CoreMedia, kCMTextMarkupAttribute_WritingDirectionSizePercentage, CFStringRef)
SOFT_LINK_AVF_POINTER(CoreMedia, kCMTextMarkupAttribute_OrthogonalLinePositionPercentageRelativeToWritingDirection, CFStringRef)
SOFT_LINK_AVF_POINTER(CoreMedia, kCMTextMarkupAttribute_VerticalLayout, CFStringRef)
SOFT_LINK_AVF_POINTER(CoreMedia, kCMTextVerticalLayout_LeftToRight, CFStringRef)
SOFT_LINK_AVF_POINTER(CoreMedia, kCMTextVerticalLayout_RightToLeft, CFStringRef)
SOFT_LINK_AVF_POINTER(CoreMedia, kCMTextMarkupAttribute_BaseFontSizePercentageRelativeToVideoHeight, CFStringRef)
SOFT_LINK_AVF_POINTER(CoreMedia, kCMTextMarkupAttribute_RelativeFontSize, CFStringRef)
SOFT_LINK_AVF_POINTER(CoreMedia, kCMTextMarkupAttribute_FontFamilyName, CFStringRef)
SOFT_LINK_AVF_POINTER(CoreMedia, kCMTextMarkupAttribute_ForegroundColorARGB, CFStringRef)
SOFT_LINK_AVF_POINTER(CoreMedia, kCMTextMarkupAttribute_BackgroundColorARGB, CFStringRef)
SOFT_LINK_AVF_POINTER(CoreMedia, kCMTextMarkupAttribute_CharacterBackgroundColorARGB, CFStringRef)

#define kCMTextMarkupAttribute_Alignment getkCMTextMarkupAttribute_Alignment()
#define kCMTextMarkupAlignmentType_Start getkCMTextMarkupAlignmentType_Start()
#define kCMTextMarkupAlignmentType_Middle getkCMTextMarkupAlignmentType_Middle()
#define kCMTextMarkupAlignmentType_End getkCMTextMarkupAlignmentType_End()
#define kCMTextMarkupAttribute_BoldStyle getkCMTextMarkupAttribute_BoldStyle()
#define kCMTextMarkupAttribute_ItalicStyle getkCMTextMarkupAttribute_ItalicStyle()
#define kCMTextMarkupAttribute_UnderlineStyle getkCMTextMarkupAttribute_UnderlineStyle()
#define kCMTextMarkupAttribute_TextPositionPercentageRelativeToWritingDirection getkCMTextMarkupAttribute_TextPositionPercentageRelativeToWritingDirection()
#define kCMTextMarkupAttribute_WritingDirectionSizePercentage getkCMTextMarkupAttribute_WritingDirectionSizePercentage()
#define kCMTextMarkupAttribute_OrthogonalLinePositionPercentageRelativeToWritingDirection getkCMTextMarkupAttribute_OrthogonalLinePositionPercentageRelativeToWritingDirection()
#define kCMTextMarkupAttribute_VerticalLayout getkCMTextMarkupAttribute_VerticalLayout()
#define kCMTextVerticalLayout_LeftToRight getkCMTextVerticalLayout_LeftToRight()
#define kCMTextVerticalLayout_RightToLeft getkCMTextVerticalLayout_RightToLeft()
#define kCMTextMarkupAttribute_BaseFontSizePercentageRelativeToVideoHeight getkCMTextMarkupAttribute_BaseFontSizePercentageRelativeToVideoHeight()
#define kCMTextMarkupAttribute_RelativeFontSize getkCMTextMarkupAttribute_RelativeFontSize()
#define kCMTextMarkupAttribute_FontFamilyName getkCMTextMarkupAttribute_FontFamilyName()
#define kCMTextMarkupAttribute_ForegroundColorARGB getkCMTextMarkupAttribute_ForegroundColorARGB()
#define kCMTextMarkupAttribute_BackgroundColorARGB getkCMTextMarkupAttribute_BackgroundColorARGB()
#define kCMTextMarkupAttribute_CharacterBackgroundColorARGB getkCMTextMarkupAttribute_CharacterBackgroundColorARGB()

using namespace std;

namespace WebCore {

AVFInbandTrackParent::~AVFInbandTrackParent()
{
}

InbandTextTrackPrivateAVF::InbandTextTrackPrivateAVF(AVFInbandTrackParent* owner)
    : m_owner(owner)
    , m_pendingCueStatus(None)
    , m_index(0)
    , m_hasBeenReported(false)
    , m_seeking(false)
{
}

InbandTextTrackPrivateAVF::~InbandTextTrackPrivateAVF()
{
    disconnect();
}

static bool makeRGBA32FromARGBCFArray(CFArrayRef colorArray, RGBA32& color)
{
    if (CFArrayGetCount(colorArray) < 4)
        return false;

    float componentArray[4];
    for (int i = 0; i < 4; i++) {
        CFNumberRef value = static_cast<CFNumberRef>(CFArrayGetValueAtIndex(colorArray, i));
        if (CFGetTypeID(value) != CFNumberGetTypeID())
            return false;

        float component;
        CFNumberGetValue(value, kCFNumberFloatType, &component);
        componentArray[i] = component;
    }

    color = makeRGBA32FromFloats(componentArray[1] * 255, componentArray[2] * 255, componentArray[3] * 255, componentArray[0] * 255);
    return true;
}

void InbandTextTrackPrivateAVF::processCueAttributes(CFAttributedStringRef attributedString, GenericCueData* cueData)
{
    // Some of the attributes we translate into per-cue WebVTT settings are are repeated on each part of an attributed string so only
    // process the first instance of each.
    enum AttributeFlags {
        Line = 1 << 0,
        Position = 1 << 1,
        Size = 1 << 2,
        Vertical = 1 << 3,
        Align = 1 << 4,
        FontName = 1 << 5
    };
    unsigned processed = 0;

    StringBuilder content;
    String attributedStringValue = CFAttributedStringGetString(attributedString);
    CFIndex length = attributedStringValue.length();
    if (!length)
        return;

    CFRange effectiveRange = CFRangeMake(0, 0);
    while ((effectiveRange.location + effectiveRange.length) < length) {

        CFDictionaryRef attributes = CFAttributedStringGetAttributes(attributedString, effectiveRange.location + effectiveRange.length, &effectiveRange);
        if (!attributes)
            continue;

        StringBuilder tagStart;
        CFStringRef valueString;
        String tagEnd;
        CFIndex attributeCount = CFDictionaryGetCount(attributes);
        Vector<const void*> keys(attributeCount);
        Vector<const void*> values(attributeCount);
        CFDictionaryGetKeysAndValues(attributes, keys.data(), values.data());

        for (CFIndex i = 0; i < attributeCount; ++i) {
            CFStringRef key = static_cast<CFStringRef>(keys[i]);
            CFTypeRef value = values[i];
            if (CFGetTypeID(key) != CFStringGetTypeID() || !CFStringGetLength(key))
                continue;

            if (CFStringCompare(key, kCMTextMarkupAttribute_Alignment, 0) == kCFCompareEqualTo) {
                valueString = static_cast<CFStringRef>(value);
                if (CFGetTypeID(valueString) != CFStringGetTypeID() || !CFStringGetLength(valueString))
                    continue;
                if (processed & Align)
                    continue;
                processed |= Align;

                if (CFStringCompare(valueString, kCMTextMarkupAlignmentType_Start, 0) == kCFCompareEqualTo)
                    cueData->setAlign(GenericCueData::Start);
                else if (CFStringCompare(valueString, kCMTextMarkupAlignmentType_Middle, 0) == kCFCompareEqualTo)
                    cueData->setAlign(GenericCueData::Middle);
                else if (CFStringCompare(valueString, kCMTextMarkupAlignmentType_End, 0) == kCFCompareEqualTo)
                    cueData->setAlign(GenericCueData::End);
                else
                    ASSERT_NOT_REACHED();

                continue;
            }

            if (CFStringCompare(key, kCMTextMarkupAttribute_BoldStyle, 0) == kCFCompareEqualTo) {
                if (static_cast<CFBooleanRef>(value) != kCFBooleanTrue)
                    continue;

                tagStart.append("<b>");
                tagEnd.insert("</b>", 0);
                continue;
            }

            if (CFStringCompare(key, kCMTextMarkupAttribute_ItalicStyle, 0) == kCFCompareEqualTo) {
                if (static_cast<CFBooleanRef>(value) != kCFBooleanTrue)
                    continue;

                tagStart.append("<i>");
                tagEnd.insert("</i>", 0);
                continue;
            }

            if (CFStringCompare(key, kCMTextMarkupAttribute_UnderlineStyle, 0) == kCFCompareEqualTo) {
                if (static_cast<CFBooleanRef>(value) != kCFBooleanTrue)
                    continue;

                tagStart.append("<u>");
                tagEnd.insert("</u>", 0);
                continue;
            }

            if (CFStringCompare(key, kCMTextMarkupAttribute_OrthogonalLinePositionPercentageRelativeToWritingDirection, 0) == kCFCompareEqualTo) {
                if (CFGetTypeID(value) != CFNumberGetTypeID())
                    continue;
                if (processed & Line)
                    continue;
                processed |= Line;

                CFNumberRef valueNumber = static_cast<CFNumberRef>(value);
                double line;
                CFNumberGetValue(valueNumber, kCFNumberFloat64Type, &line);
                cueData->setLine(line);
                continue;
            }

            if (CFStringCompare(key, kCMTextMarkupAttribute_TextPositionPercentageRelativeToWritingDirection, 0) == kCFCompareEqualTo) {
                if (CFGetTypeID(value) != CFNumberGetTypeID())
                    continue;
                if (processed & Position)
                    continue;
                processed |= Position;

                CFNumberRef valueNumber = static_cast<CFNumberRef>(value);
                double position;
                CFNumberGetValue(valueNumber, kCFNumberFloat64Type, &position);
                cueData->setPosition(position);
                continue;
            }

            if (CFStringCompare(key, kCMTextMarkupAttribute_WritingDirectionSizePercentage, 0) == kCFCompareEqualTo) {
                if (CFGetTypeID(value) != CFNumberGetTypeID())
                    continue;
                if (processed & Size)
                    continue;
                processed |= Size;

                CFNumberRef valueNumber = static_cast<CFNumberRef>(value);
                double size;
                CFNumberGetValue(valueNumber, kCFNumberFloat64Type, &size);
                cueData->setSize(size);
                continue;
            }

            if (CFStringCompare(key, kCMTextMarkupAttribute_VerticalLayout, 0) == kCFCompareEqualTo) {
                valueString = static_cast<CFStringRef>(value);
                if (CFGetTypeID(valueString) != CFStringGetTypeID() || !CFStringGetLength(valueString))
                    continue;
                
                if (CFStringCompare(valueString, kCMTextVerticalLayout_LeftToRight, 0) == kCFCompareEqualTo)
                    tagStart.append(leftToRightMark);
                else if (CFStringCompare(valueString, kCMTextVerticalLayout_RightToLeft, 0) == kCFCompareEqualTo)
                    tagStart.append(rightToLeftMark);
                continue;
            }

            if (CFStringCompare(key, kCMTextMarkupAttribute_BaseFontSizePercentageRelativeToVideoHeight, 0) == kCFCompareEqualTo) {
                if (CFGetTypeID(value) != CFNumberGetTypeID())
                    continue;
                
                CFNumberRef valueNumber = static_cast<CFNumberRef>(value);
                double baseFontSize;
                CFNumberGetValue(valueNumber, kCFNumberFloat64Type, &baseFontSize);
                cueData->setBaseFontSize(baseFontSize);
                continue;
            }

            if (CFStringCompare(key, kCMTextMarkupAttribute_RelativeFontSize, 0) == kCFCompareEqualTo) {
                if (CFGetTypeID(value) != CFNumberGetTypeID())
                    continue;
                
                CFNumberRef valueNumber = static_cast<CFNumberRef>(value);
                double relativeFontSize;
                CFNumberGetValue(valueNumber, kCFNumberFloat64Type, &relativeFontSize);
                cueData->setRelativeFontSize(relativeFontSize);
                continue;
            }

            if (CFStringCompare(key, kCMTextMarkupAttribute_FontFamilyName, 0) == kCFCompareEqualTo) {
                valueString = static_cast<CFStringRef>(value);
                if (CFGetTypeID(valueString) != CFStringGetTypeID() || !CFStringGetLength(valueString))
                    continue;
                if (processed & FontName)
                    continue;
                processed |= FontName;
                
                cueData->setFontName(valueString);
                continue;
            }

            if (CFStringCompare(key, kCMTextMarkupAttribute_ForegroundColorARGB, 0) == kCFCompareEqualTo) {
                CFArrayRef arrayValue = static_cast<CFArrayRef>(value);
                if (CFGetTypeID(arrayValue) != CFArrayGetTypeID())
                    continue;
                
                RGBA32 color;
                if (!makeRGBA32FromARGBCFArray(arrayValue, color))
                    continue;
                cueData->setForegroundColor(color);
            }
            
            if (CFStringCompare(key, kCMTextMarkupAttribute_BackgroundColorARGB, 0) == kCFCompareEqualTo) {
                CFArrayRef arrayValue = static_cast<CFArrayRef>(value);
                if (CFGetTypeID(arrayValue) != CFArrayGetTypeID())
                    continue;
                
                RGBA32 color;
                if (!makeRGBA32FromARGBCFArray(arrayValue, color))
                    continue;
                cueData->setBackgroundColor(color);
            }

            if (CFStringCompare(key, kCMTextMarkupAttribute_CharacterBackgroundColorARGB, 0) == kCFCompareEqualTo) {
                CFArrayRef arrayValue = static_cast<CFArrayRef>(value);
                if (CFGetTypeID(arrayValue) != CFArrayGetTypeID())
                    continue;
                
                RGBA32 color;
                if (!makeRGBA32FromARGBCFArray(arrayValue, color))
                    continue;
                cueData->setHighlightColor(color);
            }
        }

        content.append(tagStart);
        content.append(attributedStringValue.substring(effectiveRange.location, effectiveRange.length));
        content.append(tagEnd);
    }

    if (content.length())
        cueData->setContent(content.toString());
}

void InbandTextTrackPrivateAVF::processCue(CFArrayRef attributedStrings, double time)
{
    if (!client())
        return;

    LOG(Media, "InbandTextTrackPrivateAVF::processCue - %li cues at time %.2f\n", attributedStrings ? CFArrayGetCount(attributedStrings) : 0, time);

    if (m_pendingCueStatus != None) {
        // Cues do not have an explicit duration, they are displayed until the next "cue" (which might be empty) is emitted.
        m_currentCueEndTime = time;

        if (m_currentCueEndTime >= m_currentCueStartTime) {
            for (size_t i = 0; i < m_cues.size(); i++) {
                GenericCueData* cueData = m_cues[i].get();

                if (m_pendingCueStatus == Valid) {
                    cueData->setEndTime(m_currentCueEndTime);
                    cueData->setStatus(GenericCueData::Complete);

                    LOG(Media, "InbandTextTrackPrivateAVF::processCue(%p) - updating cue: start=%.2f, end=%.2f, content=\"%s\"", this, cueData->startTime(), m_currentCueEndTime, cueData->content().utf8().data());
                    client()->updateGenericCue(this, cueData);
                } else {
                    // We have to assume that the implicit duration is invalid for cues delivered during a seek because the AVF decode pipeline may not
                    // see every cue, so DO NOT update cue duration while seeking.
                    LOG(Media, "InbandTextTrackPrivateAVF::processCue(%p) - ignoring cue delivered during seek: start=%.2f, end=%.2f, content=\"%s\"", this, cueData->startTime(), m_currentCueEndTime, cueData->content().utf8().data());
                }
            }
        } else
            LOG(Media, "InbandTextTrackPrivateAVF::processCue negative length cue(s) ignored: start=%.2f, end=%.2f\n", m_currentCueStartTime, m_currentCueEndTime);

        resetCueValues();
    }

    if (!attributedStrings)
        return;

    CFIndex count = CFArrayGetCount(attributedStrings);
    if (!count)
        return;

    for (CFIndex i = 0; i < count; i++) {
        CFAttributedStringRef attributedString = static_cast<CFAttributedStringRef>(CFArrayGetValueAtIndex(attributedStrings, i));

        if (!attributedString || !CFAttributedStringGetLength(attributedString))
            continue;

        RefPtr<GenericCueData> cueData = GenericCueData::create();
        processCueAttributes(attributedString, cueData.get());
        if (!cueData->content().length())
            continue;

        m_cues.append(cueData);

        m_currentCueStartTime = time;
        cueData->setStartTime(m_currentCueStartTime);
        cueData->setEndTime(numeric_limits<double>::infinity());
        
        // AVFoundation cue "position" is to the center of the text so adjust relative to the edge because we will use it to
        // set CSS "left".
        if (cueData->position() >= 0 && cueData->size() > 0)
            cueData->setPosition(cueData->position() - cueData->size() / 2);
        
        LOG(Media, "InbandTextTrackPrivateAVF::processCue(%p) - adding cue for time = %.2f, position =  %.2f, line =  %.2f", this, cueData->startTime(), cueData->position(), cueData->line());

        cueData->setStatus(GenericCueData::Partial);
        client()->addGenericCue(this, cueData.release());

        m_pendingCueStatus = seeking() ? DeliveredDuringSeek : Valid;
    }
}

void InbandTextTrackPrivateAVF::beginSeeking()
{
    // Forget any partially accumulated cue data as the seek could be to a time outside of the cue's
    // range, which will mean that the next cue delivered will result in the current cue getting the
    // incorrect duration.
    resetCueValues();
    m_seeking = true;
}

void InbandTextTrackPrivateAVF::disconnect()
{
    m_owner = 0;
    m_index = 0;
}

void InbandTextTrackPrivateAVF::resetCueValues()
{
    if (m_currentCueEndTime && m_cues.size())
        LOG(Media, "InbandTextTrackPrivateAVF::resetCueValues flushing data for cues: start=%.2f\n", m_currentCueStartTime);

    if (client()) {
        for (size_t i = 0; i < m_cues.size(); i++)
            client()->removeGenericCue(this, m_cues[i].get());
    }

    m_cues.resize(0);
    m_pendingCueStatus = None;
    m_currentCueStartTime = 0;
    m_currentCueEndTime = 0;
}

void InbandTextTrackPrivateAVF::setMode(InbandTextTrackPrivate::Mode newMode)
{
    if (!m_owner)
        return;

    InbandTextTrackPrivate::Mode oldMode = mode();
    InbandTextTrackPrivate::setMode(newMode);

    if (oldMode == newMode)
        return;

    m_owner->trackModeChanged();
}

} // namespace WebCore

#endif // ENABLE(VIDEO) && (USE(AVFOUNDATION) || PLATFORM(IOS))
