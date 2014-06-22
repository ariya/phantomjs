/*
 * Copyright (C) 2006, 2007, 2008, 2013 Apple Inc. All rights reserved.
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

#include "config.h"
#include "Clipboard.h"

#include "CachedImage.h"
#include "CachedImageClient.h"
#include "DragData.h"
#include "Editor.h"
#include "FileList.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "HTMLImageElement.h"
#include "Image.h"
#include "Pasteboard.h"

namespace WebCore {

#if ENABLE(DRAG_SUPPORT)

class DragImageLoader FINAL : private CachedImageClient {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<DragImageLoader> create(Clipboard*);
    void startLoading(CachedResourceHandle<CachedImage>&);
    void stopLoading(CachedResourceHandle<CachedImage>&);

private:
    DragImageLoader(Clipboard*);
    virtual void imageChanged(CachedImage*, const IntRect*) OVERRIDE;
    Clipboard* m_clipboard;
};

#endif

Clipboard::Clipboard(ClipboardAccessPolicy policy, ClipboardType clipboardType
#if !USE(LEGACY_STYLE_ABSTRACT_CLIPBOARD_CLASS)
    , PassOwnPtr<Pasteboard> pasteboard, bool forFileDrag
#endif
)
    : m_policy(policy)
    , m_dropEffect("uninitialized")
    , m_effectAllowed("uninitialized")
    , m_dragStarted(false)
    , m_clipboardType(clipboardType)
#if !USE(LEGACY_STYLE_ABSTRACT_CLIPBOARD_CLASS)
    , m_pasteboard(pasteboard)
    , m_forFileDrag(forFileDrag)
#endif
{
}

Clipboard::~Clipboard()
{
#if !USE(LEGACY_STYLE_ABSTRACT_CLIPBOARD_CLASS) && ENABLE(DRAG_SUPPORT)
    if (m_dragImageLoader && m_dragImage)
        m_dragImageLoader->stopLoading(m_dragImage);
#endif
}
    
void Clipboard::setAccessPolicy(ClipboardAccessPolicy policy)
{
    // once you go numb, can never go back
    ASSERT(m_policy != ClipboardNumb || policy == ClipboardNumb);
    m_policy = policy;
}

bool Clipboard::canReadTypes() const
{
    return m_policy == ClipboardReadable || m_policy == ClipboardTypesReadable || m_policy == ClipboardWritable;
}

bool Clipboard::canReadData() const
{
    return m_policy == ClipboardReadable || m_policy == ClipboardWritable;
}

bool Clipboard::canWriteData() const
{
    return m_policy == ClipboardWritable;
}

bool Clipboard::canSetDragImage() const
{
    return m_clipboardType == DragAndDrop && (m_policy == ClipboardImageWritable || m_policy == ClipboardWritable);
}

// These "conversion" methods are called by both WebCore and WebKit, and never make sense to JS, so we don't
// worry about security for these. They don't allow access to the pasteboard anyway.

static DragOperation dragOpFromIEOp(const String& op)
{
    // yep, it's really just this fixed set
    if (op == "uninitialized")
        return DragOperationEvery;
    if (op == "none")
        return DragOperationNone;
    if (op == "copy")
        return DragOperationCopy;
    if (op == "link")
        return DragOperationLink;
    if (op == "move")
        return (DragOperation)(DragOperationGeneric | DragOperationMove);
    if (op == "copyLink")
        return (DragOperation)(DragOperationCopy | DragOperationLink);
    if (op == "copyMove")
        return (DragOperation)(DragOperationCopy | DragOperationGeneric | DragOperationMove);
    if (op == "linkMove")
        return (DragOperation)(DragOperationLink | DragOperationGeneric | DragOperationMove);
    if (op == "all")
        return DragOperationEvery;
    return DragOperationPrivate;  // really a marker for "no conversion"
}

static String IEOpFromDragOp(DragOperation op)
{
    bool moveSet = !!((DragOperationGeneric | DragOperationMove) & op);
    
    if ((moveSet && (op & DragOperationCopy) && (op & DragOperationLink))
        || (op == DragOperationEvery))
        return "all";
    if (moveSet && (op & DragOperationCopy))
        return "copyMove";
    if (moveSet && (op & DragOperationLink))
        return "linkMove";
    if ((op & DragOperationCopy) && (op & DragOperationLink))
        return "copyLink";
    if (moveSet)
        return "move";
    if (op & DragOperationCopy)
        return "copy";
    if (op & DragOperationLink)
        return "link";
    return "none";
}

DragOperation Clipboard::sourceOperation() const
{
    DragOperation op = dragOpFromIEOp(m_effectAllowed);
    ASSERT(op != DragOperationPrivate);
    return op;
}

DragOperation Clipboard::destinationOperation() const
{
    DragOperation op = dragOpFromIEOp(m_dropEffect);
    ASSERT(op == DragOperationCopy || op == DragOperationNone || op == DragOperationLink || op == (DragOperation)(DragOperationGeneric | DragOperationMove) || op == DragOperationEvery);
    return op;
}

void Clipboard::setSourceOperation(DragOperation op)
{
    ASSERT_ARG(op, op != DragOperationPrivate);
    m_effectAllowed = IEOpFromDragOp(op);
}

void Clipboard::setDestinationOperation(DragOperation op)
{
    ASSERT_ARG(op, op == DragOperationCopy || op == DragOperationNone || op == DragOperationLink || op == DragOperationGeneric || op == DragOperationMove || op == (DragOperation)(DragOperationGeneric | DragOperationMove));
    m_dropEffect = IEOpFromDragOp(op);
}

bool Clipboard::hasFileOfType(const String& type) const
{
    if (!canReadTypes())
        return false;
    
    RefPtr<FileList> fileList = files();
    if (fileList->isEmpty())
        return false;
    
    for (unsigned int f = 0; f < fileList->length(); f++) {
        if (equalIgnoringCase(fileList->item(f)->type(), type))
            return true;
    }
    return false;
}

bool Clipboard::hasStringOfType(const String& type) const
{
    if (!canReadTypes())
        return false;
    
    return types().contains(type); 
}
    
void Clipboard::setDropEffect(const String &effect)
{
    if (!isForDragAndDrop())
        return;

    // The attribute must ignore any attempts to set it to a value other than none, copy, link, and move. 
    if (effect != "none" && effect != "copy"  && effect != "link" && effect != "move")
        return;

    // FIXME: The spec actually allows this in all circumstances, even though there's no point in
    // setting the drop effect when this condition is not true.
    if (canReadTypes())
        m_dropEffect = effect;
}

void Clipboard::setEffectAllowed(const String &effect)
{
    if (!isForDragAndDrop())
        return;

    if (dragOpFromIEOp(effect) == DragOperationPrivate) {
        // This means that there was no conversion, and the effectAllowed that
        // we are passed isn't a valid effectAllowed, so we should ignore it,
        // and not set m_effectAllowed.

        // The attribute must ignore any attempts to set it to a value other than 
        // none, copy, copyLink, copyMove, link, linkMove, move, all, and uninitialized.
        return;
    }


    if (canWriteData())
        m_effectAllowed = effect;
}
    
DragOperation convertDropZoneOperationToDragOperation(const String& dragOperation)
{
    if (dragOperation == "copy")
        return DragOperationCopy;
    if (dragOperation == "move")
        return DragOperationMove;
    if (dragOperation == "link")
        return DragOperationLink;
    return DragOperationNone;
}

String convertDragOperationToDropZoneOperation(DragOperation operation)
{
    switch (operation) {
    case DragOperationCopy:
        return String("copy");
    case DragOperationMove:
        return String("move");
    case DragOperationLink:
        return String("link");
    default:
        return String("copy");
    }
}

bool Clipboard::hasDropZoneType(const String& keyword)
{
    if (keyword.startsWith("file:"))
        return hasFileOfType(keyword.substring(5));

    if (keyword.startsWith("string:"))
        return hasStringOfType(keyword.substring(7));

    return false;
}

#if USE(LEGACY_STYLE_ABSTRACT_CLIPBOARD_CLASS)

void Clipboard::setDragImage(Element* element, int x, int y)
{
    if (!canSetDragImage())
        return;

    if (element && isHTMLImageElement(element) && !element->inDocument())
        setDragImage(toHTMLImageElement(element)->cachedImage(), IntPoint(x, y));
    else
        setDragImageElement(element, IntPoint(x, y));
}

#else // !USE(LEGACY_STYLE_ABSTRACT_CLIPBOARD_CLASS)

PassRefPtr<Clipboard> Clipboard::createForCopyAndPaste(ClipboardAccessPolicy policy)
{
    return adoptRef(new Clipboard(policy, CopyAndPaste, policy == ClipboardWritable ? Pasteboard::createPrivate() : Pasteboard::createForCopyAndPaste()));
}

bool Clipboard::hasData()
{
    return m_pasteboard->hasData();
}

void Clipboard::clearData(const String& type)
{
    if (!canWriteData())
        return;

    m_pasteboard->clear(type);
}

void Clipboard::clearData()
{
    if (!canWriteData())
        return;

    m_pasteboard->clear();
}

String Clipboard::getData(const String& type) const
{
    if (!canReadData() || m_forFileDrag)
        return String();

    return m_pasteboard->readString(type);
}

bool Clipboard::setData(const String& type, const String& data)
{
    if (!canWriteData() || m_forFileDrag)
        return false;

    return m_pasteboard->writeString(type, data);
}

ListHashSet<String> Clipboard::types() const
{
    if (!canReadTypes())
        return ListHashSet<String>();

    return m_pasteboard->types();
}

// FIXME: We could cache the computed fileList if necessary
// Currently each access gets a new copy, setData() modifications to the
// clipboard are not reflected in any FileList objects the page has accessed and stored
PassRefPtr<FileList> Clipboard::files() const
{
    if (!canReadData() || (m_clipboardType == DragAndDrop && !m_forFileDrag))
        return FileList::create();

    Vector<String> filenames = m_pasteboard->readFilenames();
    RefPtr<FileList> fileList = FileList::create();
    for (size_t i = 0; i < filenames.size(); ++i)
        fileList->append(File::create(filenames[i], File::AllContentTypes));
    return fileList.release();
}

#if !ENABLE(DRAG_SUPPORT)

void Clipboard::setDragImage(Element*, int, int)
{
}

#else

// FIXME: Should be named createForDragAndDrop.
// FIXME: Should take const DragData& instead of DragData*.
// FIXME: Should not take Frame*.
PassRefPtr<Clipboard> Clipboard::create(ClipboardAccessPolicy policy, DragData* dragData, Frame*)
{
    return adoptRef(new Clipboard(policy, DragAndDrop, Pasteboard::createForDragAndDrop(*dragData), dragData->containsFiles()));
}

PassRefPtr<Clipboard> Clipboard::createForDragAndDrop()
{
    return adoptRef(new Clipboard(ClipboardWritable, DragAndDrop, Pasteboard::createForDragAndDrop()));
}

void Clipboard::setDragImage(Element* element, int x, int y)
{
    if (!canSetDragImage())
        return;

    CachedImage* image;
    if (element && isHTMLImageElement(element) && !element->inDocument())
        image = toHTMLImageElement(element)->cachedImage();
    else
        image = 0;

    m_dragLoc = IntPoint(x, y);

    if (m_dragImageLoader && m_dragImage)
        m_dragImageLoader->stopLoading(m_dragImage);
    m_dragImage = image;
    if (m_dragImage) {
        if (!m_dragImageLoader)
            m_dragImageLoader = DragImageLoader::create(this);
        m_dragImageLoader->startLoading(m_dragImage);
    }

    m_dragImageElement = image ? 0 : element;

    updateDragImage();
}

void Clipboard::updateDragImage()
{
    // Don't allow setting the image if we haven't started dragging yet; we'll rely on the dragging code
    // to install this drag image as part of getting the drag kicked off.
    if (!dragStarted())
        return;

    IntPoint computedHotSpot;
    DragImageRef computedImage = createDragImage(computedHotSpot);
    if (!computedImage)
        return;

    m_pasteboard->setDragImage(computedImage, computedHotSpot);
}

PassOwnPtr<DragImageLoader> DragImageLoader::create(Clipboard* clipboard)
{
    return adoptPtr(new DragImageLoader(clipboard));
}

DragImageLoader::DragImageLoader(Clipboard* clipboard)
    : m_clipboard(clipboard)
{
}

void DragImageLoader::startLoading(CachedResourceHandle<WebCore::CachedImage>& image)
{
    // FIXME: Does this really trigger a load? Does it need to?
    image->addClient(this);
}

void DragImageLoader::stopLoading(CachedResourceHandle<WebCore::CachedImage>& image)
{
    image->removeClient(this);
}

void DragImageLoader::imageChanged(CachedImage*, const IntRect*)
{
    m_clipboard->updateDragImage();
}

void Clipboard::writeRange(Range* range, Frame* frame)
{
    ASSERT(range);
    ASSERT(frame);
    // FIXME: This is a design mistake, a layering violation that should be fixed.
    // The code to write the range to a pasteboard should be an Editor function that takes a pasteboard argument.
    // FIXME: The frame argument seems redundant, since a Range is in a particular document, which has a corresponding frame.
    m_pasteboard->writeSelection(range, frame->editor().smartInsertDeleteEnabled() && frame->selection()->granularity() == WordGranularity, frame, IncludeImageAltTextForClipboard);
}

void Clipboard::writePlainText(const String& text)
{
    m_pasteboard->writePlainText(text, Pasteboard::CannotSmartReplace);
}

void Clipboard::writeURL(const KURL& url, const String& title, Frame* frame)
{
    ASSERT(frame);
    // FIXME: This is a design mistake, a layering violation that should be fixed.
    // The pasteboard writeURL function should not take a frame argument, nor does this function need a frame.
    m_pasteboard->writeURL(url, title, frame);
}

#endif // ENABLE(DRAG_SUPPORT)

#endif // !USE(LEGACY_STYLE_ABSTRACT_CLIPBOARD_CLASS)

} // namespace WebCore
