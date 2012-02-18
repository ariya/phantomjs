/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2006, 2007 Apple Inc. All rights reserved.
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
 */

#ifndef StyleSheetList_h
#define StyleSheetList_h

#include <wtf/Forward.h>
#include <wtf/RefCounted.h>
#include <wtf/PassRefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class Document;
class HTMLStyleElement;
class StyleSheet;

typedef Vector<RefPtr<StyleSheet> > StyleSheetVector;

class StyleSheetList : public RefCounted<StyleSheetList> {
public:
    static PassRefPtr<StyleSheetList> create(Document* doc) { return adoptRef(new StyleSheetList(doc)); }
    ~StyleSheetList();

    void documentDestroyed();

    unsigned length() const;
    StyleSheet* item(unsigned index);

    HTMLStyleElement* getNamedItem(const String&) const;

    void swap(StyleSheetVector& sheets)
    {
        m_sheets.swap(sheets);
    }

    Document* document()
    {
        return m_doc;
    }
    
private:
    StyleSheetList(Document*);

    Document* m_doc;
    StyleSheetVector m_sheets;
};

} // namespace WebCore

#endif // StyleSheetList_h
