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

#ifndef QTOOLBUTTON_H
#define QTOOLBUTTON_H

#include <QtGui/qabstractbutton.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_TOOLBUTTON

class QToolButtonPrivate;
class QMenu;
class QStyleOptionToolButton;

class Q_GUI_EXPORT QToolButton : public QAbstractButton
{
    Q_OBJECT
    Q_ENUMS(Qt::ToolButtonStyle Qt::ArrowType ToolButtonPopupMode)
#ifndef QT_NO_MENU
    Q_PROPERTY(ToolButtonPopupMode popupMode READ popupMode WRITE setPopupMode)
#endif
    Q_PROPERTY(Qt::ToolButtonStyle toolButtonStyle READ toolButtonStyle WRITE setToolButtonStyle)
    Q_PROPERTY(bool autoRaise READ autoRaise WRITE setAutoRaise)
    Q_PROPERTY(Qt::ArrowType arrowType READ arrowType WRITE setArrowType)

public:
    enum ToolButtonPopupMode {
        DelayedPopup,
        MenuButtonPopup,
        InstantPopup
    };

    explicit QToolButton(QWidget * parent=0);
    ~QToolButton();

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    Qt::ToolButtonStyle toolButtonStyle() const;

    Qt::ArrowType arrowType() const;
    void setArrowType(Qt::ArrowType type);

#ifndef QT_NO_MENU
    void setMenu(QMenu* menu);
    QMenu* menu() const;

    void setPopupMode(ToolButtonPopupMode mode);
    ToolButtonPopupMode popupMode() const;
#endif

    QAction *defaultAction() const;

    void setAutoRaise(bool enable);
    bool autoRaise() const;

public Q_SLOTS:
#ifndef QT_NO_MENU
    void showMenu();
#endif
    void setToolButtonStyle(Qt::ToolButtonStyle style);
    void setDefaultAction(QAction *);

Q_SIGNALS:
    void triggered(QAction *);

protected:
    QToolButton(QToolButtonPrivate &, QWidget* parent);
    bool event(QEvent *e);
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void paintEvent(QPaintEvent *);
    void actionEvent(QActionEvent *);

    void enterEvent(QEvent *);
    void leaveEvent(QEvent *);
    void timerEvent(QTimerEvent *);
    void changeEvent(QEvent *);

    bool hitButton(const QPoint &pos) const;
    void nextCheckState();
    void initStyleOption(QStyleOptionToolButton *option) const;

private:
    Q_DISABLE_COPY(QToolButton)
    Q_DECLARE_PRIVATE(QToolButton)
#ifndef QT_NO_MENU
    Q_PRIVATE_SLOT(d_func(), void _q_buttonPressed())
    Q_PRIVATE_SLOT(d_func(), void _q_updateButtonDown())
    Q_PRIVATE_SLOT(d_func(), void _q_menuTriggered(QAction*))
#endif
    Q_PRIVATE_SLOT(d_func(), void _q_actionTriggered())

#ifdef QT3_SUPPORT
public:
    enum TextPosition {
        BesideIcon,
        BelowIcon
        , Right = BesideIcon,
        Under = BelowIcon
    };

    QT3_SUPPORT_CONSTRUCTOR QToolButton(QWidget * parent, const char* name);
    QT3_SUPPORT_CONSTRUCTOR QToolButton(Qt::ArrowType type, QWidget *parent, const char* name);
    QT3_SUPPORT_CONSTRUCTOR QToolButton( const QIcon& s, const QString &textLabel,
                                       const QString& grouptext,
                                       QObject * receiver, const char* slot,
                                       QWidget * parent, const char* name=0 );
    inline QT3_SUPPORT void setPixmap(const QPixmap &pixmap) { setIcon(static_cast<QIcon>(pixmap)); }
    QT3_SUPPORT void setOnIconSet(const QIcon&);
    QT3_SUPPORT void setOffIconSet(const QIcon&);
    inline QT3_SUPPORT void setIconSet(const QIcon &icon){setIcon(icon);}
    QT3_SUPPORT void setIconSet(const QIcon &, bool on);
    inline QT3_SUPPORT void setTextLabel(const QString &text, bool tooltip = true) {
        setText(text);
#ifndef QT_NO_TOOLTIP
        if (tooltip)
            setToolTip(text);
#else
        Q_UNUSED(tooltip);
#endif
    }
    inline QT3_SUPPORT QString textLabel() const { return text(); }
    QT3_SUPPORT QIcon onIconSet() const;
    QT3_SUPPORT QIcon offIconSet() const;
    QT3_SUPPORT QIcon iconSet(bool on) const;
    inline QT3_SUPPORT QIcon iconSet() const { return icon(); }
    inline QT3_SUPPORT void openPopup()  { showMenu(); }
    inline QT3_SUPPORT void setPopup(QMenu* popup) {setMenu(popup); }
    inline QT3_SUPPORT QMenu* popup() const { return menu(); }
    inline QT3_SUPPORT bool usesBigPixmap() const { return iconSize().height() > 22; }
    inline QT3_SUPPORT bool usesTextLabel() const { return toolButtonStyle() != Qt::ToolButtonIconOnly; }
    inline QT3_SUPPORT TextPosition textPosition() const
    { return toolButtonStyle() == Qt::ToolButtonTextUnderIcon ? BelowIcon : BesideIcon; }
    QT3_SUPPORT void setPopupDelay(int delay);
    QT3_SUPPORT int popupDelay() const;

public Q_SLOTS:
    QT_MOC_COMPAT void setUsesBigPixmap(bool enable)
        { setIconSize(enable?QSize(32,32):QSize(22,22)); }
    QT_MOC_COMPAT void setUsesTextLabel(bool enable)
        { setToolButtonStyle(enable?Qt::ToolButtonTextUnderIcon : Qt::ToolButtonIconOnly); }
    QT_MOC_COMPAT void setTextPosition(QToolButton::TextPosition pos)
        { setToolButtonStyle(pos == BesideIcon ? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonTextUnderIcon); }

#endif
};

#endif // QT_NO_TOOLBUTTON

QT_END_NAMESPACE

QT_END_HEADER

#endif // QTOOLBUTTON_H
