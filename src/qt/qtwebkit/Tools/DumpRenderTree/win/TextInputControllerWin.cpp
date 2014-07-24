/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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
#include "TextInputController.h"

#include "DumpRenderTree.h"
#include <WebCore/COMPtr.h>
#include <WebKit/WebKit.h>
#include <comutil.h>
#include <string>

using namespace std;

void TextInputController::setMarkedText(JSStringRef text, unsigned int from, unsigned int length) 
{    
    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return;

    COMPtr<IWebViewPrivate> viewPrivate;
    if (FAILED(webView->QueryInterface(&viewPrivate)))
        return;

    _bstr_t bstr(wstring(JSStringGetCharactersPtr(text), JSStringGetLength(text)).data());

    viewPrivate->setCompositionForTesting(bstr, from, length);
}

bool TextInputController::hasMarkedText()
{
    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return false;

    COMPtr<IWebViewPrivate> viewPrivate;
    if (FAILED(webView->QueryInterface(&viewPrivate)))
        return false;

    BOOL result;
    viewPrivate->hasCompositionForTesting(&result);
    return result;
}

void TextInputController::unmarkText()
{
    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return;

    COMPtr<IWebViewPrivate> viewPrivate;
    if (FAILED(webView->QueryInterface(&viewPrivate)))
        return;

    _bstr_t empty;
    viewPrivate->confirmCompositionForTesting(empty);
}

vector<int> TextInputController::markedRange()
{
    // empty vector
    vector<int> result;

    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return result;

    COMPtr<IWebViewPrivate> viewPrivate;
    if (FAILED(webView->QueryInterface(&viewPrivate)))
        return result;

    unsigned int startPos;
    unsigned int length;
    if (SUCCEEDED(viewPrivate->compositionRangeForTesting(&startPos, &length))) {
        result.resize(2);
        result[0] = startPos;
        result[1] = length;
    }

    return result;
}

void TextInputController::insertText(JSStringRef text)
{
    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return;

    COMPtr<IWebViewPrivate> viewPrivate;
    if (FAILED(webView->QueryInterface(&viewPrivate)))
        return;
 
    _bstr_t bstr(wstring(JSStringGetCharactersPtr(text), JSStringGetLength(text)).data());

    viewPrivate->confirmCompositionForTesting(bstr);
}

vector<int> TextInputController::firstRectForCharacterRange(unsigned int start, unsigned int length)
{
    // empty vector
    vector<int> result;

    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return result;

    COMPtr<IWebViewPrivate> viewPrivate;
    if (FAILED(webView->QueryInterface(&viewPrivate)))
        return result;

    RECT resultRECT;
    if SUCCEEDED(viewPrivate->firstRectForCharacterRangeForTesting(start, length, &resultRECT)) {
        result.resize(4);
        result[0] = resultRECT.left;
        result[1] = resultRECT.top;
        result[2] = resultRECT.right - resultRECT.left;
        result[3] = resultRECT.bottom - resultRECT.top;
    }

    return result;
}

vector<int> TextInputController::selectedRange()
{
    // empty vector
    vector<int> result;

    COMPtr<IWebView> webView;
    if (FAILED(frame->webView(&webView)))
        return result;

    COMPtr<IWebViewPrivate> viewPrivate;
    if (FAILED(webView->QueryInterface(&viewPrivate)))
        return result;

    unsigned int startPos;
    unsigned int length;
    if (SUCCEEDED(viewPrivate->selectedRangeForTesting(&startPos, &length))) {
        result.resize(2);
        result[0] = startPos;
        result[1] = length;
    }

    return result;
}
