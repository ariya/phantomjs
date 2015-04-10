/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "PluginTest.h"

using namespace std;

class LogNPPSetWindow : public PluginTest {
public:
    LogNPPSetWindow(NPP, const string& identifier);

private:
    virtual NPError NPP_SetWindow(NPWindow*);
};

LogNPPSetWindow::LogNPPSetWindow(NPP npp, const string& identifier)
    : PluginTest(npp, identifier)
{
}

NPError LogNPPSetWindow::NPP_SetWindow(NPWindow* window)
{
    char message[1024];
    snprintf(message, 1024, "NPP_SetWindow: %s window, Rect {%i, %i, %i, %i}, Clip Rect {%i, %i, %i, %i}, Type %i",
        window->window ? "non-NULL" : "NULL", window->x, window->y, window->width, window->height,
        window->clipRect.left, window->clipRect.top, window->clipRect.right, window->clipRect.bottom,
        window->type);
    
    char script[1536];
    snprintf(script, 1536, "window.setTimeout('windowWasSet(\"%s\");', 0);", message);

    executeScript(script);

    return NPERR_NO_ERROR;
}

static PluginTest::Register<LogNPPSetWindow> registrar("log-npp-set-window");
