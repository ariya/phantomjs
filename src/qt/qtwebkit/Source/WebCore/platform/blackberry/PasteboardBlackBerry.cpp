/*
 * Copyright (C) 2009, 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "Pasteboard.h"

#include "DocumentFragment.h"
#include "Frame.h"
#include "KURL.h"
#include "NotImplemented.h"
#include "Range.h"
#include "markup.h"

#include <BlackBerryPlatformClipboard.h>

namespace WebCore {

Pasteboard* Pasteboard::generalPasteboard()
{
    static Pasteboard* pasteboard = new Pasteboard();
    return pasteboard;
}

Pasteboard::Pasteboard()
{
}

bool Pasteboard::canSmartReplace()
{
    notImplemented();
    return false;
}

void Pasteboard::clear()
{
    BlackBerry::Platform::Clipboard::clearClipboard();
}

void Pasteboard::writeImage(Node*, KURL const&, String const&)
{
    notImplemented();
}

void Pasteboard::writeClipboard(Clipboard*)
{
    notImplemented();
}

void Pasteboard::writeSelection(Range* selectedRange, bool, Frame* frame, ShouldSerializeSelectedTextForClipboard shouldSerializeSelectedTextForClipboard)
{
    WTF::String text = shouldSerializeSelectedTextForClipboard == IncludeImageAltTextForClipboard ? frame->editor()->selectedTextForClipboard() : frame->editor()->selectedText();
    WTF::String html = createMarkup(selectedRange, 0, AnnotateForInterchange);
    DEFINE_STATIC_LOCAL(AtomicString, invokeString, ("href=\"invoke://"));
    size_t startOfInvoke = html.find(invokeString);
    if (startOfInvoke != notFound) {
        size_t endOfInvoke = html.find("\"", startOfInvoke + invokeString.length()) + 1;
        html.remove(startOfInvoke, endOfInvoke - startOfInvoke);
    }
    WTF::String url = frame->document()->url().string();

    BlackBerry::Platform::Clipboard::write(text, html, url);
}

void Pasteboard::writeURL(KURL const& url, String const&, Frame*)
{
    ASSERT(!url.isEmpty());
    BlackBerry::Platform::Clipboard::writeURL(url.string());
}

void Pasteboard::writePlainText(const String& text, SmartReplaceOption)
{
    BlackBerry::Platform::Clipboard::writePlainText(text);
}

String Pasteboard::plainText(Frame*)
{
    return BlackBerry::Platform::Clipboard::readPlainText();
}

PassRefPtr<DocumentFragment> Pasteboard::documentFragment(Frame* frame, PassRefPtr<Range> context, bool allowPlainText, bool& chosePlainText)
{
    chosePlainText = false;

    // Note: We are able to check if the format exists prior to reading but the check & the early return
    // path of get_clipboard_data are the exact same, so just use get_clipboard_data and validate the
    // return value to determine if the data was present.
    String html = BlackBerry::Platform::Clipboard::readHTML();
    RefPtr<DocumentFragment> fragment;
    if (!html.isEmpty()) {
        String url = BlackBerry::Platform::Clipboard::readURL();
        if (fragment = createFragmentFromMarkup(frame->document(), html, url, DisallowScriptingAndPluginContent))
            return fragment.release();
    }

    if (!allowPlainText)
        return 0;

    String text = BlackBerry::Platform::Clipboard::readPlainText();
    if (fragment = createFragmentFromText(context.get(), text)) {
        chosePlainText = true;
        return fragment.release();
    }
    return 0;
}

PassOwnPtr<Pasteboard> Pasteboard::createForCopyAndPaste()
{
    return adoptPtr(new Pasteboard);
}

PassOwnPtr<Pasteboard> Pasteboard::createPrivate()
{
    return createForCopyAndPaste();
}

bool Pasteboard::hasData()
{
    return BlackBerry::Platform::Clipboard::hasPlainText() || BlackBerry::Platform::Clipboard::hasHTML() || BlackBerry::Platform::Clipboard::hasURL();
}

void Pasteboard::clear(const String& type)
{
    BlackBerry::Platform::Clipboard::clearClipboardByType(type.utf8().data());
}

String Pasteboard::readString(const String& type)
{
    return String::fromUTF8(BlackBerry::Platform::Clipboard::readClipboardByType(type.utf8().data()).c_str());
}

bool Pasteboard::writeString(const String& type, const String& text)
{
    if (type == "text/plain") {
        BlackBerry::Platform::Clipboard::writePlainText(text);
        return true;
    }
    if (type == "text/html") {
        BlackBerry::Platform::Clipboard::writeHTML(text);
        return true;
    }
    if (type == "text/url") {
        BlackBerry::Platform::Clipboard::writeURL(text);
        return true;
    }
    return false;
}

ListHashSet<String> Pasteboard::types()
{
    // We use hardcoded list here since there seems to be no API to get the list.
    // FIXME: Should omit types where we have no data, using the same functions used above in Pasteboard::hasData.
    ListHashSet<String> types;
    types.add("text/plain");
    types.add("text/html");
    types.add("text/url");
    return types;
}

Vector<String> Pasteboard::readFilenames()
{
    notImplemented();
    return Vector<String>();
}

} // namespace WebCore
