/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QtStyleOptionWebComboBox_h
#define QtStyleOptionWebComboBox_h

#include "HTMLSelectElement.h"
#include "RenderObject.h"

#include <QStyleOption>

namespace WebCore {

class RenderObject;

class QtStyleOptionWebComboBox : public QStyleOptionComboBox {
public:
    QtStyleOptionWebComboBox(RenderObject* o)
        : QStyleOptionComboBox()
    #if ENABLE(NO_LISTBOX_RENDERING)
        , m_multiple(checkMultiple(o))
    #else
        , m_multiple(false)
    #endif
    {
    }

    bool multiple() const { return m_multiple; }

private:
    bool m_multiple;

    bool checkMultiple(RenderObject* o)
    {
        HTMLSelectElement* select = o ? static_cast<HTMLSelectElement*>(o->node()) : 0;
        return select ? select->multiple() : false;
    }
};

}

#endif // QtStyleOptionWebComboBox_h
