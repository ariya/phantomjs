/*
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 1999 Waldo Bastian (bastian@kde.org)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmial.com)
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

#ifndef StyleBase_h
#define StyleBase_h

#include <wtf/Forward.h>
#include <wtf/RefCounted.h>

namespace WebCore {

    class KURL;
    class Node;
    class StyleSheet;

    // Base class for most CSS DOM objects.

    // FIXME: We don't need these to all share one base class.
    // Refactor so they don't any more.

    class StyleBase : public RefCounted<StyleBase> {
    public:
        virtual ~StyleBase() { }

        StyleBase* parent() const { return m_parent; }
        void setParent(StyleBase* parent) { m_parent = parent; }

        // returns the url of the style sheet this object belongs to
        KURL baseURL() const;

        virtual bool isCSSStyleSheet() const { return false; }
        virtual bool isCharsetRule() { return false; }
        virtual bool isFontFaceRule() { return false; }
        virtual bool isImportRule() { return false; }
        virtual bool isKeyframeRule() { return false; }
        virtual bool isKeyframesRule() { return false; }
        virtual bool isMediaRule() { return false; }
        virtual bool isPageRule() { return false; }
        
        virtual bool isRule() { return false; }
        virtual bool isStyleRule() { return false; }
        virtual bool isStyleSheet() const { return false; }
        virtual bool isXSLStyleSheet() const { return false; }

        virtual bool isMutableStyleDeclaration() const { return false; }

        virtual String cssText() const;

        virtual void checkLoaded();

        bool useStrictParsing() const { return !m_parent || m_parent->useStrictParsing(); }

        virtual void insertedIntoParent() { }

        StyleSheet* stylesheet();
        Node* node();

    protected:
        StyleBase(StyleBase* parent)
            : m_parent(parent)
        {
        }

    private:
        StyleBase* m_parent;
    };
}

#endif
