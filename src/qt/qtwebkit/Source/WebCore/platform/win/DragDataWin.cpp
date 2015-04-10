/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Baidu Inc. All rights reserved.
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
#include "DragData.h"

#include "COMPtr.h"
#include "ClipboardUtilitiesWin.h"
#include "Frame.h"
#include "DocumentFragment.h"
#include "Markup.h"
#include "Range.h"
#include "TextEncoding.h"
#include <objidl.h>
#include <shlwapi.h>
#include <wininet.h>
#include <wtf/Forward.h>
#include <wtf/Hashmap.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

DragData::DragData(const DragDataMap& data, const IntPoint& clientPosition, const IntPoint& globalPosition,
    DragOperation sourceOperationMask, DragApplicationFlags flags)
    : m_clientPosition(clientPosition)
    , m_globalPosition(globalPosition)
    , m_platformDragData(0)
    , m_draggingSourceOperationMask(sourceOperationMask)
    , m_applicationFlags(flags)
    , m_dragDataMap(data)
{
}

bool DragData::containsURL(Frame*, FilenameConversionPolicy filenamePolicy) const
{
    if (m_platformDragData)
        return SUCCEEDED(m_platformDragData->QueryGetData(urlWFormat())) 
            || SUCCEEDED(m_platformDragData->QueryGetData(urlFormat()))
            || (filenamePolicy == ConvertFilenames
                && (SUCCEEDED(m_platformDragData->QueryGetData(filenameWFormat()))
                    || SUCCEEDED(m_platformDragData->QueryGetData(filenameFormat()))));
    return m_dragDataMap.contains(urlWFormat()->cfFormat) || m_dragDataMap.contains(urlFormat()->cfFormat)
        || (filenamePolicy == ConvertFilenames && (m_dragDataMap.contains(filenameWFormat()->cfFormat) || m_dragDataMap.contains(filenameFormat()->cfFormat)));
}

const DragDataMap& DragData::dragDataMap()
{
    if (!m_dragDataMap.isEmpty() || !m_platformDragData)
        return m_dragDataMap;
    // Enumerate clipboard content and load it in the map.
    COMPtr<IEnumFORMATETC> itr;

    if (FAILED(m_platformDragData->EnumFormatEtc(DATADIR_GET, &itr)) || !itr)
        return m_dragDataMap;

    FORMATETC dataFormat;
    while (itr->Next(1, &dataFormat, 0) == S_OK) {
        Vector<String> dataStrings;
        getClipboardData(m_platformDragData, &dataFormat, dataStrings);
        if (!dataStrings.isEmpty())
            m_dragDataMap.set(dataFormat.cfFormat, dataStrings); 
    }
    return m_dragDataMap;
}

void DragData::getDragFileDescriptorData(int& size, String& pathname)
{
    size = 0;
    if (m_platformDragData)
        getFileDescriptorData(m_platformDragData, size, pathname);
}

void DragData::getDragFileContentData(int size, void* dataBlob)
{
    if (m_platformDragData)
        getFileContentData(m_platformDragData, size, dataBlob);
}

String DragData::asURL(Frame*, FilenameConversionPolicy filenamePolicy, String* title) const
{
    return (m_platformDragData) ? getURL(m_platformDragData, filenamePolicy, title) : getURL(&m_dragDataMap, filenamePolicy, title);
}

bool DragData::containsFiles() const
{
#if USE(CF)
    return (m_platformDragData) ? SUCCEEDED(m_platformDragData->QueryGetData(cfHDropFormat())) : m_dragDataMap.contains(cfHDropFormat()->cfFormat);
#else
    return false;
#endif
}

unsigned DragData::numberOfFiles() const
{
#if USE(CF)
    if (!m_platformDragData)
        return 0;

    STGMEDIUM medium;
    if (FAILED(m_platformDragData->GetData(cfHDropFormat(), &medium)))
        return 0;

    HDROP hdrop = static_cast<HDROP>(GlobalLock(medium.hGlobal));

    if (!hdrop)
        return 0;

    unsigned numFiles = DragQueryFileW(hdrop, 0xFFFFFFFF, 0, 0);

    DragFinish(hdrop);
    GlobalUnlock(medium.hGlobal);

    return numFiles;
#else
    return 0;
#endif
}

void DragData::asFilenames(Vector<String>& result) const
{
#if USE(CF)
    if (m_platformDragData) {
        WCHAR filename[MAX_PATH];

        STGMEDIUM medium;
        if (FAILED(m_platformDragData->GetData(cfHDropFormat(), &medium)))
            return;

        HDROP hdrop = reinterpret_cast<HDROP>(GlobalLock(medium.hGlobal)); 

        if (!hdrop)
            return;

        const unsigned numFiles = DragQueryFileW(hdrop, 0xFFFFFFFF, 0, 0);
        for (unsigned i = 0; i < numFiles; i++) {
            if (!DragQueryFileW(hdrop, i, filename, WTF_ARRAY_LENGTH(filename)))
                continue;
            result.append(static_cast<UChar*>(filename)); 
        }

        // Free up memory from drag
        DragFinish(hdrop);

        GlobalUnlock(medium.hGlobal);
        return;
    }
    result = m_dragDataMap.get(cfHDropFormat()->cfFormat);
#endif
}

bool DragData::containsPlainText() const
{
    if (m_platformDragData)
        return SUCCEEDED(m_platformDragData->QueryGetData(plainTextWFormat()))
            || SUCCEEDED(m_platformDragData->QueryGetData(plainTextFormat()));
    return m_dragDataMap.contains(plainTextWFormat()->cfFormat) || m_dragDataMap.contains(plainTextFormat()->cfFormat);
}

String DragData::asPlainText(Frame*) const
{
    return (m_platformDragData) ? getPlainText(m_platformDragData) : getPlainText(&m_dragDataMap);
}

bool DragData::containsColor() const
{
    return false;
}

bool DragData::canSmartReplace() const
{
    if (m_platformDragData)
        return SUCCEEDED(m_platformDragData->QueryGetData(smartPasteFormat()));
    return m_dragDataMap.contains(smartPasteFormat()->cfFormat);
}

bool DragData::containsCompatibleContent() const
{
    return containsPlainText() || containsURL(0) 
        || ((m_platformDragData) ? (containsHTML(m_platformDragData) || containsFilenames(m_platformDragData))
            : (containsHTML(&m_dragDataMap) || containsFilenames(&m_dragDataMap)))
        || containsColor();
}

PassRefPtr<DocumentFragment> DragData::asFragment(Frame* frame, PassRefPtr<Range>, bool, bool&) const
{     
    /*
     * Order is richest format first. On OSX this is:
     * * Web Archive
     * * Filenames
     * * HTML
     * * RTF
     * * TIFF
     * * PICT
     */
     
    if (m_platformDragData) {
        if (containsFilenames(m_platformDragData)) {
            if (PassRefPtr<DocumentFragment> fragment = fragmentFromFilenames(frame->document(), m_platformDragData))
                return fragment;
        }

        if (containsHTML(m_platformDragData)) {
            if (PassRefPtr<DocumentFragment> fragment = fragmentFromHTML(frame->document(), m_platformDragData))
                return fragment;
        }
    } else {
        if (containsFilenames(&m_dragDataMap)) {
            if (PassRefPtr<DocumentFragment> fragment = fragmentFromFilenames(frame->document(), &m_dragDataMap))
                return fragment;
        }

        if (containsHTML(&m_dragDataMap)) {
            if (PassRefPtr<DocumentFragment> fragment = fragmentFromHTML(frame->document(), &m_dragDataMap))
                return fragment;
        }
    }
    return 0;
}

Color DragData::asColor() const
{
    return Color();
}

}
