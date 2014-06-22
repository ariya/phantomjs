/*
 * Copyright (C) 2005, 2006, 2007, 2008, 2009 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DumpRenderTree.h"

#include "EditingDelegate.h"
#include "FrameLoadDelegate.h"
#include "HistoryDelegate.h"
#include "JavaScriptThreading.h"
#include "PixelDumpSupport.h"
#include "PolicyDelegate.h"
#include "ResourceLoadDelegate.h"
#include "TestRunner.h"
#include "UIDelegate.h"
#include "WebCoreTestSupport.h"
#include "WorkQueueItem.h"
#include "WorkQueue.h"

#include <comutil.h>
#include <fcntl.h>
#include <io.h>
#include <math.h>
#include <shlwapi.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <wtf/RetainPtr.h>
#include <wtf/Vector.h>
#include <windows.h>
#include <CoreFoundation/CoreFoundation.h>
#include <WebCore/FileSystem.h>
#include <WebKit/WebKit.h>
#include <WebKit/WebKitCOMAPI.h>

#if USE(CFNETWORK)
#include <CFNetwork/CFHTTPCookiesPriv.h>
#include <CFNetwork/CFURLCachePriv.h>
#endif

using namespace std;

#ifdef DEBUG_ALL
const LPWSTR TestPluginDir = L"TestNetscapePlugin_Debug";
#else
const LPWSTR TestPluginDir = L"TestNetscapePlugin";
#endif

static LPCWSTR fontsEnvironmentVariable = L"WEBKIT_TESTFONTS";
static LPCWSTR dumpRenderTreeTemp = L"DUMPRENDERTREE_TEMP";
#define USE_MAC_FONTS

static CFStringRef WebDatabaseDirectoryDefaultsKey = CFSTR("WebDatabaseDirectory");
static CFStringRef WebKitLocalCacheDefaultsKey = CFSTR("WebKitLocalCache");
static CFStringRef WebStorageDirectoryDefaultsKey = CFSTR("WebKitLocalStorageDatabasePathPreferenceKey");

const LPCWSTR kDumpRenderTreeClassName = L"DumpRenderTreeWindow";

static bool dumpTree = true;
static bool dumpPixelsForAllTests = false;
static bool dumpPixelsForCurrentTest;
static bool dumpAllPixels;
static bool printSeparators;
static bool leakChecking = false;
static bool threaded = false;
static bool forceComplexText = false;
static bool printSupportedFeatures = false;
static RetainPtr<CFStringRef> persistentUserStyleSheetLocation;

volatile bool done;
// This is the topmost frame that is loading, during a given load, or nil when no load is 
// in progress.  Usually this is the same as the main frame, but not always.  In the case
// where a frameset is loaded, and then new content is loaded into one of the child frames,
// that child frame is the "topmost frame that is loading".
IWebFrame* topLoadingFrame;     // !nil iff a load is in progress
static COMPtr<IWebHistoryItem> prevTestBFItem;  // current b/f item at the end of the previous test
PolicyDelegate* policyDelegate; 
COMPtr<FrameLoadDelegate> sharedFrameLoadDelegate;
COMPtr<UIDelegate> sharedUIDelegate;
COMPtr<EditingDelegate> sharedEditingDelegate;
COMPtr<HistoryDelegate> sharedHistoryDelegate;

IWebFrame* frame;
HWND webViewWindow;

RefPtr<TestRunner> gTestRunner;

UINT_PTR waitToDumpWatchdog = 0;

void setPersistentUserStyleSheetLocation(CFStringRef url)
{
    persistentUserStyleSheetLocation = url;
}

bool setAlwaysAcceptCookies(bool alwaysAcceptCookies)
{
#if USE(CFNETWORK)
    COMPtr<IWebCookieManager> cookieManager;
    if (FAILED(WebKitCreateInstance(CLSID_WebCookieManager, 0, IID_IWebCookieManager, reinterpret_cast<void**>(&cookieManager))))
        return false;
    CFHTTPCookieStorageRef cookieStorage = 0;
    if (FAILED(cookieManager->cookieStorage(&cookieStorage)) || !cookieStorage)
        return false;

    WebKitCookieStorageAcceptPolicy cookieAcceptPolicy = alwaysAcceptCookies ? WebKitCookieStorageAcceptPolicyAlways : WebKitCookieStorageAcceptPolicyOnlyFromMainDocumentDomain;
    CFHTTPCookieStorageSetCookieAcceptPolicy(cookieStorage, cookieAcceptPolicy);
    return true;
#else
    // FIXME: Implement!
    return false;
#endif
}

static RetainPtr<CFStringRef> substringFromIndex(CFStringRef string, CFIndex index)
{
    return adoptCF(CFStringCreateWithSubstring(kCFAllocatorDefault, string, CFRangeMake(index, CFStringGetLength(string) - index)));
}

wstring urlSuitableForTestResult(const wstring& urlString)
{
    RetainPtr<CFURLRef> url = adoptCF(CFURLCreateWithBytes(kCFAllocatorDefault, reinterpret_cast<const UInt8*>(urlString.c_str()), urlString.length() * sizeof(wstring::value_type), kCFStringEncodingUTF16, 0));

    RetainPtr<CFStringRef> scheme = adoptCF(CFURLCopyScheme(url.get()));
    if (scheme && CFStringCompare(scheme.get(), CFSTR("file"), kCFCompareCaseInsensitive) != kCFCompareEqualTo)
        return urlString;

    COMPtr<IWebDataSource> dataSource;
    if (FAILED(frame->dataSource(&dataSource))) {
        if (FAILED(frame->provisionalDataSource(&dataSource)))
            return urlString;
    }

    COMPtr<IWebMutableURLRequest> request;
    if (FAILED(dataSource->request(&request)))
        return urlString;

    _bstr_t requestURLString;
    if (FAILED(request->URL(requestURLString.GetAddress())))
        return urlString;

    RetainPtr<CFURLRef> requestURL = adoptCF(CFURLCreateWithBytes(kCFAllocatorDefault, reinterpret_cast<const UInt8*>(requestURLString.GetBSTR()), requestURLString.length() * sizeof(OLECHAR), kCFStringEncodingUTF16, 0));
    RetainPtr<CFURLRef> baseURL = adoptCF(CFURLCreateCopyDeletingLastPathComponent(kCFAllocatorDefault, requestURL.get()));

    RetainPtr<CFStringRef> basePath = adoptCF(CFURLCopyPath(baseURL.get()));
    RetainPtr<CFStringRef> path = adoptCF(CFURLCopyPath(url.get()));

    return cfStringRefToWString(substringFromIndex(path.get(), CFStringGetLength(basePath.get())).get());
}

wstring lastPathComponent(const wstring& urlString)
{
    if (urlString.empty())
        return urlString;

    RetainPtr<CFURLRef> url = adoptCF(CFURLCreateWithBytes(kCFAllocatorDefault, reinterpret_cast<const UInt8*>(urlString.c_str()), urlString.length() * sizeof(wstring::value_type), kCFStringEncodingUTF16, 0));
    RetainPtr<CFStringRef> lastPathComponent = adoptCF(CFURLCopyLastPathComponent(url.get()));

    return cfStringRefToWString(lastPathComponent.get());
}

static string toUTF8(const wchar_t* wideString, size_t length)
{
    int result = WideCharToMultiByte(CP_UTF8, 0, wideString, length + 1, 0, 0, 0, 0);
    Vector<char> utf8Vector(result);
    result = WideCharToMultiByte(CP_UTF8, 0, wideString, length + 1, utf8Vector.data(), result, 0, 0);
    if (!result)
        return string();

    return string(utf8Vector.data(), utf8Vector.size() - 1);
}

#if USE(CF)
static String libraryPathForDumpRenderTree()
{
    DWORD size = ::GetEnvironmentVariable(dumpRenderTreeTemp, 0, 0);
    Vector<TCHAR> buffer(size);
    if (::GetEnvironmentVariable(dumpRenderTreeTemp, buffer.data(), buffer.size())) {
        wstring path = buffer.data();
        if (!path.empty() && (path[path.length() - 1] != L'\\'))
            path.append(L"\\");
        return String (path.data(), path.length());
    }

    return WebCore::localUserSpecificStorageDirectory();
}
#endif

string toUTF8(BSTR bstr)
{
    return toUTF8(bstr, SysStringLen(bstr));
}

string toUTF8(const wstring& wideString)
{
    return toUTF8(wideString.c_str(), wideString.length());
}

wstring cfStringRefToWString(CFStringRef cfStr)
{
    Vector<wchar_t> v(CFStringGetLength(cfStr));
    CFStringGetCharacters(cfStr, CFRangeMake(0, CFStringGetLength(cfStr)), (UniChar *)v.data());

    return wstring(v.data(), v.size());
}

static LRESULT CALLBACK DumpRenderTreeWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_DESTROY:
            for (unsigned i = openWindows().size() - 1; i >= 0; --i) {
                if (openWindows()[i] == hWnd) {
                    openWindows().remove(i);
                    windowToWebViewMap().remove(hWnd);
                    break;
                }
            }
            return 0;
            break;
        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}

static const wstring& exePath()
{
    static wstring path;
    static bool initialized;

    if (initialized)
        return path;
    initialized = true;

    TCHAR buffer[MAX_PATH];
    GetModuleFileName(GetModuleHandle(0), buffer, ARRAYSIZE(buffer));
    path = buffer;
    int lastSlash = path.rfind('\\');
    if (lastSlash != -1 && lastSlash + 1 < path.length())
        path = path.substr(0, lastSlash + 1);

    return path;
}

static const wstring& fontsPath()
{
    static wstring path;
    static bool initialized;

    if (initialized)
        return path;
    initialized = true;

    DWORD size = GetEnvironmentVariable(fontsEnvironmentVariable, 0, 0);
    Vector<TCHAR> buffer(size);
    if (GetEnvironmentVariable(fontsEnvironmentVariable, buffer.data(), buffer.size())) {
        path = buffer.data();
        if (path[path.length() - 1] != '\\')
            path.append(L"\\");
        return path;
    }

    path = exePath() + TEXT("DumpRenderTree.resources\\");
    return path;
}

static void addQTDirToPATH()
{
    static LPCWSTR pathEnvironmentVariable = L"PATH";
    static LPCWSTR quickTimeKeyName = L"Software\\Apple Computer, Inc.\\QuickTime";
    static LPCWSTR quickTimeSysDir = L"QTSysDir";
    static bool initialized;

    if (initialized)
        return;
    initialized = true;

    // Get the QuickTime dll directory from the registry. The key can be in either HKLM or HKCU.
    WCHAR qtPath[MAX_PATH];
    DWORD qtPathBufferLen = sizeof(qtPath);
    DWORD keyType;
    HRESULT result = SHGetValue(HKEY_LOCAL_MACHINE, quickTimeKeyName, quickTimeSysDir, &keyType, (LPVOID)qtPath, &qtPathBufferLen);
    if (result != ERROR_SUCCESS || !qtPathBufferLen || keyType != REG_SZ) {
        qtPathBufferLen = sizeof(qtPath);
        result = SHGetValue(HKEY_CURRENT_USER, quickTimeKeyName, quickTimeSysDir, &keyType, (LPVOID)qtPath, &qtPathBufferLen);
        if (result != ERROR_SUCCESS || !qtPathBufferLen || keyType != REG_SZ)
            return;
    }

    // Read the current PATH.
    DWORD pathSize = GetEnvironmentVariableW(pathEnvironmentVariable, 0, 0);
    Vector<WCHAR> oldPath(pathSize);
    if (!GetEnvironmentVariableW(pathEnvironmentVariable, oldPath.data(), oldPath.size()))
        return;

    // And add the QuickTime dll.
    wstring newPath;
    newPath.append(qtPath);
    newPath.append(L";");
    newPath.append(oldPath.data(), oldPath.size());
    SetEnvironmentVariableW(pathEnvironmentVariable, newPath.data());
}

#ifdef DEBUG_ALL
#define WEBKITDLL TEXT("WebKit_debug.dll")
#else
#define WEBKITDLL TEXT("WebKit.dll")
#endif

static void initialize()
{
    if (HMODULE webKitModule = LoadLibrary(WEBKITDLL))
        if (FARPROC dllRegisterServer = GetProcAddress(webKitModule, "DllRegisterServer"))
            dllRegisterServer();

    // Init COM
    OleInitialize(0);

    static LPCTSTR fontsToInstall[] = {
        TEXT("AHEM____.ttf"),
        TEXT("Apple Chancery.ttf"),
        TEXT("Courier Bold.ttf"),
        TEXT("Courier.ttf"),
        TEXT("Helvetica Bold Oblique.ttf"),
        TEXT("Helvetica Bold.ttf"),
        TEXT("Helvetica Oblique.ttf"),
        TEXT("Helvetica.ttf"),
        TEXT("Helvetica Neue Bold Italic.ttf"),
        TEXT("Helvetica Neue Bold.ttf"),
        TEXT("Helvetica Neue Condensed Black.ttf"),
        TEXT("Helvetica Neue Condensed Bold.ttf"),
        TEXT("Helvetica Neue Italic.ttf"),
        TEXT("Helvetica Neue Light Italic.ttf"),
        TEXT("Helvetica Neue Light.ttf"),
        TEXT("Helvetica Neue UltraLight Italic.ttf"),
        TEXT("Helvetica Neue UltraLight.ttf"),
        TEXT("Helvetica Neue.ttf"),
        TEXT("Lucida Grande.ttf"),
        TEXT("Lucida Grande Bold.ttf"),
        TEXT("Monaco.ttf"),
        TEXT("Papyrus.ttf"),
        TEXT("Times Bold Italic.ttf"),
        TEXT("Times Bold.ttf"),
        TEXT("Times Italic.ttf"),
        TEXT("Times Roman.ttf"),
        TEXT("WebKit Layout Tests 2.ttf"),
        TEXT("WebKit Layout Tests.ttf"),
        TEXT("WebKitWeightWatcher100.ttf"),
        TEXT("WebKitWeightWatcher200.ttf"),
        TEXT("WebKitWeightWatcher300.ttf"),
        TEXT("WebKitWeightWatcher400.ttf"),
        TEXT("WebKitWeightWatcher500.ttf"),
        TEXT("WebKitWeightWatcher600.ttf"),
        TEXT("WebKitWeightWatcher700.ttf"),
        TEXT("WebKitWeightWatcher800.ttf"),
        TEXT("WebKitWeightWatcher900.ttf")
    };

    wstring resourcesPath = fontsPath();

    COMPtr<IWebTextRenderer> textRenderer;
    if (SUCCEEDED(WebKitCreateInstance(CLSID_WebTextRenderer, 0, IID_IWebTextRenderer, (void**)&textRenderer)))
        for (int i = 0; i < ARRAYSIZE(fontsToInstall); ++i)
            textRenderer->registerPrivateFont(wstring(resourcesPath + fontsToInstall[i]).c_str());

    // Add the QuickTime dll directory to PATH or QT 7.6 will fail to initialize on systems
    // linked with older versions of qtmlclientlib.dll.
    addQTDirToPATH();

    // Register a host window
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = DumpRenderTreeWndProc;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = 0;
    wcex.hInstance     = GetModuleHandle(0);
    wcex.hIcon         = 0;
    wcex.hCursor       = LoadCursor(0, IDC_ARROW);
    wcex.hbrBackground = 0;
    wcex.lpszMenuName  = 0;
    wcex.lpszClassName = kDumpRenderTreeClassName;
    wcex.hIconSm       = 0;

    RegisterClassEx(&wcex);
}

void displayWebView()
{
    ::InvalidateRect(webViewWindow, 0, TRUE);
    ::SendMessage(webViewWindow, WM_PAINT, 0, 0);
}

void dumpFrameScrollPosition(IWebFrame* frame)
{
    if (!frame)
        return;

    COMPtr<IWebFramePrivate> framePrivate;
    if (FAILED(frame->QueryInterface(&framePrivate)))
        return;

    SIZE scrollPosition;
    if (FAILED(framePrivate->scrollOffset(&scrollPosition)))
        return;

    if (abs(scrollPosition.cx) > 0.00000001 || abs(scrollPosition.cy) > 0.00000001) {
        COMPtr<IWebFrame> parent;
        if (FAILED(frame->parentFrame(&parent)))
            return;
        if (parent) {
            BSTR name;
            if (FAILED(frame->name(&name)))
                return;
            printf("frame '%S' ", name ? name : L"");
            SysFreeString(name);
        }
        printf("scrolled to %.f,%.f\n", (double)scrollPosition.cx, (double)scrollPosition.cy);
    }

    if (::gTestRunner->dumpChildFrameScrollPositions()) {
        COMPtr<IEnumVARIANT> enumKids;
        if (FAILED(frame->childFrames(&enumKids)))
            return;
        VARIANT var;
        VariantInit(&var);
        while (enumKids->Next(1, &var, 0) == S_OK) {
            ASSERT(V_VT(&var) == VT_UNKNOWN);
            COMPtr<IWebFrame> framePtr;
            V_UNKNOWN(&var)->QueryInterface(IID_IWebFrame, (void**)&framePtr);
            dumpFrameScrollPosition(framePtr.get());
            VariantClear(&var);
        }
    }
}

static wstring dumpFramesAsText(IWebFrame* frame)
{
    if (!frame)
        return L"";

    COMPtr<IDOMDocument> document;
    if (FAILED(frame->DOMDocument(&document)))
        return L"";

    COMPtr<IDOMElement> documentElement;
    if (FAILED(document->documentElement(&documentElement)))
        return L"";

    wstring result;

    // Add header for all but the main frame.
    COMPtr<IWebFrame> parent;
    if (FAILED(frame->parentFrame(&parent)))
        return L"";
    if (parent) {
        BSTR name = L"";
        if (FAILED(frame->name(&name)))
            return L"";

        result.append(L"\n--------\nFrame: '");
        result.append(name ? name : L"", SysStringLen(name));
        result.append(L"'\n--------\n");

        SysFreeString(name);
    }

    BSTR innerText = 0;
    COMPtr<IDOMElementPrivate> docPrivate;
    if (SUCCEEDED(documentElement->QueryInterface(&docPrivate)))
        docPrivate->innerText(&innerText);

    result.append(innerText ? innerText : L"", SysStringLen(innerText));
    result.append(L"\n");

    SysFreeString(innerText);

    if (::gTestRunner->dumpChildFramesAsText()) {
        COMPtr<IEnumVARIANT> enumKids;
        if (FAILED(frame->childFrames(&enumKids)))
            return L"";
        VARIANT var;
        VariantInit(&var);
        while (enumKids->Next(1, &var, 0) == S_OK) {
            ASSERT(V_VT(&var) == VT_UNKNOWN);
            COMPtr<IWebFrame> framePtr;
            V_UNKNOWN(&var)->QueryInterface(IID_IWebFrame, (void**)&framePtr);
            result.append(dumpFramesAsText(framePtr.get()));
            VariantClear(&var);
        }
    }

    return result;
}

static int compareHistoryItems(const void* item1, const void* item2)
{
    COMPtr<IWebHistoryItemPrivate> itemA;
    if (FAILED((*(COMPtr<IUnknown>*)item1)->QueryInterface(&itemA)))
        return 0;

    COMPtr<IWebHistoryItemPrivate> itemB;
    if (FAILED((*(COMPtr<IUnknown>*)item2)->QueryInterface(&itemB)))
        return 0;

    BSTR targetA;
    if (FAILED(itemA->target(&targetA)))
        return 0;

    BSTR targetB;
    if (FAILED(itemB->target(&targetB))) {
        SysFreeString(targetA);
        return 0;
    }

    int result = wcsicmp(wstring(targetA, SysStringLen(targetA)).c_str(), wstring(targetB, SysStringLen(targetB)).c_str());
    SysFreeString(targetA);
    SysFreeString(targetB);
    return result;
}

static void dumpHistoryItem(IWebHistoryItem* item, int indent, bool current)
{
    ASSERT(item);

    int start = 0;
    if (current) {
        printf("curr->");
        start = 6;
    }
    for (int i = start; i < indent; i++)
        putchar(' ');

    BSTR url;
    if (FAILED(item->URLString(&url)))
        return;

    if (wcsstr(url, L"file:/") == url) {
        static wchar_t* layoutTestsString = L"/LayoutTests/";
        static wchar_t* fileTestString = L"(file test):";
        
        wchar_t* result = wcsstr(url, layoutTestsString);
        if (result == NULL)
            return;
        wchar_t* start = result + wcslen(layoutTestsString);

        BSTR newURL = SysAllocStringLen(NULL, SysStringLen(url));
        wcscpy(newURL, fileTestString);
        wcscpy(newURL + wcslen(fileTestString), start);

        SysFreeString(url);
        url = newURL;
    }

    printf("%S", url ? url : L"");
    SysFreeString(url);

    COMPtr<IWebHistoryItemPrivate> itemPrivate;
    if (FAILED(item->QueryInterface(&itemPrivate)))
        return;

    BSTR target;
    if (FAILED(itemPrivate->target(&target)))
        return;
    if (SysStringLen(target))
        printf(" (in frame \"%S\")", target);
    SysFreeString(target);
    BOOL isTargetItem = FALSE;
    if (FAILED(itemPrivate->isTargetItem(&isTargetItem)))
        return;
    if (isTargetItem)
        printf("  **nav target**");
    putchar('\n');

    unsigned kidsCount;
    SAFEARRAY* arrPtr;
    if (FAILED(itemPrivate->children(&kidsCount, &arrPtr)) || !kidsCount)
        return;

    Vector<COMPtr<IUnknown> > kidsVector;

    LONG lowerBound;
    if (FAILED(::SafeArrayGetLBound(arrPtr, 1, &lowerBound)))
        goto exit;

    LONG upperBound;
    if (FAILED(::SafeArrayGetUBound(arrPtr, 1, &upperBound)))
        goto exit;

    LONG length = upperBound - lowerBound + 1;
    if (!length)
        goto exit;
    ASSERT(length == kidsCount);

    IUnknown** safeArrayData;
    if (FAILED(::SafeArrayAccessData(arrPtr, (void**)&safeArrayData)))
        goto exit;

    for (int i = 0; i < length; ++i)
        kidsVector.append(safeArrayData[i]);
    ::SafeArrayUnaccessData(arrPtr);

    // must sort to eliminate arbitrary result ordering which defeats reproducible testing
    qsort(kidsVector.data(), kidsCount, sizeof(kidsVector[0]), compareHistoryItems);

    for (unsigned i = 0; i < kidsCount; ++i) {
        COMPtr<IWebHistoryItem> item;
        kidsVector[i]->QueryInterface(&item);
        dumpHistoryItem(item.get(), indent + 4, false);
    }

exit:
    if (arrPtr && SUCCEEDED(::SafeArrayUnlock(arrPtr)))
        ::SafeArrayDestroy(arrPtr);
}

static void dumpBackForwardList(IWebView* webView)
{
    ASSERT(webView);

    printf("\n============== Back Forward List ==============\n");

    COMPtr<IWebBackForwardList> bfList;
    if (FAILED(webView->backForwardList(&bfList)))
        return;

    // Print out all items in the list after prevTestBFItem, which was from the previous test
    // Gather items from the end of the list, the print them out from oldest to newest

    Vector<COMPtr<IUnknown> > itemsToPrint;

    int forwardListCount;
    if (FAILED(bfList->forwardListCount(&forwardListCount)))
        return;

    for (int i = forwardListCount; i > 0; --i) {
        COMPtr<IWebHistoryItem> item;
        if (FAILED(bfList->itemAtIndex(i, &item)))
            return;
        // something is wrong if the item from the last test is in the forward part of the b/f list
        ASSERT(item != prevTestBFItem);
        COMPtr<IUnknown> itemUnknown;
        item->QueryInterface(&itemUnknown);
        itemsToPrint.append(itemUnknown);
    }

    COMPtr<IWebHistoryItem> currentItem;
    if (FAILED(bfList->currentItem(&currentItem)))
        return;

    ASSERT(currentItem != prevTestBFItem);
    COMPtr<IUnknown> currentItemUnknown;
    currentItem->QueryInterface(&currentItemUnknown);
    itemsToPrint.append(currentItemUnknown);
    int currentItemIndex = itemsToPrint.size() - 1;

    int backListCount;
    if (FAILED(bfList->backListCount(&backListCount)))
        return;

    for (int i = -1; i >= -backListCount; --i) {
        COMPtr<IWebHistoryItem> item;
        if (FAILED(bfList->itemAtIndex(i, &item)))
            return;
        if (item == prevTestBFItem)
            break;
        COMPtr<IUnknown> itemUnknown;
        item->QueryInterface(&itemUnknown);
        itemsToPrint.append(itemUnknown);
    }

    for (int i = itemsToPrint.size() - 1; i >= 0; --i) {
        COMPtr<IWebHistoryItem> historyItemToPrint;
        itemsToPrint[i]->QueryInterface(&historyItemToPrint);
        dumpHistoryItem(historyItemToPrint.get(), 8, i == currentItemIndex);
    }

    printf("===============================================\n");
}

static void dumpBackForwardListForAllWindows()
{
    unsigned count = openWindows().size();
    for (unsigned i = 0; i < count; i++) {
        HWND window = openWindows()[i];
        IWebView* webView = windowToWebViewMap().get(window).get();
        dumpBackForwardList(webView);
    }
}

static void invalidateAnyPreviousWaitToDumpWatchdog()
{
    if (!waitToDumpWatchdog)
        return;

    KillTimer(0, waitToDumpWatchdog);
    waitToDumpWatchdog = 0;
}

void dump()
{
    invalidateAnyPreviousWaitToDumpWatchdog();

    COMPtr<IWebDataSource> dataSource;
    if (SUCCEEDED(frame->dataSource(&dataSource))) {
        COMPtr<IWebURLResponse> response;
        if (SUCCEEDED(dataSource->response(&response)) && response) {
            BSTR mimeType;
            if (SUCCEEDED(response->MIMEType(&mimeType)) && !_tcscmp(mimeType, TEXT("text/plain"))) {
                ::gTestRunner->setDumpAsText(true);
                ::gTestRunner->setGeneratePixelResults(false);
            }
            SysFreeString(mimeType);
        }
    }

    BSTR resultString = 0;

    if (dumpTree) {
        ::InvalidateRect(webViewWindow, 0, TRUE);
        ::SendMessage(webViewWindow, WM_PAINT, 0, 0);

        if (::gTestRunner->dumpAsText()) {
            wstring result = dumpFramesAsText(frame);
            resultString = SysAllocStringLen(result.data(), result.size());
        } else {
            COMPtr<IWebFramePrivate> framePrivate;
            if (FAILED(frame->QueryInterface(&framePrivate)))
                goto fail;
            framePrivate->renderTreeAsExternalRepresentation(gTestRunner->isPrinting(), &resultString);
        }
        
        if (!resultString)
            printf("ERROR: nil result from %s", ::gTestRunner->dumpAsText() ? "IDOMElement::innerText" : "IFrameViewPrivate::renderTreeAsExternalRepresentation");
        else {
            unsigned stringLength = SysStringLen(resultString);
            int bufferSize = ::WideCharToMultiByte(CP_UTF8, 0, resultString, stringLength, 0, 0, 0, 0);
            char* buffer = (char*)malloc(bufferSize + 1);
            ::WideCharToMultiByte(CP_UTF8, 0, resultString, stringLength, buffer, bufferSize + 1, 0, 0);
            fwrite(buffer, 1, bufferSize, stdout);
            free(buffer);
            if (!::gTestRunner->dumpAsText())
                dumpFrameScrollPosition(frame);
        }
        if (::gTestRunner->dumpBackForwardList())
            dumpBackForwardListForAllWindows();
    }

    if (printSeparators) {
        puts("#EOF");   // terminate the content block
        fputs("#EOF\n", stderr);
        fflush(stdout);
        fflush(stderr);
    }

    if (dumpPixelsForCurrentTest
     && gTestRunner->generatePixelResults()
     && !gTestRunner->dumpDOMAsWebArchive()
     && !gTestRunner->dumpSourceAsWebArchive())
        dumpWebViewAsPixelsAndCompareWithExpected(gTestRunner->expectedPixelHash());

    printf("#EOF\n");   // terminate the (possibly empty) pixels block
    fflush(stdout);

fail:
    SysFreeString(resultString);
    // This will exit from our message loop.
    PostQuitMessage(0);
    done = true;
}

static bool shouldLogFrameLoadDelegates(const char* pathOrURL)
{
    return strstr(pathOrURL, "/loading/") || strstr(pathOrURL, "\\loading\\");
}

static bool shouldLogHistoryDelegates(const char* pathOrURL)
{
    return strstr(pathOrURL, "/globalhistory/") || strstr(pathOrURL, "\\globalhistory\\");
}

static bool shouldOpenWebInspector(const char* pathOrURL)
{
    return strstr(pathOrURL, "/inspector/") || strstr(pathOrURL, "\\inspector\\");
}

static bool shouldDumpAsText(const char* pathOrURL)
{
    return strstr(pathOrURL, "/dumpAsText/") || strstr(pathOrURL, "\\dumpAsText\\");
}

static bool shouldEnableDeveloperExtras(const char* pathOrURL)
{
    return true;
}

static void resetDefaultsToConsistentValues(IWebPreferences* preferences)
{
#ifdef USE_MAC_FONTS
    static BSTR standardFamily = SysAllocString(TEXT("Times"));
    static BSTR fixedFamily = SysAllocString(TEXT("Courier"));
    static BSTR sansSerifFamily = SysAllocString(TEXT("Helvetica"));
    static BSTR cursiveFamily = SysAllocString(TEXT("Apple Chancery"));
    static BSTR fantasyFamily = SysAllocString(TEXT("Papyrus"));
    static BSTR pictographFamily = SysAllocString(TEXT("Apple Color Emoji"));
#else
    static BSTR standardFamily = SysAllocString(TEXT("Times New Roman"));
    static BSTR fixedFamily = SysAllocString(TEXT("Courier New"));
    static BSTR sansSerifFamily = SysAllocString(TEXT("Arial"));
    static BSTR cursiveFamily = SysAllocString(TEXT("Comic Sans MS")); // Not actually cursive, but it's what IE and Firefox use.
    static BSTR fantasyFamily = SysAllocString(TEXT("Times New Roman"));
    static BSTR pictographFamily = SysAllocString(TEXT("Times New Roman"));
#endif

    preferences->setStandardFontFamily(standardFamily);
    preferences->setFixedFontFamily(fixedFamily);
    preferences->setSerifFontFamily(standardFamily);
    preferences->setSansSerifFontFamily(sansSerifFamily);
    preferences->setCursiveFontFamily(cursiveFamily);
    preferences->setFantasyFontFamily(fantasyFamily);
    preferences->setPictographFontFamily(pictographFamily);

    preferences->setAutosaves(FALSE);
    preferences->setDefaultFontSize(16);
    preferences->setDefaultFixedFontSize(13);
    preferences->setMinimumFontSize(0);
    preferences->setJavaEnabled(FALSE);
    preferences->setPlugInsEnabled(TRUE);
    preferences->setDOMPasteAllowed(TRUE);
    preferences->setEditableLinkBehavior(WebKitEditableLinkOnlyLiveWithShiftKey);
    preferences->setFontSmoothing(FontSmoothingTypeStandard);
    preferences->setUsesPageCache(FALSE);
    preferences->setPrivateBrowsingEnabled(FALSE);
    preferences->setJavaScriptCanOpenWindowsAutomatically(TRUE);
    preferences->setJavaScriptEnabled(TRUE);
    preferences->setTabsToLinks(FALSE);
    preferences->setShouldPrintBackgrounds(TRUE);
    preferences->setLoadsImagesAutomatically(TRUE);
    preferences->setSeamlessIFramesEnabled(TRUE);

    if (persistentUserStyleSheetLocation) {
        Vector<wchar_t> urlCharacters(CFStringGetLength(persistentUserStyleSheetLocation.get()));
        CFStringGetCharacters(persistentUserStyleSheetLocation.get(), CFRangeMake(0, CFStringGetLength(persistentUserStyleSheetLocation.get())), (UniChar *)urlCharacters.data());
        BSTR url = SysAllocStringLen(urlCharacters.data(), urlCharacters.size());
        preferences->setUserStyleSheetLocation(url);
        SysFreeString(url);
        preferences->setUserStyleSheetEnabled(TRUE);
    } else
        preferences->setUserStyleSheetEnabled(FALSE);

    COMPtr<IWebPreferencesPrivate> prefsPrivate(Query, preferences);
    if (prefsPrivate) {
        prefsPrivate->setAllowUniversalAccessFromFileURLs(TRUE);
        prefsPrivate->setAllowFileAccessFromFileURLs(TRUE);
        prefsPrivate->setAuthorAndUserStylesEnabled(TRUE);
        prefsPrivate->setDeveloperExtrasEnabled(FALSE);
        prefsPrivate->setExperimentalNotificationsEnabled(TRUE);
        prefsPrivate->setShouldPaintNativeControls(FALSE); // FIXME - need to make DRT pass with Windows native controls <http://bugs.webkit.org/show_bug.cgi?id=25592>
        prefsPrivate->setJavaScriptCanAccessClipboard(TRUE);
        prefsPrivate->setXSSAuditorEnabled(FALSE);
        prefsPrivate->setOfflineWebApplicationCacheEnabled(TRUE);
        prefsPrivate->setLoadsSiteIconsIgnoringImageLoadingPreference(FALSE);
    }
    setAlwaysAcceptCookies(false);

    setlocale(LC_ALL, "");
}

static void resetWebViewToConsistentStateBeforeTesting()
{
    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView))) 
        return;

    webView->setPolicyDelegate(0);
    policyDelegate->setPermissive(false);
    policyDelegate->setControllerToNotifyDone(0);

    COMPtr<IWebIBActions> webIBActions(Query, webView);
    if (webIBActions) {
        webIBActions->makeTextStandardSize(0);
        webIBActions->resetPageZoom(0);
    }


    COMPtr<IWebPreferences> preferences;
    if (SUCCEEDED(webView->preferences(&preferences)))
        resetDefaultsToConsistentValues(preferences.get());

    if (gTestRunner) {
        JSGlobalContextRef context = frame->globalContext();
        WebCoreTestSupport::resetInternalsObject(context);
    }

    COMPtr<IWebViewPrivate> webViewPrivate(Query, webView);
    if (!webViewPrivate)
        return;

    HWND viewWindow;
    if (SUCCEEDED(webViewPrivate->viewWindow(reinterpret_cast<OLE_HANDLE*>(&viewWindow))) && viewWindow)
        SetFocus(viewWindow);

    webViewPrivate->clearMainFrameName();
    webViewPrivate->resetOriginAccessWhitelists();

    BSTR groupName;
    if (SUCCEEDED(webView->groupName(&groupName))) {
        webViewPrivate->removeAllUserContentFromGroup(groupName);
        SysFreeString(groupName);
    }

    sharedUIDelegate->resetUndoManager();

    sharedFrameLoadDelegate->resetToConsistentState();

    COMPtr<IWebFramePrivate> framePrivate;
    if (SUCCEEDED(frame->QueryInterface(&framePrivate)))
        framePrivate->clearOpener();
}

static void sizeWebViewForCurrentTest()
{
    bool isSVGW3CTest = (gTestRunner->testPathOrURL().find("svg\\W3C-SVG-1.1") != string::npos);
    unsigned width;
    unsigned height;
    if (isSVGW3CTest) {
        width = TestRunner::w3cSVGViewWidth;
        height = TestRunner::w3cSVGViewHeight;
    } else {
        width = TestRunner::viewWidth;
        height = TestRunner::viewHeight;
    }

    ::SetWindowPos(webViewWindow, 0, 0, 0, width, height, SWP_NOMOVE);
}

static String findFontFallback(const char* pathOrUrl)
{
    String pathToFontFallback = WebCore::directoryName(pathOrUrl);

    wchar_t fullPath[_MAX_PATH];
    if (!_wfullpath(fullPath, pathToFontFallback.charactersWithNullTermination().data(), _MAX_PATH))
        return emptyString();

    if (!::PathIsDirectoryW(fullPath))
        return emptyString();

    String pathToCheck = fullPath;

    static const String layoutTests = "LayoutTests";

    // Find the layout test root on the current path:
    size_t location = pathToCheck.find(layoutTests);
    if (WTF::notFound == location)
        return emptyString();

    String pathToTest = pathToCheck.substring(location + layoutTests.length() + 1);
    String possiblePathToLogue = WebCore::pathByAppendingComponent(pathToCheck.substring(0, location + layoutTests.length() + 1), "platform\\win");

    Vector<String> possiblePaths;
    possiblePaths.append(WebCore::pathByAppendingComponent(possiblePathToLogue, pathToTest));

    size_t nextCandidateEnd = pathToTest.reverseFind('\\');
    while (nextCandidateEnd && nextCandidateEnd != WTF::notFound) {
        pathToTest = pathToTest.substring(0, nextCandidateEnd);
        possiblePaths.append(WebCore::pathByAppendingComponent(possiblePathToLogue, pathToTest));
        nextCandidateEnd = pathToTest.reverseFind('\\');
    }

    for (Vector<String>::iterator pos = possiblePaths.begin(); pos != possiblePaths.end(); ++pos) {
        pathToFontFallback = WebCore::pathByAppendingComponent(*pos, "resources\\"); 

        if (::PathIsDirectoryW(pathToFontFallback.charactersWithNullTermination().data()))
            return pathToFontFallback;
    }

    return emptyString();
}

static void addFontFallbackIfPresent(const String& fontFallbackPath)
{
    if (fontFallbackPath.isEmpty())
        return;

    String fontFallback = WebCore::pathByAppendingComponent(fontFallbackPath, "Mac-compatible-font-fallback.css");

    if (!::PathFileExistsW(fontFallback.charactersWithNullTermination().data()))
        return;

    ::setPersistentUserStyleSheetLocation(fontFallback.createCFString().get());
}

static void removeFontFallbackIfPresent(const String& fontFallbackPath)
{
    if (fontFallbackPath.isEmpty())
        return;

    String fontFallback = WebCore::pathByAppendingComponent(fontFallbackPath, "Mac-compatible-font-fallback.css");

    if (!::PathFileExistsW(fontFallback.charactersWithNullTermination().data()))
        return;

    ::setPersistentUserStyleSheetLocation(0);
}

static void runTest(const string& inputLine)
{
    TestCommand command = parseInputLine(inputLine);
    const string& pathOrURL = command.pathOrURL;
    dumpPixelsForCurrentTest = command.shouldDumpPixels || dumpPixelsForAllTests;

    static BSTR methodBStr = SysAllocString(TEXT("GET"));

    BSTR urlBStr;
 
    CFStringRef str = CFStringCreateWithCString(0, pathOrURL.c_str(), kCFStringEncodingWindowsLatin1);
    CFURLRef url = CFURLCreateWithString(0, str, 0);

    if (!url)
        url = CFURLCreateWithFileSystemPath(0, str, kCFURLWindowsPathStyle, false);

    CFRelease(str);

    String fallbackPath = findFontFallback(pathOrURL.c_str());

    str = CFURLGetString(url);

    CFIndex length = CFStringGetLength(str);
    UniChar* buffer = new UniChar[length];

    CFStringGetCharacters(str, CFRangeMake(0, length), buffer);
    urlBStr = SysAllocStringLen((OLECHAR*)buffer, length);
    delete[] buffer;

    CFRelease(url);

    ::gTestRunner = TestRunner::create(pathOrURL, command.expectedPixelHash);
    done = false;
    topLoadingFrame = 0;

    addFontFallbackIfPresent(fallbackPath);

    sizeWebViewForCurrentTest();
    gTestRunner->setIconDatabaseEnabled(false);

    if (shouldLogFrameLoadDelegates(pathOrURL.c_str()))
        gTestRunner->setDumpFrameLoadCallbacks(true);

    COMPtr<IWebView> webView;
    if (SUCCEEDED(frame->webView(&webView))) {
        COMPtr<IWebViewPrivate> viewPrivate;
        if (SUCCEEDED(webView->QueryInterface(&viewPrivate))) {
            if (shouldLogHistoryDelegates(pathOrURL.c_str())) {
                gTestRunner->setDumpHistoryDelegateCallbacks(true);            
                viewPrivate->setHistoryDelegate(sharedHistoryDelegate.get());
            } else
                viewPrivate->setHistoryDelegate(0);
        }
    }
    COMPtr<IWebHistory> history;
    if (SUCCEEDED(WebKitCreateInstance(CLSID_WebHistory, 0, __uuidof(history), reinterpret_cast<void**>(&history))))
        history->setOptionalSharedHistory(0);

    resetWebViewToConsistentStateBeforeTesting();

    if (shouldEnableDeveloperExtras(pathOrURL.c_str())) {
        gTestRunner->setDeveloperExtrasEnabled(true);
        if (shouldOpenWebInspector(pathOrURL.c_str()))
            gTestRunner->showWebInspector();
    }
    if (shouldDumpAsText(pathOrURL.c_str())) {
        gTestRunner->setDumpAsText(true);
        gTestRunner->setGeneratePixelResults(false);
    }

    prevTestBFItem = 0;
    if (webView) {
        COMPtr<IWebBackForwardList> bfList;
        if (SUCCEEDED(webView->backForwardList(&bfList)))
            bfList->currentItem(&prevTestBFItem);
    }

    WorkQueue::shared()->clear();
    WorkQueue::shared()->setFrozen(false);

    HWND hostWindow;
    webView->hostWindow(reinterpret_cast<OLE_HANDLE*>(&hostWindow));

    COMPtr<IWebMutableURLRequest> request;
    HRESULT hr = WebKitCreateInstance(CLSID_WebMutableURLRequest, 0, IID_IWebMutableURLRequest, (void**)&request);
    if (FAILED(hr))
        goto exit;

    request->initWithURL(urlBStr, WebURLRequestUseProtocolCachePolicy, 60);

    request->setHTTPMethod(methodBStr);
    frame->loadRequest(request.get());

    MSG msg;
    while (GetMessage(&msg, 0, 0, 0)) {
        // We get spurious WM_MOUSELEAVE events which make event handling machinery think that mouse button
        // is released during dragging (see e.g. fast\dynamic\layer-hit-test-crash.html).
        // Mouse can never leave WebView during normal DumpRenderTree operation, so we just ignore all such events.
        if (msg.message == WM_MOUSELEAVE)
            continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (shouldEnableDeveloperExtras(pathOrURL.c_str())) {
        gTestRunner->closeWebInspector();
        gTestRunner->setDeveloperExtrasEnabled(false);
    }

    resetWebViewToConsistentStateBeforeTesting();

    frame->stopLoading();

    if (::gTestRunner->closeRemainingWindowsWhenComplete()) {
        Vector<HWND> windows = openWindows();
        unsigned size = windows.size();
        for (unsigned i = 0; i < size; i++) {
            HWND window = windows[i];

            // Don't try to close the main window
            if (window == hostWindow)
                continue;

            DestroyWindow(window);
        }
    }

exit:
    removeFontFallbackIfPresent(fallbackPath);
    SysFreeString(urlBStr);
    ::gTestRunner.clear();

    return;
}

Vector<HWND>& openWindows()
{
    static Vector<HWND> vector;
    return vector;
}

WindowToWebViewMap& windowToWebViewMap()
{
    static WindowToWebViewMap map;
    return map;
}

IWebView* createWebViewAndOffscreenWindow(HWND* webViewWindow)
{
    unsigned maxViewWidth = TestRunner::viewWidth;
    unsigned maxViewHeight = TestRunner::viewHeight;
    HWND hostWindow = CreateWindowEx(WS_EX_TOOLWINDOW, kDumpRenderTreeClassName, TEXT("DumpRenderTree"), WS_POPUP,
      -maxViewWidth, -maxViewHeight, maxViewWidth, maxViewHeight, 0, 0, GetModuleHandle(0), 0);

    IWebView* webView;

    HRESULT hr = WebKitCreateInstance(CLSID_WebView, 0, IID_IWebView, (void**)&webView);
    if (FAILED(hr)) {
        fprintf(stderr, "Failed to create CLSID_WebView instance, error 0x%x\n", hr);
        return 0;
    }

    if (FAILED(webView->setHostWindow((OLE_HANDLE)(ULONG64)hostWindow)))
        return 0;

    RECT clientRect;
    clientRect.bottom = clientRect.left = clientRect.top = clientRect.right = 0;
    BSTR groupName = SysAllocString(L"org.webkit.DumpRenderTree");
    bool failed = FAILED(webView->initWithFrame(clientRect, 0, groupName));
    SysFreeString(groupName);
    if (failed)
        return 0;

    COMPtr<IWebViewPrivate> viewPrivate;
    if (FAILED(webView->QueryInterface(&viewPrivate)))
        return 0;

    viewPrivate->setShouldApplyMacFontAscentHack(TRUE);
    viewPrivate->setAlwaysUsesComplexTextCodePath(forceComplexText);

    BSTR pluginPath = SysAllocStringLen(0, exePath().length() + _tcslen(TestPluginDir));
    _tcscpy(pluginPath, exePath().c_str());
    _tcscat(pluginPath, TestPluginDir);
    failed = FAILED(viewPrivate->addAdditionalPluginDirectory(pluginPath));
    SysFreeString(pluginPath);
    if (failed)
        return 0;

    HWND viewWindow;
    if (FAILED(viewPrivate->viewWindow(reinterpret_cast<OLE_HANDLE*>(&viewWindow))))
        return 0;
    if (webViewWindow)
        *webViewWindow = viewWindow;

    SetWindowPos(viewWindow, 0, 0, 0, maxViewWidth, maxViewHeight, 0);
    ShowWindow(hostWindow, SW_SHOW);

    if (FAILED(webView->setFrameLoadDelegate(sharedFrameLoadDelegate.get())))
        return 0;

    if (FAILED(viewPrivate->setFrameLoadDelegatePrivate(sharedFrameLoadDelegate.get())))
        return 0;

    if (FAILED(webView->setUIDelegate(sharedUIDelegate.get())))
        return 0;

    COMPtr<IWebViewEditing> viewEditing;
    if (FAILED(webView->QueryInterface(&viewEditing)))
        return 0;

    if (FAILED(viewEditing->setEditingDelegate(sharedEditingDelegate.get())))
        return 0;

    ResourceLoadDelegate* resourceLoadDelegate = new ResourceLoadDelegate();
    HRESULT result = webView->setResourceLoadDelegate(resourceLoadDelegate);
    resourceLoadDelegate->Release(); // The delegate is owned by the WebView, so release our reference to it.
    if (FAILED(result))
        return 0;

    openWindows().append(hostWindow);
    windowToWebViewMap().set(hostWindow, webView);
    return webView;
}

#if USE(CFNETWORK)
RetainPtr<CFURLCacheRef> sharedCFURLCache()
{
#ifndef DEBUG_ALL
    HMODULE module = GetModuleHandle(TEXT("CFNetwork.dll"));
#else
    HMODULE module = GetModuleHandle(TEXT("CFNetwork_debug.dll"));
#endif
    if (!module)
        return 0;

    typedef CFURLCacheRef (*CFURLCacheCopySharedURLCacheProcPtr)(void);
    if (CFURLCacheCopySharedURLCacheProcPtr copyCache = reinterpret_cast<CFURLCacheCopySharedURLCacheProcPtr>(GetProcAddress(module, "CFURLCacheCopySharedURLCache")))
        return adoptCF(copyCache());

    typedef CFURLCacheRef (*CFURLCacheSharedURLCacheProcPtr)(void);
    if (CFURLCacheSharedURLCacheProcPtr sharedCache = reinterpret_cast<CFURLCacheSharedURLCacheProcPtr>(GetProcAddress(module, "CFURLCacheSharedURLCache")))
        return sharedCache();

    return 0;
}
#endif

static LONG WINAPI exceptionFilter(EXCEPTION_POINTERS*)
{
    fputs("#CRASHED\n", stderr);
    fflush(stderr);
    return EXCEPTION_CONTINUE_SEARCH;
}

extern "C" __declspec(dllexport) int WINAPI dllLauncherEntryPoint(int argc, const char* argv[])
{
    // Cygwin calls ::SetErrorMode(SEM_FAILCRITICALERRORS), which we will inherit. This is bad for
    // testing/debugging, as it causes the post-mortem debugger not to be invoked. We reset the
    // error mode here to work around Cygwin's behavior. See <http://webkit.org/b/55222>.
    ::SetErrorMode(0);

    ::SetUnhandledExceptionFilter(exceptionFilter);

    leakChecking = false;

    _setmode(1, _O_BINARY);
    _setmode(2, _O_BINARY);

    initialize();

    Vector<const char*> tests;

    for (int i = 1; i < argc; ++i) {
        if (!stricmp(argv[i], "--threaded")) {
            threaded = true;
            continue;
        }

        if (!stricmp(argv[i], "--dump-all-pixels")) {
            dumpAllPixels = true;
            continue;
        }

        if (!stricmp(argv[i], "--complex-text")) {
            forceComplexText = true;
            continue;
        }

        if (!stricmp(argv[i], "--print-supported-features")) {
            printSupportedFeatures = true;
            continue;
        }

        if (!stricmp(argv[i], "--pixel-tests")) {
            dumpPixelsForAllTests = true;
            continue;
        }

        tests.append(argv[i]);
    }

    policyDelegate = new PolicyDelegate();
    sharedFrameLoadDelegate.adoptRef(new FrameLoadDelegate);
    sharedUIDelegate.adoptRef(new UIDelegate);
    sharedEditingDelegate.adoptRef(new EditingDelegate);
    sharedHistoryDelegate.adoptRef(new HistoryDelegate);

    // FIXME - need to make DRT pass with Windows native controls <http://bugs.webkit.org/show_bug.cgi?id=25592>
    COMPtr<IWebPreferences> tmpPreferences;
    if (FAILED(WebKitCreateInstance(CLSID_WebPreferences, 0, IID_IWebPreferences, reinterpret_cast<void**>(&tmpPreferences))))
        return -1;
    COMPtr<IWebPreferences> standardPreferences;
    if (FAILED(tmpPreferences->standardPreferences(&standardPreferences)))
        return -1;
    COMPtr<IWebPreferencesPrivate> standardPreferencesPrivate;
    if (FAILED(standardPreferences->QueryInterface(&standardPreferencesPrivate)))
        return -1;
    standardPreferencesPrivate->setShouldPaintNativeControls(FALSE);
    standardPreferences->setJavaScriptEnabled(TRUE);
    standardPreferences->setDefaultFontSize(16);
    standardPreferences->setAcceleratedCompositingEnabled(true);
    standardPreferences->setAVFoundationEnabled(TRUE);
    standardPreferences->setContinuousSpellCheckingEnabled(TRUE);

    if (printSupportedFeatures) {
        BOOL acceleratedCompositingAvailable;
        standardPreferences->acceleratedCompositingEnabled(&acceleratedCompositingAvailable);

#if ENABLE(3D_RENDERING)
        // In theory, we could have a software-based 3D rendering implementation that we use when
        // hardware-acceleration is not available. But we don't have any such software
        // implementation, so 3D rendering is only available when hardware-acceleration is.
        BOOL threeDRenderingAvailable = acceleratedCompositingAvailable;
#else
        BOOL threeDRenderingAvailable = FALSE;
#endif

        printf("SupportedFeatures:%s %s\n", acceleratedCompositingAvailable ? "AcceleratedCompositing" : "", threeDRenderingAvailable ? "3DRendering" : "");
        return 0;
    }

#if USE(CF)
    // Set up these values before creating the WebView so that the various initializations will see these preferred values.
    String path = libraryPathForDumpRenderTree();
    CFPreferencesSetAppValue(WebDatabaseDirectoryDefaultsKey, WebCore::pathByAppendingComponent(path, "Databases").createCFString().get(), kCFPreferencesCurrentApplication);
    CFPreferencesSetAppValue(WebStorageDirectoryDefaultsKey, WebCore::pathByAppendingComponent(path, "LocalStorage").createCFString().get(), kCFPreferencesCurrentApplication);
    CFPreferencesSetAppValue(WebKitLocalCacheDefaultsKey, WebCore::pathByAppendingComponent(path, "LocalCache").createCFString().get(), kCFPreferencesCurrentApplication);
#endif

    COMPtr<IWebView> webView(AdoptCOM, createWebViewAndOffscreenWindow(&webViewWindow));
    if (!webView)
        return -1;

    COMPtr<IWebIconDatabase> iconDatabase;
    COMPtr<IWebIconDatabase> tmpIconDatabase;
    if (FAILED(WebKitCreateInstance(CLSID_WebIconDatabase, 0, IID_IWebIconDatabase, (void**)&tmpIconDatabase)))
        return -1;
    if (FAILED(tmpIconDatabase->sharedIconDatabase(&iconDatabase)))
        return -1;
        
    if (FAILED(webView->mainFrame(&frame)))
        return -1;

#if USE(CFNETWORK)
    RetainPtr<CFURLCacheRef> urlCache = sharedCFURLCache();
    CFURLCacheRemoveAllCachedResponses(urlCache.get());
#endif

#ifdef _DEBUG
    _CrtMemState entryToMainMemCheckpoint;
    if (leakChecking)
        _CrtMemCheckpoint(&entryToMainMemCheckpoint);
#endif

    if (threaded)
        startJavaScriptThreads();

    if (tests.size() == 1 && !strcmp(tests[0], "-")) {
        char filenameBuffer[2048];
        printSeparators = true;
        while (fgets(filenameBuffer, sizeof(filenameBuffer), stdin)) {
            char* newLineCharacter = strchr(filenameBuffer, '\n');
            if (newLineCharacter)
                *newLineCharacter = '\0';
            
            if (strlen(filenameBuffer) == 0)
                continue;

            runTest(filenameBuffer);
        }
    } else {
        printSeparators = tests.size() > 1;
        for (int i = 0; i < tests.size(); i++)
            runTest(tests[i]);
    }

    if (threaded)
        stopJavaScriptThreads();
    
    delete policyDelegate;
    frame->Release();

#ifdef _DEBUG
    if (leakChecking) {
        // dump leaks to stderr
        _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
        _CrtMemDumpAllObjectsSince(&entryToMainMemCheckpoint);
    }
#endif

    shutDownWebKit();

    return 0;
}
