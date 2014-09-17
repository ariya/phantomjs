/*
 * Copyright (C) 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef OptionElement_h
#define OptionElement_h

#include "PlatformString.h"

namespace WebCore {

class Element;
class Document;
class OptionElementData;
class SelectElement;

class OptionElement {
public:
    virtual ~OptionElement() { }

    virtual bool disabled() const = 0;
    virtual bool selected() const = 0;
    virtual void setSelectedState(bool) = 0;

    virtual String text() const = 0;
    virtual String textIndentedToRespectGroupLabel() const = 0;
    virtual String value() const = 0;

protected:
    static void setSelectedState(OptionElementData&, Element*, bool selected);
    static int optionIndex(SelectElement*, const Element*);
    static String collectOptionLabelOrText(const OptionElementData&, const Element*);
    static String collectOptionTextRespectingGroupLabel(const OptionElementData&, const Element*);
    static String collectOptionValue(const OptionElementData&, const Element*);
private:
    static String collectOptionInnerText(const Element*);
    static String normalizeText(const Document*, const String&);
};

// HTMLOptionElement hold this struct as member variable
// and pass it to the static helper functions in OptionElement
class OptionElementData {
public:
    OptionElementData();
    ~OptionElementData();

    String value() const { return m_value; }
    void setValue(const String& value) { m_value = value; }

    String label() const { return m_label; }
    void setLabel(const String& label) { m_label = label; }

    bool selected() const { return m_selected; }
    void setSelected(bool selected) { m_selected = selected; }

private:
    String m_value;
    String m_label;
    bool m_selected;
};

OptionElement* toOptionElement(Element*);
bool isOptionElement(Element*);

}

#endif
