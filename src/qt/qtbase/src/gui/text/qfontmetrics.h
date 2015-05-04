/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QFONTMETRICS_H
#define QFONTMETRICS_H

#include <QtGui/qfont.h>
#include <QtCore/qsharedpointer.h>
#ifndef QT_INCLUDE_COMPAT
#include <QtCore/qrect.h>
#endif

QT_BEGIN_NAMESPACE



class QTextCodec;
class QRect;


class Q_GUI_EXPORT QFontMetrics
{
public:
    explicit QFontMetrics(const QFont &);
    QFontMetrics(const QFont &, QPaintDevice *pd);
    QFontMetrics(const QFontMetrics &);
    ~QFontMetrics();

    QFontMetrics &operator=(const QFontMetrics &);
#ifdef Q_COMPILER_RVALUE_REFS
    inline QFontMetrics &operator=(QFontMetrics &&other)
    { qSwap(d, other.d); return *this; }
#endif

    void swap(QFontMetrics &other) { qSwap(d, other.d); }

    int ascent() const;
    int descent() const;
    int height() const;
    int leading() const;
    int lineSpacing() const;
    int minLeftBearing() const;
    int minRightBearing() const;
    int maxWidth() const;

    int xHeight() const;
    int averageCharWidth() const;

    bool inFont(QChar) const;
    bool inFontUcs4(uint ucs4) const;

    int leftBearing(QChar) const;
    int rightBearing(QChar) const;
    int width(const QString &, int len = -1) const;
    int width(const QString &, int len, int flags) const;

    int width(QChar) const;
    int charWidth(const QString &str, int pos) const;

    QRect boundingRect(QChar) const;

    QRect boundingRect(const QString &text) const;
    QRect boundingRect(const QRect &r, int flags, const QString &text, int tabstops=0, int *tabarray=0) const;
    inline QRect boundingRect(int x, int y, int w, int h, int flags, const QString &text,
                              int tabstops=0, int *tabarray=0) const
        { return boundingRect(QRect(x, y, w, h), flags, text, tabstops, tabarray); }
    QSize size(int flags, const QString& str, int tabstops=0, int *tabarray=0) const;

    QRect tightBoundingRect(const QString &text) const;

    QString elidedText(const QString &text, Qt::TextElideMode mode, int width, int flags = 0) const;

    int underlinePos() const;
    int overlinePos() const;
    int strikeOutPos() const;
    int lineWidth() const;

    bool operator==(const QFontMetrics &other) const;
    inline bool operator !=(const QFontMetrics &other) const { return !operator==(other); }

private:
    friend class QFontMetricsF;
    friend class QStackTextEngine;

    QExplicitlySharedDataPointer<QFontPrivate> d;
};

Q_DECLARE_SHARED(QFontMetrics)

class Q_GUI_EXPORT QFontMetricsF
{
public:
    explicit QFontMetricsF(const QFont &);
    QFontMetricsF(const QFont &, QPaintDevice *pd);
    QFontMetricsF(const QFontMetrics &);
    QFontMetricsF(const QFontMetricsF &);
    ~QFontMetricsF();

    QFontMetricsF &operator=(const QFontMetricsF &);
    QFontMetricsF &operator=(const QFontMetrics &);
#ifdef Q_COMPILER_RVALUE_REFS
    inline QFontMetricsF &operator=(QFontMetricsF &&other)
    { qSwap(d, other.d); return *this; }
#endif

    void swap(QFontMetricsF &other) { qSwap(d, other.d); }

    qreal ascent() const;
    qreal descent() const;
    qreal height() const;
    qreal leading() const;
    qreal lineSpacing() const;
    qreal minLeftBearing() const;
    qreal minRightBearing() const;
    qreal maxWidth() const;

    qreal xHeight() const;
    qreal averageCharWidth() const;

    bool inFont(QChar) const;
    bool inFontUcs4(uint ucs4) const;

    qreal leftBearing(QChar) const;
    qreal rightBearing(QChar) const;
    qreal width(const QString &string) const;

    qreal width(QChar) const;

    QRectF boundingRect(const QString &string) const;
    QRectF boundingRect(QChar) const;
    QRectF boundingRect(const QRectF &r, int flags, const QString& string, int tabstops=0, int *tabarray=0) const;
    QSizeF size(int flags, const QString& str, int tabstops=0, int *tabarray=0) const;

    QRectF tightBoundingRect(const QString &text) const;

    QString elidedText(const QString &text, Qt::TextElideMode mode, qreal width, int flags = 0) const;

    qreal underlinePos() const;
    qreal overlinePos() const;
    qreal strikeOutPos() const;
    qreal lineWidth() const;

    bool operator==(const QFontMetricsF &other) const;
    inline bool operator !=(const QFontMetricsF &other) const { return !operator==(other); }

private:
    QExplicitlySharedDataPointer<QFontPrivate> d;
};

Q_DECLARE_SHARED(QFontMetricsF)

QT_END_NAMESPACE

#endif // QFONTMETRICS_H
