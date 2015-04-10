/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2013 Xueqing Huang <huangxueqing@baidu.com>
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
#include "Pasteboard.h"

#include "BitmapInfo.h"
#include "CachedImage.h"
#include "ClipboardUtilitiesWin.h"
#include "Document.h"
#include "DocumentFragment.h"
#include "Editor.h"
#include "Element.h"
#include "Frame.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "HWndDC.h"
#include "HitTestResult.h"
#include "Image.h"
#include "KURL.h"
#include "NotImplemented.h"
#include "Page.h"
#include "Range.h"
#include "RenderImage.h"
#include "SharedBuffer.h"
#include "TextEncoding.h"
#include "WebCoreInstanceHandle.h"
#include "WindowsExtras.h"
#include "markup.h"
#include <wtf/text/CString.h>

namespace WebCore {

// We provide the IE clipboard types (URL and Text), and the clipboard types specified in the WHATWG Web Applications 1.0 draft
// see http://www.whatwg.org/specs/web-apps/current-work/ Section 6.3.5.3

static UINT HTMLClipboardFormat = 0;
static UINT BookmarkClipboardFormat = 0;
static UINT WebSmartPasteFormat = 0;

static LRESULT CALLBACK PasteboardOwnerWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lresult = 0;

    switch (message) {
    case WM_RENDERFORMAT:
        // This message comes when SetClipboardData was sent a null data handle 
        // and now it's come time to put the data on the clipboard.
        break;
    case WM_RENDERALLFORMATS:
        // This message comes when SetClipboardData was sent a null data handle
        // and now this application is about to quit, so it must put data on 
        // the clipboard before it exits.
        break;
    case WM_DESTROY:
        break;
#if !OS(WINCE)
    case WM_DRAWCLIPBOARD:
        break;
    case WM_CHANGECBCHAIN:
        break;
#endif
    default:
        lresult = DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }
    return lresult;
}

Pasteboard* Pasteboard::generalPasteboard() 
{
    static Pasteboard* pasteboard = new Pasteboard;
    return pasteboard;
}

PassOwnPtr<Pasteboard> Pasteboard::createForCopyAndPaste()
{
    OwnPtr<Pasteboard> pasteboard = adoptPtr(new Pasteboard);
    COMPtr<IDataObject> clipboardData;
#if !OS(WINCE)
    if (!SUCCEEDED(OleGetClipboard(&clipboardData)))
        clipboardData = 0;
#endif
    pasteboard->setExternalDataObject(clipboardData.get());
    return pasteboard.release();
}

PassOwnPtr<Pasteboard> Pasteboard::createPrivate()
{
    // Windows has no "Private pasteboard" concept.
    return createForCopyAndPaste();
}

#if ENABLE(DRAG_SUPPORT)
PassOwnPtr<Pasteboard> Pasteboard::createForDragAndDrop()
{
    COMPtr<WCDataObject> dataObject;
    WCDataObject::createInstance(&dataObject);
    return adoptPtr(new Pasteboard(dataObject.get()));
}

// static
PassOwnPtr<Pasteboard> Pasteboard::createForDragAndDrop(const DragData& dragData)
{
    if (dragData.platformData())
        return adoptPtr(new Pasteboard(dragData.platformData()));
    // FIXME: Should add a const overload of dragDataMap so we don't need a const_cast here.
    return adoptPtr(new Pasteboard(const_cast<DragData&>(dragData).dragDataMap()));
}
#endif

void Pasteboard::finishCreatingPasteboard()
{
    WNDCLASS wc;
    memset(&wc, 0, sizeof(WNDCLASS));
    wc.lpfnWndProc    = PasteboardOwnerWndProc;
    wc.hInstance      = WebCore::instanceHandle();
    wc.lpszClassName  = L"PasteboardOwnerWindowClass";
    RegisterClass(&wc);

    m_owner = ::CreateWindow(L"PasteboardOwnerWindowClass", L"PasteboardOwnerWindow", 0, 0, 0, 0, 0,
        HWND_MESSAGE, 0, 0, 0);

    HTMLClipboardFormat = ::RegisterClipboardFormat(L"HTML Format");
    BookmarkClipboardFormat = ::RegisterClipboardFormat(L"UniformResourceLocatorW");
    WebSmartPasteFormat = ::RegisterClipboardFormat(L"WebKit Smart Paste Format");
}

Pasteboard::Pasteboard()
    : m_dataObject(0)
    , m_writableDataObject(0)
{
    finishCreatingPasteboard();
}

Pasteboard::Pasteboard(IDataObject* dataObject)
    : m_dataObject(dataObject)
    , m_writableDataObject(0)
{
    finishCreatingPasteboard();
}

Pasteboard::Pasteboard(WCDataObject* dataObject)
    : m_dataObject(dataObject)
    , m_writableDataObject(dataObject)
{
    finishCreatingPasteboard();
}

Pasteboard::Pasteboard(const DragDataMap& dataMap)
    : m_dataObject(0)
    , m_writableDataObject(0)
    , m_dragDataMap(dataMap)
{
    finishCreatingPasteboard();
}

void Pasteboard::clear()
{
    if (::OpenClipboard(m_owner)) {
        ::EmptyClipboard();
        ::CloseClipboard();
    }
}

enum ClipboardDataType { ClipboardDataTypeNone, ClipboardDataTypeURL, ClipboardDataTypeText, ClipboardDataTypeTextHTML };

static ClipboardDataType clipboardTypeFromMIMEType(const String& type)
{
    String qType = type.stripWhiteSpace().lower();

    // two special cases for IE compatibility
    if (qType == "text" || qType == "text/plain" || qType.startsWith("text/plain;"))
        return ClipboardDataTypeText;
    if (qType == "url" || qType == "text/uri-list")
        return ClipboardDataTypeURL;
    if (qType == "text/html")
        return ClipboardDataTypeTextHTML;

    return ClipboardDataTypeNone;
}

void Pasteboard::clear(const String& type)
{
    if (!m_writableDataObject)
        return;

    ClipboardDataType dataType = clipboardTypeFromMIMEType(type);

    if (dataType == ClipboardDataTypeURL) {
        m_writableDataObject->clearData(urlWFormat()->cfFormat);
        m_writableDataObject->clearData(urlFormat()->cfFormat);
    }
    if (dataType == ClipboardDataTypeText) {
        m_writableDataObject->clearData(plainTextFormat()->cfFormat);
        m_writableDataObject->clearData(plainTextWFormat()->cfFormat);
    }
}

bool Pasteboard::hasData()
{
    if (!m_dataObject && m_dragDataMap.isEmpty())
        return false;

    if (m_dataObject) {
        COMPtr<IEnumFORMATETC> itr;
        if (FAILED(m_dataObject->EnumFormatEtc(DATADIR_GET, &itr)))
            return false;

        if (!itr)
            return false;

        FORMATETC data;

        // IEnumFORMATETC::Next returns S_FALSE if there are no more items.
        if (itr->Next(1, &data, 0) == S_OK) {
            // There is at least one item in the IDataObject
            return true;
        }

        return false;
    }
    return !m_dragDataMap.isEmpty();
}

static void addMimeTypesForFormat(ListHashSet<String>& results, const FORMATETC& format)
{
    // URL and Text are provided for compatibility with IE's model
    if (format.cfFormat == urlFormat()->cfFormat || format.cfFormat == urlWFormat()->cfFormat) {
        results.add("URL");
        results.add("text/uri-list");
    }

    if (format.cfFormat == plainTextWFormat()->cfFormat || format.cfFormat == plainTextFormat()->cfFormat) {
        results.add("Text");
        results.add("text/plain");
    }
}

ListHashSet<String> Pasteboard::types()
{
    ListHashSet<String> results;

    if (!m_dataObject && m_dragDataMap.isEmpty())
        return results;

    if (m_dataObject) {
        COMPtr<IEnumFORMATETC> itr;

        if (FAILED(m_dataObject->EnumFormatEtc(DATADIR_GET, &itr)))
            return results;

        if (!itr)
            return results;

        FORMATETC data;

        // IEnumFORMATETC::Next returns S_FALSE if there are no more items.
        while (itr->Next(1, &data, 0) == S_OK)
            addMimeTypesForFormat(results, data);
    } else {
        for (DragDataMap::const_iterator it = m_dragDataMap.begin(); it != m_dragDataMap.end(); ++it) {
            FORMATETC data;
            data.cfFormat = (*it).key;
            addMimeTypesForFormat(results, data);
        }
    }

    return results;
}

String Pasteboard::readString(const String& type)
{
    if (!m_dataObject && m_dragDataMap.isEmpty())
        return "";

    ClipboardDataType dataType = clipboardTypeFromMIMEType(type);
    if (dataType == ClipboardDataTypeText)
        return m_dataObject ? getPlainText(m_dataObject.get()) : getPlainText(&m_dragDataMap);
    if (dataType == ClipboardDataTypeURL)
        return m_dataObject ? getURL(m_dataObject.get(), DragData::DoNotConvertFilenames) : getURL(&m_dragDataMap, DragData::DoNotConvertFilenames);
    if (dataType == ClipboardDataTypeTextHTML) {
        String data = m_dataObject ? getTextHTML(m_dataObject.get()) : getTextHTML(&m_dragDataMap);
        if (!data.isEmpty())
            return data;
        return m_dataObject ? getCFHTML(m_dataObject.get()) : getCFHTML(&m_dragDataMap);
    }

    return "";
}

Vector<String> Pasteboard::readFilenames()
{
    Vector<String> fileNames;

#if USE(CF)
    if (m_dataObject) {
        STGMEDIUM medium;
        if (FAILED(m_dataObject->GetData(cfHDropFormat(), &medium)))
            return fileNames;

        HDROP hdrop = reinterpret_cast<HDROP>(GlobalLock(medium.hGlobal));
        if (!hdrop)
            return fileNames;

        WCHAR filename[MAX_PATH];
        UINT fileCount = DragQueryFileW(hdrop, 0xFFFFFFFF, 0, 0);
        for (UINT i = 0; i < fileCount; i++) {
            if (!DragQueryFileW(hdrop, i, filename, WTF_ARRAY_LENGTH(filename)))
                continue;
            fileNames.append(filename);
        }

        GlobalUnlock(medium.hGlobal);
        ReleaseStgMedium(&medium);
        return fileNames;
    }
    if (!m_dragDataMap.contains(cfHDropFormat()->cfFormat))
        return fileNames;
    return m_dragDataMap.get(cfHDropFormat()->cfFormat);
#else
    notImplemented();
    return fileNames;
#endif
}

static bool writeURL(WCDataObject *data, const KURL& url, String title, bool withPlainText, bool withHTML)
{
    ASSERT(data);

    if (url.isEmpty())
        return false;

    if (title.isEmpty()) {
        title = url.lastPathComponent();
        if (title.isEmpty())
            title = url.host();
    }

    STGMEDIUM medium = {0};
    medium.tymed = TYMED_HGLOBAL;

    medium.hGlobal = createGlobalData(url, title);
    bool success = false;
    if (medium.hGlobal && FAILED(data->SetData(urlWFormat(), &medium, TRUE)))
        ::GlobalFree(medium.hGlobal);
    else
        success = true;

    if (withHTML) {
        Vector<char> cfhtmlData;
        markupToCFHTML(urlToMarkup(url, title), "", cfhtmlData);
        medium.hGlobal = createGlobalData(cfhtmlData);
        if (medium.hGlobal && FAILED(data->SetData(htmlFormat(), &medium, TRUE)))
            ::GlobalFree(medium.hGlobal);
        else
            success = true;
    }

    if (withPlainText) {
        medium.hGlobal = createGlobalData(url.string());
        if (medium.hGlobal && FAILED(data->SetData(plainTextWFormat(), &medium, TRUE)))
            ::GlobalFree(medium.hGlobal);
        else
            success = true;
    }

    return success;
}

bool Pasteboard::writeString(const String& type, const String& data)
{
    if (!m_writableDataObject)
        return false;

    ClipboardDataType winType = clipboardTypeFromMIMEType(type);

    if (winType == ClipboardDataTypeURL)
        return WebCore::writeURL(m_writableDataObject.get(), KURL(KURL(), data), String(), false, true);

    if (winType == ClipboardDataTypeText) {
        STGMEDIUM medium = {0};
        medium.tymed = TYMED_HGLOBAL;
        medium.hGlobal = createGlobalData(data);
        if (!medium.hGlobal)
            return false;

        if (FAILED(m_writableDataObject->SetData(plainTextWFormat(), &medium, TRUE))) {
            ::GlobalFree(medium.hGlobal);
            return false;
        }
        return true;
    }

    return false;
}

#if ENABLE(DRAG_SUPPORT)
void Pasteboard::setDragImage(DragImageRef, const IntPoint&)
{
    // Do nothing in Windows.
}
#endif

void Pasteboard::writeRangeToDataObject(Range* selectedRange, Frame* frame)
{
    ASSERT(selectedRange);
    if (!m_writableDataObject)
        return;

    STGMEDIUM medium = {0};
    medium.tymed = TYMED_HGLOBAL;

    Vector<char> data;
    markupToCFHTML(createMarkup(selectedRange, 0, AnnotateForInterchange),
        selectedRange->startContainer()->document()->url().string(), data);
    medium.hGlobal = createGlobalData(data);
    if (medium.hGlobal && FAILED(m_writableDataObject->SetData(htmlFormat(), &medium, TRUE)))
        ::GlobalFree(medium.hGlobal);

    String str = frame->editor().selectedTextForClipboard();
    replaceNewlinesWithWindowsStyleNewlines(str);
    replaceNBSPWithSpace(str);
    medium.hGlobal = createGlobalData(str);
    if (medium.hGlobal && FAILED(m_writableDataObject->SetData(plainTextWFormat(), &medium, TRUE)))
        ::GlobalFree(medium.hGlobal);

    medium.hGlobal = 0;
    if (frame->editor().canSmartCopyOrDelete())
        m_writableDataObject->SetData(smartPasteFormat(), &medium, TRUE);
}

void Pasteboard::writeSelection(Range* selectedRange, bool canSmartCopyOrDelete, Frame* frame, ShouldSerializeSelectedTextForClipboard shouldSerializeSelectedTextForClipboard)
{
    clear();

    // Put CF_HTML format on the pasteboard 
    if (::OpenClipboard(m_owner)) {
        Vector<char> data;
        markupToCFHTML(createMarkup(selectedRange, 0, AnnotateForInterchange),
            selectedRange->startContainer()->document()->url().string(), data);
        HGLOBAL cbData = createGlobalData(data);
        if (!::SetClipboardData(HTMLClipboardFormat, cbData))
            ::GlobalFree(cbData);
        ::CloseClipboard();
    }
    
    // Put plain string on the pasteboard. CF_UNICODETEXT covers CF_TEXT as well
    String str = shouldSerializeSelectedTextForClipboard == IncludeImageAltTextForClipboard ? frame->editor().selectedTextForClipboard() : frame->editor().selectedText();
    replaceNewlinesWithWindowsStyleNewlines(str);
    replaceNBSPWithSpace(str);
    if (::OpenClipboard(m_owner)) {
        HGLOBAL cbData = createGlobalData(str);
        if (!::SetClipboardData(CF_UNICODETEXT, cbData))
            ::GlobalFree(cbData);
        ::CloseClipboard();
    }

    // enable smart-replacing later on by putting dummy data on the pasteboard
    if (canSmartCopyOrDelete) {
        if (::OpenClipboard(m_owner)) {
            ::SetClipboardData(WebSmartPasteFormat, 0);
            ::CloseClipboard();
        }
    }

    writeRangeToDataObject(selectedRange, frame);
}

void Pasteboard::writePlainTextToDataObject(const String& text, SmartReplaceOption smartReplaceOption)
{
    if (!m_writableDataObject)
        return;

    STGMEDIUM medium = {0};
    medium.tymed = TYMED_HGLOBAL;

    String str = text;
    replaceNewlinesWithWindowsStyleNewlines(str);
    replaceNBSPWithSpace(str);
    medium.hGlobal = createGlobalData(str);
    if (medium.hGlobal && FAILED(m_writableDataObject->SetData(plainTextWFormat(), &medium, TRUE)))
        ::GlobalFree(medium.hGlobal);        

    medium.hGlobal = 0;
}

void Pasteboard::writePlainText(const String& text, SmartReplaceOption smartReplaceOption)
{
    clear();

    // Put plain string on the pasteboard. CF_UNICODETEXT covers CF_TEXT as well
    String str = text;
    replaceNewlinesWithWindowsStyleNewlines(str);
    if (::OpenClipboard(m_owner)) {
        HGLOBAL cbData = createGlobalData(str);
        if (!::SetClipboardData(CF_UNICODETEXT, cbData))
            ::GlobalFree(cbData);
        ::CloseClipboard();
    }

    // enable smart-replacing later on by putting dummy data on the pasteboard
    if (smartReplaceOption == CanSmartReplace) {
        if (::OpenClipboard(m_owner)) {
            ::SetClipboardData(WebSmartPasteFormat, 0);
            ::CloseClipboard();
        }
    }

    writePlainTextToDataObject(text, smartReplaceOption);
}

#if !OS(WINCE)
static inline void pathRemoveBadFSCharacters(PWSTR psz, size_t length)
{
    size_t writeTo = 0;
    size_t readFrom = 0;
    while (readFrom < length) {
        UINT type = PathGetCharType(psz[readFrom]);
        if (!psz[readFrom] || type & (GCT_LFNCHAR | GCT_SHORTCHAR))
            psz[writeTo++] = psz[readFrom];

        readFrom++;
    }
    psz[writeTo] = 0;
}
#endif

static String filesystemPathFromUrlOrTitle(const String& url, const String& title, const UChar* extension, bool isLink)
{
#if OS(WINCE)
    notImplemented();
    return String();
#else
    static const size_t fsPathMaxLengthExcludingNullTerminator = MAX_PATH - 1;
    bool usedURL = false;
    WCHAR fsPathBuffer[MAX_PATH];
    fsPathBuffer[0] = 0;
    int extensionLen = extension ? lstrlen(extension) : 0;
    int fsPathMaxLengthExcludingExtension = fsPathMaxLengthExcludingNullTerminator - extensionLen;

    if (!title.isEmpty()) {
        size_t len = std::min<size_t>(title.length(), fsPathMaxLengthExcludingExtension);
        CopyMemory(fsPathBuffer, title.characters(), len * sizeof(UChar));
        fsPathBuffer[len] = 0;
        pathRemoveBadFSCharacters(fsPathBuffer, len);
    }

    if (!lstrlen(fsPathBuffer)) {
        KURL kurl(KURL(), url);
        usedURL = true;
        // The filename for any content based drag or file url should be the last element of 
        // the path. If we can't find it, or we're coming up with the name for a link
        // we just use the entire url.
        DWORD len = fsPathMaxLengthExcludingExtension;
        String lastComponent = kurl.lastPathComponent();
        if (kurl.isLocalFile() || (!isLink && !lastComponent.isEmpty())) {
            len = std::min<DWORD>(fsPathMaxLengthExcludingExtension, lastComponent.length());
            CopyMemory(fsPathBuffer, lastComponent.characters(), len * sizeof(UChar));
        } else {
            len = std::min<DWORD>(fsPathMaxLengthExcludingExtension, url.length());
            CopyMemory(fsPathBuffer, url.characters(), len * sizeof(UChar));
        }
        fsPathBuffer[len] = 0;
        pathRemoveBadFSCharacters(fsPathBuffer, len);
    }

    if (!extension)
        return String(static_cast<UChar*>(fsPathBuffer));

    if (!isLink && usedURL) {
        PathRenameExtension(fsPathBuffer, extension);
        return String(static_cast<UChar*>(fsPathBuffer));
    }

    return makeString(static_cast<const UChar*>(fsPathBuffer), extension);
#endif
}

// writeFileToDataObject takes ownership of fileDescriptor and fileContent
static HRESULT writeFileToDataObject(IDataObject* dataObject, HGLOBAL fileDescriptor, HGLOBAL fileContent, HGLOBAL hDropContent)
{
    HRESULT hr = S_OK;
    FORMATETC* fe;
    STGMEDIUM medium = {0};
    medium.tymed = TYMED_HGLOBAL;

    if (!fileDescriptor || !fileContent)
        goto exit;

    // Descriptor
    fe = fileDescriptorFormat();

    medium.hGlobal = fileDescriptor;

    if (FAILED(hr = dataObject->SetData(fe, &medium, TRUE)))
        goto exit;

    // Contents
    fe = fileContentFormatZero();
    medium.hGlobal = fileContent;
    if (FAILED(hr = dataObject->SetData(fe, &medium, TRUE)))
        goto exit;

#if USE(CF)
    // HDROP
    if (hDropContent) {
        medium.hGlobal = hDropContent;
        hr = dataObject->SetData(cfHDropFormat(), &medium, TRUE);
    }
#endif

exit:
    if (FAILED(hr)) {
        if (fileDescriptor)
            GlobalFree(fileDescriptor);
        if (fileContent)
            GlobalFree(fileContent);
        if (hDropContent)
            GlobalFree(hDropContent);
    }
    return hr;
}

void Pasteboard::writeURLToDataObject(const KURL& kurl, const String& titleStr, Frame* frame)
{
    if (!m_writableDataObject)
        return;
    WebCore::writeURL(m_writableDataObject.get(), kurl, titleStr, true, true);

    String url = kurl.string();
    ASSERT(url.containsOnlyASCII()); // KURL::string() is URL encoded.

    String fsPath = filesystemPathFromUrlOrTitle(url, titleStr, L".URL", true);
    String contentString("[InternetShortcut]\r\nURL=" + url + "\r\n");
    CString content = contentString.latin1();

    if (fsPath.length() <= 0)
        return;

    HGLOBAL urlFileDescriptor = GlobalAlloc(GPTR, sizeof(FILEGROUPDESCRIPTOR));
    if (!urlFileDescriptor)
        return;

    HGLOBAL urlFileContent = GlobalAlloc(GPTR, content.length());
    if (!urlFileContent) {
        GlobalFree(urlFileDescriptor);
        return;
    }

    FILEGROUPDESCRIPTOR* fgd = static_cast<FILEGROUPDESCRIPTOR*>(GlobalLock(urlFileDescriptor));
    ZeroMemory(fgd, sizeof(FILEGROUPDESCRIPTOR));
    fgd->cItems = 1;
    fgd->fgd[0].dwFlags = FD_FILESIZE;
    fgd->fgd[0].nFileSizeLow = content.length();

    unsigned maxSize = std::min<unsigned>(fsPath.length(), WTF_ARRAY_LENGTH(fgd->fgd[0].cFileName));
    CopyMemory(fgd->fgd[0].cFileName, fsPath.characters(), maxSize * sizeof(UChar));
    GlobalUnlock(urlFileDescriptor);

    char* fileContents = static_cast<char*>(GlobalLock(urlFileContent));
    CopyMemory(fileContents, content.data(), content.length());
    GlobalUnlock(urlFileContent);

    writeFileToDataObject(m_writableDataObject.get(), urlFileDescriptor, urlFileContent, 0);
}

void Pasteboard::writeURL(const KURL& url, const String& titleStr, Frame* frame)
{
    ASSERT(!url.isEmpty());

    clear();

    String title(titleStr);
    if (title.isEmpty()) {
        title = url.lastPathComponent();
        if (title.isEmpty())
            title = url.host();
    }

    // write to clipboard in format com.apple.safari.bookmarkdata to be able to paste into the bookmarks view with appropriate title
    if (::OpenClipboard(m_owner)) {
        HGLOBAL cbData = createGlobalData(url, title);
        if (!::SetClipboardData(BookmarkClipboardFormat, cbData))
            ::GlobalFree(cbData);
        ::CloseClipboard();
    }

    // write to clipboard in format CF_HTML to be able to paste into contenteditable areas as a link
    if (::OpenClipboard(m_owner)) {
        Vector<char> data;
        markupToCFHTML(urlToMarkup(url, title), "", data);
        HGLOBAL cbData = createGlobalData(data);
        if (!::SetClipboardData(HTMLClipboardFormat, cbData))
            ::GlobalFree(cbData);
        ::CloseClipboard();
    }

    // bare-bones CF_UNICODETEXT support
    if (::OpenClipboard(m_owner)) {
        HGLOBAL cbData = createGlobalData(url.string());
        if (!::SetClipboardData(CF_UNICODETEXT, cbData))
            ::GlobalFree(cbData);
        ::CloseClipboard();
    }

    writeURLToDataObject(url, titleStr, frame);
}

void Pasteboard::writeImage(Node* node, const KURL&, const String&)
{
    ASSERT(node);

    if (!(node->renderer() && node->renderer()->isImage()))
        return;

    RenderImage* renderer = toRenderImage(node->renderer());
    CachedImage* cachedImage = renderer->cachedImage();
    if (!cachedImage || cachedImage->errorOccurred())
        return;
    Image* image = cachedImage->imageForRenderer(renderer);
    ASSERT(image);

    clear();

    HWndDC dc(0);
    HDC compatibleDC = CreateCompatibleDC(0);
    HDC sourceDC = CreateCompatibleDC(0);
    OwnPtr<HBITMAP> resultBitmap = adoptPtr(CreateCompatibleBitmap(dc, image->width(), image->height()));
    HGDIOBJ oldBitmap = SelectObject(compatibleDC, resultBitmap.get());

    BitmapInfo bmInfo = BitmapInfo::create(image->size());

    HBITMAP coreBitmap = CreateDIBSection(dc, &bmInfo, DIB_RGB_COLORS, 0, 0, 0);
    HGDIOBJ oldSource = SelectObject(sourceDC, coreBitmap);
    image->getHBITMAP(coreBitmap);

    BitBlt(compatibleDC, 0, 0, image->width(), image->height(), sourceDC, 0, 0, SRCCOPY);

    SelectObject(sourceDC, oldSource);
    DeleteObject(coreBitmap);

    SelectObject(compatibleDC, oldBitmap);
    DeleteDC(sourceDC);
    DeleteDC(compatibleDC);

    if (::OpenClipboard(m_owner)) {
        ::SetClipboardData(CF_BITMAP, resultBitmap.leakPtr());
        ::CloseClipboard();
    }
}

void Pasteboard::writePasteboard(const Pasteboard& sourcePasteboard)
{
    notImplemented();
}

void Pasteboard::writeClipboard(Clipboard*)
{
    notImplemented();
}

bool Pasteboard::canSmartReplace()
{ 
    return ::IsClipboardFormatAvailable(WebSmartPasteFormat);
}

String Pasteboard::plainText(Frame* frame)
{
    if (::IsClipboardFormatAvailable(CF_UNICODETEXT) && ::OpenClipboard(m_owner)) {
        HANDLE cbData = ::GetClipboardData(CF_UNICODETEXT);
        if (cbData) {
            UChar* buffer = static_cast<UChar*>(GlobalLock(cbData));
            String fromClipboard(buffer);
            GlobalUnlock(cbData);
            ::CloseClipboard();
            return fromClipboard;
        }
        ::CloseClipboard();
    }

    if (::IsClipboardFormatAvailable(CF_TEXT) && ::OpenClipboard(m_owner)) {
        HANDLE cbData = ::GetClipboardData(CF_TEXT);
        if (cbData) {
            char* buffer = static_cast<char*>(GlobalLock(cbData));
            String fromClipboard(buffer);
            GlobalUnlock(cbData);
            ::CloseClipboard();
            return fromClipboard;
        }
        ::CloseClipboard();
    }

    return String();
}

PassRefPtr<DocumentFragment> Pasteboard::documentFragment(Frame* frame, PassRefPtr<Range> context, bool allowPlainText, bool& chosePlainText)
{
    chosePlainText = false;
    
    if (::IsClipboardFormatAvailable(HTMLClipboardFormat) && ::OpenClipboard(m_owner)) {
        // get data off of clipboard
        HANDLE cbData = ::GetClipboardData(HTMLClipboardFormat);
        if (cbData) {
            SIZE_T dataSize = ::GlobalSize(cbData);
            String cfhtml(UTF8Encoding().decode(static_cast<char*>(GlobalLock(cbData)), dataSize));
            GlobalUnlock(cbData);
            ::CloseClipboard();

            PassRefPtr<DocumentFragment> fragment = fragmentFromCFHTML(frame->document(), cfhtml);
            if (fragment)
                return fragment;
        } else 
            ::CloseClipboard();
    }
     
    if (allowPlainText && ::IsClipboardFormatAvailable(CF_UNICODETEXT)) {
        chosePlainText = true;
        if (::OpenClipboard(m_owner)) {
            HANDLE cbData = ::GetClipboardData(CF_UNICODETEXT);
            if (cbData) {
                UChar* buffer = static_cast<UChar*>(GlobalLock(cbData));
                String str(buffer);
                GlobalUnlock(cbData);
                ::CloseClipboard();
                RefPtr<DocumentFragment> fragment = createFragmentFromText(context.get(), str);
                if (fragment)
                    return fragment.release();
            } else 
                ::CloseClipboard();
        }
    }

    if (allowPlainText && ::IsClipboardFormatAvailable(CF_TEXT)) {
        chosePlainText = true;
        if (::OpenClipboard(m_owner)) {
            HANDLE cbData = ::GetClipboardData(CF_TEXT);
            if (cbData) {
                char* buffer = static_cast<char*>(GlobalLock(cbData));
                String str(buffer);
                GlobalUnlock(cbData);
                ::CloseClipboard();
                RefPtr<DocumentFragment> fragment = createFragmentFromText(context.get(), str);
                if (fragment)
                    return fragment.release();
            } else
                ::CloseClipboard();
        }
    }
    
    return 0;
}

void Pasteboard::setExternalDataObject(IDataObject *dataObject)
{
    m_writableDataObject = 0;
    m_dataObject = dataObject;
}

static CachedImage* getCachedImage(Element* element)
{
    // Attempt to pull CachedImage from element
    ASSERT(element);
    RenderObject* renderer = element->renderer();
    if (!renderer || !renderer->isImage()) 
        return 0;

    RenderImage* image = toRenderImage(renderer);
    if (image->cachedImage() && !image->cachedImage()->errorOccurred())
        return image->cachedImage();

    return 0;
}

static HGLOBAL createGlobalImageFileDescriptor(const String& url, const String& title, CachedImage* image)
{
    ASSERT_ARG(image, image);
    ASSERT(image->image()->data());

    HRESULT hr = S_OK;
    HGLOBAL memObj = 0;
    String fsPath;
    memObj = GlobalAlloc(GPTR, sizeof(FILEGROUPDESCRIPTOR));
    if (!memObj)
        return 0;

    FILEGROUPDESCRIPTOR* fgd = (FILEGROUPDESCRIPTOR*)GlobalLock(memObj);
    memset(fgd, 0, sizeof(FILEGROUPDESCRIPTOR));
    fgd->cItems = 1;
    fgd->fgd[0].dwFlags = FD_FILESIZE;
    fgd->fgd[0].nFileSizeLow = image->image()->data()->size();

    const String& preferredTitle = title.isEmpty() ? image->response().suggestedFilename() : title;
    String extension = image->image()->filenameExtension();
    if (extension.isEmpty()) {
        // Do not continue processing in the rare and unusual case where a decoded image is not able 
        // to provide a filename extension. Something tricky (like a bait-n-switch) is going on
        return 0;
    }
    extension.insert(".", 0);
    fsPath = filesystemPathFromUrlOrTitle(url, preferredTitle, extension.charactersWithNullTermination().data(), false);

    if (fsPath.length() <= 0) {
        GlobalUnlock(memObj);
        GlobalFree(memObj);
        return 0;
    }

    int maxSize = std::min<int>(fsPath.length(), WTF_ARRAY_LENGTH(fgd->fgd[0].cFileName));
    CopyMemory(fgd->fgd[0].cFileName, (LPCWSTR)fsPath.characters(), maxSize * sizeof(UChar));
    GlobalUnlock(memObj);

    return memObj;
}

static HGLOBAL createGlobalImageFileContent(SharedBuffer* data)
{
    HGLOBAL memObj = GlobalAlloc(GPTR, data->size());
    if (!memObj) 
        return 0;

    char* fileContents = (PSTR)GlobalLock(memObj);

    CopyMemory(fileContents, data->data(), data->size());

    GlobalUnlock(memObj);

    return memObj;
}

static HGLOBAL createGlobalHDropContent(const KURL& url, String& fileName, SharedBuffer* data)
{
    if (fileName.isEmpty() || !data)
        return 0;

    WCHAR filePath[MAX_PATH];

    if (url.isLocalFile()) {
        String localPath = decodeURLEscapeSequences(url.path());
        // windows does not enjoy a leading slash on paths
        if (localPath[0] == '/')
            localPath = localPath.substring(1);
        const Vector<UChar>& localPathWide = localPath.charactersWithNullTermination();
        LPCWSTR localPathStr = localPathWide.data();
        if (wcslen(localPathStr) + 1 < MAX_PATH)
            wcscpy_s(filePath, MAX_PATH, localPathStr);
        else
            return 0;
    } else {
#if OS(WINCE)
        notImplemented();
        return 0;
#else
        WCHAR tempPath[MAX_PATH];
        WCHAR extension[MAX_PATH];
        if (!::GetTempPath(WTF_ARRAY_LENGTH(tempPath), tempPath))
            return 0;
        if (!::PathAppend(tempPath, fileName.charactersWithNullTermination().data()))
            return 0;
        LPCWSTR foundExtension = ::PathFindExtension(tempPath);
        if (foundExtension) {
            if (wcscpy_s(extension, MAX_PATH, foundExtension))
                return 0;
        } else
            *extension = 0;
        ::PathRemoveExtension(tempPath);
        for (int i = 1; i < 10000; i++) {
            if (swprintf_s(filePath, MAX_PATH, TEXT("%s-%d%s"), tempPath, i, extension) == -1)
                return 0;
            if (!::PathFileExists(filePath))
                break;
        }
        HANDLE tempFileHandle = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (tempFileHandle == INVALID_HANDLE_VALUE)
            return 0;

        // Write the data to this temp file.
        DWORD written;
        BOOL tempWriteSucceeded = WriteFile(tempFileHandle, data->data(), data->size(), &written, 0);
        CloseHandle(tempFileHandle);
        if (!tempWriteSucceeded)
            return 0;
#endif
    }

    SIZE_T dropFilesSize = sizeof(DROPFILES) + (sizeof(WCHAR) * (wcslen(filePath) + 2));
    HGLOBAL memObj = GlobalAlloc(GHND | GMEM_SHARE, dropFilesSize);
    if (!memObj) 
        return 0;

    DROPFILES* dropFiles = (DROPFILES*) GlobalLock(memObj);
    dropFiles->pFiles = sizeof(DROPFILES);
    dropFiles->fWide = TRUE;
    wcscpy((LPWSTR)(dropFiles + 1), filePath);    
    GlobalUnlock(memObj);

    return memObj;
}

void Pasteboard::writeImageToDataObject(Element* element, const KURL& url)
{
    // Shove image data into a DataObject for use as a file
    CachedImage* cachedImage = getCachedImage(element);
    if (!cachedImage || !cachedImage->imageForRenderer(element->renderer()) || !cachedImage->isLoaded())
        return;

    SharedBuffer* imageBuffer = cachedImage->imageForRenderer(element->renderer())->data();
    if (!imageBuffer || !imageBuffer->size())
        return;

    HGLOBAL imageFileDescriptor = createGlobalImageFileDescriptor(url.string(), element->getAttribute(HTMLNames::altAttr), cachedImage);
    if (!imageFileDescriptor)
        return;

    HGLOBAL imageFileContent = createGlobalImageFileContent(imageBuffer);
    if (!imageFileContent) {
        GlobalFree(imageFileDescriptor);
        return;
    }

    String fileName = cachedImage->response().suggestedFilename();
    HGLOBAL hDropContent = createGlobalHDropContent(url, fileName, imageBuffer);
    if (!hDropContent) {
        GlobalFree(hDropContent);
        return;
    }

    writeFileToDataObject(m_writableDataObject.get(), imageFileDescriptor, imageFileContent, hDropContent);
}

void Pasteboard::writeURLToWritableDataObject(const KURL& url, const String& title)
{
    WebCore::writeURL(m_writableDataObject.get(), url, title, true, false);
}

} // namespace WebCore
