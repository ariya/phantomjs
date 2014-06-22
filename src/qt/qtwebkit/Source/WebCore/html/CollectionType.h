/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef CollectionType_h
#define CollectionType_h

namespace WebCore {

enum CollectionType {
    // Unnamed HTMLCollection types cached in the document.
    DocImages,    // all <img> elements in the document
    DocApplets,   // all <object> and <applet> elements
    DocEmbeds,    // all <embed> elements
    DocForms,     // all <form> elements
    DocLinks,     // all <a> _and_ <area> elements with a value for href
    DocAnchors,   // all <a> elements with a value for name
    DocScripts,   // all <script> elements
    DocAll,       // "all" elements (IE)

    // Named collection types cached in the document.
    WindowNamedItems,
    DocumentNamedItems,

    // Unnamed HTMLCollection types cached in elements.
    NodeChildren, // first-level children (IE)
    TableTBodies, // all <tbody> elements in this table
    TSectionRows, // all row elements in this table section
    TableRows,
    TRCells,      // all cells in this row
    SelectOptions,
    SelectedOptions,
    DataListOptions,
    MapAreas,
#if ENABLE(MICRODATA)
    ItemProperties, // Microdata item properties in the document
#endif
    FormControls,

    // Live NodeList.
    ChildNodeListType,
    ClassNodeListType,
    NameNodeListType,
    TagNodeListType,
    HTMLTagNodeListType,
    RadioNodeListType,
    LabelsNodeListType,
    MicroDataItemListType,
    PropertyNodeListType,
};

static const CollectionType FirstNodeListType = ChildNodeListType;

inline bool isNodeList(CollectionType type)
{
    return type >= FirstNodeListType;
}

} // namespace

#endif
