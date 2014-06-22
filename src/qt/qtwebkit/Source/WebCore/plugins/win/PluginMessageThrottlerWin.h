/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2008 Collabora, Ltd. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PluginMessageThrottlerWin_h
#define PluginMessageThrottlerWin_h

#include "Timer.h"

#include <windows.h>

namespace WebCore {
    class PluginView;

    struct PluginMessage {
        HWND hWnd;
        UINT msg;
        WPARAM wParam;
        LPARAM lParam;

        struct PluginMessage* next;
    };

    class PluginMessageThrottlerWin {
    public:
        PluginMessageThrottlerWin(PluginView*);
        ~PluginMessageThrottlerWin();

        void appendMessage(HWND, UINT msg, WPARAM, LPARAM);

    private:
        void processQueuedMessage();
        void messageThrottleTimerFired(Timer<PluginMessageThrottlerWin>*);
        PluginMessage* allocateMessage();
        bool isInlineMessage(PluginMessage* message);
        void freeMessage(PluginMessage* message);

        PluginView* m_pluginView;
        PluginMessage* m_back;
        PluginMessage* m_front;

        static const int NumInlineMessages = 4;
        PluginMessage m_inlineMessages[NumInlineMessages];
        PluginMessage* m_freeInlineMessages;

        Timer<PluginMessageThrottlerWin> m_messageThrottleTimer;
        double m_lastMessageTime;
    };

} // namespace WebCore

#endif // PluginMessageThrottlerWin_h
