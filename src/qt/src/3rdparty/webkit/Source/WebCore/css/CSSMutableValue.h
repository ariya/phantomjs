/*
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
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

#ifndef CSSMutableValue_h
#define CSSMutableValue_h

#include "CSSValue.h"
#include "Node.h"

namespace WebCore {

class CSSMutableValue : public CSSValue {
public:
    CSSMutableValue()
        : m_node(0)
    {
    }

    virtual ~CSSMutableValue() { }
    virtual bool isMutableValue() const { return true; }

    Node* node() const { return m_node; }
    void setNode(Node* node) { m_node = node; }

    void setNeedsStyleRecalc()
    {
        if (!m_node)
            return;
        m_node->setNeedsStyleRecalc(FullStyleChange);
    }

private:
    Node* m_node;
};

} // namespace WebCore

#endif // CSSMutableValue_h
