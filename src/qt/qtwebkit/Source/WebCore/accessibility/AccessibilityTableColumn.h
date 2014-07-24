/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AccessibilityTableColumn_h
#define AccessibilityTableColumn_h

#include "AccessibilityMockObject.h"
#include "AccessibilityTable.h"
#include "IntRect.h"

namespace WebCore {
    
class RenderTableSection;

class AccessibilityTableColumn : public AccessibilityMockObject {
    
private:
    AccessibilityTableColumn();
public:
    static PassRefPtr<AccessibilityTableColumn> create();
    virtual ~AccessibilityTableColumn();
    
    AccessibilityObject* headerObject();
        
    virtual AccessibilityRole roleValue() const { return ColumnRole; }
    virtual bool isTableColumn() const { return true; }
    
    void setColumnIndex(int columnIndex) { m_columnIndex = columnIndex; }
    int columnIndex() const { return m_columnIndex; }    
    
    virtual void addChildren();
    virtual void setParent(AccessibilityObject*);
    
    virtual LayoutRect elementRect() const;
    
private:    
    unsigned m_columnIndex;
    LayoutRect m_columnRect;
    
    AccessibilityObject* headerObjectForSection(RenderTableSection*, bool thTagRequired);
    virtual bool computeAccessibilityIsIgnored() const;
};
   
} // namespace WebCore 

#endif // AccessibilityTableColumn_h
