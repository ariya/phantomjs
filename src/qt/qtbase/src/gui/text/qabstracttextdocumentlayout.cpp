/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qabstracttextdocumentlayout.h>
#include <qtextformat.h>
#include "qtextdocument_p.h"
#include "qtextengine_p.h"

#include "qabstracttextdocumentlayout_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QAbstractTextDocumentLayout
    \reentrant

    \brief The QAbstractTextDocumentLayout class is an abstract base
    class used to implement custom layouts for QTextDocuments.
    \inmodule QtGui

    \ingroup richtext-processing

    The standard layout provided by Qt can handle simple word processing
    including inline images, lists and tables.

    Some applications, e.g., a word processor or a DTP application might need
    more features than the ones provided by Qt's layout engine, in which case
    you can subclass QAbstractTextDocumentLayout to provide custom layout
    behavior for your text documents.

    An instance of the QAbstractTextDocumentLayout subclass can be installed
    on a QTextDocument object with the
    \l{QTextDocument::}{setDocumentLayout()} function.

    You can insert custom objects into a QTextDocument; see the
    QTextObjectInterface class description for details.

    \sa QTextObjectInterface
*/

/*!
    \class QTextObjectInterface
    \brief The QTextObjectInterface class allows drawing of
           custom text objects in \l{QTextDocument}s.
    \since 4.5
    \inmodule QtGui

    A text object describes the structure of one or more elements in a
    text document; for instance, images imported from HTML are
    implemented using text objects. A text object knows how to lay out
    and draw its elements when a document is being rendered.

    Qt allows custom text objects to be inserted into a document by
    registering a custom \l{QTextCharFormat::objectType()}{object
    type} with QTextCharFormat. A QTextObjectInterface must also be
    implemented for this type and be
    \l{QAbstractTextDocumentLayout::registerHandler()}{registered}
    with the QAbstractTextDocumentLayout of the document. When the
    object type is encountered while rendering a QTextDocument, the
    intrinsicSize() and drawObject() functions of the interface are
    called.

    The following list explains the required steps of inserting a
    custom text object into a document:

    \list
        \li Choose an \a objectType. The \a objectType is an integer with a
            value greater or equal to QTextFormat::UserObject.
         \li Create a QTextCharFormat object and set the object type to the
            chosen type using the setObjectType() function.
         \li Implement the QTextObjectInterface class.
         \li Call QAbstractTextDocumentLayout::registerHandler() with an instance of your
            QTextObjectInterface subclass to register your object type.
         \li Insert QChar::ObjectReplacementCharacter with the aforementioned
            QTextCharFormat of the chosen object type into the document.
            As mentioned, the functions of QTextObjectInterface
            \l{QTextObjectInterface::}{intrinsicSize()} and
            \l{QTextObjectInterface::}{drawObject()} will then be called with the
            QTextFormat as parameter whenever the replacement character is
            encountered.
    \endlist

    A class implementing a text object needs to inherit both QObject
    and QTextObjectInterface. QObject must be the first class
    inherited. For instance:

    \snippet qtextobject/textobjectinterface.h 0

    The data of a text object is usually stored in the QTextCharFormat
    using QTextCharFormat::setProperty(), and then retrieved with
    QTextCharFormat::property().

    \warning Copy and Paste operations ignore custom text objects.

    \sa {Text Object Example}, QTextCharFormat, QTextLayout
*/

/*!
    \fn QTextObjectInterface::~QTextObjectInterface()

    Destroys this QTextObjectInterface.
*/

/*!
    \fn virtual QSizeF QTextObjectInterface::intrinsicSize(QTextDocument *doc, int posInDocument, const QTextFormat &format) = 0

    The intrinsicSize() function returns the size of the text object
    represented by \a format in the given document (\a doc) at the
    given position (\a posInDocument).

    The size calculated will be used for subsequent calls to
    drawObject() for this \a format.

    \sa drawObject()
*/

/*!
    \fn virtual void QTextObjectInterface::drawObject(QPainter *painter, const QRectF &rect, QTextDocument *doc, int posInDocument, const QTextFormat &format) = 0

    Draws this text object using the specified \a painter.

    The size of the rectangle, \a rect, to draw in is the size
    previously calculated by intrinsicSize(). The rectangles position
    is relative to the \a painter.

    You also get the document (\a doc) and the position (\a
    posInDocument) of the \a format in that document.

    \sa intrinsicSize()
*/

/*!
    \fn void QAbstractTextDocumentLayout::update(const QRectF &rect)

    This signal is emitted when the rectangle \a rect has been updated.

    Subclasses of QAbstractTextDocumentLayout should emit this signal when
    the layout of the contents change in order to repaint.
*/

/*!
   \fn void QAbstractTextDocumentLayout::updateBlock(const QTextBlock &block)
   \since 4.4

   This signal is emitted when the specified \a block has been updated.

   Subclasses of QAbstractTextDocumentLayout should emit this signal when
   the layout of \a block has changed in order to repaint.
*/

/*!
    \fn void QAbstractTextDocumentLayout::documentSizeChanged(const QSizeF &newSize)

    This signal is emitted when the size of the document layout changes to
    \a newSize.

    Subclasses of QAbstractTextDocumentLayout should emit this signal when the
    document's entire layout size changes. This signal is useful for widgets
    that display text documents since it enables them to update their scroll
    bars correctly.

    \sa documentSize()
*/

/*!
    \fn void QAbstractTextDocumentLayout::pageCountChanged(int newPages)

    This signal is emitted when the number of pages in the layout changes;
    \a newPages is the updated page count.

    Subclasses of QAbstractTextDocumentLayout should emit this signal when
    the number of pages in the layout has changed. Changes to the page count
    are caused by changes to the layout or the document content itself.

    \sa pageCount()
*/

/*!
    \fn int QAbstractTextDocumentLayout::pageCount() const

    Returns the number of pages contained in the layout.

    \sa pageCountChanged()
*/

/*!
    \fn QSizeF QAbstractTextDocumentLayout::documentSize() const

    Returns the total size of the document's layout.

    This information can be used by display widgets to update their scroll bars
    correctly.

    \sa documentSizeChanged(), QTextDocument::pageSize
*/

/*!
    \fn void QAbstractTextDocumentLayout::draw(QPainter *painter, const PaintContext &context)

    Draws the layout with the given \a painter using the given \a context.
*/

/*!
    \fn int QAbstractTextDocumentLayout::hitTest(const QPointF &point, Qt::HitTestAccuracy accuracy) const

    Returns the cursor position for the given \a point with the specified
    \a accuracy. Returns -1 if no valid cursor position was found.
*/

/*!
    \fn void QAbstractTextDocumentLayout::documentChanged(int position, int charsRemoved, int charsAdded)

    This function is called whenever the contents of the document change. A
    change occurs when text is inserted, removed, or a combination of these
    two. The change is specified by \a position, \a charsRemoved, and
    \a charsAdded corresponding to the starting character position of the
    change, the number of characters removed from the document, and the
    number of characters added.

    For example, when inserting the text "Hello" into an empty document,
    \a charsRemoved would be 0 and \a charsAdded would be 5 (the length of
    the string).

    Replacing text is a combination of removing and inserting. For example, if
    the text "Hello" gets replaced by "Hi", \a charsRemoved would be 5 and
    \a charsAdded would be 2.

    For subclasses of QAbstractTextDocumentLayout, this is the central function
    where a large portion of the work to lay out and position document contents
    is done.

    For example, in a subclass that only arranges blocks of text, an
    implementation of this function would have to do the following:

    \list
        \li Determine the list of changed \l{QTextBlock}(s) using the parameters
            provided.
        \li Each QTextBlock object's corresponding QTextLayout object needs to
            be processed. You can access the \l{QTextBlock}'s layout using the
            QTextBlock::layout() function. This processing should take the
            document's page size into consideration.
        \li If the total number of pages changed, the pageCountChanged() signal
            should be emitted.
        \li If the total size changed, the documentSizeChanged() signal should
            be emitted.
        \li The update() signal should be emitted to schedule a repaint of areas
            in the layout that require repainting.
    \endlist

    \sa QTextLayout
*/

/*!
    \class QAbstractTextDocumentLayout::PaintContext
    \reentrant
    \inmodule QtGui

    \brief The QAbstractTextDocumentLayout::PaintContext class is a convenience
    class defining the parameters used when painting a document's layout.

    A paint context is used when rendering custom layouts for QTextDocuments
    with the QAbstractTextDocumentLayout::draw() function. It is specified by
    a \l {cursorPosition}{cursor position}, \l {palette}{default text color},
    \l clip rectangle and a collection of \l selections.

    \sa QAbstractTextDocumentLayout
*/

/*!
    \fn QAbstractTextDocumentLayout::PaintContext::PaintContext()
    \internal
*/

/*!
    \variable QAbstractTextDocumentLayout::PaintContext::cursorPosition

    \brief the position within the document, where the cursor line should be
    drawn.

    The default value is -1.
*/

/*!
    \variable QAbstractTextDocumentLayout::PaintContext::palette

    \brief the default color that is used for the text, when no color is
    specified.

    The default value is the application's default palette.
*/

/*!
    \variable QAbstractTextDocumentLayout::PaintContext::clip

    \brief a hint to the layout specifying the area around paragraphs, frames
    or text require painting.

    Everything outside of this rectangle does not need to be painted.

    Specifying a clip rectangle can speed up drawing of large documents
    significantly. Note that the clip rectangle is in document coordinates (not
    in viewport coordinates). It is not a substitute for a clip region set on
    the painter but merely a hint.

    The default value is a null rectangle indicating everything needs to be
    painted.
*/

/*!
    \variable QAbstractTextDocumentLayout::PaintContext::selections

    \brief the collection of selections that will be rendered when passing this
    paint context to QAbstractTextDocumentLayout's draw() function.

    The default value is an empty vector indicating no selection.
*/

/*!
    \class QAbstractTextDocumentLayout::Selection
    \reentrant
    \inmodule QtGui

    \brief The QAbstractTextDocumentLayout::Selection class is a convenience
    class defining the parameters of a selection.

    A selection can be used to specify a part of a document that should be
    highlighted when drawing custom layouts for QTextDocuments with the
    QAbstractTextDocumentLayout::draw() function. It is specified using
    \l cursor and a \l format.

    \sa QAbstractTextDocumentLayout, PaintContext
*/

/*!
    \variable QAbstractTextDocumentLayout::Selection::format

    \brief the format of the selection

    The default value is QTextFormat::InvalidFormat.
*/

/*!
    \variable QAbstractTextDocumentLayout::Selection::cursor
    \brief the selection's cursor

    The default value is a null cursor.
*/

/*!
    Creates a new text document layout for the given \a document.
*/
QAbstractTextDocumentLayout::QAbstractTextDocumentLayout(QTextDocument *document)
    : QObject(*new QAbstractTextDocumentLayoutPrivate, document)
{
    Q_D(QAbstractTextDocumentLayout);
    d->setDocument(document);
}

/*!
    \internal
*/
QAbstractTextDocumentLayout::QAbstractTextDocumentLayout(QAbstractTextDocumentLayoutPrivate &p, QTextDocument *document)
    :QObject(p, document)
{
    Q_D(QAbstractTextDocumentLayout);
    d->setDocument(document);
}

/*!
    \internal
*/
QAbstractTextDocumentLayout::~QAbstractTextDocumentLayout()
{
}

/*!
    Registers the given \a component as a handler for items of the given \a objectType.

    \note registerHandler() has to be called once for each object type. This
    means that there is only one handler for multiple replacement characters
    of the same object type.

    The text document layout does not take ownership of \c component.
*/
void QAbstractTextDocumentLayout::registerHandler(int objectType, QObject *component)
{
    Q_D(QAbstractTextDocumentLayout);

    QTextObjectInterface *iface = qobject_cast<QTextObjectInterface *>(component);
    if (!iface)
        return; // ### print error message on terminal?

    connect(component, SIGNAL(destroyed(QObject*)), this, SLOT(_q_handlerDestroyed(QObject*)));

    QTextObjectHandler h;
    h.iface = iface;
    h.component = component;
    d->handlers.insert(objectType, h);
}

/*!
    \since 5.2

    Unregisters the given \a component as a handler for items of the given \a objectType, or
    any handler if the \a component is not specified.
*/
void QAbstractTextDocumentLayout::unregisterHandler(int objectType, QObject *component)
{
    Q_D(QAbstractTextDocumentLayout);

    HandlerHash::iterator it = d->handlers.find(objectType);
    if (it != d->handlers.end() && (!component || component == it->component)) {
        if (component)
            disconnect(component, SIGNAL(destroyed(QObject*)), this, SLOT(_q_handlerDestroyed(QObject*)));
        d->handlers.erase(it);
    }
}

/*!
    Returns a handler for objects of the given \a objectType.
*/
QTextObjectInterface *QAbstractTextDocumentLayout::handlerForObject(int objectType) const
{
    Q_D(const QAbstractTextDocumentLayout);

    QTextObjectHandler handler = d->handlers.value(objectType);
    if (!handler.component)
        return 0;

    return handler.iface;
}

/*!
    Sets the size of the inline object \a item corresponding to the text
    \a format.

    \a posInDocument specifies the position of the object within the document.

    The default implementation resizes the \a item to the size returned by
    the object handler's intrinsicSize() function. This function is called only
    within Qt. Subclasses can reimplement this function to customize the
    resizing of inline objects.
*/
void QAbstractTextDocumentLayout::resizeInlineObject(QTextInlineObject item, int posInDocument, const QTextFormat &format)
{
    Q_D(QAbstractTextDocumentLayout);

    QTextCharFormat f = format.toCharFormat();
    Q_ASSERT(f.isValid());
    QTextObjectHandler handler = d->handlers.value(f.objectType());
    if (!handler.component)
        return;

    QSizeF s = handler.iface->intrinsicSize(document(), posInDocument, format);
    item.setWidth(s.width());
    item.setAscent(s.height());
    item.setDescent(0);
}

/*!
    Lays out the inline object \a item using the given text \a format.

    \a posInDocument specifies the position of the object within the document.

    The default implementation does nothing. This function is called only
    within Qt. Subclasses can reimplement this function to customize the
    position of inline objects.

    \sa drawInlineObject()
*/
void QAbstractTextDocumentLayout::positionInlineObject(QTextInlineObject item, int posInDocument, const QTextFormat &format)
{
    Q_UNUSED(item);
    Q_UNUSED(posInDocument);
    Q_UNUSED(format);
}

/*!
    \fn void QAbstractTextDocumentLayout::drawInlineObject(QPainter *painter, const QRectF &rect, QTextInlineObject object, int posInDocument, const QTextFormat &format)

    This function is called to draw the inline object, \a object, with the
    given \a painter within the rectangle specified by \a rect using the
    specified text \a format.

    \a posInDocument specifies the position of the object within the document.

    The default implementation calls drawObject() on the object handlers. This
    function is called only within Qt. Subclasses can reimplement this function
    to customize the drawing of inline objects.

    \sa draw()
*/
void QAbstractTextDocumentLayout::drawInlineObject(QPainter *p, const QRectF &rect, QTextInlineObject item,
                                                   int posInDocument, const QTextFormat &format)
{
    Q_UNUSED(item);
    Q_D(QAbstractTextDocumentLayout);

    QTextCharFormat f = format.toCharFormat();
    Q_ASSERT(f.isValid());
    QTextObjectHandler handler = d->handlers.value(f.objectType());
    if (!handler.component)
        return;

    handler.iface->drawObject(p, rect, document(), posInDocument, format);
}

void QAbstractTextDocumentLayoutPrivate::_q_handlerDestroyed(QObject *obj)
{
    HandlerHash::Iterator it = handlers.begin();
    while (it != handlers.end())
        if ((*it).component == obj)
            it = handlers.erase(it);
        else
            ++it;
}

/*!
    \internal

    Returns the index of the format at position \a pos.
*/
int QAbstractTextDocumentLayout::formatIndex(int pos)
{
    QTextDocumentPrivate *pieceTable = qobject_cast<QTextDocument *>(parent())->docHandle();
    return pieceTable->find(pos).value()->format;
}

/*!
    \fn QTextCharFormat QAbstractTextDocumentLayout::format(int position)

    Returns the character format that is applicable at the given \a position.
*/
QTextCharFormat QAbstractTextDocumentLayout::format(int pos)
{
    QTextDocumentPrivate *pieceTable = qobject_cast<QTextDocument *>(parent())->docHandle();
    int idx = pieceTable->find(pos).value()->format;
    return pieceTable->formatCollection()->charFormat(idx);
}



/*!
    Returns the text document that this layout is operating on.
*/
QTextDocument *QAbstractTextDocumentLayout::document() const
{
    Q_D(const QAbstractTextDocumentLayout);
    return d->document;
}

/*!
    \fn QString QAbstractTextDocumentLayout::anchorAt(const QPointF &position) const

    Returns the reference of the anchor the given \a position, or an empty
    string if no anchor exists at that point.
*/
QString QAbstractTextDocumentLayout::anchorAt(const QPointF& pos) const
{
    int cursorPos = hitTest(pos, Qt::ExactHit);
    if (cursorPos == -1)
        return QString();

    // compensate for preedit in the hit text block
    QTextBlock block = document()->firstBlock();
    while (block.isValid()) {
        QRectF blockBr = blockBoundingRect(block);
        if (blockBr.contains(pos)) {
            QTextLayout *layout = block.layout();
            int relativeCursorPos = cursorPos - block.position();
            const int preeditLength = layout ? layout->preeditAreaText().length() : 0;
            if (preeditLength > 0 && relativeCursorPos > layout->preeditAreaPosition())
                cursorPos -= qMin(cursorPos - layout->preeditAreaPosition(), preeditLength);
            break;
        }
        block = block.next();
    }

    QTextDocumentPrivate *pieceTable = qobject_cast<const QTextDocument *>(parent())->docHandle();
    QTextDocumentPrivate::FragmentIterator it = pieceTable->find(cursorPos);
    QTextCharFormat fmt = pieceTable->formatCollection()->charFormat(it->format);
    return fmt.anchorHref();
}

/*!
    \fn QRectF QAbstractTextDocumentLayout::frameBoundingRect(QTextFrame *frame) const

    Returns the bounding rectangle of \a frame.
*/

/*!
    \fn QRectF QAbstractTextDocumentLayout::blockBoundingRect(const QTextBlock &block) const

    Returns the bounding rectangle of \a block.
*/

/*!
    Sets the paint device used for rendering the document's layout to the given
    \a device.

    \sa paintDevice()
*/
void QAbstractTextDocumentLayout::setPaintDevice(QPaintDevice *device)
{
    Q_D(QAbstractTextDocumentLayout);
    d->paintDevice = device;
}

/*!
    Returns the paint device used to render the document's layout.

    \sa setPaintDevice()
*/
QPaintDevice *QAbstractTextDocumentLayout::paintDevice() const
{
    Q_D(const QAbstractTextDocumentLayout);
    return d->paintDevice;
}

QT_END_NAMESPACE

#include "moc_qabstracttextdocumentlayout.cpp"
