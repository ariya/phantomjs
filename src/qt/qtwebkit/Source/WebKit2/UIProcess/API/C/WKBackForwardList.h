/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef WKBackForwardList_h
#define WKBackForwardList_h

#include <WebKit2/WKBase.h>

#ifdef __cplusplus
extern "C" {
#endif

WK_EXPORT WKTypeID WKBackForwardListGetTypeID();

WK_EXPORT WKBackForwardListItemRef WKBackForwardListGetCurrentItem(WKBackForwardListRef list);
WK_EXPORT WKBackForwardListItemRef WKBackForwardListGetBackItem(WKBackForwardListRef list);
WK_EXPORT WKBackForwardListItemRef WKBackForwardListGetForwardItem(WKBackForwardListRef list);
WK_EXPORT WKBackForwardListItemRef WKBackForwardListGetItemAtIndex(WKBackForwardListRef list, int index);

WK_EXPORT unsigned WKBackForwardListGetBackListCount(WKBackForwardListRef list);
WK_EXPORT unsigned WKBackForwardListGetForwardListCount(WKBackForwardListRef list);

WK_EXPORT WKArrayRef WKBackForwardListCopyBackListWithLimit(WKBackForwardListRef list, unsigned limit);
WK_EXPORT WKArrayRef WKBackForwardListCopyForwardListWithLimit(WKBackForwardListRef list, unsigned limit);

#ifdef __cplusplus
}
#endif

#endif // WKBackForwardList_h
