/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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

#ifndef WebBackForwardListProxy_h
#define WebBackForwardListProxy_h

#include <WebCore/BackForwardList.h>
#include <wtf/HashSet.h>
#include <wtf/PassRefPtr.h>

namespace WebKit {

class WebPage;

class WebBackForwardListProxy : public WebCore::BackForwardList {
public: 
    static PassRefPtr<WebBackForwardListProxy> create(WebPage* page) { return adoptRef(new WebBackForwardListProxy(page)); }

    static WebCore::HistoryItem* itemForID(uint64_t);
    static uint64_t idForItem(WebCore::HistoryItem*);
    static void removeItem(uint64_t itemID);

    static void addItemFromUIProcess(uint64_t itemID, PassRefPtr<WebCore::HistoryItem>);
    static void setHighestItemIDFromUIProcess(uint64_t itemID);
    
    void clear();

private:
    WebBackForwardListProxy(WebPage*);

    virtual void addItem(PassRefPtr<WebCore::HistoryItem>);

    virtual void goToItem(WebCore::HistoryItem*);
        
    virtual WebCore::HistoryItem* itemAtIndex(int);
    virtual int backListCount();
    virtual int forwardListCount();

    virtual bool isActive();

    virtual void close();

    WebPage* m_page;
    HashSet<uint64_t> m_associatedItemIDs;
};

} // namespace WebKit

#endif // WebBackForwardListProxy_h
