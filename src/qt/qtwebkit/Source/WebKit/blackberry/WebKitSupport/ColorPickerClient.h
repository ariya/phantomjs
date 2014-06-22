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

#ifndef ColorPickerClient_h
#define ColorPickerClient_h

#include "PagePopupClient.h"
#include <BlackBerryPlatformInputEvents.h>

namespace WebCore {
class HTMLInputElement;
}

namespace BlackBerry {
namespace Platform {
class String;
}

namespace WebKit {

class ColorPickerClient : public PagePopupClient {
public:
    ColorPickerClient(const BlackBerry::Platform::String& value, WebPagePrivate*, WebCore::HTMLInputElement*);

    void setValueAndClosePopup(const String&);
    void didClosePopup();

private:
    void generateHTML(const BlackBerry::Platform::String& value);

    RefPtr<WebCore::HTMLInputElement> m_element;
};

}
}
#endif
