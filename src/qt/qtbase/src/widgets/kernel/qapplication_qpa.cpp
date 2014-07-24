/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include "qapplication_p.h"
#include "qcolormap.h"
#include "qpalette.h"
#include "qpixmapcache.h"
#ifndef QT_NO_CURSOR
#include "private/qcursor_p.h"
#endif
#include "qscreen.h"

#include "private/qwidget_p.h"
#include "private/qevent_p.h"

#include <qpa/qplatformintegrationfactory_p.h>
#include <qdesktopwidget.h>
#include <qpa/qplatformcursor.h>
#include <qpa/qplatformtheme.h>
#include <qpa/qplatformwindow.h>

#include <qdebug.h>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qwindowsysteminterface_p.h>
#include <qpa/qplatformintegration.h>

#include "qdesktopwidget_qpa_p.h"
#include "qwidgetwindow_qpa_p.h"
#include "qtooltip.h"

#ifdef Q_OS_WIN
#  include <QtCore/qt_windows.h> // for qt_win_display_dc()
#endif

QT_BEGIN_NAMESPACE

static QString appFont;
static bool popupGrabOk;
extern QWidget *qt_button_down;
extern QWidget *qt_popup_down;
extern bool qt_replay_popup_mouse_event;
int openPopupCount = 0;
extern QPointer<QWidget> qt_last_mouse_receiver;

void QApplicationPrivate::createEventDispatcher()
{
    QGuiApplicationPrivate::createEventDispatcher();
}

bool qt_try_modal(QWidget *widget, QEvent::Type type)
{
    QWidget * top = 0;

    if (QApplicationPrivate::tryModalHelper(widget, &top))
        return true;

    bool block_event  = false;

    switch (type) {
#if 0
    case QEvent::Focus:
        if (!static_cast<QWSFocusEvent*>(event)->simpleData.get_focus)
            break;
        // drop through
#endif
    case QEvent::MouseButtonPress:                        // disallow mouse/key events
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
        block_event         = true;
        break;
    default:
        break;
    }

    if (block_event && top && top->parentWidget() == 0)
        top->raise();

    return !block_event;
}

bool QApplicationPrivate::modalState()
{
    return !self->modalWindowList.isEmpty();
}

QWidget *qt_tlw_for_window(QWindow *wnd)
{
    // QTBUG-32177, wnd might be a QQuickView embedded via window container.
    while (wnd && !wnd->isTopLevel()) {
        QWindow *parent = wnd->parent();
        // Don't end up in windows not belonging to this application
        if (parent && parent->type() != Qt::ForeignWindow)
            wnd = wnd->parent();
        else
            break;
    }
    if (wnd)
        foreach (QWidget *tlw, qApp->topLevelWidgets())
            if (tlw->windowHandle() == wnd)
                return tlw;
    return 0;
}

void QApplicationPrivate::notifyActiveWindowChange(QWindow *previous)
{
    Q_UNUSED(previous);
    QWindow *wnd = QGuiApplicationPrivate::focus_window;
    if (inPopupMode()) // some delayed focus event to ignore
        return;
    QWidget *tlw = qt_tlw_for_window(wnd);
    QApplication::setActiveWindow(tlw);
}

static void ungrabKeyboardForPopup(QWidget *popup)
{
    if (QWidget::keyboardGrabber())
        qt_widget_private(QWidget::keyboardGrabber())->stealKeyboardGrab(true);
    else
        qt_widget_private(popup)->stealKeyboardGrab(false);
}

static void ungrabMouseForPopup(QWidget *popup)
{
    if (QWidget::mouseGrabber())
        qt_widget_private(QWidget::mouseGrabber())->stealMouseGrab(true);
    else
        qt_widget_private(popup)->stealMouseGrab(false);
}

static void grabForPopup(QWidget *popup)
{
    Q_ASSERT(popup->testAttribute(Qt::WA_WState_Created));
    popupGrabOk = qt_widget_private(popup)->stealKeyboardGrab(true);
    if (popupGrabOk) {
        popupGrabOk = qt_widget_private(popup)->stealMouseGrab(true);
        if (!popupGrabOk) {
            // transfer grab back to the keyboard grabber if any
            ungrabKeyboardForPopup(popup);
        }
    }
}

void QApplicationPrivate::closePopup(QWidget *popup)
{
    if (!popupWidgets)
        return;
    popupWidgets->removeAll(popup);

     if (popup == qt_popup_down) {
         qt_button_down = 0;
         qt_popup_down = 0;
     }

    if (QApplicationPrivate::popupWidgets->count() == 0) { // this was the last popup
        delete QApplicationPrivate::popupWidgets;
        QApplicationPrivate::popupWidgets = 0;

        if (popupGrabOk) {
            popupGrabOk = false;

            if (popup->geometry().contains(QPoint(QGuiApplicationPrivate::mousePressX,
                                                  QGuiApplicationPrivate::mousePressY))
                || popup->testAttribute(Qt::WA_NoMouseReplay)) {
                // mouse release event or inside
                qt_replay_popup_mouse_event = false;
            } else { // mouse press event
                qt_replay_popup_mouse_event = true;
            }

            // transfer grab back to mouse grabber if any, otherwise release the grab
            ungrabMouseForPopup(popup);

            // transfer grab back to keyboard grabber if any, otherwise release the grab
            ungrabKeyboardForPopup(popup);
        }

        if (active_window) {
            if (QWidget *fw = active_window->focusWidget()) {
                if (fw != QApplication::focusWidget()) {
                    fw->setFocus(Qt::PopupFocusReason);
                } else {
                    QFocusEvent e(QEvent::FocusIn, Qt::PopupFocusReason);
                    QCoreApplication::sendEvent(fw, &e);
                }
            }
        }

    } else {
        // A popup was closed, so the previous popup gets the focus.
        QWidget* aw = QApplicationPrivate::popupWidgets->last();
        if (QWidget *fw = aw->focusWidget())
            fw->setFocus(Qt::PopupFocusReason);

        if (QApplicationPrivate::popupWidgets->count() == 1) // grab mouse/keyboard
            grabForPopup(aw);
    }

}

void QApplicationPrivate::openPopup(QWidget *popup)
{
    openPopupCount++;
    if (!popupWidgets) // create list
        popupWidgets = new QWidgetList;
    popupWidgets->append(popup); // add to end of list

    if (QApplicationPrivate::popupWidgets->count() == 1) // grab mouse/keyboard
        grabForPopup(popup);

    // popups are not focus-handled by the window system (the first
    // popup grabbed the keyboard), so we have to do that manually: A
    // new popup gets the focus
    if (popup->focusWidget()) {
        popup->focusWidget()->setFocus(Qt::PopupFocusReason);
    } else if (popupWidgets->count() == 1) { // this was the first popup
        if (QWidget *fw = QApplication::focusWidget()) {
            QFocusEvent e(QEvent::FocusOut, Qt::PopupFocusReason);
            QApplication::sendEvent(fw, &e);
        }
    }
}

void QApplicationPrivate::initializeMultitouch_sys()
{
}

void QApplicationPrivate::cleanupMultitouch_sys()
{
}

static void setPossiblePalette(const QPalette *palette, const char *className)
{
    if (palette == 0)
        return;
    QApplicationPrivate::setPalette_helper(*palette, className, false);
}


void QApplicationPrivate::initializeWidgetPaletteHash()
{
    QPlatformTheme *platformTheme = QGuiApplicationPrivate::platformTheme();
    if (!platformTheme)
        return;
    qt_app_palettes_hash()->clear();

    setPossiblePalette(platformTheme->palette(QPlatformTheme::ToolButtonPalette), "QToolButton");
    setPossiblePalette(platformTheme->palette(QPlatformTheme::ButtonPalette), "QAbstractButton");
    setPossiblePalette(platformTheme->palette(QPlatformTheme::CheckBoxPalette), "QCheckBox");
    setPossiblePalette(platformTheme->palette(QPlatformTheme::RadioButtonPalette), "QRadioButton");
    setPossiblePalette(platformTheme->palette(QPlatformTheme::HeaderPalette), "QHeaderView");
    setPossiblePalette(platformTheme->palette(QPlatformTheme::ItemViewPalette), "QAbstractItemView");
    setPossiblePalette(platformTheme->palette(QPlatformTheme::MessageBoxLabelPalette), "QMessageBoxLabel");
    setPossiblePalette(platformTheme->palette(QPlatformTheme::TabBarPalette), "QTabBar");
    setPossiblePalette(platformTheme->palette(QPlatformTheme::LabelPalette), "QLabel");
    setPossiblePalette(platformTheme->palette(QPlatformTheme::GroupBoxPalette), "QGroupBox");
    setPossiblePalette(platformTheme->palette(QPlatformTheme::MenuPalette), "QMenu");
    setPossiblePalette(platformTheme->palette(QPlatformTheme::MenuBarPalette), "QMenuBar");
    setPossiblePalette(platformTheme->palette(QPlatformTheme::TextEditPalette), "QTextEdit");
    setPossiblePalette(platformTheme->palette(QPlatformTheme::TextEditPalette), "QTextControl");
    setPossiblePalette(platformTheme->palette(QPlatformTheme::TextLineEditPalette), "QLineEdit");
}

void QApplicationPrivate::initializeWidgetFontHash()
{
    const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme();
    if (!theme)
        return;
    FontHash *fontHash = qt_app_fonts_hash();
    fontHash->clear();

    if (const QFont *font = theme->font(QPlatformTheme::MenuFont))
        fontHash->insert(QByteArrayLiteral("QMenu"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::MenuBarFont))
        fontHash->insert(QByteArrayLiteral("QMenuBar"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::MenuItemFont))
        fontHash->insert(QByteArrayLiteral("QMenuItem"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::MessageBoxFont))
        fontHash->insert(QByteArrayLiteral("QMessageBox"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::LabelFont))
        fontHash->insert(QByteArrayLiteral("QLabel"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::TipLabelFont))
        fontHash->insert(QByteArrayLiteral("QTipLabel"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::TitleBarFont))
        fontHash->insert(QByteArrayLiteral("QTitleBar"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::StatusBarFont))
        fontHash->insert(QByteArrayLiteral("QStatusBar"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::MdiSubWindowTitleFont))
        fontHash->insert(QByteArrayLiteral("QMdiSubWindowTitleBar"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::DockWidgetTitleFont))
        fontHash->insert(QByteArrayLiteral("QDockWidgetTitle"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::PushButtonFont))
        fontHash->insert(QByteArrayLiteral("QPushButton"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::CheckBoxFont))
        fontHash->insert(QByteArrayLiteral("QCheckBox"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::RadioButtonFont))
        fontHash->insert(QByteArrayLiteral("QRadioButton"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::ToolButtonFont))
        fontHash->insert(QByteArrayLiteral("QToolButton"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::ItemViewFont))
        fontHash->insert(QByteArrayLiteral("QAbstractItemView"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::ListViewFont))
        fontHash->insert(QByteArrayLiteral("QListViewFont"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::HeaderViewFont))
        fontHash->insert(QByteArrayLiteral("QHeaderViewFont"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::ListBoxFont))
        fontHash->insert(QByteArrayLiteral("QListBox"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::ComboMenuItemFont))
        fontHash->insert(QByteArrayLiteral("QComboMenuItemFont"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::ComboLineEditFont))
        fontHash->insert(QByteArrayLiteral("QComboLineEditFont"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::SmallFont))
        fontHash->insert(QByteArrayLiteral("QSmallFont"), *font);
    if (const QFont *font = theme->font(QPlatformTheme::MiniFont))
        fontHash->insert(QByteArrayLiteral("QMiniFont"), *font);
}

#ifndef QT_NO_WHEELEVENT
void QApplication::setWheelScrollLines(int lines)
{
    QApplicationPrivate::wheel_scroll_lines = lines;
}

int QApplication::wheelScrollLines()
{
    return QApplicationPrivate::wheel_scroll_lines;
}
#endif

static inline int uiEffectToFlag(Qt::UIEffect effect)
{
    switch (effect) {
    case Qt::UI_General:
        return QPlatformTheme::GeneralUiEffect;
    case Qt::UI_AnimateMenu:
        return QPlatformTheme::AnimateMenuUiEffect;
    case Qt::UI_FadeMenu:
        return QPlatformTheme::FadeMenuUiEffect;
    case Qt::UI_AnimateCombo:
        return QPlatformTheme::AnimateComboUiEffect;
    case Qt::UI_AnimateTooltip:
        return QPlatformTheme::AnimateTooltipUiEffect;
    case Qt::UI_FadeTooltip:
        return QPlatformTheme::FadeTooltipUiEffect;
    case Qt::UI_AnimateToolBox:
        return QPlatformTheme::AnimateToolBoxUiEffect;
    }
    return 0;
}

void QApplication::setEffectEnabled(Qt::UIEffect effect, bool enable)
{
    int effectFlags = uiEffectToFlag(effect);
    if (enable) {
        if (effectFlags & QPlatformTheme::FadeMenuUiEffect)
            effectFlags |= QPlatformTheme::AnimateMenuUiEffect;
        if (effectFlags & QPlatformTheme::FadeTooltipUiEffect)
            effectFlags |= QPlatformTheme::AnimateTooltipUiEffect;
        QApplicationPrivate::enabledAnimations |= effectFlags;
    } else {
        QApplicationPrivate::enabledAnimations &= ~effectFlags;
    }
}

bool QApplication::isEffectEnabled(Qt::UIEffect effect)
{
    return QColormap::instance().depth() >= 16
           && (QApplicationPrivate::enabledAnimations & QPlatformTheme::GeneralUiEffect)
           && (QApplicationPrivate::enabledAnimations & uiEffectToFlag(effect));
}

QWidget *QApplication::topLevelAt(const QPoint &pos)
{
    QList<QScreen *> screens = QGuiApplication::screens();
    QList<QScreen *>::const_iterator screen = screens.constBegin();
    QList<QScreen *>::const_iterator end = screens.constEnd();

    while (screen != end) {
        if ((*screen)->geometry().contains(pos)) {
            QWidgetWindow *w = qobject_cast<QWidgetWindow *>((*screen)->handle()->topLevelAt(pos));
            return w ? w->widget() : 0;
        }
        ++screen;
    }
    return 0;
}

void QApplication::beep()
{
    QMetaObject::invokeMethod(QGuiApplication::platformNativeInterface(), "beep");
}

void QApplication::alert(QWidget *widget, int duration)
{
    if (widget) {
       if (widget->window()->isActiveWindow() && !(widget->window()->windowState() & Qt::WindowMinimized))
            return;
        if (QWindow *window= QApplicationPrivate::windowForWidget(widget))
            window->alert(duration);
    } else {
        foreach (QWidget *topLevel, topLevelWidgets())
            QApplication::alert(topLevel, duration);
    }
}

void qt_init(QApplicationPrivate *priv, int type)
{
    Q_UNUSED(priv);
    Q_UNUSED(type);

    QColormap::initialize();

#ifndef QT_NO_TOOLTIP
    if (const QPalette *toolTipPalette = QGuiApplicationPrivate::platformTheme()->palette(QPlatformTheme::ToolTipPalette))
        QToolTip::setPalette(*toolTipPalette);
#endif

    QApplicationPrivate::initializeWidgetFontHash();
}

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
// #fixme: Remove.
static HDC         displayDC        = 0;                // display device context

Q_WIDGETS_EXPORT HDC qt_win_display_dc()                        // get display DC
{
    Q_ASSERT(qApp && qApp->thread() == QThread::currentThread());
    if (!displayDC)
        displayDC = GetDC(0);
    return displayDC;
}
#endif

void qt_cleanup()
{
    QPixmapCache::clear();
    QColormap::cleanup();

    QApplicationPrivate::active_window = 0; //### this should not be necessary
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    if (displayDC) {
        ReleaseDC(0, displayDC);
        displayDC = 0;
    }
#endif
}


QT_END_NAMESPACE
