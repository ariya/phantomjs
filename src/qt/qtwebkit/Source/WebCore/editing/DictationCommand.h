/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef DictationCommand_h
#define DictationCommand_h

#include "DictationAlternative.h"
#include "TextInsertionBaseCommand.h"

namespace WebCore {

class DocumentMarker;

class DictationCommand : public TextInsertionBaseCommand {
    friend class DictationCommandLineOperation;
public:
    static void insertText(Document*, const String&, const Vector<DictationAlternative>& alternatives, const VisibleSelection&);
    virtual bool isDictationCommand() const { return true; }
private:
    static PassRefPtr<DictationCommand> create(Document* document, const String& text, const Vector<DictationAlternative>& alternatives)
    {
        return adoptRef(new DictationCommand(document, text, alternatives));
    }

    DictationCommand(Document*, const String& text, const Vector<DictationAlternative>& alternatives);
    
    virtual void doApply();

    void insertTextRunWithoutNewlines(size_t lineStart, size_t lineLength);
    void insertParagraphSeparator();
    void collectDictationAlternativesInRange(size_t rangeStart, size_t rangeLength, Vector<DictationAlternative>&);

    String m_textToInsert;
    Vector<DictationAlternative> m_alternatives;
};
}

#endif // DictationCommand_h
