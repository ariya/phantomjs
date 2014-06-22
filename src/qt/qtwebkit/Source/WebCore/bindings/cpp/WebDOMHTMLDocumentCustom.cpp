/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "WebDOMHTMLDocument.h"

#include "HTMLDocument.h"
#include "SegmentedString.h"
#include "WebExceptionHandler.h"
#include <wtf/Forward.h>
#include <wtf/unicode/CharacterNames.h>

static inline void documentWrite(const WebDOMString& text, WebCore::HTMLDocument* document, bool addNewline)
{
    WebCore::SegmentedString segmentedString = WTF::String(text);
    if (addNewline)
        segmentedString.append(WebCore::SegmentedString(WTF::String(&WTF::Unicode::newlineCharacter)));
    document->write(segmentedString);
}

void WebDOMHTMLDocument::write(const WebDOMString& text)
{
    if (!impl())
        return;

    documentWrite(text, impl(), false);
}

void WebDOMHTMLDocument::writeln(const WebDOMString& text)
{
    if (!impl())
        return;

    documentWrite(text, impl(), true);
}
