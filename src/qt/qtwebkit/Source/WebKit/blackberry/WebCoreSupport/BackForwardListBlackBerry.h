/*
 * Copyright (C) 2013 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef BackForwardListBlackBerry_h
#define BackForwardListBlackBerry_h

#include "BackForwardListImpl.h"

namespace BlackBerry {
namespace WebKit {
class WebPagePrivate;
}
}

namespace WebCore {

class BackForwardListBlackBerry : public WebCore::BackForwardList {
public:
    static PassRefPtr<BackForwardListBlackBerry> create(BlackBerry::WebKit::WebPagePrivate* pagePrivate)
    {
        return adoptRef(new BackForwardListBlackBerry(pagePrivate));
    }
    virtual ~BackForwardListBlackBerry();

    void clear();
    HistoryItemVector& entries();
    HistoryItem* currentItem();
    void backListWithLimit(int, HistoryItemVector&);
    void forwardListWithLimit(int, HistoryItemVector&);
    int capacity();

private:
    explicit BackForwardListBlackBerry(BlackBerry::WebKit::WebPagePrivate*);

    virtual void addItem(PassRefPtr<WebCore::HistoryItem>);
    virtual void goToItem(WebCore::HistoryItem*);
    virtual WebCore::HistoryItem* itemAtIndex(int);
    virtual int backListCount();
    virtual int forwardListCount();
    virtual bool isActive();
    virtual void close();

    int current();
    void notifyBackForwardListChanged();

    RefPtr<BackForwardListImpl> m_impl;
    BlackBerry::WebKit::WebPagePrivate* m_webPagePrivate;
};

} // namespace WebCore

#endif // BackForwardListBlackBerry_h
