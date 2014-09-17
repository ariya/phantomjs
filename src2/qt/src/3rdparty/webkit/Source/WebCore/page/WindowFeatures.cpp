/*
 *  Copyright (C) 2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2006 Jon Shier (jshier@iastate.edu)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2010 Apple Inc. All rights reseved.
 *  Copyright (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#include "config.h"
#include "WindowFeatures.h"

#include "FloatRect.h"
#include "PlatformString.h"
#include <wtf/Assertions.h>
#include <wtf/MathExtras.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

// Though isspace() considers \t and \v to be whitespace, Win IE doesn't when parsing window features.
static bool isWindowFeaturesSeparator(UChar c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '=' || c == ',' || c == '\0';
}

WindowFeatures::WindowFeatures(const String& features)
    : xSet(false)
    , ySet(false)
    , widthSet(false)
    , heightSet(false)
    , fullscreen(false)
    , dialog(false)
{
    /*
     The IE rule is: all features except for channelmode and fullscreen default to YES, but
     if the user specifies a feature string, all features default to NO. (There is no public
     standard that applies to this method.)

     <http://msdn.microsoft.com/workshop/author/dhtml/reference/methods/open_0.asp>
     We always allow a window to be resized, which is consistent with Firefox.
     */

    if (features.length() == 0) {
        menuBarVisible = true;
        statusBarVisible = true;
        toolBarVisible = true;
        locationBarVisible = true;
        scrollbarsVisible = true;
        resizable = true;
        return;
    }

    menuBarVisible = false;
    statusBarVisible = false;
    toolBarVisible = false;
    locationBarVisible = false;
    scrollbarsVisible = false;
    resizable = true;

    // Tread lightly in this code -- it was specifically designed to mimic Win IE's parsing behavior.
    int keyBegin, keyEnd;
    int valueBegin, valueEnd;

    int i = 0;
    int length = features.length();
    String buffer = features.lower();
    while (i < length) {
        // skip to first non-separator, but don't skip past the end of the string
        while (isWindowFeaturesSeparator(buffer[i])) {
            if (i >= length)
                break;
            i++;
        }
        keyBegin = i;

        // skip to first separator
        while (!isWindowFeaturesSeparator(buffer[i]))
            i++;
        keyEnd = i;

        // skip to first '=', but don't skip past a ',' or the end of the string
        while (buffer[i] != '=') {
            if (buffer[i] == ',' || i >= length)
                break;
            i++;
        }

        // skip to first non-separator, but don't skip past a ',' or the end of the string
        while (isWindowFeaturesSeparator(buffer[i])) {
            if (buffer[i] == ',' || i >= length)
                break;
            i++;
        }
        valueBegin = i;

        // skip to first separator
        while (!isWindowFeaturesSeparator(buffer[i]))
            i++;
        valueEnd = i;

        ASSERT(i <= length);

        String keyString(buffer.substring(keyBegin, keyEnd - keyBegin));
        String valueString(buffer.substring(valueBegin, valueEnd - valueBegin));
        setWindowFeature(keyString, valueString);
    }
}

void WindowFeatures::setWindowFeature(const String& keyString, const String& valueString)
{
    int value;

    // Listing a key with no value is shorthand for key=yes
    if (valueString.isEmpty() || valueString == "yes")
        value = 1;
    else
        value = valueString.toInt();

    // We treat keyString of "resizable" here as an additional feature rather than setting resizeable to true.
    // This is consistent with Firefox, but could also be handled at another level.

    if (keyString == "left" || keyString == "screenx") {
        xSet = true;
        x = value;
    } else if (keyString == "top" || keyString == "screeny") {
        ySet = true;
        y = value;
    } else if (keyString == "width" || keyString == "innerwidth") {
        widthSet = true;
        width = value;
    } else if (keyString == "height" || keyString == "innerheight") {
        heightSet = true;
        height = value;
    } else if (keyString == "menubar")
        menuBarVisible = value;
    else if (keyString == "toolbar")
        toolBarVisible = value;
    else if (keyString == "location")
        locationBarVisible = value;
    else if (keyString == "status")
        statusBarVisible = value;
    else if (keyString == "fullscreen")
        fullscreen = value;
    else if (keyString == "scrollbars")
        scrollbarsVisible = value;
    else if (value == 1)
        additionalFeatures.append(keyString);
}

WindowFeatures::WindowFeatures(const String& dialogFeaturesString, const FloatRect& screenAvailableRect)
    : widthSet(true)
    , heightSet(true)
    , menuBarVisible(false)
    , toolBarVisible(false)
    , locationBarVisible(false)
    , fullscreen(false)
    , dialog(true)
{
    DialogFeaturesMap features;
    parseDialogFeatures(dialogFeaturesString, features);

    const bool trusted = false;

    // The following features from Microsoft's documentation are not implemented:
    // - default font settings
    // - width, height, left, and top specified in units other than "px"
    // - edge (sunken or raised, default is raised)
    // - dialogHide: trusted && boolFeature(features, "dialoghide"), makes dialog hide when you print
    // - help: boolFeature(features, "help", true), makes help icon appear in dialog (what does it do on Windows?)
    // - unadorned: trusted && boolFeature(features, "unadorned");

    width = floatFeature(features, "dialogwidth", 100, screenAvailableRect.width(), 620); // default here came from frame size of dialog in MacIE
    height = floatFeature(features, "dialogheight", 100, screenAvailableRect.height(), 450); // default here came from frame size of dialog in MacIE

    x = floatFeature(features, "dialogleft", screenAvailableRect.x(), screenAvailableRect.maxX() - width, -1);
    xSet = x > 0;
    y = floatFeature(features, "dialogtop", screenAvailableRect.y(), screenAvailableRect.maxY() - height, -1);
    ySet = y > 0;

    if (boolFeature(features, "center", true)) {
        if (!xSet) {
            x = screenAvailableRect.x() + (screenAvailableRect.width() - width) / 2;
            xSet = true;
        }
        if (!ySet) {
            y = screenAvailableRect.y() + (screenAvailableRect.height() - height) / 2;
            ySet = true;
        }
    }

    resizable = boolFeature(features, "resizable");
    scrollbarsVisible = boolFeature(features, "scroll", true);
    statusBarVisible = boolFeature(features, "status", !trusted);
}

bool WindowFeatures::boolFeature(const DialogFeaturesMap& features, const char* key, bool defaultValue)
{
    DialogFeaturesMap::const_iterator it = features.find(key);
    if (it == features.end())
        return defaultValue;
    const String& value = it->second;
    return value.isNull() || value == "1" || value == "yes" || value == "on";
}

float WindowFeatures::floatFeature(const DialogFeaturesMap& features, const char* key, float min, float max, float defaultValue)
{
    DialogFeaturesMap::const_iterator it = features.find(key);
    if (it == features.end())
        return defaultValue;
    // FIXME: The toDouble function does not offer a way to tell "0q" from string with no digits in it: Both
    // return the number 0 and false for ok. But "0q" should yield the minimum rather than the default.
    bool ok;
    double parsedNumber = it->second.toDouble(&ok);
    if ((parsedNumber == 0 && !ok) || isnan(parsedNumber))
        return defaultValue;
    if (parsedNumber < min || max <= min)
        return min;
    if (parsedNumber > max)
        return max;
    // FIXME: Seems strange to cast a double to int and then convert back to a float. Why is this a good idea?
    return static_cast<int>(parsedNumber);
}

void WindowFeatures::parseDialogFeatures(const String& string, DialogFeaturesMap& map)
{
    Vector<String> vector;
    string.split(';', vector);
    size_t size = vector.size();
    for (size_t i = 0; i < size; ++i) {
        const String& featureString = vector[i];

        size_t separatorPosition = featureString.find('=');
        size_t colonPosition = featureString.find(':');
        if (separatorPosition != notFound && colonPosition != notFound)
            continue; // ignore strings that have both = and :
        if (separatorPosition == notFound)
            separatorPosition = colonPosition;

        String key = featureString.left(separatorPosition).stripWhiteSpace().lower();

        // Null string for value indicates key without value.
        String value;
        if (separatorPosition != notFound) {
            value = featureString.substring(separatorPosition + 1).stripWhiteSpace().lower();
            value = value.left(value.find(' '));
        }

        map.set(key, value);
    }
}

} // namespace WebCore
