/*
 * Copyright (C) 2010 Patrick Gansterer <paroga@paroga.com>
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
#include "WebView.h"
#include <windows.h>
#include <wtf/PassOwnPtr.h>

static const LPCWSTR kMainWindowTitle = L"WebKit for WinCE";
static const LPCWSTR kMainWindowClassName = L"MainWindowClass";


static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);

    CoInitializeEx(0, COINIT_MULTITHREADED);
    WebView::initialize(hInstance);

    LPCWSTR homeUrl = lpCmdLine;
    bool enableDoubleBuffer = true;
    bool fullscreen = false;

    if (homeUrl[0] == '-') {
        for (; *homeUrl && *homeUrl != ' '; ++homeUrl) {
            switch (*homeUrl) {
            case 'd':
                enableDoubleBuffer = false;
                break;
            case 'f':
                fullscreen = true;
                break;
            default:
                break;
            }
        }
        if (*homeUrl)
            ++homeUrl;
    }

    DWORD styles = WS_VISIBLE;

    if (!fullscreen) {
        styles |= WS_CAPTION
            | WS_MAXIMIZEBOX
            | WS_MINIMIZEBOX
            | WS_OVERLAPPED
            | WS_SYSMENU
            | WS_THICKFRAME;
    }

    WNDCLASSW wc;
    wc.style          = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc    = WndProc;
    wc.cbClsExtra     = 0;
    wc.cbWndExtra     = 0;
    wc.hInstance      = hInstance;
    wc.hIcon          = 0;
    wc.hCursor        = 0;
    wc.hbrBackground  = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
    wc.lpszMenuName   = 0;
    wc.lpszClassName  = kMainWindowClassName;
    RegisterClass(&wc);

    HWND hMainWindow = CreateWindowW(kMainWindowClassName, kMainWindowTitle, styles,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 0, 0, hInstance, 0);

    if (fullscreen) {
        SetWindowPos(hMainWindow, HWND_TOPMOST, 0, 0, GetSystemMetrics(SM_CXSCREEN),
            GetSystemMetrics(SM_CYSCREEN), SWP_SHOWWINDOW);
    }

    ShowWindow(hMainWindow, nCmdShow);
    UpdateWindow(hMainWindow);

    WTF::OwnPtr<WebView> webView = WTF::adoptPtr(new WebView(hMainWindow, enableDoubleBuffer ? WebView::EnableDoubleBuffering : WebView::NoFeature));
    webView->load(homeUrl);

    // Main message loop:
    MSG msg;
    BOOL bRet;
    while (bRet = GetMessage(&msg, 0, 0, 0)) {
        if (bRet == -1) {
            // FIXME: Handle the error and possibly exit.
        } else {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    webView.clear();
    DestroyWindow(hMainWindow);

    WebView::cleanup();
    CoUninitialize();

    return msg.wParam;
}
