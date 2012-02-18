/*
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *               1999 Waldo Bastian (bastian@kde.org)
 * Copyright (C) 2004, 2006, 2008 Apple Inc. All rights reserved.
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

#ifndef StyleList_h
#define StyleList_h

#include "StyleBase.h"
#include <wtf/Forward.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

    // a style class which has a list of children (StyleSheets for example)
    class StyleList : public StyleBase {
    public:
        unsigned length() { return m_children.size(); }
        StyleBase* item(unsigned num) { return num < length() ? m_children[num].get() : 0; }

        void append(PassRefPtr<StyleBase>);
        void insert(unsigned position, PassRefPtr<StyleBase>);
        void remove(unsigned position);

    protected:
        StyleList(StyleBase* parent) : StyleBase(parent) { }

        Vector<RefPtr<StyleBase> > m_children;
    };
}

#endif
