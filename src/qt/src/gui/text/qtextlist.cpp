/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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


#include "qtextlist.h"
#include "qtextobject_p.h"
#include "qtextcursor.h"
#include "qtextdocument_p.h"
#include <qdebug.h>

QT_BEGIN_NAMESPACE

class QTextListPrivate : public QTextBlockGroupPrivate
{
public:
    QTextListPrivate(QTextDocument *doc)
        : QTextBlockGroupPrivate(doc)
    {
    }
};

/*!
    \class QTextList
    \reentrant

    \brief The QTextList class provides a decorated list of items in a QTextDocument.

    \ingroup richtext-processing

    A list contains a sequence of text blocks, each of which is marked with a
    bullet point or other symbol. Multiple levels of lists can be used, and
    the automatic numbering feature provides support for ordered numeric and
    alphabetical lists.

    Lists are created by using a text cursor to insert an empty list at the
    current position or by moving existing text into a new list.
    The \l{QTextCursor::insertList()} function inserts an empty block into the
    document at the cursor position, and makes it the first item in a list.

    \snippet doc/src/snippets/textdocument-lists/mainwindow.cpp 0

    The \l{QTextCursor::createList()} function takes the contents of the
    cursor's current block and turns it into the first item of a new list.

    The cursor's current list is found with \l{QTextCursor::currentList()}.

    The number of items in a list is given by count(). Each item can be
    obtained by its index in the list with the item() function. Similarly,
    the index of a given item can be found with itemNumber(). The text of
    each item can be found with the itemText() function.

    Note that the items in the list may not be adjacent elements in the
    document. For example, the top-level items in a multi-level list will
    be separated by the items in lower levels of the list.

    List items can be deleted by index with the removeItem() function.
    remove() deletes the specified item in the list.

    The list's format is set with setFormat() and read with format().
    The format describes the decoration of the list itself, and not the
    individual items.

    \sa QTextBlock, QTextListFormat, QTextCursor
*/

/*!
    \fn bool QTextList::isEmpty() const
    \obsolete

    Returns true if the list has no items; otherwise returns false.

    \bold{Note:} Empty lists are automatically deleted by the QTextDocument that owns
    them.

    \sa count()
*/

/*! \internal
 */
QTextList::QTextList(QTextDocument *doc)
    : QTextBlockGroup(*new QTextListPrivate(doc), doc)
{
}

/*!
  \internal
*/
QTextList::~QTextList()
{
}

/*!
    Returns the number of items in the list.
*/
int QTextList::count() const
{
    Q_D(const QTextList);
    return d->blocks.count();
}

/*!
    Returns the \a{i}-th text block in the list.

    \sa count() itemText()
*/
QTextBlock QTextList::item(int i) const
{
    Q_D(const QTextList);
    if (i < 0 || i >= d->blocks.size())
        return QTextBlock();
    return d->blocks.at(i);
}

/*!
    \fn void QTextList::setFormat(const QTextListFormat &format)

    Sets the list's format to \a format.
*/

/*!
    \fn QTextListFormat QTextList::format() const

    Returns the list's format.
*/

/*!
    \fn int QTextList::itemNumber(const QTextBlock &block) const

    Returns the index of the list item that corresponds to the given \a block.
    Returns -1 if the block was not present in the list.
*/
int QTextList::itemNumber(const QTextBlock &blockIt) const
{
    Q_D(const QTextList);
    return d->blocks.indexOf(blockIt);
}

/*!
    \fn QString QTextList::itemText(const QTextBlock &block) const

    Returns the text of the list item that corresponds to the given \a block.
*/
QString QTextList::itemText(const QTextBlock &blockIt) const
{
    Q_D(const QTextList);
    int item = d->blocks.indexOf(blockIt) + 1;
    if (item <= 0)
        return QString();

    QTextBlock block = d->blocks.at(item-1);
    QTextBlockFormat blockFormat = block.blockFormat();

    QString result;

    const int style = format().style();
    QString numberPrefix;
    QString numberSuffix = QLatin1String(".");

    if (format().hasProperty(QTextFormat::ListNumberPrefix))
        numberPrefix = format().numberPrefix();
    if (format().hasProperty(QTextFormat::ListNumberSuffix))
        numberSuffix = format().numberSuffix();

    switch (style) {
        case QTextListFormat::ListDecimal:
            result = QString::number(item);
            break;
            // from the old richtext
        case QTextListFormat::ListLowerAlpha:
        case QTextListFormat::ListUpperAlpha:
            {
                const char baseChar = style == QTextListFormat::ListUpperAlpha ? 'A' : 'a';

                int c = item;
                while (c > 0) {
                    c--;
                    result.prepend(QChar(baseChar + (c % 26)));
                    c /= 26;
                }
            }
            break;
        case QTextListFormat::ListLowerRoman:
        case QTextListFormat::ListUpperRoman:
            {
                if (item < 5000) {
                    QByteArray romanNumeral;

                    // works for up to 4999 items
                    static const char romanSymbolsLower[] = "iiivixxxlxcccdcmmmm";
                    static const char romanSymbolsUpper[] = "IIIVIXXXLXCCCDCMMMM";
                    QByteArray romanSymbols; // wrap to have "mid"
                    if (style == QTextListFormat::ListLowerRoman)
                        romanSymbols = QByteArray::fromRawData(romanSymbolsLower, sizeof(romanSymbolsLower));
                    else
                        romanSymbols = QByteArray::fromRawData(romanSymbolsUpper, sizeof(romanSymbolsUpper));

                    int c[] = { 1, 4, 5, 9, 10, 40, 50, 90, 100, 400, 500, 900, 1000 };
                    int n = item;
                    for (int i = 12; i >= 0; n %= c[i], i--) {
                        int q = n / c[i];
                        if (q > 0) {
                            int startDigit = i + (i+3)/4;
                            int numDigits;
                            if (i % 4) {
                                // c[i] == 4|5|9|40|50|90|400|500|900
                                if ((i-2) % 4) {
                                    // c[i] == 4|9|40|90|400|900 => with subtraction (IV, IX, XL, XC, ...)
                                    numDigits = 2;
                                }
                                else {
                                    // c[i] == 5|50|500 (V, L, D)
                                    numDigits = 1;
                                }
                            }
                            else {
                                // c[i] == 1|10|100|1000 (I, II, III, X, XX, ...)
                                numDigits = q;
                            }

                            romanNumeral.append(romanSymbols.mid(startDigit, numDigits));
                        }
                    }
                    result = QString::fromLatin1(romanNumeral);
                }
                else {
                    result = QLatin1String("?");
                }

	    }
	    break;
        default:
            Q_ASSERT(false);
    }
    if (blockIt.textDirection() == Qt::RightToLeft)
        return numberSuffix + result + numberPrefix;
    else
        return numberPrefix + result + numberSuffix;
}

/*!
    Removes the item at item position \a i from the list. When the last item in the
    list is removed, the list is automatically deleted by the QTextDocument that owns
    it.

    \sa add(), remove()
*/
void QTextList::removeItem(int i)
{
    Q_D(QTextList);
    if (i < 0 || i >= d->blocks.size())
        return;

    QTextBlock block = d->blocks.at(i);
    remove(block);
}


/*!
    Removes the given \a block from the list.

    \sa add(), removeItem()
*/
void QTextList::remove(const QTextBlock &block)
{
    QTextBlockFormat fmt = block.blockFormat();
    fmt.setIndent(fmt.indent() + format().indent());
    fmt.setObjectIndex(-1);
    block.docHandle()->setBlockFormat(block, block, fmt, QTextDocumentPrivate::SetFormat);
}

/*!
    Makes the given \a block part of the list.

    \sa remove(), removeItem()
*/
void QTextList::add(const QTextBlock &block)
{
    QTextBlockFormat fmt = block.blockFormat();
    fmt.setObjectIndex(objectIndex());
    block.docHandle()->setBlockFormat(block, block, fmt, QTextDocumentPrivate::SetFormat);
}

QT_END_NAMESPACE
