/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#ifndef QABSTRACTSCROLLAREA_H
#define QABSTRACTSCROLLAREA_H

#include <QtWidgets/qframe.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_SCROLLAREA

class QMargins;
class QScrollBar;
class QAbstractScrollAreaPrivate;

class Q_WIDGETS_EXPORT QAbstractScrollArea : public QFrame
{
    Q_OBJECT

    Q_ENUMS(SizeAdjustPolicy)
    Q_PROPERTY(Qt::ScrollBarPolicy verticalScrollBarPolicy READ verticalScrollBarPolicy WRITE setVerticalScrollBarPolicy)
    Q_PROPERTY(Qt::ScrollBarPolicy horizontalScrollBarPolicy READ horizontalScrollBarPolicy WRITE setHorizontalScrollBarPolicy)
    Q_PROPERTY(SizeAdjustPolicy sizeAdjustPolicy READ sizeAdjustPolicy WRITE setSizeAdjustPolicy)

public:
    explicit QAbstractScrollArea(QWidget* parent=0);
    ~QAbstractScrollArea();

    enum SizeAdjustPolicy {
        AdjustIgnored,
        AdjustToContentsOnFirstShow,
        AdjustToContents
    };

    Qt::ScrollBarPolicy verticalScrollBarPolicy() const;
    void setVerticalScrollBarPolicy(Qt::ScrollBarPolicy);
    QScrollBar *verticalScrollBar() const;
    void setVerticalScrollBar(QScrollBar *scrollbar);

    Qt::ScrollBarPolicy horizontalScrollBarPolicy() const;
    void setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy);
    QScrollBar *horizontalScrollBar() const;
    void setHorizontalScrollBar(QScrollBar *scrollbar);

    QWidget *cornerWidget() const;
    void setCornerWidget(QWidget *widget);

    void addScrollBarWidget(QWidget *widget, Qt::Alignment alignment);
    QWidgetList scrollBarWidgets(Qt::Alignment alignment);

    QWidget *viewport() const;
    void setViewport(QWidget *widget);
    QSize maximumViewportSize() const;

    QSize minimumSizeHint() const;

    QSize sizeHint() const;

    virtual void setupViewport(QWidget *viewport);

    SizeAdjustPolicy sizeAdjustPolicy() const;
    void setSizeAdjustPolicy(SizeAdjustPolicy policy);

protected:
    QAbstractScrollArea(QAbstractScrollAreaPrivate &dd, QWidget *parent = 0);
    void setViewportMargins(int left, int top, int right, int bottom);
    void setViewportMargins(const QMargins &margins);

    bool eventFilter(QObject *, QEvent *);
    bool event(QEvent *);
    virtual bool viewportEvent(QEvent *);

    void resizeEvent(QResizeEvent *);
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseDoubleClickEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
#ifndef QT_NO_WHEELEVENT
    void wheelEvent(QWheelEvent *);
#endif
#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QContextMenuEvent *);
#endif
#ifndef QT_NO_DRAGANDDROP
    void dragEnterEvent(QDragEnterEvent *);
    void dragMoveEvent(QDragMoveEvent *);
    void dragLeaveEvent(QDragLeaveEvent *);
    void dropEvent(QDropEvent *);
#endif

    void keyPressEvent(QKeyEvent *);

    virtual void scrollContentsBy(int dx, int dy);

    virtual QSize viewportSizeHint() const;

private:
    Q_DECLARE_PRIVATE(QAbstractScrollArea)
    Q_DISABLE_COPY(QAbstractScrollArea)
    Q_PRIVATE_SLOT(d_func(), void _q_hslide(int))
    Q_PRIVATE_SLOT(d_func(), void _q_vslide(int))
    Q_PRIVATE_SLOT(d_func(), void _q_showOrHideScrollBars())

    friend class QStyleSheetStyle;
    friend class QWidgetPrivate;
};

#endif // QT_NO_SCROLLAREA

QT_END_NAMESPACE

#endif // QABSTRACTSCROLLAREA_H
