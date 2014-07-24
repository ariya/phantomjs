/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef HTMLBDIElement_h
#define HTMLBDIElement_h

#include "HTMLElement.h"

namespace WebCore {

class HTMLBDIElement FINAL : public HTMLElement {
public:
    static PassRefPtr<HTMLBDIElement> create(const QualifiedName& name, Document* document)
    {
        return adoptRef(new HTMLBDIElement(name, document));
    }

private:
    HTMLBDIElement(const QualifiedName& name, Document* document)
        : HTMLElement(name, document)
    {
        // FIXME: Rename setSelfOrAncestorHasDirAutoAttribute to reflect the fact bdi also uses this flag.
        setSelfOrAncestorHasDirAutoAttribute(true);
    }
};

} // namespace WebCore

#endif
