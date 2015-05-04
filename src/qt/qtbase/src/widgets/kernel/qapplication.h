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

#ifndef QAPPLICATION_H
#define QAPPLICATION_H

#include <QtCore/qcoreapplication.h>
#include <QtGui/qwindowdefs.h>
#include <QtCore/qpoint.h>
#include <QtCore/qsize.h>
#include <QtGui/qcursor.h>
#ifdef QT_INCLUDE_COMPAT
# include <QtWidgets/qdesktopwidget.h>
#endif
#ifdef Q_NO_USING_KEYWORD
#include <QtGui/qpalette.h>
#endif
#include <QtGui/qguiapplication.h>

QT_BEGIN_NAMESPACE


class QDesktopWidget;
class QStyle;
class QEventLoop;
class QIcon;
template <typename T> class QList;
class QLocale;
class QPlatformNativeInterface;

class QApplication;
class QApplicationPrivate;
#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<QApplication *>(QCoreApplication::instance()))

class Q_WIDGETS_EXPORT QApplication : public QGuiApplication
{
    Q_OBJECT
    Q_PROPERTY(QIcon windowIcon READ windowIcon WRITE setWindowIcon)
    Q_PROPERTY(int cursorFlashTime READ cursorFlashTime WRITE setCursorFlashTime)
    Q_PROPERTY(int doubleClickInterval  READ doubleClickInterval WRITE setDoubleClickInterval)
    Q_PROPERTY(int keyboardInputInterval READ keyboardInputInterval WRITE setKeyboardInputInterval)
#ifndef QT_NO_WHEELEVENT
    Q_PROPERTY(int wheelScrollLines  READ wheelScrollLines WRITE setWheelScrollLines)
#endif
    Q_PROPERTY(QSize globalStrut READ globalStrut WRITE setGlobalStrut)
    Q_PROPERTY(int startDragTime  READ startDragTime WRITE setStartDragTime)
    Q_PROPERTY(int startDragDistance  READ startDragDistance WRITE setStartDragDistance)
#ifndef QT_NO_STYLE_STYLESHEET
    Q_PROPERTY(QString styleSheet READ styleSheet WRITE setStyleSheet)
#endif
#ifdef Q_OS_WINCE
    Q_PROPERTY(int autoMaximizeThreshold READ autoMaximizeThreshold WRITE setAutoMaximizeThreshold)
#endif
    Q_PROPERTY(bool autoSipEnabled READ autoSipEnabled WRITE setAutoSipEnabled)

public:
#ifdef Q_QDOC
    QApplication(int &argc, char **argv);
#else
    QApplication(int &argc, char **argv, int = ApplicationFlags);
#endif
    virtual ~QApplication();

    static QStyle *style();
    static void setStyle(QStyle*);
    static QStyle *setStyle(const QString&);
    enum ColorSpec { NormalColor=0, CustomColor=1, ManyColor=2 };
    static int colorSpec();
    static void setColorSpec(int);
#if QT_DEPRECATED_SINCE(5, 0)
    QT_DEPRECATED static inline void setGraphicsSystem(const QString &) {}
#endif

#if defined(Q_NO_USING_KEYWORD) && !defined(Q_QDOC)
    static QPalette palette() { return QGuiApplication::palette(); }
#else
    using QGuiApplication::palette;
#endif
    static QPalette palette(const QWidget *);
    static QPalette palette(const char *className);
    static void setPalette(const QPalette &, const char* className = 0);
    static QFont font();
    static QFont font(const QWidget*);
    static QFont font(const char *className);
    static void setFont(const QFont &, const char* className = 0);
    static QFontMetrics fontMetrics();

#if QT_VERSION < 0x060000 // remove these forwarders in Qt 6
    static void setWindowIcon(const QIcon &icon);
    static QIcon windowIcon();
#endif

    static QWidgetList allWidgets();
    static QWidgetList topLevelWidgets();

    static QDesktopWidget *desktop();

    static QWidget *activePopupWidget();
    static QWidget *activeModalWidget();
    static QWidget *focusWidget();

    static QWidget *activeWindow();
    static void setActiveWindow(QWidget* act);

    static QWidget *widgetAt(const QPoint &p);
    static inline QWidget *widgetAt(int x, int y) { return widgetAt(QPoint(x, y)); }
    static QWidget *topLevelAt(const QPoint &p);
    static inline QWidget *topLevelAt(int x, int y)  { return topLevelAt(QPoint(x, y)); }

#if QT_DEPRECATED_SINCE(5, 0)
    QT_DEPRECATED static inline void syncX() {}
#endif
    static void beep();
    static void alert(QWidget *widget, int duration = 0);

    static void setCursorFlashTime(int);
    static int cursorFlashTime();

    static void setDoubleClickInterval(int);
    static int doubleClickInterval();

    static void setKeyboardInputInterval(int);
    static int keyboardInputInterval();

#ifndef QT_NO_WHEELEVENT
    static void setWheelScrollLines(int);
    static int wheelScrollLines();
#endif
    static void setGlobalStrut(const QSize &);
    static QSize globalStrut();

    static void setStartDragTime(int ms);
    static int startDragTime();
    static void setStartDragDistance(int l);
    static int startDragDistance();

    static bool isEffectEnabled(Qt::UIEffect);
    static void setEffectEnabled(Qt::UIEffect, bool enable = true);

#if QT_DEPRECATED_SINCE(5, 0)
    QT_DEPRECATED static QLocale keyboardInputLocale()
    { return qApp ? qApp->inputMethod()->locale() : QLocale::c(); }
    QT_DEPRECATED static Qt::LayoutDirection keyboardInputDirection()
    { return qApp ? qApp->inputMethod()->inputDirection() : Qt::LeftToRight; }
#endif

    static int exec();
    bool notify(QObject *, QEvent *);

#ifdef QT_KEYPAD_NAVIGATION
    static Q_DECL_DEPRECATED void setKeypadNavigationEnabled(bool);
    static bool keypadNavigationEnabled();
    static void setNavigationMode(Qt::NavigationMode mode);
    static Qt::NavigationMode navigationMode();
#endif

Q_SIGNALS:
    void focusChanged(QWidget *old, QWidget *now);

public:
    QString styleSheet() const;
public Q_SLOTS:
#ifndef QT_NO_STYLE_STYLESHEET
    void setStyleSheet(const QString& sheet);
#endif
#ifdef Q_OS_WINCE
    void setAutoMaximizeThreshold(const int threshold);
    int autoMaximizeThreshold() const;
#endif
    void setAutoSipEnabled(const bool enabled);
    bool autoSipEnabled() const;
    static void closeAllWindows();
    static void aboutQt();

protected:
    bool event(QEvent *);
    bool compressEvent(QEvent *, QObject *receiver, QPostEventList *);

private:
    Q_DISABLE_COPY(QApplication)
    Q_DECLARE_PRIVATE(QApplication)

    friend class QGraphicsWidget;
    friend class QGraphicsItem;
    friend class QGraphicsScene;
    friend class QGraphicsScenePrivate;
    friend class QWidget;
    friend class QWidgetPrivate;
    friend class QWidgetWindow;
    friend class QTranslator;
    friend class QWidgetAnimator;
#ifndef QT_NO_SHORTCUT
    friend class QShortcut;
    friend class QLineEdit;
    friend class QWidgetTextControl;
#endif
    friend class QAction;

#ifndef QT_NO_GESTURES
    friend class QGestureManager;
#endif
};

QT_END_NAMESPACE

#endif // QAPPLICATION_H
