/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
 *
 */

#include "config.h"
#include "FileInputType.h"

#include "Event.h"
#include "File.h"
#include "FileList.h"
#include "FileSystem.h"
#include "FormDataList.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "LocalizedStrings.h"
#include "RenderFileUploadControl.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

using namespace HTMLNames;

inline FileInputType::FileInputType(HTMLInputElement* element)
    : BaseButtonInputType(element)
    , m_fileList(FileList::create())
{
}

PassOwnPtr<InputType> FileInputType::create(HTMLInputElement* element)
{
    return adoptPtr(new FileInputType(element));
}

const AtomicString& FileInputType::formControlType() const
{
    return InputTypeNames::file();
}

bool FileInputType::appendFormData(FormDataList& encoding, bool multipart) const
{
    FileList* fileList = element()->files();
    unsigned numFiles = fileList->length();
    if (!multipart) {
        // Send only the basenames.
        // 4.10.16.4 and 4.10.16.6 sections in HTML5.

        // Unlike the multipart case, we have no special handling for the empty
        // fileList because Netscape doesn't support for non-multipart
        // submission of file inputs, and Firefox doesn't add "name=" query
        // parameter.
        for (unsigned i = 0; i < numFiles; ++i)
            encoding.appendData(element()->name(), fileList->item(i)->fileName());
        return true;
    }

    // If no filename at all is entered, return successful but empty.
    // Null would be more logical, but Netscape posts an empty file. Argh.
    if (!numFiles) {
        encoding.appendBlob(element()->name(), File::create(""));
        return true;
    }

    for (unsigned i = 0; i < numFiles; ++i)
        encoding.appendBlob(element()->name(), fileList->item(i));
    return true;
}

bool FileInputType::valueMissing(const String& value) const
{
    return value.isEmpty();
}

String FileInputType::valueMissingText() const
{
    return element()->multiple() ? validationMessageValueMissingForMultipleFileText() : validationMessageValueMissingForFileText();
}

void FileInputType::handleDOMActivateEvent(Event* event)
{
    if (element()->disabled() || !element()->renderer())
        return;
    toRenderFileUploadControl(element()->renderer())->click();
    event->setDefaultHandled();
}

RenderObject* FileInputType::createRenderer(RenderArena* arena, RenderStyle*) const
{
    return new (arena) RenderFileUploadControl(element());
}

bool FileInputType::canSetStringValue() const
{
    return false;
}

bool FileInputType::canChangeFromAnotherType() const
{
    // Don't allow the type to be changed to file after the first type change.
    // In other engines this might mean a JavaScript programmer could set a text
    // field's value to something like /etc/passwd and then change it to a file input.
    // I don't think this would actually occur in WebKit, but this rule still may be
    // important for compatibility.
    return false;
}

FileList* FileInputType::files()
{
    return m_fileList.get();
}

bool FileInputType::canSetValue(const String& value)
{
    // For security reasons, we don't allow setting the filename, but we do allow clearing it.
    // The HTML5 spec (as of the 10/24/08 working draft) says that the value attribute isn't
    // applicable to the file upload control at all, but for now we are keeping this behavior
    // to avoid breaking existing websites that may be relying on this.
    return value.isEmpty();
}

bool FileInputType::getTypeSpecificValue(String& value)
{
    if (m_fileList->isEmpty()) {
        value = String();
        return true;
    }

    // HTML5 tells us that we're supposed to use this goofy value for
    // file input controls. Historically, browsers revealed the real
    // file path, but that's a privacy problem. Code on the web
    // decided to try to parse the value by looking for backslashes
    // (because that's what Windows file paths use). To be compatible
    // with that code, we make up a fake path for the file.
    value = "C:\\fakepath\\" + m_fileList->item(0)->fileName();
    return true;
}

bool FileInputType::storesValueSeparateFromAttribute()
{
    return true;
}

void FileInputType::setFileList(const Vector<String>& paths)
{
    m_fileList->clear();
    size_t size = paths.size();

#if ENABLE(DIRECTORY_UPLOAD)
    // If a directory is being selected, the UI allows a directory to be chosen
    // and the paths provided here share a root directory somewhere up the tree;
    // we want to store only the relative paths from that point.
    if (size && element()->fastHasAttribute(webkitdirectoryAttr)) {
        // Find the common root path.
        String rootPath = directoryName(paths[0]);
        for (size_t i = 1; i < size; i++) {
            while (!paths[i].startsWith(rootPath))
                rootPath = directoryName(rootPath);
        }
        rootPath = directoryName(rootPath);
        ASSERT(rootPath.length());
        for (size_t i = 0; i < size; i++) {
            // Normalize backslashes to slashes before exposing the relative path to script.
            String relativePath = paths[i].substring(1 + rootPath.length()).replace('\\', '/');
            m_fileList->append(File::create(relativePath, paths[i]));
        }
        return;
    }
#endif

    for (size_t i = 0; i < size; i++)
        m_fileList->append(File::create(paths[i]));
}

bool FileInputType::isFileUpload() const
{
    return true;
}

} // namespace WebCore
