/*
 * Copyright (C) 2006, 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef Pasteboard_h
#define Pasteboard_h

#include "DragImage.h"
#include <wtf/ListHashSet.h>
#include <wtf/Noncopyable.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

#if PLATFORM(GTK)
#include "DragData.h"
typedef struct _GtkClipboard GtkClipboard;
#endif

#if PLATFORM(QT)
#include <QMimeData>
#endif

#if PLATFORM(WIN)
#include "COMPtr.h"
#include "WCDataObject.h"
#include <objidl.h>
#include <windows.h>
typedef struct HWND__* HWND;
#endif

// FIXME: This class is too high-level to be in the platform directory, since it
// uses the DOM and makes calls to Editor. It should either be divested of its
// knowledge of the frame and editor or moved into the editing directory.

namespace WebCore {

#if PLATFORM(MAC)
#if PLATFORM(IOS)
// FIXME: This is only temporary until Pasteboard is refactored for iOS.
extern NSString *WebArchivePboardType;
#else
extern const char* WebArchivePboardType;
#endif
extern const char* WebSmartPastePboardType;
extern const char* WebURLNamePboardType;
extern const char* WebURLPboardType;
extern const char* WebURLsWithTitlesPboardType;
#endif

class Clipboard;
class DocumentFragment;
class DragData;
class Element;
class Frame;
class HitTestResult;
class KURL;
class Node;
class Range;
class SharedBuffer;

enum ShouldSerializeSelectedTextForClipboard { DefaultSelectedTextType, IncludeImageAltTextForClipboard };
    
class Pasteboard {
    WTF_MAKE_NONCOPYABLE(Pasteboard); WTF_MAKE_FAST_ALLOCATED;
public:
    enum SmartReplaceOption {
        CanSmartReplace,
        CannotSmartReplace
    };

#if PLATFORM(MAC) && !PLATFORM(IOS)
    static PassOwnPtr<Pasteboard> create(const String& pasteboardName);
    String name() const { return m_pasteboardName; }

    void writeSelectionForTypes(const Vector<String>& pasteboardTypes, bool canSmartCopyOrDelete, Frame*, ShouldSerializeSelectedTextForClipboard);
    explicit Pasteboard(const String& pasteboardName);
    static PassRefPtr<SharedBuffer> getDataSelection(Frame*, const String& pasteboardType);
#endif

#if PLATFORM(GTK)
    static PassOwnPtr<Pasteboard> create(PassRefPtr<DataObjectGtk>);
    static PassOwnPtr<Pasteboard> create(GtkClipboard*);
    PassRefPtr<DataObjectGtk> dataObject() const;
#endif

#if PLATFORM(QT)
    static PassOwnPtr<Pasteboard> create(const QMimeData* readableClipboard = 0, bool isForDragAndDrop = false);
    QMimeData* clipboardData() const { return m_writableData; }
    void invalidateWritableData() const { m_writableData = 0; }
    bool isForDragAndDrop() const { return m_isForDragAndDrop; }
    bool isForCopyAndPaste() const { return !m_isForDragAndDrop; }
    void updateSystemPasteboard();
#endif

    // Deprecated. Use createForCopyAndPaste instead.
    static Pasteboard* generalPasteboard();

    static PassOwnPtr<Pasteboard> createForCopyAndPaste();
    static PassOwnPtr<Pasteboard> createPrivate(); // Corresponds to the "unique pasteboard" concept on Mac. Used in editing, not sure exactly for what purpose.

#if ENABLE(DRAG_SUPPORT)
    static PassOwnPtr<Pasteboard> createForDragAndDrop();
    static PassOwnPtr<Pasteboard> createForDragAndDrop(const DragData&);
#endif

    bool hasData();
    ListHashSet<String> types();

    String readString(const String& type);
    Vector<String> readFilenames();

    bool writeString(const String& type, const String& data);
    void writeSelection(Range*, bool canSmartCopyOrDelete, Frame*, ShouldSerializeSelectedTextForClipboard = DefaultSelectedTextType);
    void writePlainText(const String&, SmartReplaceOption);
#if !PLATFORM(IOS)
    void writeURL(const KURL&, const String&, Frame* = 0);
    void writeImage(Node*, const KURL&, const String& title);
#else
    void writeImage(Node*, Frame*);
    void writePlainText(const String&, Frame*);
    static NSArray* supportedPasteboardTypes();
#endif
    void writePasteboard(const Pasteboard& sourcePasteboard);
    void writeClipboard(Clipboard*);

    void clear();
    void clear(const String& type);

    bool canSmartReplace();

#if ENABLE(DRAG_SUPPORT)
    void setDragImage(DragImageRef, const IntPoint& hotSpot);
#endif

    // FIXME: Having these functions here is a layering violation.
    // These functions need to move to the editing directory even if they have platform-specific aspects.
    PassRefPtr<DocumentFragment> documentFragment(Frame*, PassRefPtr<Range>, bool allowPlainText, bool& chosePlainText);
    String plainText(Frame* = 0);
    
#if PLATFORM(QT) || PLATFORM(GTK)
    bool isSelectionMode() const;
    void setSelectionMode(bool);
#else
    bool isSelectionMode() const { return false; }
    void setSelectionMode(bool) { }
#endif

#if PLATFORM(WIN)
    COMPtr<IDataObject> dataObject() const { return m_dataObject; }
    void setExternalDataObject(IDataObject*);
    void writeURLToWritableDataObject(const KURL&, const String&);
    COMPtr<WCDataObject> writableDataObject() const { return m_writableDataObject; }
    void writeImageToDataObject(Element*, const KURL&);
#endif

#if PLATFORM(GTK) || PLATFORM(QT)
    ~Pasteboard();
#endif

private:
    Pasteboard();

#if PLATFORM(GTK)
    Pasteboard(PassRefPtr<DataObjectGtk>);
    Pasteboard(GtkClipboard*);
#endif

#if PLATFORM(QT)
    Pasteboard(const QMimeData* , bool);
#endif

#if PLATFORM(WIN)
    explicit Pasteboard(IDataObject*);
    explicit Pasteboard(WCDataObject*);
    explicit Pasteboard(const DragDataMap&);
#endif

#if PLATFORM(MAC) && !PLATFORM(IOS)
    String m_pasteboardName;
    long m_changeCount;
#endif

#if PLATFORM(IOS)
    PassRefPtr<DocumentFragment> documentFragmentForPasteboardItemAtIndex(Frame*, int index, bool allowPlainText, bool& chosePlainText);
#endif

#if PLATFORM(WIN)
    void finishCreatingPasteboard();
    void writeRangeToDataObject(Range*, Frame*);
    void writeURLToDataObject(const KURL&, const String&, Frame*);
    void writePlainTextToDataObject(const String&, SmartReplaceOption);

    HWND m_owner;
    COMPtr<IDataObject> m_dataObject;
    COMPtr<WCDataObject> m_writableDataObject;
    DragDataMap m_dragDataMap;
#endif

#if PLATFORM(QT)
    const QMimeData* readData() const;

    bool m_selectionMode;
    const QMimeData* m_readableData;
    mutable QMimeData* m_writableData;
    bool m_isForDragAndDrop;
#endif

#if PLATFORM(GTK)
    RefPtr<DataObjectGtk> m_dataObject;
    GtkClipboard* m_gtkClipboard;
#endif

};

} // namespace WebCore

#endif // Pasteboard_h
