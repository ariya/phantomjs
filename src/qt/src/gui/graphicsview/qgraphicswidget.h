/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QGRAPHICSWIDGET_H
#define QGRAPHICSWIDGET_H

#include <QtGui/qfont.h>
#include <QtGui/qgraphicslayoutitem.h>
#include <QtGui/qgraphicsitem.h>
#include <QtGui/qpalette.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QFont;
class QFontMetrics;
class QGraphicsLayout;
class QGraphicsSceneMoveEvent;
class QGraphicsWidgetPrivate;
class QGraphicsSceneResizeEvent;
class QStyle;
class QStyleOption;

#if !defined(QT_NO_GRAPHICSVIEW) || (QT_EDITION & QT_MODULE_GRAPHICSVIEW) != QT_MODULE_GRAPHICSVIEW

class QGraphicsWidgetPrivate;

class Q_GUI_EXPORT QGraphicsWidget : public QGraphicsObject, public QGraphicsLayoutItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem QGraphicsLayoutItem)
    Q_PROPERTY(QPalette palette READ palette WRITE setPalette)
    Q_PROPERTY(QFont font READ font WRITE setFont)
    Q_PROPERTY(Qt::LayoutDirection layoutDirection READ layoutDirection WRITE setLayoutDirection RESET unsetLayoutDirection)
    Q_PROPERTY(QSizeF size READ size WRITE resize NOTIFY geometryChanged)
    Q_PROPERTY(QSizeF minimumSize READ minimumSize WRITE setMinimumSize)
    Q_PROPERTY(QSizeF preferredSize READ preferredSize WRITE setPreferredSize)
    Q_PROPERTY(QSizeF maximumSize READ maximumSize WRITE setMaximumSize)
    Q_PROPERTY(QSizePolicy sizePolicy READ sizePolicy WRITE setSizePolicy)
    Q_PROPERTY(Qt::FocusPolicy focusPolicy READ focusPolicy WRITE setFocusPolicy)
    Q_PROPERTY(Qt::WindowFlags windowFlags READ windowFlags WRITE setWindowFlags)
    Q_PROPERTY(QString windowTitle READ windowTitle WRITE setWindowTitle)
    Q_PROPERTY(QRectF geometry READ geometry WRITE setGeometry NOTIFY geometryChanged)
    Q_PROPERTY(bool autoFillBackground READ autoFillBackground WRITE setAutoFillBackground)
    Q_PROPERTY(QGraphicsLayout* layout READ layout WRITE setLayout NOTIFY layoutChanged)
public:
    QGraphicsWidget(QGraphicsItem *parent = 0, Qt::WindowFlags wFlags = 0);
    ~QGraphicsWidget();
    QGraphicsLayout *layout() const;
    void setLayout(QGraphicsLayout *layout);
    void adjustSize();

    Qt::LayoutDirection layoutDirection() const;
    void setLayoutDirection(Qt::LayoutDirection direction);
    void unsetLayoutDirection();

    QStyle *style() const;
    void setStyle(QStyle *style);

    QFont font() const;
    void setFont(const QFont &font);

    QPalette palette() const;
    void setPalette(const QPalette &palette);

    bool autoFillBackground() const;
    void setAutoFillBackground(bool enabled);

    void resize(const QSizeF &size);
    inline void resize(qreal w, qreal h) { resize(QSizeF(w, h)); }
    QSizeF size() const;

    void setGeometry(const QRectF &rect);
    inline void setGeometry(qreal x, qreal y, qreal w, qreal h);
    inline QRectF rect() const { return QRectF(QPointF(), size()); }

    void setContentsMargins(qreal left, qreal top, qreal right, qreal bottom);
    void getContentsMargins(qreal *left, qreal *top, qreal *right, qreal *bottom) const;

    void setWindowFrameMargins(qreal left, qreal top, qreal right, qreal bottom);
    void getWindowFrameMargins(qreal *left, qreal *top, qreal *right, qreal *bottom) const;
    void unsetWindowFrameMargins();
    QRectF windowFrameGeometry() const;
    QRectF windowFrameRect() const;

    // Window handling
    Qt::WindowFlags windowFlags() const;
    Qt::WindowType windowType() const;
    void setWindowFlags(Qt::WindowFlags wFlags);
    bool isActiveWindow() const;
    void setWindowTitle(const QString &title);
    QString windowTitle() const;

    // Focus handling
    Qt::FocusPolicy focusPolicy() const;
    void setFocusPolicy(Qt::FocusPolicy policy);
    static void setTabOrder(QGraphicsWidget *first, QGraphicsWidget *second);
    QGraphicsWidget *focusWidget() const;

#ifndef QT_NO_SHORTCUT
    int grabShortcut(const QKeySequence &sequence, Qt::ShortcutContext context = Qt::WindowShortcut);
    void releaseShortcut(int id);
    void setShortcutEnabled(int id, bool enabled = true);
    void setShortcutAutoRepeat(int id, bool enabled = true);
#endif

#ifndef QT_NO_ACTION
    //actions
    void addAction(QAction *action);
    void addActions(QList<QAction*> actions);
    void insertAction(QAction *before, QAction *action);
    void insertActions(QAction *before, QList<QAction*> actions);
    void removeAction(QAction *action);
    QList<QAction*> actions() const;
#endif

    void setAttribute(Qt::WidgetAttribute attribute, bool on = true);
    bool testAttribute(Qt::WidgetAttribute attribute) const;

    enum {
        Type = 11
    };
    int type() const;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
    virtual void paintWindowFrame(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
    QRectF boundingRect() const;
    QPainterPath shape() const;

#if 0
    void dumpFocusChain();
#endif

    // ### Qt 5: Disambiguate
#ifdef Q_NO_USING_KEYWORD
    const QObjectList &children() const { return QObject::children(); }
#else
    using QObject::children;
#endif

Q_SIGNALS:
    void geometryChanged();
    void layoutChanged();

public Q_SLOTS:
    bool close();

protected:
    virtual void initStyleOption(QStyleOption *option) const;

    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const;
    void updateGeometry();

    // Notification
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    virtual QVariant propertyChange(const QString &propertyName, const QVariant &value);

    // Scene events
    bool sceneEvent(QEvent *event);
    virtual bool windowFrameEvent(QEvent *e);
    virtual Qt::WindowFrameSection windowFrameSectionAt(const QPointF& pos) const;

    // Base event handlers
    bool event(QEvent *event);
    //virtual void actionEvent(QActionEvent *event);
    virtual void changeEvent(QEvent *event);
    virtual void closeEvent(QCloseEvent *event);
    //void create(WId window = 0, bool initializeWindow = true, bool destroyOldWindow = true);
    //void destroy(bool destroyWindow = true, bool destroySubWindows = true);
    void focusInEvent(QFocusEvent *event);
    virtual bool focusNextPrevChild(bool next);
    void focusOutEvent(QFocusEvent *event);
    virtual void hideEvent(QHideEvent *event);
    //virtual bool macEvent(EventHandlerCallRef caller, EventRef event);
    //virtual int metric(PaintDeviceMetric m ) const;
    virtual void moveEvent(QGraphicsSceneMoveEvent *event);
    virtual void polishEvent();
    //virtual bool qwsEvent(QWSEvent *event);
    //void resetInputContext ();
    virtual void resizeEvent(QGraphicsSceneResizeEvent *event);
    virtual void showEvent(QShowEvent *event);
    //virtual void tabletEvent(QTabletEvent *event);
    //virtual bool winEvent(MSG *message, long *result);
    //virtual bool x11Event(XEvent *event);
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    virtual void grabMouseEvent(QEvent *event);
    virtual void ungrabMouseEvent(QEvent *event);
    virtual void grabKeyboardEvent(QEvent *event);
    virtual void ungrabKeyboardEvent(QEvent *event);
    QGraphicsWidget(QGraphicsWidgetPrivate &, QGraphicsItem *parent, QGraphicsScene *, Qt::WindowFlags wFlags = 0);

private:
    Q_DISABLE_COPY(QGraphicsWidget)
    Q_DECLARE_PRIVATE_D(QGraphicsItem::d_ptr.data(), QGraphicsWidget)
    Q_PRIVATE_SLOT(d_func(), void _q_relayout())

    friend class QGraphicsScene;
    friend class QGraphicsScenePrivate;
    friend class QGraphicsView;
    friend class QGraphicsItem;
    friend class QGraphicsItemPrivate;
    friend class QGraphicsLayout;
    friend class QWidget;
    friend class QApplication;
};

inline void QGraphicsWidget::setGeometry(qreal ax, qreal ay, qreal aw, qreal ah)
{ setGeometry(QRectF(ax, ay, aw, ah)); }

#endif

QT_END_NAMESPACE

QT_END_HEADER

#endif

