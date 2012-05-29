/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSPLITTER_H
#define QSPLITTER_H

#include <QtGui/qframe.h>
#include <QtGui/qsizepolicy.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_SPLITTER

class QSplitterPrivate;
class QTextStream;
template <typename T> class QList;

class QSplitterHandle;

class Q_GUI_EXPORT QSplitter : public QFrame
{
    Q_OBJECT

    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation)
    Q_PROPERTY(bool opaqueResize READ opaqueResize WRITE setOpaqueResize)
    Q_PROPERTY(int handleWidth READ handleWidth WRITE setHandleWidth)
    Q_PROPERTY(bool childrenCollapsible READ childrenCollapsible WRITE setChildrenCollapsible)

public:
    explicit QSplitter(QWidget* parent = 0);
    explicit QSplitter(Qt::Orientation, QWidget* parent = 0);
    ~QSplitter();

    void addWidget(QWidget *widget);
    void insertWidget(int index, QWidget *widget);

    void setOrientation(Qt::Orientation);
    Qt::Orientation orientation() const;

    void setChildrenCollapsible(bool);
    bool childrenCollapsible() const;

    void setCollapsible(int index, bool);
    bool isCollapsible(int index) const;
    void setOpaqueResize(bool opaque = true);
    bool opaqueResize() const;
    void refresh();

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    QList<int> sizes() const;
    void setSizes(const QList<int> &list);

    QByteArray saveState() const;
    bool restoreState(const QByteArray &state);

    int handleWidth() const;
    void setHandleWidth(int);

    int indexOf(QWidget *w) const;
    QWidget *widget(int index) const;
    int count() const;

    void getRange(int index, int *, int *) const;
    QSplitterHandle *handle(int index) const;

    void setStretchFactor(int index, int stretch);

Q_SIGNALS:
    void splitterMoved(int pos, int index);

protected:
    virtual QSplitterHandle *createHandle();

    void childEvent(QChildEvent *);

    bool event(QEvent *);
    void resizeEvent(QResizeEvent *);

    void changeEvent(QEvent *);
    void moveSplitter(int pos, int index);
    void setRubberBand(int position);
    int closestLegalPosition(int, int);

#ifdef QT3_SUPPORT
public:
    QT3_SUPPORT_CONSTRUCTOR QSplitter(QWidget* parent, const char* name);
    QT3_SUPPORT_CONSTRUCTOR QSplitter(Qt::Orientation, QWidget* parent, const char* name);
    enum ResizeMode { Stretch, KeepSize, FollowSizeHint, Auto };
    QT3_SUPPORT void setResizeMode(QWidget *w, ResizeMode mode);
    inline QT3_SUPPORT void moveToFirst(QWidget *w) { insertWidget(0,w); }
    inline QT3_SUPPORT void moveToLast(QWidget *w) { addWidget(w); }
    inline QT3_SUPPORT void setCollapsible(QWidget *w, bool collapse)
    { setCollapsible(indexOf(w), collapse); }
    QT3_SUPPORT void setMargin(int margin) { setContentsMargins(margin, margin, margin, margin); }
    QT3_SUPPORT int margin() const 
    { int margin; int dummy; getContentsMargins(&margin, &dummy, &dummy, &dummy);  return margin; }    
#endif

private:
    Q_DISABLE_COPY(QSplitter)
    Q_DECLARE_PRIVATE(QSplitter)
private:
    friend class QSplitterHandle;
};

//#ifdef QT3_SUPPORT
#ifndef QT_NO_TEXTSTREAM
Q_GUI_EXPORT QTextStream& operator<<(QTextStream&, const QSplitter&);
Q_GUI_EXPORT QTextStream& operator>>(QTextStream&, QSplitter&);
#endif
//#endif

class QSplitterHandlePrivate;
class Q_GUI_EXPORT QSplitterHandle : public QWidget
{
    Q_OBJECT
public:
    QSplitterHandle(Qt::Orientation o, QSplitter *parent);
    void setOrientation(Qt::Orientation o);
    Qt::Orientation orientation() const;
    bool opaqueResize() const;
    QSplitter *splitter() const;

    QSize sizeHint() const;

protected:
    void paintEvent(QPaintEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void resizeEvent(QResizeEvent *);
    bool event(QEvent *);

    void moveSplitter(int p);
    int closestLegalPosition(int p);

private:
    Q_DISABLE_COPY(QSplitterHandle)
    Q_DECLARE_PRIVATE(QSplitterHandle)
};

#endif // QT_NO_SPLITTER

QT_END_NAMESPACE

QT_END_HEADER

#endif // QSPLITTER_H
