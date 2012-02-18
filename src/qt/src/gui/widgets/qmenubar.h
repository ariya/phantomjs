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

#ifndef QMENUBAR_H
#define QMENUBAR_H

#include <QtGui/qmenu.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_MENUBAR

class QMenuBarPrivate;
class QStyleOptionMenuItem;
class QWindowsStyle;
#ifdef QT3_SUPPORT
class QMenuItem;
#endif

class Q_GUI_EXPORT QMenuBar : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(bool defaultUp READ isDefaultUp WRITE setDefaultUp)
    Q_PROPERTY(bool nativeMenuBar READ isNativeMenuBar WRITE setNativeMenuBar)

public:
    explicit QMenuBar(QWidget *parent = 0);
    ~QMenuBar();

#ifdef Q_NO_USING_KEYWORD
    void addAction(QAction *action) { QWidget::addAction(action); }
#else
    using QWidget::addAction;
#endif
    QAction *addAction(const QString &text);
    QAction *addAction(const QString &text, const QObject *receiver, const char* member);

    QAction *addMenu(QMenu *menu);
    QMenu *addMenu(const QString &title);
    QMenu *addMenu(const QIcon &icon, const QString &title);


    QAction *addSeparator();
    QAction *insertSeparator(QAction *before);

    QAction *insertMenu(QAction *before, QMenu *menu);

    void clear();

    QAction *activeAction() const;
    void setActiveAction(QAction *action);

    void setDefaultUp(bool);
    bool isDefaultUp() const;

    QSize sizeHint() const;
    QSize minimumSizeHint() const;
    int heightForWidth(int) const;

    QRect actionGeometry(QAction *) const;
    QAction *actionAt(const QPoint &) const;

    void setCornerWidget(QWidget *w, Qt::Corner corner = Qt::TopRightCorner);
    QWidget *cornerWidget(Qt::Corner corner = Qt::TopRightCorner) const;

#ifdef Q_WS_MAC
    OSMenuRef macMenu();
    static bool macUpdateMenuBar();
#endif

#ifdef Q_WS_WINCE
    void setDefaultAction(QAction *);
    QAction *defaultAction() const;

    static void wceCommands(uint command);
    static void wceRefresh();
#endif

    bool isNativeMenuBar() const;
    void setNativeMenuBar(bool nativeMenuBar);

public Q_SLOTS:
    virtual void setVisible(bool visible);

Q_SIGNALS:
    void triggered(QAction *action);
    void hovered(QAction *action);

protected:
    void changeEvent(QEvent *);
    void keyPressEvent(QKeyEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void leaveEvent(QEvent *);
    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *);
    void actionEvent(QActionEvent *);
    void focusOutEvent(QFocusEvent *);
    void focusInEvent(QFocusEvent *);
    void timerEvent(QTimerEvent *);
    bool eventFilter(QObject *, QEvent *);
    bool event(QEvent *);
    void initStyleOption(QStyleOptionMenuItem *option, const QAction *action) const;

#ifdef QT3_SUPPORT
public:
    QT3_SUPPORT_CONSTRUCTOR QMenuBar(QWidget *parent, const char *name);
    inline QT3_SUPPORT uint count() const { return actions().count(); }
    inline QT3_SUPPORT int insertItem(const QString &text, const QObject *receiver, const char* member,
                                    const QKeySequence& shortcut = 0, int id = -1, int index = -1) {
        return insertAny(0, &text, receiver, member, &shortcut, 0, id, index);
    }
    inline QT3_SUPPORT int insertItem(const QIcon& icon, const QString &text,
                                    const QObject *receiver, const char* member,
                                    const QKeySequence& shortcut = 0, int id = -1, int index = -1) {
        return insertAny(&icon, &text, receiver, member, &shortcut, 0, id, index);
    }
    inline QT3_SUPPORT int insertItem(const QPixmap &pixmap, const QObject *receiver, const char* member,
                                    const QKeySequence& shortcut = 0, int id = -1, int index = -1) {
        QIcon icon(pixmap);
        return insertAny(&icon, 0, receiver, member, &shortcut, 0, id, index);
    }
    inline QT3_SUPPORT int insertItem(const QString &text, int id=-1, int index=-1) {
        return insertAny(0, &text, 0, 0, 0, 0, id, index);
    }
    inline QT3_SUPPORT int insertItem(const QIcon& icon, const QString &text, int id=-1, int index=-1) {
        return insertAny(&icon, &text, 0, 0, 0, 0, id, index);
    }
    inline QT3_SUPPORT int insertItem(const QString &text, QMenu *popup, int id=-1, int index=-1) {
        return insertAny(0, &text, 0, 0, 0, popup, id, index);
    }
    inline QT3_SUPPORT int insertItem(const QIcon& icon, const QString &text, QMenu *popup, int id=-1, int index=-1) {
        return insertAny(&icon, &text, 0, 0, 0, popup, id, index);
    }
    inline QT3_SUPPORT int insertItem(const QPixmap &pixmap, int id=-1, int index=-1) {
        QIcon icon(pixmap);
        return insertAny(&icon, 0, 0, 0, 0, 0, id, index);
    }
    inline QT3_SUPPORT int insertItem(const QPixmap &pixmap, QMenu *popup, int id=-1, int index=-1) {
        QIcon icon(pixmap);
        return insertAny(&icon, 0, 0, 0, 0, popup, id, index);
    }
    QT3_SUPPORT int insertSeparator(int index=-1);
    inline QT3_SUPPORT void removeItem(int id) {
        if(QAction *act = findActionForId(id))
            removeAction(act); }
    inline QT3_SUPPORT void removeItemAt(int index) {
        if(QAction *act = actions().value(index))
            removeAction(act); }
#ifndef QT_NO_SHORTCUT
    inline QT3_SUPPORT QKeySequence accel(int id) const {
        if(QAction *act = findActionForId(id))
            return act->shortcut();
        return QKeySequence(); }
    inline QT3_SUPPORT void setAccel(const QKeySequence& key, int id) {
        if(QAction *act = findActionForId(id))
            act->setShortcut(key);
    }
#endif
    inline QT3_SUPPORT QIcon iconSet(int id) const {
        if(QAction *act = findActionForId(id))
            return act->icon();
        return QIcon(); }
    inline QT3_SUPPORT QString text(int id) const {
        if(QAction *act = findActionForId(id))
            return act->text();
        return QString(); }
    inline QT3_SUPPORT QPixmap pixmap(int id) const {
        if(QAction *act = findActionForId(id))
            return act->icon().pixmap(QSize(22,22));
        return QPixmap(); }
    inline QT3_SUPPORT void setWhatsThis(int id, const QString &w) {
        if(QAction *act = findActionForId(id))
            act->setWhatsThis(w); }
    inline QT3_SUPPORT QString whatsThis(int id) const {
        if(QAction *act = findActionForId(id))
            return act->whatsThis();
        return QString(); }

    inline QT3_SUPPORT void changeItem(int id, const QString &text) {
        if(QAction *act = findActionForId(id))
            act->setText(text); }
    inline QT3_SUPPORT void changeItem(int id, const QPixmap &pixmap) {
        if(QAction *act = findActionForId(id))
            act->setIcon(QIcon(pixmap)); }
    inline QT3_SUPPORT void changeItem(int id, const QIcon &icon, const QString &text) {
        if(QAction *act = findActionForId(id)) {
            act->setIcon(icon);
            act->setText(text);
        }
    }
    inline QT3_SUPPORT bool isItemActive(int id) const { return findActionForId(id) == activeAction(); }
    inline QT3_SUPPORT bool isItemEnabled(int id) const {
        if(QAction *act = findActionForId(id))
            return act->isEnabled();
        return false; }
    inline QT3_SUPPORT void setItemEnabled(int id, bool enable) {
        if(QAction *act = findActionForId(id))
            act->setEnabled(enable); }
    inline QT3_SUPPORT bool isItemChecked(int id) const {
        if(QAction *act = findActionForId(id))
            return act->isChecked();
        return false; }
    inline QT3_SUPPORT void setItemChecked(int id, bool check) {
        if(QAction *act = findActionForId(id))
            act->setChecked(check); }
    inline QT3_SUPPORT bool isItemVisible(int id) const {
        if(QAction *act = findActionForId(id))
            return act->isVisible();
        return false; }
    inline QT3_SUPPORT void setItemVisible(int id, bool visible) {
        if(QAction *act = findActionForId(id))
            act->setVisible(visible); }
    inline QT3_SUPPORT int indexOf(int id) const { return actions().indexOf(findActionForId(id)); }
    inline QT3_SUPPORT int idAt(int index) const {
        return index >= 0 && index < actions().size()
                        ? findIdForAction(actions().at(index))
                        : -1;
    }
    inline QT3_SUPPORT void activateItemAt(int index) {
        if(QAction *ret = actions().value(index))
            setActiveAction(ret);
    }
    inline QT3_SUPPORT bool connectItem(int id, const QObject *receiver, const char* member) {
        if(QAction *act = findActionForId(id)) {
            QObject::connect(act, SIGNAL(triggered()), receiver, member);
            return true;
        }
        return false;
    }
    inline QT3_SUPPORT bool disconnectItem(int id,const QObject *receiver, const char* member) {
        if(QAction *act = findActionForId(id)) {
            QObject::disconnect(act, SIGNAL(triggered()), receiver, member);
            return true;
        }
        return false;
    }
    inline QT3_SUPPORT QMenuItem *findItem(int id) const {
        return (QMenuItem*)findActionForId(id);
    }
    QT3_SUPPORT bool setItemParameter(int id, int param);
    QT3_SUPPORT int itemParameter(int id) const;

    //frame
    QT3_SUPPORT int frameWidth() const;

    QT3_SUPPORT void setFrameRect(QRect) {}
    QT3_SUPPORT QRect frameRect() const { return QRect(); }
    enum DummyFrame { Box, Sunken, Plain, Raised, MShadow, NoFrame, Panel, StyledPanel,
                      HLine, VLine, GroupBoxPanel, WinPanel, ToolBarPanel, MenuBarPanel,
                      PopupPanel, LineEditPanel, TabWidgetPanel, MShape };
    QT3_SUPPORT void setFrameShadow(DummyFrame) {}
    QT3_SUPPORT DummyFrame frameShadow() const { return Plain; }
    QT3_SUPPORT void setFrameShape(DummyFrame) {}
    QT3_SUPPORT DummyFrame frameShape() const { return NoFrame; }
    QT3_SUPPORT void setFrameStyle(int) {}
    QT3_SUPPORT int frameStyle() const  { return 0; }
    QT3_SUPPORT void setLineWidth(int) {}
    QT3_SUPPORT int lineWidth() const { return 0; }
    QT3_SUPPORT void setMargin(int margin) { setContentsMargins(margin, margin, margin, margin); }
    QT3_SUPPORT int margin() const
    { int margin; int dummy; getContentsMargins(&margin, &dummy, &dummy, &dummy);  return margin; }
    QT3_SUPPORT void setMidLineWidth(int) {}
    QT3_SUPPORT int midLineWidth() const { return 0; }

    //menubar
    enum Separator { Never=0, InWindowsStyle=1 };
    inline QT3_SUPPORT Separator separator() const { return InWindowsStyle; }
    inline QT3_SUPPORT void setSeparator(Separator) { }

    QT3_SUPPORT void setAutoGeometry(bool);
    QT3_SUPPORT bool autoGeometry() const;

Q_SIGNALS:
    QT_MOC_COMPAT void activated(int itemId);
    QT_MOC_COMPAT void highlighted(int itemId);

protected:
    inline QT3_SUPPORT QRect itemRect(int index) {
        if(QAction *act = actions().value(index))
            return actionGeometry(act);
        return QRect();
    }
    inline QT3_SUPPORT int itemAtPos(const QPoint &p) {
        return findIdForAction(actionAt(p));
    }
private:
    QAction *findActionForId(int id) const;
    int insertAny(const QIcon *icon, const QString *text, const QObject *receiver, const char *member,
                  const QKeySequence *shorcut, const QMenu *popup, int id, int index);
    int findIdForAction(QAction*) const;
#endif

private:
    Q_DECLARE_PRIVATE(QMenuBar)
    Q_DISABLE_COPY(QMenuBar)
    Q_PRIVATE_SLOT(d_func(), void _q_actionTriggered())
    Q_PRIVATE_SLOT(d_func(), void _q_actionHovered())
    Q_PRIVATE_SLOT(d_func(), void _q_internalShortcutActivated(int))
    Q_PRIVATE_SLOT(d_func(), void _q_updateLayout())

#ifdef Q_WS_WINCE
    Q_PRIVATE_SLOT(d_func(), void _q_updateDefaultAction())
#endif

    friend class QMenu;
    friend class QMenuPrivate;
    friend class QWindowsStyle;

#ifdef Q_WS_MAC
    friend class QApplicationPrivate;
    friend class QWidgetPrivate;
    friend bool qt_mac_activate_action(MenuRef, uint, QAction::ActionEvent, bool);
#endif
};

#endif // QT_NO_MENUBAR

QT_END_NAMESPACE

QT_END_HEADER

#endif // QMENUBAR_H
