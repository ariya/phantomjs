/*
 * Copyright (C) 2006 Apple Computer, Inc.
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

#ifndef FrameTree_h
#define FrameTree_h

#include <wtf/text/AtomicString.h>

namespace WebCore {

    class Frame;
    class TreeScope;

    class FrameTree {
        WTF_MAKE_NONCOPYABLE(FrameTree);
    public:
        const static unsigned invalidCount = static_cast<unsigned>(-1);

        FrameTree(Frame* thisFrame, Frame* parentFrame) 
            : m_thisFrame(thisFrame)
            , m_parent(parentFrame)
            , m_previousSibling(0)
            , m_lastChild(0)
            , m_scopedChildCount(invalidCount)
        {
        }

        ~FrameTree();

        const AtomicString& name() const { return m_name; }
        const AtomicString& uniqueName() const { return m_uniqueName; }
        void setName(const AtomicString&);
        void clearName();
        Frame* parent() const;
        void setParent(Frame* parent) { m_parent = parent; }
        
        Frame* nextSibling() const { return m_nextSibling.get(); }
        Frame* previousSibling() const { return m_previousSibling; }
        Frame* firstChild() const { return m_firstChild.get(); }
        Frame* lastChild() const { return m_lastChild; }

        bool isDescendantOf(const Frame* ancestor) const;
        Frame* traverseNext(const Frame* stayWithin = 0) const;
        Frame* traverseNextWithWrap(bool) const;
        Frame* traversePreviousWithWrap(bool) const;
        
        void appendChild(PassRefPtr<Frame>);
        bool transferChild(PassRefPtr<Frame>);
        void detachFromParent() { m_parent = 0; }
        void removeChild(Frame*);

        Frame* child(unsigned index) const;
        Frame* child(const AtomicString& name) const;
        Frame* find(const AtomicString& name) const;
        unsigned childCount() const;

        AtomicString uniqueChildName(const AtomicString& requestedName) const;

        Frame* top() const;

        Frame* scopedChild(unsigned index) const;
        Frame* scopedChild(const AtomicString& name) const;
        unsigned scopedChildCount() const;

    private:
        Frame* deepLastChild() const;
        void actuallyAppendChild(PassRefPtr<Frame>);

        bool scopedBy(TreeScope*) const;
        Frame* scopedChild(unsigned index, TreeScope*) const;
        Frame* scopedChild(const AtomicString& name, TreeScope*) const;
        unsigned scopedChildCount(TreeScope*) const;

        Frame* m_thisFrame;

        Frame* m_parent;
        AtomicString m_name; // The actual frame name (may be empty).
        AtomicString m_uniqueName;

        RefPtr<Frame> m_nextSibling;
        Frame* m_previousSibling;
        RefPtr<Frame> m_firstChild;
        Frame* m_lastChild;
        mutable unsigned m_scopedChildCount;
    };

} // namespace WebCore

#ifndef NDEBUG
// Outside the WebCore namespace for ease of invocation from gdb.
void showFrameTree(const WebCore::Frame*);
#endif

#endif // FrameTree_h
