/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * (C) 2002-2003 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2002, 2006 Apple Computer, Inc.
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

#ifndef CSSRuleList_h
#define CSSRuleList_h

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class CSSRule;
class StyleList;

class CSSRuleList : public RefCounted<CSSRuleList> {
public:
    static PassRefPtr<CSSRuleList> create(StyleList* list, bool omitCharsetRules = false)
    {
        return adoptRef(new CSSRuleList(list, omitCharsetRules));
    }
    static PassRefPtr<CSSRuleList> create()
    {
        return adoptRef(new CSSRuleList);
    }
    ~CSSRuleList();

    unsigned length() const;
    CSSRule* item(unsigned index);

    // FIXME: Not part of the DOM.  Only used by media rules.  We should be able to remove them if we changed media rules to work
    // as StyleLists instead.
    unsigned insertRule(CSSRule*, unsigned index);
    void deleteRule(unsigned index);
    void append(CSSRule*);

    StyleList* styleList()
    {
        return m_list.get();
    }

private:
    CSSRuleList();
    CSSRuleList(StyleList*, bool omitCharsetRules);

    RefPtr<StyleList> m_list;
    Vector<RefPtr<CSSRule> > m_lstCSSRules; // FIXME: Want to eliminate, but used by IE rules() extension and still used by media rules.
};

} // namespace WebCore

#endif // CSSRuleList_h
