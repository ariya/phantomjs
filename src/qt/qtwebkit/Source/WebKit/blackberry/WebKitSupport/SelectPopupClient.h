/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
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

#ifndef SelectPopupClient_h
#define SelectPopupClient_h

#include "IntSize.h"
#include "PagePopupClient.h"
#include "ScopePointer.h"
#include "Timer.h"
#include <wtf/text/WTFString.h>

namespace WebCore {
class HTMLSelectElement;
}

namespace BlackBerry {
namespace Platform {
class String;
}

namespace WebKit {

class SelectPopupClient : public PagePopupClient {
public:
    SelectPopupClient(bool multiple, int size, const ScopeArray<BlackBerry::Platform::String>& labels, bool* enableds, const int* itemType, bool* selecteds, WebPagePrivate*, WebCore::HTMLSelectElement*);
    ~SelectPopupClient();

    virtual void setValueAndClosePopup(const String&);
    virtual void didClosePopup();

private:
    void generateHTML(bool multiple, int size, const ScopeArray<BlackBerry::Platform::String>& labels, bool* enableds, const int* itemType, bool* selecteds);
    void notifySelectionChange(WebCore::Timer<SelectPopupClient> *);

    bool m_multiple;
    unsigned m_size;
    RefPtr<WebCore::HTMLSelectElement> m_element;
    WebCore::Timer<SelectPopupClient> m_notifyChangeTimer;
};

}
}
#endif
