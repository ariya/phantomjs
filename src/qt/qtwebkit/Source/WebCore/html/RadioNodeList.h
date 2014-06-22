/*
 * Copyright (c) 2012 Motorola Mobility, Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY MOTOROLA MOBILITY, INC. AND ITS CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MOTOROLA MOBILITY, INC. OR ITS
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef RadioNodeList_h
#define RadioNodeList_h

#include "LiveNodeList.h"
#include <wtf/PassRefPtr.h>
#include <wtf/text/AtomicString.h>

namespace WebCore {

class RadioNodeList : public LiveNodeList {
public:
    static PassRefPtr<RadioNodeList> create(Node* rootNode, CollectionType type, const AtomicString& name)
    {
        ASSERT_UNUSED(type, type == RadioNodeListType);
        return adoptRef(new RadioNodeList(rootNode, name));
    }

    ~RadioNodeList();

    String value() const;
    void setValue(const String&);

protected:
    virtual bool nodeMatches(Element*) const;

private:
    RadioNodeList(Node*, const AtomicString& name);
    bool checkElementMatchesRadioNodeListFilter(Element*) const;

    AtomicString m_name;
};

} // namepsace

#endif

