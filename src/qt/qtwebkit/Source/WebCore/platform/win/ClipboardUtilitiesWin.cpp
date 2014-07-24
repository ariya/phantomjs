/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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
#include "ClipboardUtilitiesWin.h"

#include "DocumentFragment.h"
#include "KURL.h"
#include "TextEncoding.h"
#include "markup.h"
#include <shlobj.h>
#include <wininet.h> // for INTERNET_MAX_URL_LENGTH
#include <wtf/StringExtras.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/WTFString.h>

#if !OS(WINCE)
#include <shlwapi.h>
#endif

#if USE(CF)
#include <CoreFoundation/CoreFoundation.h>
#include <wtf/RetainPtr.h>
#endif

namespace WebCore {

#if USE(CF)
FORMATETC* cfHDropFormat()
{
    static FORMATETC urlFormat = {CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    return &urlFormat;
}

static bool urlFromPath(CFStringRef path, String& url)
{
    if (!path)
        return false;

    RetainPtr<CFURLRef> cfURL = adoptCF(CFURLCreateWithFileSystemPath(0, path, kCFURLWindowsPathStyle, false));
    if (!cfURL)
        return false;

    url = CFURLGetString(cfURL.get());

    // Work around <rdar://problem/6708300>, where CFURLCreateWithFileSystemPath makes URLs with "localhost".
    if (url.startsWith("file://localhost/"))
        url.remove(7, 9);

    return true;
}
#endif

static bool getDataMapItem(const DragDataMap* dataObject, FORMATETC* format, String& item)
{
    DragDataMap::const_iterator found = dataObject->find(format->cfFormat);
    if (found == dataObject->end())
        return false;
    item = found->value[0];
    return true;
}

static bool getWebLocData(IDataObject* dataObject, String& url, String* title) 
{
    bool succeeded = false;
#if USE(CF)
    WCHAR filename[MAX_PATH];
    WCHAR urlBuffer[INTERNET_MAX_URL_LENGTH];

    STGMEDIUM medium;
    if (FAILED(dataObject->GetData(cfHDropFormat(), &medium)))
        return false;

    HDROP hdrop = static_cast<HDROP>(GlobalLock(medium.hGlobal));

    if (!hdrop)
        return false;

    if (!DragQueryFileW(hdrop, 0, filename, WTF_ARRAY_LENGTH(filename)))
        goto exit;

    if (_wcsicmp(PathFindExtensionW(filename), L".url"))
        goto exit;    
    
    if (!GetPrivateProfileStringW(L"InternetShortcut", L"url", 0, urlBuffer, WTF_ARRAY_LENGTH(urlBuffer), filename))
        goto exit;
    
    if (title) {
        PathRemoveExtension(filename);
        *title = String((UChar*)filename);
    }
    
    url = String((UChar*)urlBuffer);
    succeeded = true;

exit:
    // Free up memory.
    DragFinish(hdrop);
    GlobalUnlock(medium.hGlobal);
#endif
    return succeeded;
}

static bool getWebLocData(const DragDataMap* dataObject, String& url, String* title) 
{
#if USE(CF)
    WCHAR filename[MAX_PATH];
    WCHAR urlBuffer[INTERNET_MAX_URL_LENGTH];

    if (!dataObject->contains(cfHDropFormat()->cfFormat))
        return false;

    wcscpy(filename, dataObject->get(cfHDropFormat()->cfFormat)[0].charactersWithNullTermination().data());
    if (_wcsicmp(PathFindExtensionW(filename), L".url"))
        return false;    

    if (!GetPrivateProfileStringW(L"InternetShortcut", L"url", 0, urlBuffer, WTF_ARRAY_LENGTH(urlBuffer), filename))
        return false;

    if (title) {
        PathRemoveExtension(filename);
        *title = filename;
    }
    
    url = urlBuffer;
    return true;
#else
    return false;
#endif
}

static String extractURL(const String &inURL, String* title)
{
    String url = inURL;
    int splitLoc = url.find('\n');
    if (splitLoc > 0) {
        if (title)
            *title = url.substring(splitLoc+1);
        url.truncate(splitLoc);
    } else if (title)
        *title = url;
    return url;
}

// Firefox text/html
static FORMATETC* texthtmlFormat() 
{
    static UINT cf = RegisterClipboardFormat(L"text/html");
    static FORMATETC texthtmlFormat = {cf, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    return &texthtmlFormat;
}

HGLOBAL createGlobalData(const KURL& url, const String& title)
{
    String mutableURL(url.string());
    String mutableTitle(title);
    SIZE_T size = mutableURL.length() + mutableTitle.length() + 2; // +1 for "\n" and +1 for null terminator
    HGLOBAL cbData = ::GlobalAlloc(GPTR, size * sizeof(UChar));

    if (cbData) {
        PWSTR buffer = static_cast<PWSTR>(GlobalLock(cbData));
        _snwprintf(buffer, size, L"%s\n%s", mutableURL.charactersWithNullTermination().data(), mutableTitle.charactersWithNullTermination().data());
        GlobalUnlock(cbData);
    }
    return cbData;
}

HGLOBAL createGlobalData(const String& str)
{
    HGLOBAL vm = ::GlobalAlloc(GPTR, (str.length() + 1) * sizeof(UChar));
    if (!vm)
        return 0;
    UChar* buffer = static_cast<UChar*>(GlobalLock(vm));
    memcpy(buffer, str.characters(), str.length() * sizeof(UChar));
    buffer[str.length()] = 0;
    GlobalUnlock(vm);
    return vm;
}

HGLOBAL createGlobalData(const Vector<char>& vector)
{
    HGLOBAL vm = ::GlobalAlloc(GPTR, vector.size() + 1);
    if (!vm)
        return 0;
    char* buffer = static_cast<char*>(GlobalLock(vm));
    memcpy(buffer, vector.data(), vector.size());
    buffer[vector.size()] = 0;
    GlobalUnlock(vm);
    return vm;
}

static String getFullCFHTML(IDataObject* data)
{
    STGMEDIUM store;
    if (SUCCEEDED(data->GetData(htmlFormat(), &store))) {
        // MS HTML Format parsing
        char* data = static_cast<char*>(GlobalLock(store.hGlobal));
        SIZE_T dataSize = ::GlobalSize(store.hGlobal);
        String cfhtml(UTF8Encoding().decode(data, dataSize));
        GlobalUnlock(store.hGlobal);
        ReleaseStgMedium(&store);
        return cfhtml;
    }
    return String();
}

static void append(Vector<char>& vector, const char* string)
{
    vector.append(string, strlen(string));
}

static void append(Vector<char>& vector, const CString& string)
{
    vector.append(string.data(), string.length());
}

// Find the markup between "<!--StartFragment -->" and "<!--EndFragment -->", accounting for browser quirks.
static String extractMarkupFromCFHTML(const String& cfhtml)
{
    unsigned markupStart = cfhtml.find("<html", 0, false);
    unsigned tagStart = cfhtml.find("startfragment", markupStart, false);
    unsigned fragmentStart = cfhtml.find('>', tagStart) + 1;
    unsigned tagEnd = cfhtml.find("endfragment", fragmentStart, false);
    unsigned fragmentEnd = cfhtml.reverseFind('<', tagEnd);
    return cfhtml.substring(fragmentStart, fragmentEnd - fragmentStart).stripWhiteSpace();
}

// Documentation for the CF_HTML format is available at http://msdn.microsoft.com/workshop/networking/clipboard/htmlclipboard.asp
void markupToCFHTML(const String& markup, const String& srcURL, Vector<char>& result)
{
    if (markup.isEmpty())
        return;

    #define MAX_DIGITS 10
    #define MAKE_NUMBER_FORMAT_1(digits) MAKE_NUMBER_FORMAT_2(digits)
    #define MAKE_NUMBER_FORMAT_2(digits) "%0" #digits "u"
    #define NUMBER_FORMAT MAKE_NUMBER_FORMAT_1(MAX_DIGITS)

    const char* header = "Version:0.9\n"
        "StartHTML:" NUMBER_FORMAT "\n"
        "EndHTML:" NUMBER_FORMAT "\n"
        "StartFragment:" NUMBER_FORMAT "\n"
        "EndFragment:" NUMBER_FORMAT "\n";
    const char* sourceURLPrefix = "SourceURL:";

    const char* startMarkup = "<HTML>\n<BODY>\n<!--StartFragment-->\n";
    const char* endMarkup = "\n<!--EndFragment-->\n</BODY>\n</HTML>";

    CString sourceURLUTF8 = srcURL == blankURL() ? "" : srcURL.utf8();
    CString markupUTF8 = markup.utf8();

    // calculate offsets
    unsigned startHTMLOffset = strlen(header) - strlen(NUMBER_FORMAT) * 4 + MAX_DIGITS * 4;
    if (sourceURLUTF8.length())
        startHTMLOffset += strlen(sourceURLPrefix) + sourceURLUTF8.length() + 1;
    unsigned startFragmentOffset = startHTMLOffset + strlen(startMarkup);
    unsigned endFragmentOffset = startFragmentOffset + markupUTF8.length();
    unsigned endHTMLOffset = endFragmentOffset + strlen(endMarkup);

    unsigned headerBufferLength = startHTMLOffset + 1; // + 1 for '\0' terminator.
    char* headerBuffer = (char*)malloc(headerBufferLength);
    snprintf(headerBuffer, headerBufferLength, header, startHTMLOffset, endHTMLOffset, startFragmentOffset, endFragmentOffset);
    append(result, CString(headerBuffer));
    free(headerBuffer);
    if (sourceURLUTF8.length()) {
        append(result, sourceURLPrefix);
        append(result, sourceURLUTF8);
        result.append('\n');
    }
    append(result, startMarkup);
    append(result, markupUTF8);
    append(result, endMarkup);

    #undef MAX_DIGITS
    #undef MAKE_NUMBER_FORMAT_1
    #undef MAKE_NUMBER_FORMAT_2
    #undef NUMBER_FORMAT
}

void replaceNewlinesWithWindowsStyleNewlines(String& str)
{
    DEFINE_STATIC_LOCAL(String, windowsNewline, (ASCIILiteral("\r\n")));
    StringBuilder result;
    for (unsigned index = 0; index < str.length(); ++index) {
        if (str[index] != '\n' || (index > 0 && str[index - 1] == '\r'))
            result.append(str[index]);
        else
            result.append(windowsNewline);
    }
    str = result.toString();
}

void replaceNBSPWithSpace(String& str)
{
    static const UChar NonBreakingSpaceCharacter = 0xA0;
    static const UChar SpaceCharacter = ' ';
    str.replace(NonBreakingSpaceCharacter, SpaceCharacter);
}

FORMATETC* urlWFormat()
{
    static UINT cf = RegisterClipboardFormat(L"UniformResourceLocatorW");
    static FORMATETC urlFormat = {cf, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    return &urlFormat;
}

FORMATETC* urlFormat()
{
    static UINT cf = RegisterClipboardFormat(L"UniformResourceLocator");
    static FORMATETC urlFormat = {cf, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    return &urlFormat;
}

FORMATETC* plainTextFormat()
{
    static FORMATETC textFormat = {CF_TEXT, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    return &textFormat;
}

FORMATETC* plainTextWFormat()
{
    static FORMATETC textFormat = {CF_UNICODETEXT, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    return &textFormat;
}

FORMATETC* filenameWFormat()
{
    static UINT cf = RegisterClipboardFormat(L"FileNameW");
    static FORMATETC urlFormat = {cf, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    return &urlFormat;
}

FORMATETC* filenameFormat()
{
    static UINT cf = RegisterClipboardFormat(L"FileName");
    static FORMATETC urlFormat = {cf, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    return &urlFormat;
}

// MSIE HTML Format
FORMATETC* htmlFormat() 
{
    static UINT cf = RegisterClipboardFormat(L"HTML Format");
    static FORMATETC htmlFormat = {cf, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    return &htmlFormat;
}

FORMATETC* smartPasteFormat()
{
    static UINT cf = RegisterClipboardFormat(L"WebKit Smart Paste Format");
    static FORMATETC htmlFormat = {cf, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    return &htmlFormat;
}

FORMATETC* fileDescriptorFormat()
{
    static UINT cf = RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR);
    static FORMATETC fileDescriptorFormat = { cf, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    return &fileDescriptorFormat;
}

FORMATETC* fileContentFormatZero()
{
    static UINT cf = RegisterClipboardFormat(CFSTR_FILECONTENTS);
    static FORMATETC fileContentFormat = { cf, 0, DVASPECT_CONTENT, 0, TYMED_HGLOBAL };
    return &fileContentFormat;
}

void getFileDescriptorData(IDataObject* dataObject, int& size, String& pathname)
{
    STGMEDIUM store;
    size = 0;
    if (FAILED(dataObject->GetData(fileDescriptorFormat(), &store)))
        return;

    FILEGROUPDESCRIPTOR* fgd = static_cast<FILEGROUPDESCRIPTOR*>(GlobalLock(store.hGlobal));
    size = fgd->fgd[0].nFileSizeLow;
    pathname = fgd->fgd[0].cFileName;

    GlobalUnlock(store.hGlobal);
    ::ReleaseStgMedium(&store);
}

void getFileContentData(IDataObject* dataObject, int size, void* dataBlob)
{
    STGMEDIUM store;
    if (FAILED(dataObject->GetData(fileContentFormatZero(), &store)))
        return;
    void* data = GlobalLock(store.hGlobal);
    ::CopyMemory(dataBlob, data, size);

    GlobalUnlock(store.hGlobal);
    ::ReleaseStgMedium(&store);
}

void setFileDescriptorData(IDataObject* dataObject, int size, const String& passedPathname)
{
    String pathname = passedPathname;

    STGMEDIUM medium = { 0 };
    medium.tymed = TYMED_HGLOBAL;

    medium.hGlobal = ::GlobalAlloc(GPTR, sizeof(FILEGROUPDESCRIPTOR));
    if (!medium.hGlobal)
        return;

    FILEGROUPDESCRIPTOR* fgd = static_cast<FILEGROUPDESCRIPTOR*>(GlobalLock(medium.hGlobal));
    ::ZeroMemory(fgd, sizeof(FILEGROUPDESCRIPTOR));
    fgd->cItems = 1;
    fgd->fgd[0].dwFlags = FD_FILESIZE;
    fgd->fgd[0].nFileSizeLow = size;

    int maxSize = std::min<int>(pathname.length(), WTF_ARRAY_LENGTH(fgd->fgd[0].cFileName));
    CopyMemory(fgd->fgd[0].cFileName, pathname.charactersWithNullTermination().data(), maxSize * sizeof(UChar));
    GlobalUnlock(medium.hGlobal);

    dataObject->SetData(fileDescriptorFormat(), &medium, TRUE);
}

void setFileContentData(IDataObject* dataObject, int size, void* dataBlob)
{
    STGMEDIUM medium = { 0 };
    medium.tymed = TYMED_HGLOBAL;

    medium.hGlobal = ::GlobalAlloc(GPTR, size);
    if (!medium.hGlobal)
        return;
    void* fileContents = GlobalLock(medium.hGlobal);
    ::CopyMemory(fileContents, dataBlob, size);
    GlobalUnlock(medium.hGlobal);

    dataObject->SetData(fileContentFormatZero(), &medium, TRUE);
}

String getURL(IDataObject* dataObject, DragData::FilenameConversionPolicy filenamePolicy, String* title)
{
    STGMEDIUM store;
    String url;
    if (getWebLocData(dataObject, url, title))
        return url;

    if (SUCCEEDED(dataObject->GetData(urlWFormat(), &store))) {
        // URL using Unicode
        UChar* data = static_cast<UChar*>(GlobalLock(store.hGlobal));
        url = extractURL(String(data), title);
        GlobalUnlock(store.hGlobal);
        ReleaseStgMedium(&store);
    } else if (SUCCEEDED(dataObject->GetData(urlFormat(), &store))) {
        // URL using ASCII
        char* data = static_cast<char*>(GlobalLock(store.hGlobal));
        url = extractURL(String(data), title);
        GlobalUnlock(store.hGlobal);
        ReleaseStgMedium(&store);
    }
#if USE(CF)
    else if (filenamePolicy == DragData::ConvertFilenames) {
        if (SUCCEEDED(dataObject->GetData(filenameWFormat(), &store))) {
            // file using unicode
            wchar_t* data = static_cast<wchar_t*>(GlobalLock(store.hGlobal));
            if (data && data[0] && (PathFileExists(data) || PathIsUNC(data))) {
                RetainPtr<CFStringRef> pathAsCFString = adoptCF(CFStringCreateWithCharacters(kCFAllocatorDefault, (const UniChar*)data, wcslen(data)));
                if (urlFromPath(pathAsCFString.get(), url) && title)
                    *title = url;
            }
            GlobalUnlock(store.hGlobal);
            ReleaseStgMedium(&store);
        } else if (SUCCEEDED(dataObject->GetData(filenameFormat(), &store))) {
            // filename using ascii
            char* data = static_cast<char*>(GlobalLock(store.hGlobal));
            if (data && data[0] && (PathFileExistsA(data) || PathIsUNCA(data))) {
                RetainPtr<CFStringRef> pathAsCFString = adoptCF(CFStringCreateWithCString(kCFAllocatorDefault, data, kCFStringEncodingASCII));
                if (urlFromPath(pathAsCFString.get(), url) && title)
                    *title = url;
            }
            GlobalUnlock(store.hGlobal);
            ReleaseStgMedium(&store);
        }
    }
#endif
    return url;
}

String getURL(const DragDataMap* data, DragData::FilenameConversionPolicy filenamePolicy, String* title)
{
    String url;

    if (getWebLocData(data, url, title))
        return url;
    if (getDataMapItem(data, urlWFormat(), url))
        return extractURL(url, title);
    if (getDataMapItem(data, urlFormat(), url))
        return extractURL(url, title);
#if USE(CF)
    if (filenamePolicy != DragData::ConvertFilenames)
        return url;

    String stringData;
    if (!getDataMapItem(data, filenameWFormat(), stringData))
        getDataMapItem(data, filenameFormat(), stringData);

    if (stringData.isEmpty() || (!PathFileExists(stringData.charactersWithNullTermination().data()) && !PathIsUNC(stringData.charactersWithNullTermination().data())))
        return url;
    RetainPtr<CFStringRef> pathAsCFString = adoptCF(CFStringCreateWithCharacters(kCFAllocatorDefault, (const UniChar *)stringData.charactersWithNullTermination().data(), wcslen(stringData.charactersWithNullTermination().data())));
    if (urlFromPath(pathAsCFString.get(), url) && title)
        *title = url;
#endif
    return url;
}

String getPlainText(IDataObject* dataObject)
{
    STGMEDIUM store;
    String text;
    if (SUCCEEDED(dataObject->GetData(plainTextWFormat(), &store))) {
        // Unicode text
        UChar* data = static_cast<UChar*>(GlobalLock(store.hGlobal));
        text = String(data);
        GlobalUnlock(store.hGlobal);
        ReleaseStgMedium(&store);
    } else if (SUCCEEDED(dataObject->GetData(plainTextFormat(), &store))) {
        // ASCII text
        char* data = static_cast<char*>(GlobalLock(store.hGlobal));
        text = String(data);
        GlobalUnlock(store.hGlobal);
        ReleaseStgMedium(&store);
    } else {
        // FIXME: Originally, we called getURL() here because dragging and dropping files doesn't
        // populate the drag with text data. Per https://bugs.webkit.org/show_bug.cgi?id=38826, this
        // is undesirable, so maybe this line can be removed.
        text = getURL(dataObject, DragData::DoNotConvertFilenames);
    }
    return text;
}

String getPlainText(const DragDataMap* data)
{
    String text;
    
    if (getDataMapItem(data, plainTextWFormat(), text))
        return text;
    if (getDataMapItem(data, plainTextFormat(), text))
        return text;
    return getURL(data, DragData::DoNotConvertFilenames);
}

String getTextHTML(IDataObject* data)
{
    STGMEDIUM store;
    String html;
    if (SUCCEEDED(data->GetData(texthtmlFormat(), &store))) {
        UChar* data = static_cast<UChar*>(GlobalLock(store.hGlobal));
        html = String(data);
        GlobalUnlock(store.hGlobal);
        ReleaseStgMedium(&store);
    }
    return html;
}

String getTextHTML(const DragDataMap* data)
{
    String text;
    getDataMapItem(data, texthtmlFormat(), text);
    return text;
}

String getCFHTML(IDataObject* data)
{
    String cfhtml = getFullCFHTML(data);
    if (!cfhtml.isEmpty())
        return extractMarkupFromCFHTML(cfhtml);
    return String();
}

String getCFHTML(const DragDataMap* dataMap)
{
    String cfhtml;
    getDataMapItem(dataMap, htmlFormat(), cfhtml);
    return extractMarkupFromCFHTML(cfhtml);
}

PassRefPtr<DocumentFragment> fragmentFromFilenames(Document*, const IDataObject*)
{
    // FIXME: We should be able to create fragments from files
    return 0;
}

PassRefPtr<DocumentFragment> fragmentFromFilenames(Document*, const DragDataMap*)
{
    // FIXME: We should be able to create fragments from files
    return 0;
}

bool containsFilenames(const IDataObject*)
{
    // FIXME: We'll want to update this once we can produce fragments from files
    return false;
}

bool containsFilenames(const DragDataMap*)
{
    // FIXME: We'll want to update this once we can produce fragments from files
    return false;
}

// Convert a String containing CF_HTML formatted text to a DocumentFragment
PassRefPtr<DocumentFragment> fragmentFromCFHTML(Document* doc, const String& cfhtml)
{
    // obtain baseURL if present
    String srcURLStr("sourceURL:");
    String srcURL;
    unsigned lineStart = cfhtml.find(srcURLStr, 0, false);
    if (lineStart != -1) {
        unsigned srcEnd = cfhtml.find("\n", lineStart, false);
        unsigned srcStart = lineStart+srcURLStr.length();
        String rawSrcURL = cfhtml.substring(srcStart, srcEnd-srcStart);
        replaceNBSPWithSpace(rawSrcURL);
        srcURL = rawSrcURL.stripWhiteSpace();
    }

    String markup = extractMarkupFromCFHTML(cfhtml);
    return createFragmentFromMarkup(doc, markup, srcURL, DisallowScriptingAndPluginContent);
}

PassRefPtr<DocumentFragment> fragmentFromHTML(Document* doc, IDataObject* data) 
{
    if (!doc || !data)
        return 0;

    String cfhtml = getFullCFHTML(data);
    if (!cfhtml.isEmpty()) {
        if (RefPtr<DocumentFragment> fragment = fragmentFromCFHTML(doc, cfhtml))
            return fragment.release();
    }

    String html = getTextHTML(data);
    String srcURL;
    if (!html.isEmpty())
        return createFragmentFromMarkup(doc, html, srcURL, DisallowScriptingAndPluginContent);

    return 0;
}

PassRefPtr<DocumentFragment> fragmentFromHTML(Document* document, const DragDataMap* data) 
{
    if (!document || !data || data->isEmpty())
        return 0;

    String stringData;
    if (getDataMapItem(data, htmlFormat(), stringData)) {
        if (RefPtr<DocumentFragment> fragment = fragmentFromCFHTML(document, stringData))
            return fragment.release();
    }

    String srcURL;
    if (getDataMapItem(data, texthtmlFormat(), stringData))
        return createFragmentFromMarkup(document, stringData, srcURL, DisallowScriptingAndPluginContent);

    return 0;
}

bool containsHTML(IDataObject* data)
{
    return SUCCEEDED(data->QueryGetData(texthtmlFormat())) || SUCCEEDED(data->QueryGetData(htmlFormat()));
}

bool containsHTML(const DragDataMap* data)
{
    return data->contains(texthtmlFormat()->cfFormat) || data->contains(htmlFormat()->cfFormat);
}

typedef void (*GetStringFunction)(IDataObject*, FORMATETC*, Vector<String>&);
typedef void (*SetStringFunction)(IDataObject*, FORMATETC*, const Vector<String>&);

struct ClipboardDataItem {
    GetStringFunction getString;
    SetStringFunction setString;
    FORMATETC* format;

    ClipboardDataItem(FORMATETC* format, GetStringFunction getString, SetStringFunction setString): format(format), getString(getString), setString(setString) { }
};

typedef HashMap<UINT, ClipboardDataItem*> ClipboardFormatMap;

// Getter functions.

template<typename T> void getStringData(IDataObject* data, FORMATETC* format, Vector<String>& dataStrings)
{
    STGMEDIUM store;
    if (FAILED(data->GetData(format, &store)))
        return;
    dataStrings.append(String(static_cast<T*>(GlobalLock(store.hGlobal)), ::GlobalSize(store.hGlobal) / sizeof(T)));
    GlobalUnlock(store.hGlobal);
    ReleaseStgMedium(&store);
}

void getUtf8Data(IDataObject* data, FORMATETC* format, Vector<String>& dataStrings)
{
    STGMEDIUM store;
    if (FAILED(data->GetData(format, &store)))
        return;
    dataStrings.append(String(UTF8Encoding().decode(static_cast<char*>(GlobalLock(store.hGlobal)), GlobalSize(store.hGlobal))));
    GlobalUnlock(store.hGlobal);
    ReleaseStgMedium(&store);
}

#if USE(CF)
void getCFData(IDataObject* data, FORMATETC* format, Vector<String>& dataStrings)
{
    STGMEDIUM store;
    if (FAILED(data->GetData(format, &store)))
        return;

    HDROP hdrop = reinterpret_cast<HDROP>(GlobalLock(store.hGlobal));
    if (!hdrop)
        return;

    WCHAR filename[MAX_PATH];
    UINT fileCount = DragQueryFileW(hdrop, 0xFFFFFFFF, 0, 0);
    for (UINT i = 0; i < fileCount; i++) {
        if (!DragQueryFileW(hdrop, i, filename, WTF_ARRAY_LENGTH(filename)))
            continue;
        dataStrings.append(static_cast<UChar*>(filename));
    }

    GlobalUnlock(store.hGlobal);
    ReleaseStgMedium(&store);
}
#endif

// Setter functions.

void setUCharData(IDataObject* data, FORMATETC* format, const Vector<String>& dataStrings)
{
    STGMEDIUM medium = {0};
    medium.tymed = TYMED_HGLOBAL;

    medium.hGlobal = createGlobalData(dataStrings.first());
    if (!medium.hGlobal)
        return;
    data->SetData(format, &medium, FALSE);
    ::GlobalFree(medium.hGlobal);
}

void setUtf8Data(IDataObject* data, FORMATETC* format, const Vector<String>& dataStrings)
{
    STGMEDIUM medium = {0};
    medium.tymed = TYMED_HGLOBAL;

    CString charString = dataStrings.first().utf8();
    size_t stringLength = charString.length();
    medium.hGlobal = ::GlobalAlloc(GPTR, stringLength + 1);
    if (!medium.hGlobal)
        return;
    char* buffer = static_cast<char*>(GlobalLock(medium.hGlobal));
    memcpy(buffer, charString.data(), stringLength);
    buffer[stringLength] = 0;
    GlobalUnlock(medium.hGlobal);
    data->SetData(format, &medium, FALSE);
    ::GlobalFree(medium.hGlobal);
}

#if USE(CF)
void setCFData(IDataObject* data, FORMATETC* format, const Vector<String>& dataStrings)
{
    STGMEDIUM medium = {0};
    medium.tymed = TYMED_HGLOBAL;

    SIZE_T dropFilesSize = sizeof(DROPFILES) + (sizeof(WCHAR) * (dataStrings.first().length() + 2));
    medium.hGlobal = ::GlobalAlloc(GHND | GMEM_SHARE, dropFilesSize);
    if (!medium.hGlobal) 
        return;

    DROPFILES* dropFiles = reinterpret_cast<DROPFILES *>(GlobalLock(medium.hGlobal));
    dropFiles->pFiles = sizeof(DROPFILES);
    dropFiles->fWide = TRUE;
    String filename = dataStrings.first();
    wcscpy(reinterpret_cast<LPWSTR>(dropFiles + 1), filename.charactersWithNullTermination().data());    
    GlobalUnlock(medium.hGlobal);
    data->SetData(format, &medium, FALSE);
    ::GlobalFree(medium.hGlobal);
}
#endif

static const ClipboardFormatMap& getClipboardMap()
{
    static ClipboardFormatMap formatMap;
    if (formatMap.isEmpty()) {
        formatMap.add(htmlFormat()->cfFormat, new ClipboardDataItem(htmlFormat(), getUtf8Data, setUtf8Data));
        formatMap.add(texthtmlFormat()->cfFormat, new ClipboardDataItem(texthtmlFormat(), getStringData<UChar>, setUCharData));
        formatMap.add(plainTextFormat()->cfFormat,  new ClipboardDataItem(plainTextFormat(), getStringData<char>, setUtf8Data));
        formatMap.add(plainTextWFormat()->cfFormat,  new ClipboardDataItem(plainTextWFormat(), getStringData<UChar>, setUCharData));
#if USE(CF)
        formatMap.add(cfHDropFormat()->cfFormat,  new ClipboardDataItem(cfHDropFormat(), getCFData, setCFData));
#endif
        formatMap.add(filenameFormat()->cfFormat,  new ClipboardDataItem(filenameFormat(), getStringData<char>, setUtf8Data));
        formatMap.add(filenameWFormat()->cfFormat,  new ClipboardDataItem(filenameWFormat(), getStringData<UChar>, setUCharData));
        formatMap.add(urlFormat()->cfFormat,  new ClipboardDataItem(urlFormat(), getStringData<char>, setUtf8Data));
        formatMap.add(urlWFormat()->cfFormat,  new ClipboardDataItem(urlWFormat(), getStringData<UChar>, setUCharData));
    }
    return formatMap;
}

void getClipboardData(IDataObject* dataObject, FORMATETC* format, Vector<String>& dataStrings)
{
    const ClipboardFormatMap& formatMap = getClipboardMap();
    ClipboardFormatMap::const_iterator found = formatMap.find(format->cfFormat);
    if (found == formatMap.end())
        return;
    found->value->getString(dataObject, found->value->format, dataStrings);
}

void setClipboardData(IDataObject* dataObject, UINT format, const Vector<String>& dataStrings)
{
    const ClipboardFormatMap& formatMap = getClipboardMap();
    ClipboardFormatMap::const_iterator found = formatMap.find(format);
    if (found == formatMap.end())
        return;
    found->value->setString(dataObject, found->value->format, dataStrings);
}

} // namespace WebCore
