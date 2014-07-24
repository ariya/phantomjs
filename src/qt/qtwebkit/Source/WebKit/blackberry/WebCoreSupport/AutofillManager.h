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

#ifndef AutofillManager_h
#define AutofillManager_h

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WTF {
class String;
}

namespace BlackBerry {
namespace WebKit {
class WebPagePrivate;
}
}

namespace WebCore {
class HTMLFormElement;
class HTMLInputElement;

class AutofillManager : public RefCounted<AutofillManager> {
public:
    static PassRefPtr<AutofillManager> create(BlackBerry::WebKit::WebPagePrivate*);

    void didChangeInTextField(HTMLInputElement*);
    void textFieldDidEndEditing(HTMLInputElement*);
    void autofillTextField(const WTF::String&);
    void saveTextFields(HTMLFormElement*);

    static void clear();

private:
    AutofillManager(BlackBerry::WebKit::WebPagePrivate* page) : m_webPagePrivate(page), m_element(0) { }

    BlackBerry::WebKit::WebPagePrivate* m_webPagePrivate;
    HTMLInputElement* m_element;
};

} // WebCore

#endif // AutofillManager_h
