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

#ifndef AccessibilityTableRow_h
#define AccessibilityTableRow_h

#include "AccessibilityRenderObject.h"

namespace WebCore {
    
class AccessibilityTableRow : public AccessibilityRenderObject {
    
protected:
    explicit AccessibilityTableRow(RenderObject*);
public:
    static PassRefPtr<AccessibilityTableRow> create(RenderObject*);
    virtual ~AccessibilityTableRow();
    
    virtual bool isTableRow() const;

    // retrieves the "row" header (a th tag in the rightmost column)
    virtual AccessibilityObject* headerObject();
    virtual AccessibilityObject* parentTable() const;
    
    void setRowIndex(int rowIndex) { m_rowIndex = rowIndex; }
    int rowIndex() const { return m_rowIndex; }

    // allows the table to add other children that may not originate
    // in the row, but their col/row spans overlap into it
    void appendChild(AccessibilityObject*);
    
protected:
    virtual AccessibilityRole determineAccessibilityRole();

private:
    int m_rowIndex;
    
    virtual AccessibilityObject* observableObject() const;
    virtual bool computeAccessibilityIsIgnored() const;
}; 
   
} // namespace WebCore 

#endif // AccessibilityTableRow_h
