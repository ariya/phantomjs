/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef Text_h
#define Text_h

#include "CharacterData.h"

namespace WebCore {

class RenderText;

class Text : public CharacterData {
public:
    static const unsigned defaultLengthLimit = 1 << 16;

    static PassRefPtr<Text> create(Document*, const String&);
    static PassRefPtr<Text> createWithLengthLimit(Document*, const String&, unsigned positionInString, unsigned lengthLimit = defaultLengthLimit);
    static PassRefPtr<Text> createEditingText(Document*, const String&);

    PassRefPtr<Text> splitText(unsigned offset, ExceptionCode&);

    // DOM Level 3: http://www.w3.org/TR/DOM-Level-3-Core/core.html#ID-1312295772

    String wholeText() const;
    PassRefPtr<Text> replaceWholeText(const String&, ExceptionCode&);
    
    void recalcTextStyle(StyleChange);
    void createTextRendererIfNeeded();
    bool textRendererIsNeeded(const NodeRenderingContext&);
    RenderText* createTextRenderer(RenderArena*, RenderStyle*);
    void updateTextRenderer(unsigned offsetOfReplacedData, unsigned lengthOfReplacedData);

    virtual void attach(const AttachContext& = AttachContext()) OVERRIDE FINAL;
    
    virtual bool canContainRangeEndPoint() const OVERRIDE FINAL { return true; }

protected:
    Text(Document* document, const String& data, ConstructionType type)
        : CharacterData(document, data, type)
    {
    }

private:
    virtual String nodeName() const OVERRIDE;
    virtual NodeType nodeType() const OVERRIDE;
    virtual PassRefPtr<Node> cloneNode(bool deep) OVERRIDE;
    virtual bool childTypeAllowed(NodeType) const OVERRIDE;

    virtual PassRefPtr<Text> virtualCreate(const String&);

#ifndef NDEBUG
    virtual void formatForDebugger(char* buffer, unsigned length) const;
#endif
};

inline Text* toText(Node* node)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!node || node->isTextNode());
    return static_cast<Text*>(node);
}

} // namespace WebCore

#endif // Text_h
