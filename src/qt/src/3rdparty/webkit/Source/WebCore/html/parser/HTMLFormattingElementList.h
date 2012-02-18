/*
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GOOGLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef HTMLFormattingElementList_h
#define HTMLFormattingElementList_h

#include <wtf/Forward.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class Element;

// This may end up merged into HTMLElementStack.
class HTMLFormattingElementList {
    WTF_MAKE_NONCOPYABLE(HTMLFormattingElementList);
public:
    HTMLFormattingElementList();
    ~HTMLFormattingElementList();

    // Ideally Entry would be private, but HTMLTreeBuilder has to coordinate
    // between the HTMLFormattingElementList and HTMLElementStack and needs
    // access to Entry::isMarker() and Entry::replaceElement() to do so.
    class Entry {
    public:
        // Inline because they're hot and Vector<T> uses them.
        explicit Entry(Element* element)
            : m_element(element)
        {
            ASSERT(element);
        }
        enum MarkerEntryType { MarkerEntry };
        Entry(MarkerEntryType)
            : m_element(0)
        {
        }
        ~Entry() {}

        bool isMarker() const { return !m_element; }

        Element* element() const
        {
            // The fact that !m_element == isMarker() is an implementation detail
            // callers should check isMarker() before calling element().
            ASSERT(m_element);
            return m_element.get();
        }
        void replaceElement(PassRefPtr<Element> element) { m_element = element; }

        // Needed for use with Vector.  These are super-hot and must be inline.
        bool operator==(Element* element) const { return m_element == element; }
        bool operator!=(Element* element) const { return m_element != element; }

    private:
        RefPtr<Element> m_element;
    };

    class Bookmark {
    public:
        Bookmark(Entry* entry)
            : m_hasBeenMoved(false)
            , m_mark(entry)
        {
        }

        void moveToAfter(Entry* before)
        {
            m_hasBeenMoved = true;
            m_mark = before;
        }

        bool hasBeenMoved() const { return m_hasBeenMoved; }
        Entry* mark() const { return m_mark; }

    private:
        bool m_hasBeenMoved;
        Entry* m_mark;
    };

    bool isEmpty() const { return !size(); }
    size_t size() const { return m_entries.size(); }

    Element* closestElementInScopeWithName(const AtomicString&);

    Entry* find(Element*);
    bool contains(Element*);
    void append(Element*);
    void remove(Element*);

    Bookmark bookmarkFor(Element*);
    void swapTo(Element* oldElement, Element* newElement, const Bookmark&);

    void appendMarker();
    // clearToLastMarker also clears the marker (per the HTML5 spec).
    void clearToLastMarker();

    const Entry& at(size_t i) const { return m_entries[i]; }
    Entry& at(size_t i) { return m_entries[i]; }

#ifndef NDEBUG
    void show();
#endif

private:
    Entry* first() { return &at(0); }

    Vector<Entry> m_entries;
};

}

#endif // HTMLFormattingElementList_h
