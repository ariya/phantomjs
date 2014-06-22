/*
 * Copyright (C) 2013 Google Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(VIDEO_TRACK) && ENABLE(WEBVTT_REGIONS)

#include "TextTrackRegion.h"

#include "ExceptionCodePlaceholder.h"
#include "Logging.h"
#include "WebVTTParser.h"
#include <wtf/MathExtras.h>
#include <wtf/text/StringBuilder.h>

namespace WebCore {

// The region occupies by default 100% of the width of the video viewport.
static const float defaultWidth = 100;

// The region has, by default, 3 lines of text.
static const long defaultHeightInLines = 3;

// The region and viewport are anchored in the bottom left corner.
static const float defaultAnchorPointX = 0;
static const float defaultAnchorPointY = 100;

// The region doesn't have scrolling text, by default.
static const bool defaultScroll = false;

TextTrackRegion::TextTrackRegion()
    : m_id(emptyString())
    , m_width(defaultWidth)
    , m_heightInLines(defaultHeightInLines)
    , m_regionAnchor(FloatPoint(defaultAnchorPointX, defaultAnchorPointY))
    , m_viewportAnchor(FloatPoint(defaultAnchorPointX, defaultAnchorPointY))
    , m_scroll(defaultScroll)
    , m_track(0)
{
}

TextTrackRegion::~TextTrackRegion()
{
}

void TextTrackRegion::setTrack(TextTrack* track)
{
    m_track = track;
}

void TextTrackRegion::setId(const String& id)
{
    m_id = id;
}

void TextTrackRegion::setWidth(double value, ExceptionCode& ec)
{
    if (std::isinf(value) || std::isnan(value)) {
        ec = TypeError;
        return;
    }

    if (value < 0 || value > 100) {
        ec = INDEX_SIZE_ERR;
        return;
    }

    m_width = value;
}

void TextTrackRegion::setHeight(long value, ExceptionCode& ec)
{
    if (value < 0) {
        ec = INDEX_SIZE_ERR;
        return;
    }

    m_heightInLines = value;
}

void TextTrackRegion::setRegionAnchorX(double value, ExceptionCode& ec)
{
    if (std::isinf(value) || std::isnan(value)) {
        ec = TypeError;
        return;
    }

    if (value < 0 || value > 100) {
        ec = INDEX_SIZE_ERR;
        return;
    }

    m_regionAnchor.setX(value);
}

void TextTrackRegion::setRegionAnchorY(double value, ExceptionCode& ec)
{
    if (std::isinf(value) || std::isnan(value)) {
        ec = TypeError;
        return;
    }

    if (value < 0 || value > 100) {
        ec = INDEX_SIZE_ERR;
        return;
    }

    m_regionAnchor.setY(value);
}

void TextTrackRegion::setViewportAnchorX(double value, ExceptionCode& ec)
{
    if (std::isinf(value) || std::isnan(value)) {
        ec = TypeError;
        return;
    }

    if (value < 0 || value > 100) {
        ec = INDEX_SIZE_ERR;
        return;
    }

    m_viewportAnchor.setX(value);
}

void TextTrackRegion::setViewportAnchorY(double value, ExceptionCode& ec)
{
    if (std::isinf(value) || std::isnan(value)) {
        ec = TypeError;
        return;
    }

    if (value < 0 || value > 100) {
        ec = INDEX_SIZE_ERR;
        return;
    }

    m_viewportAnchor.setY(value);
}

const AtomicString TextTrackRegion::scroll() const
{
    DEFINE_STATIC_LOCAL(const AtomicString, upScrollValueKeyword, ("up", AtomicString::ConstructFromLiteral));

    if (m_scroll)
        return upScrollValueKeyword;

    return "";
}

void TextTrackRegion::setScroll(const AtomicString& value, ExceptionCode& ec)
{
    DEFINE_STATIC_LOCAL(const AtomicString, upScrollValueKeyword, ("up", AtomicString::ConstructFromLiteral));

    if (value != emptyString() && value != upScrollValueKeyword) {
        ec = SYNTAX_ERR;
        return;
    }

    m_scroll = value == upScrollValueKeyword;
}

void TextTrackRegion::updateParametersFromRegion(TextTrackRegion* region)
{
    m_heightInLines = region->height();
    m_width = region->width();

    m_regionAnchor = FloatPoint(region->regionAnchorX(), region->regionAnchorY());
    m_viewportAnchor = FloatPoint(region->viewportAnchorX(), region->viewportAnchorY());

    setScroll(region->scroll(), ASSERT_NO_EXCEPTION);
}

void TextTrackRegion::setRegionSettings(const String& input)
{
    m_settings = input;
    unsigned position = 0;

    while (position < input.length()) {
        while (position < input.length() && WebVTTParser::isValidSettingDelimiter(input[position]))
            position++;

        if (position >= input.length())
            break;

        parseSetting(input, &position);
    }
}

TextTrackRegion::RegionSetting TextTrackRegion::getSettingFromString(const String& setting)
{
    DEFINE_STATIC_LOCAL(const AtomicString, idKeyword, ("id", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(const AtomicString, heightKeyword, ("height", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(const AtomicString, widthKeyword, ("width", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(const AtomicString, regionAnchorKeyword, ("regionanchor", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(const AtomicString, viewportAnchorKeyword, ("viewportanchor", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(const AtomicString, scrollKeyword, ("scroll", AtomicString::ConstructFromLiteral));

    if (setting == idKeyword)
        return Id;
    if (setting == heightKeyword)
        return Height;
    if (setting == widthKeyword)
        return Width;
    if (setting == viewportAnchorKeyword)
        return ViewportAnchor;
    if (setting == regionAnchorKeyword)
        return RegionAnchor;
    if (setting == scrollKeyword)
        return Scroll;

    return None;
}

void TextTrackRegion::parseSettingValue(RegionSetting setting, const String& value)
{
    DEFINE_STATIC_LOCAL(const AtomicString, scrollUpValueKeyword, ("up", AtomicString::ConstructFromLiteral));

    bool isValidSetting;
    String numberAsString;
    int number;
    unsigned position;
    FloatPoint anchorPosition;

    switch (setting) {
    case Id:
        if (value.find("-->") == notFound)
            m_id = value;
        break;
    case Width:
        number = WebVTTParser::parseFloatPercentageValue(value, isValidSetting);
        if (isValidSetting)
            m_width = number;
        else
            LOG(Media, "TextTrackRegion::parseSettingValue, invalid Width");
        break;
    case Height:
        position = 0;

        numberAsString = WebVTTParser::collectDigits(value, &position);
        number = value.toInt(&isValidSetting);

        if (isValidSetting && number >= 0)
            m_heightInLines = number;
        else
            LOG(Media, "TextTrackRegion::parseSettingValue, invalid Height");
        break;
    case RegionAnchor:
        anchorPosition = WebVTTParser::parseFloatPercentageValuePair(value, ',', isValidSetting);
        if (isValidSetting)
            m_regionAnchor = anchorPosition;
        else
            LOG(Media, "TextTrackRegion::parseSettingValue, invalid RegionAnchor");
        break;
    case ViewportAnchor:
        anchorPosition = WebVTTParser::parseFloatPercentageValuePair(value, ',', isValidSetting);
        if (isValidSetting)
            m_viewportAnchor = anchorPosition;
        else
            LOG(Media, "TextTrackRegion::parseSettingValue, invalid ViewportAnchor");
        break;
    case Scroll:
        if (value == scrollUpValueKeyword)
            m_scroll = true;
        else
            LOG(Media, "TextTrackRegion::parseSettingValue, invalid Scroll");
        break;
    case None:
        break;
    }
}

void TextTrackRegion::parseSetting(const String& input, unsigned* position)
{
    String setting = WebVTTParser::collectWord(input, position);

    size_t equalOffset = setting.find('=', 1);
    if (equalOffset == notFound || !equalOffset || equalOffset == setting.length() - 1)
        return;

    RegionSetting name = getSettingFromString(setting.substring(0, equalOffset));
    String value = setting.substring(equalOffset + 1, setting.length() - 1);

    parseSettingValue(name, value);
}

} // namespace WebCore

#endif
