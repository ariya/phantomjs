/*
 * Copyright (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2001 Tobias Anton (anton@stud.fbi.fh-darmstadt.de)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2003, 2004, 2005, 2006, 2008, 2013 Apple Inc. All rights reserved.
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

#ifndef Clipboard_h
#define Clipboard_h

#include "CachedResourceHandle.h"
#include "ClipboardAccessPolicy.h"
#include "DragActions.h"
#include "DragImage.h"
#include "IntPoint.h"
#include "Node.h"

// This DOM object now works by calling through to classes in the platform layer.
// Specifically, the class currently named Pasteboard. The legacy style instead
// uses this as an abstract base class.
#define WTF_USE_LEGACY_STYLE_ABSTRACT_CLIPBOARD_CLASS PLATFORM(IOS)

#if USE(LEGACY_STYLE_ABSTRACT_CLIPBOARD_CLASS)
#define LEGACY_VIRTUAL virtual
#else
#define LEGACY_VIRTUAL
#endif

#if USE(LEGACY_STYLE_ABSTRACT_CLIPBOARD_CLASS)
#define LEGACY_PURE = 0
#else
#define LEGACY_PURE
#endif

namespace WebCore {

    class CachedImage;
    class DataTransferItemList;
    class DragData;
    class DragImageLoader;
    class FileList;
    class Frame;
    class Pasteboard;

    // State available during IE's events for drag and drop and copy/paste
    class Clipboard : public RefCounted<Clipboard> {
    public:
        // Whether this clipboard is serving a drag-drop or copy-paste request.
        enum ClipboardType {
            CopyAndPaste,
            DragAndDrop,
        };
        
        static PassRefPtr<Clipboard> create(ClipboardAccessPolicy, DragData*, Frame*);

        LEGACY_VIRTUAL ~Clipboard();

        bool isForCopyAndPaste() const { return m_clipboardType == CopyAndPaste; }
        bool isForDragAndDrop() const { return m_clipboardType == DragAndDrop; }

        String dropEffect() const { return dropEffectIsUninitialized() ? "none" : m_dropEffect; }
        void setDropEffect(const String&);
        bool dropEffectIsUninitialized() const { return m_dropEffect == "uninitialized"; }
        String effectAllowed() const { return m_effectAllowed; }
        void setEffectAllowed(const String&);
    
        LEGACY_VIRTUAL void clearData(const String& type) LEGACY_PURE;
        LEGACY_VIRTUAL void clearData() LEGACY_PURE;

        void setDragImage(Element*, int x, int y);
#if USE(LEGACY_STYLE_ABSTRACT_CLIPBOARD_CLASS)
        virtual void setDragImage(CachedImage*, const IntPoint&) = 0;
        virtual void setDragImageElement(Node*, const IntPoint&) = 0;
#endif

        LEGACY_VIRTUAL String getData(const String& type) const LEGACY_PURE;
        LEGACY_VIRTUAL bool setData(const String& type, const String& data) LEGACY_PURE;
    
        LEGACY_VIRTUAL ListHashSet<String> types() const LEGACY_PURE;
        LEGACY_VIRTUAL PassRefPtr<FileList> files() const LEGACY_PURE;

        IntPoint dragLocation() const { return m_dragLoc; }
        CachedImage* dragImage() const { return m_dragImage.get(); }
        Node* dragImageElement() const { return m_dragImageElement.get(); }
        
        LEGACY_VIRTUAL DragImageRef createDragImage(IntPoint& dragLocation) const LEGACY_PURE;
#if ENABLE(DRAG_SUPPORT)
        LEGACY_VIRTUAL void declareAndWriteDragImage(Element*, const KURL&, const String& title, Frame*) LEGACY_PURE;
#endif
        LEGACY_VIRTUAL void writeURL(const KURL&, const String&, Frame*) LEGACY_PURE;
        LEGACY_VIRTUAL void writeRange(Range*, Frame*) LEGACY_PURE;
        LEGACY_VIRTUAL void writePlainText(const String&) LEGACY_PURE;

        LEGACY_VIRTUAL bool hasData() LEGACY_PURE;

        void setAccessPolicy(ClipboardAccessPolicy);
        bool canReadTypes() const;
        bool canReadData() const;
        bool canWriteData() const;
        // Note that the spec doesn't actually allow drag image modification outside the dragstart
        // event. This capability is maintained for backwards compatiblity for ports that have
        // supported this in the past. On many ports, attempting to set a drag image outside the
        // dragstart operation is a no-op anyway.
        bool canSetDragImage() const;

        DragOperation sourceOperation() const;
        DragOperation destinationOperation() const;
        void setSourceOperation(DragOperation);
        void setDestinationOperation(DragOperation);
        
        bool hasDropZoneType(const String&);
        
        void setDragHasStarted() { m_dragStarted = true; }

#if ENABLE(DATA_TRANSFER_ITEMS)
        LEGACY_VIRTUAL PassRefPtr<DataTransferItemList> items() = 0;
#endif
        
#if !USE(LEGACY_STYLE_ABSTRACT_CLIPBOARD_CLASS)
        static PassRefPtr<Clipboard> createForCopyAndPaste(ClipboardAccessPolicy);

        const Pasteboard& pasteboard() { return *m_pasteboard; }
#endif

#if !USE(LEGACY_STYLE_ABSTRACT_CLIPBOARD_CLASS) && ENABLE(DRAG_SUPPORT)
        static PassRefPtr<Clipboard> createForDragAndDrop();

        void updateDragImage();
#endif

    protected:
#if !USE(LEGACY_STYLE_ABSTRACT_CLIPBOARD_CLASS)
        Clipboard(ClipboardAccessPolicy, ClipboardType, PassOwnPtr<Pasteboard>, bool forFileDrag = false);
#else
        Clipboard(ClipboardAccessPolicy, ClipboardType);
#endif

        bool dragStarted() const { return m_dragStarted; }
        
    private:
        bool hasFileOfType(const String&) const;
        bool hasStringOfType(const String&) const;

        // Instead of using this member directly, prefer to use the can*() methods above.
        ClipboardAccessPolicy m_policy;
        String m_dropEffect;
        String m_effectAllowed;
        bool m_dragStarted;
        ClipboardType m_clipboardType;
        
    protected:
        IntPoint m_dragLoc;
        CachedResourceHandle<CachedImage> m_dragImage;
        RefPtr<Node> m_dragImageElement;

#if !USE(LEGACY_STYLE_ABSTRACT_CLIPBOARD_CLASS)
    private:
        OwnPtr<Pasteboard> m_pasteboard;
        bool m_forFileDrag;
#if ENABLE(DRAG_SUPPORT)
        OwnPtr<DragImageLoader> m_dragImageLoader;
#endif
#endif
    };

    DragOperation convertDropZoneOperationToDragOperation(const String& dragOperation);
    String convertDragOperationToDropZoneOperation(DragOperation);

#undef LEGACY_VIRTUAL
#undef LEGACY_PURE

} // namespace WebCore

#endif // Clipboard_h
