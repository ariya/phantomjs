/*
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *               1999 Waldo Bastian (bastian@kde.org)
 *               2001 Andreas Schlapbach (schlpbch@iam.unibe.ch)
 *               2001-2003 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2002, 2006, 2008 Apple Inc. All rights reserved.
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
#include "config.h"
#include "StyleBase.h"

#include "CSSMutableStyleDeclaration.h"
#include "Document.h"
#include "Node.h"
#include "StyleSheet.h"

namespace WebCore {

String StyleBase::cssText() const
{
    return "";
}

void StyleBase::checkLoaded()
{
    if (parent())
        parent()->checkLoaded();
}

Node* StyleBase::node()
{
    if (isStyleSheet())
        return static_cast<StyleSheet*>(this)->ownerNode();

    if (isMutableStyleDeclaration())
        return static_cast<CSSMutableStyleDeclaration*>(this)->node();

    return 0;
}

StyleSheet* StyleBase::stylesheet()
{
    StyleBase *b = this;
    while (b && !b->isStyleSheet())
        b = b->parent();
    return static_cast<StyleSheet*>(b);
}

KURL StyleBase::baseURL() const
{
    // Try to find the style sheet. If found look for its URL.
    // If it has none, get the URL from the parent sheet or the parent node.

    StyleSheet* sheet = const_cast<StyleBase*>(this)->stylesheet();
    if (!sheet)
        return KURL();
    if (!sheet->finalURL().isNull())
        return sheet->finalURL();
    if (sheet->parent())
        return sheet->parent()->baseURL();
    if (!sheet->ownerNode()) 
        return KURL();
    return sheet->ownerNode()->document()->baseURL();
}

}
