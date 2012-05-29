/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2011 Apple Inc. All rights reserved.
 * Copyright (C) Research In Motion Limited 2009. All rights reserved.
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

#if ENABLE(FULLSCREEN_API)

#include "FullScreenController.h"

#include "Element.h"
#include "FullScreenControllerClient.h"
#include "IntRect.h"
#include "MediaPlayerPrivateFullscreenWindow.h"
#include "Timer.h"
#include "WebCoreInstanceHandle.h"
#include <wtf/RefPtr.h>

using namespace WebCore;

static const int kFullScreenAnimationDuration = 500; // milliseconds 

class FullScreenController::Private : public MediaPlayerPrivateFullscreenClient  {
public:
    Private(FullScreenController* controller, FullScreenControllerClient* client) 
        : m_controller(controller)
        , m_client(client)
        , m_originalHost(0)
        , m_isFullScreen(false)
    {
    }
    virtual ~Private() { }

    virtual LRESULT fullscreenClientWndProc(HWND, UINT, WPARAM, LPARAM);
    
    FullScreenController* m_controller;
    FullScreenControllerClient* m_client;
    OwnPtr<MediaPlayerPrivateFullscreenWindow> m_fullScreenWindow;
    OwnPtr<MediaPlayerPrivateFullscreenWindow> m_backgroundWindow;
    IntRect m_fullScreenFrame;
    IntRect m_originalFrame;
    HWND m_originalHost;
    bool m_isFullScreen;
};

LRESULT FullScreenController::Private::fullscreenClientWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;

    switch (msg) {
    case WM_MOVE:
        m_fullScreenFrame.setX(LOWORD(lParam));
        m_fullScreenFrame.setY(HIWORD(lParam));
        break;
    case WM_SIZE:
        m_fullScreenFrame.setWidth(LOWORD(lParam));
        m_fullScreenFrame.setHeight(HIWORD(lParam));
        if (m_client->fullScreenClientWindow())
            ::SetWindowPos(m_client->fullScreenClientWindow(), 0, 0, 0, m_fullScreenFrame.width(), m_fullScreenFrame.height(), SWP_NOREPOSITION  | SWP_NOMOVE);
        break;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            m_controller->exitFullScreen();
            break;
        }
        // Fall through.
    default:
        lResult = ::DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return lResult;
}

FullScreenController::FullScreenController(FullScreenControllerClient* client)
    : m_private(adoptPtr(new FullScreenController::Private(this, client)))
{
    ASSERT_ARG(client, client);
}

FullScreenController::~FullScreenController()
{
}

bool FullScreenController::isFullScreen() const
{
    return m_private->m_isFullScreen;
}

void FullScreenController::enterFullScreen()
{
    if (m_private->m_isFullScreen)
        return;
    m_private->m_isFullScreen = true;

    m_private->m_originalHost = m_private->m_client->fullScreenClientParentWindow();
    RECT originalFrame = {0, 0, 0, 0};
    ::GetClientRect(m_private->m_client->fullScreenClientWindow(), &originalFrame);
    ::MapWindowPoints(m_private->m_client->fullScreenClientWindow(), m_private->m_originalHost, reinterpret_cast<LPPOINT>(&originalFrame), 2);
    m_private->m_originalFrame = originalFrame;

    ASSERT(!m_private->m_backgroundWindow);
    m_private->m_backgroundWindow = adoptPtr(new MediaPlayerPrivateFullscreenWindow(m_private.get()));
    m_private->m_backgroundWindow->createWindow(0);
    ::AnimateWindow(m_private->m_backgroundWindow->hwnd(), kFullScreenAnimationDuration, AW_BLEND | AW_ACTIVATE);

    m_private->m_client->fullScreenClientWillEnterFullScreen();

    ASSERT(!m_private->m_fullScreenWindow);
    m_private->m_fullScreenWindow = adoptPtr(new MediaPlayerPrivateFullscreenWindow(m_private.get()));
    ASSERT(m_private->m_fullScreenWindow);
    m_private->m_fullScreenWindow->createWindow(0);

    m_private->m_client->fullScreenClientSetParentWindow(m_private->m_fullScreenWindow->hwnd());

    IntRect viewFrame(IntPoint(), m_private->m_fullScreenFrame.size());
    ::SetWindowPos(m_private->m_fullScreenWindow->hwnd(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    ::SetWindowPos(m_private->m_client->fullScreenClientWindow(), HWND_TOP, 0, 0, viewFrame.width(), viewFrame.height(), SWP_NOACTIVATE);
    ::RedrawWindow(m_private->m_client->fullScreenClientWindow(), 0, 0, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ALLCHILDREN);

    m_private->m_client->fullScreenClientDidEnterFullScreen();
    ::AnimateWindow(m_private->m_fullScreenWindow->hwnd(), kFullScreenAnimationDuration, AW_BLEND | AW_ACTIVATE);
}

void FullScreenController::exitFullScreen()
{
    if (!m_private->m_isFullScreen)
        return;
    m_private->m_isFullScreen = false;

    ::AnimateWindow(m_private->m_fullScreenWindow->hwnd(), kFullScreenAnimationDuration, AW_HIDE | AW_BLEND);

    m_private->m_client->fullScreenClientWillExitFullScreen();
    m_private->m_client->fullScreenClientSetParentWindow(m_private->m_originalHost);
    m_private->m_fullScreenWindow = nullptr;

    m_private->m_client->fullScreenClientDidExitFullScreen();
    ::SetWindowPos(m_private->m_client->fullScreenClientWindow(), 0, m_private->m_originalFrame.x(), m_private->m_originalFrame.y(), m_private->m_originalFrame.width(), m_private->m_originalFrame.height(), SWP_NOACTIVATE | SWP_NOZORDER);
    ::RedrawWindow(m_private->m_client->fullScreenClientWindow(), 0, 0, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ALLCHILDREN);

    ASSERT(m_private->m_backgroundWindow);
    ::AnimateWindow(m_private->m_backgroundWindow->hwnd(), kFullScreenAnimationDuration, AW_HIDE | AW_BLEND);
    m_private->m_backgroundWindow = nullptr;
}

#endif
