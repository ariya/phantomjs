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

#include <QtCore/qhash.h>
#include <QtGui/qpushbutton.h>
#include <QtGui/qstyle.h>
#include <QtGui/qlayout.h>
#include <QtGui/qdialog.h>
#include <QtGui/qapplication.h>
#include <QtGui/private/qwidget_p.h>
#include <QtGui/qaction.h>

#include "qdialogbuttonbox.h"

#ifdef QT_SOFTKEYS_ENABLED
#include <QtGui/qaction.h>
#endif


QT_BEGIN_NAMESPACE

/*!
    \class QDialogButtonBox
    \since 4.2
    \brief The QDialogButtonBox class is a widget that presents buttons in a
    layout that is appropriate to the current widget style.

    \ingroup dialog-classes


    Dialogs and message boxes typically present buttons in a layout that
    conforms to the interface guidelines for that platform. Invariably,
    different platforms have different layouts for their dialogs.
    QDialogButtonBox allows a developer to add buttons to it and will
    automatically use the appropriate layout for the user's desktop
    environment.

    Most buttons for a dialog follow certain roles. Such roles include:

    \list
    \o Accepting or rejecting the dialog.
    \o Asking for help.
    \o Performing actions on the dialog itself (such as resetting fields or
       applying changes).
    \endlist

    There can also be alternate ways of dismissing the dialog which may cause
    destructive results.

    Most dialogs have buttons that can almost be considered standard (e.g.
    \gui OK and \gui Cancel buttons). It is sometimes convenient to create these
    buttons in a standard way.

    There are a couple ways of using QDialogButtonBox. One ways is to create
    the buttons (or button texts) yourself and add them to the button box,
    specifying their role.

    \snippet examples/dialogs/extension/finddialog.cpp 1
    \snippet examples/dialogs/extension/finddialog.cpp 6

    Alternatively, QDialogButtonBox provides several standard buttons (e.g. OK, Cancel, Save)
    that you can use. They exist as flags so you can OR them together in the constructor.

    \snippet examples/dialogs/tabdialog/tabdialog.cpp 2

    You can mix and match normal buttons and standard buttons.

    Currently the buttons are laid out in the following way if the button box is horizontal:
    \table
    \row \o \inlineimage buttonbox-gnomelayout-horizontal.png GnomeLayout Horizontal
         \o Button box laid out in horizontal GnomeLayout
    \row \o \inlineimage buttonbox-kdelayout-horizontal.png KdeLayout Horizontal
         \o Button box laid out in horizontal KdeLayout
    \row \o \inlineimage buttonbox-maclayout-horizontal.png MacLayout Horizontal
         \o Button box laid out in horizontal MacLayout
    \row \o \inlineimage buttonbox-winlayout-horizontal.png  WinLayout Horizontal
         \o Button box laid out in horizontal WinLayout
    \endtable

    The buttons are laid out the following way if the button box is vertical:

    \table
    \row \o GnomeLayout
         \o KdeLayout
         \o MacLayout
         \o WinLayout
    \row \o \inlineimage buttonbox-gnomelayout-vertical.png GnomeLayout Vertical
         \o \inlineimage buttonbox-kdelayout-vertical.png KdeLayout Vertical
         \o \inlineimage buttonbox-maclayout-vertical.png MacLayout Vertical
         \o \inlineimage buttonbox-winlayout-vertical.png WinLayout Vertical
    \endtable

    Additionally, button boxes that contain only buttons with ActionRole or
    HelpRole can be considered modeless and have an alternate look on Mac OS X:

    \table
    \row \o modeless horizontal MacLayout
         \o \inlineimage buttonbox-mac-modeless-horizontal.png Screenshot of modeless horizontal MacLayout
    \endtable

    When a button is clicked in the button box, the clicked() signal is emitted
    for the actual button is that is pressed. For convenience, if the button
    has an AcceptRole, RejectRole, or HelpRole, the accepted(), rejected(), or
    helpRequested() signals are emitted respectively.

    If you want a specific button to be default you need to call
    QPushButton::setDefault() on it yourself. However, if there is no default
    button set and to preserve which button is the default button across
    platforms when using the QPushButton::autoDefault property, the first push
    button with the accept role is made the default button when the
    QDialogButtonBox is shown,

    \sa QMessageBox, QPushButton, QDialog
*/

enum {
    AcceptRole      = QDialogButtonBox::AcceptRole,
    RejectRole      = QDialogButtonBox::RejectRole,
    DestructiveRole = QDialogButtonBox::DestructiveRole,
    ActionRole      = QDialogButtonBox::ActionRole,
    HelpRole        = QDialogButtonBox::HelpRole,
    YesRole         = QDialogButtonBox::YesRole,
    NoRole          = QDialogButtonBox::NoRole,
    ApplyRole       = QDialogButtonBox::ApplyRole,
    ResetRole       = QDialogButtonBox::ResetRole,

    AlternateRole   = 0x10000000,
    Stretch         = 0x20000000,
    EOL             = 0x40000000,
    Reverse         = 0x80000000
};

static QDialogButtonBox::ButtonRole roleFor(QDialogButtonBox::StandardButton button)
{
    switch (button) {
    case QDialogButtonBox::Ok:
    case QDialogButtonBox::Save:
    case QDialogButtonBox::Open:
    case QDialogButtonBox::SaveAll:
    case QDialogButtonBox::Retry:
    case QDialogButtonBox::Ignore:
        return QDialogButtonBox::AcceptRole;

    case QDialogButtonBox::Cancel:
    case QDialogButtonBox::Close:
    case QDialogButtonBox::Abort:
        return QDialogButtonBox::RejectRole;

    case QDialogButtonBox::Discard:
        return QDialogButtonBox::DestructiveRole;

    case QDialogButtonBox::Help:
        return QDialogButtonBox::HelpRole;

    case QDialogButtonBox::Apply:
        return QDialogButtonBox::ApplyRole;

    case QDialogButtonBox::Yes:
    case QDialogButtonBox::YesToAll:
        return QDialogButtonBox::YesRole;

    case QDialogButtonBox::No:
    case QDialogButtonBox::NoToAll:
        return QDialogButtonBox::NoRole;

    case QDialogButtonBox::RestoreDefaults:
    case QDialogButtonBox::Reset:
        return QDialogButtonBox::ResetRole;

    case QDialogButtonBox::NoButton:    // NoButton means zero buttons, not "No" button
        ;
    }

    return QDialogButtonBox::InvalidRole;
}

static const int layouts[2][5][14] =
{
    // Qt::Horizontal
    {
        // WinLayout
        { ResetRole, Stretch, YesRole, AcceptRole, AlternateRole, DestructiveRole, NoRole, ActionRole, RejectRole, ApplyRole,
           HelpRole, EOL, EOL, EOL },

        // MacLayout
        { HelpRole, ResetRole, ApplyRole, ActionRole, Stretch, DestructiveRole | Reverse,
          AlternateRole | Reverse, RejectRole | Reverse, AcceptRole | Reverse, NoRole | Reverse, YesRole | Reverse, EOL, EOL },

        // KdeLayout
        { HelpRole, ResetRole, Stretch, YesRole, NoRole, ActionRole, AcceptRole, AlternateRole,
          ApplyRole, DestructiveRole, RejectRole, EOL },

        // GnomeLayout
        { HelpRole, ResetRole, Stretch, ActionRole, ApplyRole | Reverse, DestructiveRole | Reverse,
          AlternateRole | Reverse, RejectRole | Reverse, AcceptRole | Reverse, NoRole | Reverse, YesRole | Reverse, EOL },

        // Mac modeless
        { ResetRole, ApplyRole, ActionRole, Stretch, HelpRole, EOL, EOL, EOL, EOL, EOL, EOL, EOL, EOL, EOL }
    },

    // Qt::Vertical
    {
        // WinLayout
        { ActionRole, YesRole, AcceptRole, AlternateRole, DestructiveRole, NoRole, RejectRole, ApplyRole, ResetRole,
          HelpRole, Stretch, EOL, EOL, EOL },

        // MacLayout
        { YesRole, NoRole, AcceptRole, RejectRole, AlternateRole, DestructiveRole, Stretch, ActionRole, ApplyRole,
          ResetRole, HelpRole, EOL, EOL },

        // KdeLayout
        { AcceptRole, AlternateRole, ApplyRole, ActionRole, YesRole, NoRole, Stretch, ResetRole,
          DestructiveRole, RejectRole, HelpRole, EOL },

        // GnomeLayout
        { YesRole, NoRole, AcceptRole, RejectRole, AlternateRole, DestructiveRole, ApplyRole, ActionRole, Stretch,
          ResetRole, HelpRole, EOL, EOL, EOL },

        // Mac modeless
        { ActionRole, ApplyRole, ResetRole, Stretch, HelpRole, EOL, EOL, EOL, EOL, EOL, EOL, EOL, EOL, EOL }
    }
};

#if defined(QT_SOFTKEYS_ENABLED) && !defined(QT_NO_ACTION)
class QDialogButtonEnabledProxy : public QObject
{
public:
    QDialogButtonEnabledProxy(QObject *parent, QWidget *src, QAction *trg) : QObject(parent), source(src), target(trg)
    {
        source->installEventFilter(this);
        target->setEnabled(source->isEnabled());
    }
    ~QDialogButtonEnabledProxy()
    {
        source->removeEventFilter(this);
    }
    bool eventFilter(QObject *object, QEvent *event)
    {
        if (object == source && event->type() == QEvent::EnabledChange) {
            target->setEnabled(source->isEnabled());
        }
        return false;
    };
private:
    QWidget *source;
    QAction *target;
};
#endif

class QDialogButtonBoxPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QDialogButtonBox)

public:
    QDialogButtonBoxPrivate(Qt::Orientation orient);

    QList<QAbstractButton *> buttonLists[QDialogButtonBox::NRoles];
    QHash<QPushButton *, QDialogButtonBox::StandardButton> standardButtonHash;
#ifdef QT_SOFTKEYS_ENABLED
    QHash<QAbstractButton *, QAction *> softKeyActions;
#endif

    Qt::Orientation orientation;
    QDialogButtonBox::ButtonLayout layoutPolicy;
    QBoxLayout *buttonLayout;
    bool internalRemove;
    bool center;

    void createStandardButtons(QDialogButtonBox::StandardButtons buttons);

    void layoutButtons();
    void initLayout();
    void resetLayout();
    QPushButton *createButton(QDialogButtonBox::StandardButton button, bool doLayout = true);
    void addButton(QAbstractButton *button, QDialogButtonBox::ButtonRole role, bool doLayout = true);
    void _q_handleButtonDestroyed();
    void _q_handleButtonClicked();
    void addButtonsToLayout(const QList<QAbstractButton *> &buttonList, bool reverse);
    void retranslateStrings();
    const char *standardButtonText(QDialogButtonBox::StandardButton sbutton) const;
#if defined(QT_SOFTKEYS_ENABLED) && !defined(QT_NO_ACTION)
    QAction *createSoftKey(QAbstractButton *button, QDialogButtonBox::ButtonRole role);
#endif
};

QDialogButtonBoxPrivate::QDialogButtonBoxPrivate(Qt::Orientation orient)
    : orientation(orient), buttonLayout(0), internalRemove(false), center(false)
{
}

void QDialogButtonBoxPrivate::initLayout()
{
    Q_Q(QDialogButtonBox);
    layoutPolicy = QDialogButtonBox::ButtonLayout(q->style()->styleHint(QStyle::SH_DialogButtonLayout, 0, q));
    bool createNewLayout = buttonLayout == 0
        || (orientation == Qt::Horizontal && qobject_cast<QVBoxLayout *>(buttonLayout) != 0)
        || (orientation == Qt::Vertical && qobject_cast<QHBoxLayout *>(buttonLayout) != 0);
    if (createNewLayout) {
        delete buttonLayout;
        if (orientation == Qt::Horizontal)
            buttonLayout = new QHBoxLayout(q);
        else
            buttonLayout = new QVBoxLayout(q);
    }

    int left, top, right, bottom;
    setLayoutItemMargins(QStyle::SE_PushButtonLayoutItem);
    getLayoutItemMargins(&left, &top, &right, &bottom);
    buttonLayout->setContentsMargins(-left, -top, -right, -bottom);

    if (!q->testAttribute(Qt::WA_WState_OwnSizePolicy)) {
        QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Fixed, QSizePolicy::ButtonBox);
        if (orientation == Qt::Vertical)
            sp.transpose();
        q->setSizePolicy(sp);
        q->setAttribute(Qt::WA_WState_OwnSizePolicy, false);
    }

    // ### move to a real init() function
    q->setFocusPolicy(Qt::TabFocus);
}

void QDialogButtonBoxPrivate::resetLayout()
{
    //delete buttonLayout;
    initLayout();
    layoutButtons();
}

void QDialogButtonBoxPrivate::addButtonsToLayout(const QList<QAbstractButton *> &buttonList,
                                                 bool reverse)
{
    int start = reverse ? buttonList.count() - 1 : 0;
    int end = reverse ? -1 : buttonList.count();
    int step = reverse ? -1 : 1;

    for (int i = start; i != end; i += step) {
        QAbstractButton *button = buttonList.at(i);
        buttonLayout->addWidget(button);
        button->show();
    }
}

void QDialogButtonBoxPrivate::layoutButtons()
{
    Q_Q(QDialogButtonBox);
    const int MacGap = 36 - 8;    // 8 is the default gap between a widget and a spacer item

    for (int i = buttonLayout->count() - 1; i >= 0; --i) {
        QLayoutItem *item = buttonLayout->takeAt(i);
        if (QWidget *widget = item->widget())
            widget->hide();
        delete item;
    }

    int tmpPolicy = layoutPolicy;

    static const int M = 5;
    static const int ModalRoles[M] = { AcceptRole, RejectRole, DestructiveRole, YesRole, NoRole };
    if (tmpPolicy == QDialogButtonBox::MacLayout) {
        bool hasModalButton = false;
        for (int i = 0; i < M; ++i) {
            if (!buttonLists[ModalRoles[i]].isEmpty()) {
                hasModalButton = true;
                break;
            }
        }
        if (!hasModalButton)
            tmpPolicy = 4;  // Mac modeless
    }

    const int *currentLayout = layouts[orientation == Qt::Vertical][tmpPolicy];

    if (center)
        buttonLayout->addStretch();

    QList<QAbstractButton *> acceptRoleList = buttonLists[AcceptRole];

    while (*currentLayout != EOL) {
        int role = (*currentLayout & ~Reverse);
        bool reverse = (*currentLayout & Reverse);

        switch (role) {
        case Stretch:
            if (!center)
                buttonLayout->addStretch();
            break;
        case AcceptRole: {
            if (acceptRoleList.isEmpty())
                break;
            // Only the first one
            QAbstractButton *button = acceptRoleList.first();
            buttonLayout->addWidget(button);
            button->show();
        }
            break;
        case AlternateRole:
            {
                if (acceptRoleList.size() < 2)
                    break;
                QList<QAbstractButton *> list = acceptRoleList;
                list.removeFirst();
                addButtonsToLayout(list, reverse);
            }
            break;
        case DestructiveRole:
            {
                const QList<QAbstractButton *> &list = buttonLists[role];

                /*
                    Mac: Insert a gap on the left of the destructive
                    buttons to ensure that they don't get too close to
                    the help and action buttons (but only if there are
                    some buttons to the left of the destructive buttons
                    (and the stretch, whence buttonLayout->count() > 1
                    and not 0)).
                */
                if (tmpPolicy == QDialogButtonBox::MacLayout
                        && !list.isEmpty() && buttonLayout->count() > 1)
                    buttonLayout->addSpacing(MacGap);

                addButtonsToLayout(list, reverse);

                /*
                    Insert a gap between the destructive buttons and the
                    accept and reject buttons.
                */
                if (tmpPolicy == QDialogButtonBox::MacLayout && !list.isEmpty())
                    buttonLayout->addSpacing(MacGap);
            }
            break;
        case RejectRole:
        case ActionRole:
        case HelpRole:
        case YesRole:
        case NoRole:
        case ApplyRole:
        case ResetRole:
            addButtonsToLayout(buttonLists[role], reverse);
        }
        ++currentLayout;
    }

    QWidget *lastWidget = 0;
    q->setFocusProxy(0);
    for (int i = 0; i < buttonLayout->count(); ++i) {
        QLayoutItem *item = buttonLayout->itemAt(i);
        if (QWidget *widget = item->widget()) {
            if (lastWidget)
                QWidget::setTabOrder(lastWidget, widget);
            else
                q->setFocusProxy(widget);
            lastWidget = widget;
        }
    }

    if (center)
        buttonLayout->addStretch();
}

QPushButton *QDialogButtonBoxPrivate::createButton(QDialogButtonBox::StandardButton sbutton,
                                                   bool doLayout)
{
    Q_Q(QDialogButtonBox);
    const char *buttonText = 0;
    int icon = 0;

    switch (sbutton) {
    case QDialogButtonBox::Ok:
        icon = QStyle::SP_DialogOkButton;
        break;
    case QDialogButtonBox::Save:
        icon = QStyle::SP_DialogSaveButton;
        break;
    case QDialogButtonBox::Open:
        icon = QStyle::SP_DialogOpenButton;
        break;
    case QDialogButtonBox::Cancel:
        icon = QStyle::SP_DialogCancelButton;
        break;
    case QDialogButtonBox::Close:
        icon = QStyle::SP_DialogCloseButton;
        break;
    case QDialogButtonBox::Apply:
        icon = QStyle::SP_DialogApplyButton;
        break;
    case QDialogButtonBox::Reset:
        icon = QStyle::SP_DialogResetButton;
        break;
    case QDialogButtonBox::Help:
        icon = QStyle::SP_DialogHelpButton;
        break;
    case QDialogButtonBox::Discard:
        icon = QStyle::SP_DialogDiscardButton;
        break;
    case QDialogButtonBox::Yes:
        icon = QStyle::SP_DialogYesButton;
        break;
    case QDialogButtonBox::No:
        icon = QStyle::SP_DialogNoButton;
        break;
    case QDialogButtonBox::YesToAll:
    case QDialogButtonBox::NoToAll:
    case QDialogButtonBox::SaveAll:
    case QDialogButtonBox::Abort:
    case QDialogButtonBox::Retry:
    case QDialogButtonBox::Ignore:
    case QDialogButtonBox::RestoreDefaults:
        break;
    case QDialogButtonBox::NoButton:
        return 0;
        ;
    }
    buttonText = standardButtonText(sbutton);

    QPushButton *button = new QPushButton(QDialogButtonBox::tr(buttonText), q);
    QStyle *style = q->style();
    if (style->styleHint(QStyle::SH_DialogButtonBox_ButtonsHaveIcons, 0, q) && icon != 0)
        button->setIcon(style->standardIcon(QStyle::StandardPixmap(icon), 0, q));
    if (style != QApplication::style()) // Propagate style
        button->setStyle(style);
    standardButtonHash.insert(button, sbutton);
    if (roleFor(sbutton) != QDialogButtonBox::InvalidRole) {
        addButton(button, roleFor(sbutton), doLayout);
    } else {
        qWarning("QDialogButtonBox::createButton: Invalid ButtonRole, button not added");
    }

#ifdef Q_WS_MAC
    // Since mnemonics is off by default on Mac, we add a Cmd-D
    // shortcut here to e.g. make the "Don't Save" button work nativly:
    if (sbutton == QDialogButtonBox::Discard)
        button->setShortcut(QKeySequence(QLatin1String("Ctrl+D")));
#endif

    return button;
}

void QDialogButtonBoxPrivate::addButton(QAbstractButton *button, QDialogButtonBox::ButtonRole role,
                                        bool doLayout)
{
    Q_Q(QDialogButtonBox);
    QObject::connect(button, SIGNAL(clicked()), q, SLOT(_q_handleButtonClicked()));
    QObject::connect(button, SIGNAL(destroyed()), q, SLOT(_q_handleButtonDestroyed()));
    buttonLists[role].append(button);
#if defined(QT_SOFTKEYS_ENABLED) && !defined(QT_NO_ACTION)
    QAction *action = createSoftKey(button, role);
    softKeyActions.insert(button, action);
    new QDialogButtonEnabledProxy(action, button, action);
#endif
    if (doLayout)
        layoutButtons();
}

#if defined(QT_SOFTKEYS_ENABLED) && !defined(QT_NO_ACTION)
QAction* QDialogButtonBoxPrivate::createSoftKey(QAbstractButton *button, QDialogButtonBox::ButtonRole role)
{
    Q_Q(QDialogButtonBox);
    QAction::SoftKeyRole softkeyRole;

    QAction *action = new QAction(button->text(), button);

    switch (role) {
    case ApplyRole:
    case AcceptRole:
    case YesRole:
    case ActionRole:
    case HelpRole:
        softkeyRole = QAction::PositiveSoftKey;
        break;
    case RejectRole:
    case DestructiveRole:
    case NoRole:
    case ResetRole:
        softkeyRole = QAction::NegativeSoftKey;
        break;
    default:
        break;
    }
    QObject::connect(action, SIGNAL(triggered()), button, SIGNAL(clicked()));
    action->setSoftKeyRole(softkeyRole);


    QWidget *dialog = 0;
    QWidget *p = q;
    while (p && !p->isWindow()) {
        p = p->parentWidget();
        if ((dialog = qobject_cast<QDialog *>(p)))
            break;
    }

    if (dialog) {
        dialog->addAction(action);
    } else {
        q->addAction(action);
    }

    return action;
}
#endif

void QDialogButtonBoxPrivate::createStandardButtons(QDialogButtonBox::StandardButtons buttons)
{
    uint i = QDialogButtonBox::FirstButton;
    while (i <= QDialogButtonBox::LastButton) {
        if (i & buttons) {
            createButton(QDialogButtonBox::StandardButton(i), false);
        }
        i = i << 1;
    }
    layoutButtons();
}

const char *QDialogButtonBoxPrivate::standardButtonText(QDialogButtonBox::StandardButton sbutton) const
{
    const char *buttonText = 0;
    bool gnomeLayout = (layoutPolicy == QDialogButtonBox::GnomeLayout);
    switch (sbutton) {
    case QDialogButtonBox::Ok:
        buttonText = gnomeLayout ? QT_TRANSLATE_NOOP("QDialogButtonBox", "&OK") : QT_TRANSLATE_NOOP("QDialogButtonBox", "OK");
        break;
    case QDialogButtonBox::Save:
        buttonText = gnomeLayout ? QT_TRANSLATE_NOOP("QDialogButtonBox", "&Save") : QT_TRANSLATE_NOOP("QDialogButtonBox", "Save");
        break;
    case QDialogButtonBox::Open:
        buttonText = QT_TRANSLATE_NOOP("QDialogButtonBox", "Open");
        break;
    case QDialogButtonBox::Cancel:
        buttonText = gnomeLayout ? QT_TRANSLATE_NOOP("QDialogButtonBox", "&Cancel") : QT_TRANSLATE_NOOP("QDialogButtonBox", "Cancel");
        break;
    case QDialogButtonBox::Close:
        buttonText = gnomeLayout ? QT_TRANSLATE_NOOP("QDialogButtonBox", "&Close") : QT_TRANSLATE_NOOP("QDialogButtonBox", "Close");
        break;
    case QDialogButtonBox::Apply:
        buttonText = QT_TRANSLATE_NOOP("QDialogButtonBox", "Apply");
        break;
    case QDialogButtonBox::Reset:
        buttonText = QT_TRANSLATE_NOOP("QDialogButtonBox", "Reset");
        break;
    case QDialogButtonBox::Help:
        buttonText = QT_TRANSLATE_NOOP("QDialogButtonBox", "Help");
        break;
    case QDialogButtonBox::Discard:
        if (layoutPolicy == QDialogButtonBox::MacLayout)
            buttonText = QT_TRANSLATE_NOOP("QDialogButtonBox", "Don't Save");
        else if (layoutPolicy == QDialogButtonBox::GnomeLayout)
            buttonText = QT_TRANSLATE_NOOP("QDialogButtonBox", "Close without Saving");
        else
            buttonText = QT_TRANSLATE_NOOP("QDialogButtonBox", "Discard");
        break;
    case QDialogButtonBox::Yes:
        buttonText = QT_TRANSLATE_NOOP("QDialogButtonBox", "&Yes");
        break;
    case QDialogButtonBox::YesToAll:
        buttonText = QT_TRANSLATE_NOOP("QDialogButtonBox", "Yes to &All");
        break;
    case QDialogButtonBox::No:
        buttonText = QT_TRANSLATE_NOOP("QDialogButtonBox", "&No");
        break;
    case QDialogButtonBox::NoToAll:
        buttonText = QT_TRANSLATE_NOOP("QDialogButtonBox", "N&o to All");
        break;
    case QDialogButtonBox::SaveAll:
        buttonText = QT_TRANSLATE_NOOP("QDialogButtonBox", "Save All");
        break;
    case QDialogButtonBox::Abort:
        buttonText = QT_TRANSLATE_NOOP("QDialogButtonBox", "Abort");
        break;
    case QDialogButtonBox::Retry:
        buttonText = QT_TRANSLATE_NOOP("QDialogButtonBox", "Retry");
        break;
    case QDialogButtonBox::Ignore:
        buttonText = QT_TRANSLATE_NOOP("QDialogButtonBox", "Ignore");
        break;
    case QDialogButtonBox::RestoreDefaults:
        buttonText = QT_TRANSLATE_NOOP("QDialogButtonBox", "Restore Defaults");
        break;
    case QDialogButtonBox::NoButton:
        ;
    } // switch
    return buttonText;
}

void QDialogButtonBoxPrivate::retranslateStrings()
{
    const char *buttonText = 0;
    QHash<QPushButton *, QDialogButtonBox::StandardButton>::iterator it =  standardButtonHash.begin();
    while (it != standardButtonHash.end()) {
        buttonText = standardButtonText(it.value());
        if (buttonText) {
            QPushButton *button = it.key();
            button->setText(QDialogButtonBox::tr(buttonText));
#if defined(QT_SOFTKEYS_ENABLED) && !defined(QT_NO_ACTION)
            QAction *action = softKeyActions.value(button, 0);
            if (action)
                action->setText(button->text());
#endif
        }
        ++it;
    }
}

/*!
    Constructs an empty, horizontal button box with the given \a parent.

    \sa orientation, addButton()
*/
QDialogButtonBox::QDialogButtonBox(QWidget *parent)
    : QWidget(*new QDialogButtonBoxPrivate(Qt::Horizontal), parent, 0)
{
    d_func()->initLayout();
}

/*!
    Constructs an empty button box with the given \a orientation and \a parent.

    \sa orientation, addButton()
*/
QDialogButtonBox::QDialogButtonBox(Qt::Orientation orientation, QWidget *parent)
    : QWidget(*new QDialogButtonBoxPrivate(orientation), parent, 0)
{
    d_func()->initLayout();
}

/*!
    Constructs a button box with the given \a orientation and \a parent, containing
    the standard buttons specified by \a buttons.

    \sa orientation, addButton()
*/
QDialogButtonBox::QDialogButtonBox(StandardButtons buttons, Qt::Orientation orientation,
                                   QWidget *parent)
    : QWidget(*new QDialogButtonBoxPrivate(orientation), parent, 0)
{
    d_func()->initLayout();
    d_func()->createStandardButtons(buttons);
}

/*!
    Destroys the button box.
*/
QDialogButtonBox::~QDialogButtonBox()
{
}

/*!
    \enum QDialogButtonBox::ButtonRole
    \enum QMessageBox::ButtonRole

    This enum describes the roles that can be used to describe buttons in
    the button box. Combinations of these roles are as flags used to
    describe different aspects of their behavior.

    \value InvalidRole The button is invalid.
    \value AcceptRole Clicking the button causes the dialog to be accepted
           (e.g. OK).
    \value RejectRole Clicking the button causes the dialog to be rejected
           (e.g. Cancel).
    \value DestructiveRole Clicking the button causes a destructive change
           (e.g. for Discarding Changes) and closes the dialog.
    \value ActionRole Clicking the button causes changes to the elements within
           the dialog.
    \value HelpRole The button can be clicked to request help.
    \value YesRole The button is a "Yes"-like button.
    \value NoRole The button is a "No"-like button.
    \value ApplyRole The button applies current changes.
    \value ResetRole The button resets the dialog's fields to default values.

    \omitvalue NRoles

    \sa StandardButton
*/

/*!
    \enum QDialogButtonBox::StandardButton

    These enums describe flags for standard buttons. Each button has a
    defined \l ButtonRole.

    \value Ok An "OK" button defined with the \l AcceptRole.
    \value Open A "Open" button defined with the \l AcceptRole.
    \value Save A "Save" button defined with the \l AcceptRole.
    \value Cancel A "Cancel" button defined with the \l RejectRole.
    \value Close A "Close" button defined with the \l RejectRole.
    \value Discard A "Discard" or "Don't Save" button, depending on the platform,
                    defined with the \l DestructiveRole.
    \value Apply An "Apply" button defined with the \l ApplyRole.
    \value Reset A "Reset" button defined with the \l ResetRole.
    \value RestoreDefaults A "Restore Defaults" button defined with the \l ResetRole.
    \value Help A "Help" button defined with the \l HelpRole.
    \value SaveAll A "Save All" button defined with the \l AcceptRole.
    \value Yes A "Yes" button defined with the \l YesRole.
    \value YesToAll A "Yes to All" button defined with the \l YesRole.
    \value No A "No" button defined with the \l NoRole.
    \value NoToAll A "No to All" button defined with the \l NoRole.
    \value Abort An "Abort" button defined with the \l RejectRole.
    \value Retry A "Retry" button defined with the \l AcceptRole.
    \value Ignore An "Ignore" button defined with the \l AcceptRole.

    \value NoButton An invalid button.

    \omitvalue FirstButton
    \omitvalue LastButton

    \sa ButtonRole, standardButtons
*/

/*!
    \enum QDialogButtonBox::ButtonLayout

    This enum describes the layout policy to be used when arranging the buttons
    contained in the button box.

    \value WinLayout Use a policy appropriate for applications on Windows.
    \value MacLayout Use a policy appropriate for applications on Mac OS X.
    \value KdeLayout Use a policy appropriate for applications on KDE.
    \value GnomeLayout Use a policy appropriate for applications on GNOME.

    The button layout is specified by the \l{style()}{current style}. However,
    on the X11 platform, it may be influenced by the desktop environment.
*/

/*!
    \fn void QDialogButtonBox::clicked(QAbstractButton *button)

    This signal is emitted when a button inside the button box is clicked. The
    specific button that was pressed is specified by \a button.

    \sa accepted(), rejected(), helpRequested()
*/

/*!
    \fn void QDialogButtonBox::accepted()

    This signal is emitted when a button inside the button box is clicked, as long
    as it was defined with the \l AcceptRole or \l YesRole.

    \sa rejected(), clicked() helpRequested()
*/

/*!
    \fn void QDialogButtonBox::rejected()

    This signal is emitted when a button inside the button box is clicked, as long
    as it was defined with the \l RejectRole or \l NoRole.

    \sa accepted() helpRequested() clicked()
*/

/*!
    \fn void QDialogButtonBox::helpRequested()

    This signal is emitted when a button inside the button box is clicked, as long
    as it was defined with the \l HelpRole.

    \sa accepted() rejected() clicked()
*/

/*!
    \property QDialogButtonBox::orientation
    \brief the orientation of the button box

    By default, the orientation is horizontal (i.e. the buttons are laid out
    side by side). The possible orientations are Qt::Horizontal and
    Qt::Vertical.
*/
Qt::Orientation QDialogButtonBox::orientation() const
{
    return d_func()->orientation;
}

void QDialogButtonBox::setOrientation(Qt::Orientation orientation)
{
    Q_D(QDialogButtonBox);
    if (orientation == d->orientation)
        return;

    d->orientation = orientation;
    d->resetLayout();
}

/*!
    Clears the button box, deleting all buttons within it.

    \sa removeButton(), addButton()
*/
void QDialogButtonBox::clear()
{
    Q_D(QDialogButtonBox);
#ifdef QT_SOFTKEYS_ENABLED
    // Delete softkey actions as they have the buttons as parents
    qDeleteAll(d->softKeyActions.values());
    d->softKeyActions.clear();
#endif
    // Remove the created standard buttons, they should be in the other lists, which will
    // do the deletion
    d->standardButtonHash.clear();
    for (int i = 0; i < NRoles; ++i) {
        QList<QAbstractButton *> &list = d->buttonLists[i];
        while (list.count()) {
            QAbstractButton *button = list.takeAt(0);
            QObject::disconnect(button, SIGNAL(destroyed()), this, SLOT(_q_handleButtonDestroyed()));
            delete button;
        }
    }
}

/*!
    Returns a list of all the buttons that have been added to the button box.

    \sa buttonRole(), addButton(), removeButton()
*/
QList<QAbstractButton *> QDialogButtonBox::buttons() const
{
    Q_D(const QDialogButtonBox);
    QList<QAbstractButton *> finalList;
    for (int i = 0; i < NRoles; ++i) {
        const QList<QAbstractButton *> &list = d->buttonLists[i];
        for (int j = 0; j < list.count(); ++j)
            finalList.append(list.at(j));
    }
    return finalList;
}

/*!
    Returns the button role for the specified \a button. This function returns
    \l InvalidRole if \a button is 0 or has not been added to the button box.

    \sa buttons(), addButton()
*/
QDialogButtonBox::ButtonRole QDialogButtonBox::buttonRole(QAbstractButton *button) const
{
    Q_D(const QDialogButtonBox);
    for (int i = 0; i < NRoles; ++i) {
        const QList<QAbstractButton *> &list = d->buttonLists[i];
        for (int j = 0; j < list.count(); ++j) {
            if (list.at(j) == button)
                return ButtonRole(i);
        }
    }
    return InvalidRole;
}

/*!
    Removes \a button from the button box without deleting it and sets its parent to zero.

    \sa clear(), buttons(), addButton()
*/
void QDialogButtonBox::removeButton(QAbstractButton *button)
{
    Q_D(QDialogButtonBox);

    if (!button)
        return;

    // Remove it from the standard button hash first and then from the roles
    if (QPushButton *pushButton = qobject_cast<QPushButton *>(button))
        d->standardButtonHash.remove(pushButton);
    for (int i = 0; i < NRoles; ++i) {
        QList<QAbstractButton *> &list = d->buttonLists[i];
        for (int j = 0; j < list.count(); ++j) {
            if (list.at(j) == button) {
                list.takeAt(j);
                if (!d->internalRemove) {
                    disconnect(button, SIGNAL(clicked()), this, SLOT(_q_handleButtonClicked()));
                    disconnect(button, SIGNAL(destroyed()), this, SLOT(_q_handleButtonDestroyed()));
                }
                break;
            }
        }
    }
#if defined(QT_SOFTKEYS_ENABLED) && !defined(QT_NO_ACTION)
    QAction *action = d->softKeyActions.value(button, 0);
    if (action) {
        d->softKeyActions.remove(button);
        delete action;
    }
#endif
    if (!d->internalRemove)
        button->setParent(0);
}

/*!
    Adds the given \a button to the button box with the specified \a role.
    If the role is invalid, the button is not added.

    If the button has already been added, it is removed and added again with the
    new role.

    \note The button box takes ownership of the button.

    \sa removeButton(), clear()
*/
void QDialogButtonBox::addButton(QAbstractButton *button, ButtonRole role)
{
    Q_D(QDialogButtonBox);
    if (role <= InvalidRole || role >= NRoles) {
        qWarning("QDialogButtonBox::addButton: Invalid ButtonRole, button not added");
        return;
    }
    removeButton(button);
    button->setParent(this);
    d->addButton(button, role);
}

/*!
    Creates a push button with the given \a text, adds it to the button box for the
    specified \a role, and returns the corresponding push button. If \a role is
    invalid, no button is created, and zero is returned.

    \sa removeButton(), clear()
*/
QPushButton *QDialogButtonBox::addButton(const QString &text, ButtonRole role)
{
    Q_D(QDialogButtonBox);
    if (role <= InvalidRole || role >= NRoles) {
        qWarning("QDialogButtonBox::addButton: Invalid ButtonRole, button not added");
        return 0;
    }
    QPushButton *button = new QPushButton(text, this);
    d->addButton(button, role);
    return button;
}

/*!
    Adds a standard \a button to the button box if it is valid to do so, and returns
    a push button. If \a button is invalid, it is not added to the button box, and
    zero is returned.

    \sa removeButton(), clear()
*/
QPushButton *QDialogButtonBox::addButton(StandardButton button)
{
    Q_D(QDialogButtonBox);
    return d->createButton(button);
}

/*!
    \property QDialogButtonBox::standardButtons
    \brief collection of standard buttons in the button box

    This property controls which standard buttons are used by the button box.

    \sa addButton()
*/
void QDialogButtonBox::setStandardButtons(StandardButtons buttons)
{
    Q_D(QDialogButtonBox);
#ifdef QT_SOFTKEYS_ENABLED
    // Delete softkey actions since they have the buttons as parents
    qDeleteAll(d->softKeyActions.values());
    d->softKeyActions.clear();
#endif
    // Clear out all the old standard buttons, then recreate them.
    qDeleteAll(d->standardButtonHash.keys());
    d->standardButtonHash.clear();

    d->createStandardButtons(buttons);
}

QDialogButtonBox::StandardButtons QDialogButtonBox::standardButtons() const
{
    Q_D(const QDialogButtonBox);
    StandardButtons standardButtons = NoButton;
    QHash<QPushButton *, StandardButton>::const_iterator it = d->standardButtonHash.constBegin();
    while (it != d->standardButtonHash.constEnd()) {
        standardButtons |= it.value();
        ++it;
    }
    return standardButtons;
}

/*!
    Returns the QPushButton corresponding to the standard button \a which,
    or 0 if the standard button doesn't exist in this button box.

    \sa standardButton(), standardButtons(), buttons()
*/
QPushButton *QDialogButtonBox::button(StandardButton which) const
{
    Q_D(const QDialogButtonBox);
    return d->standardButtonHash.key(which);
}

/*!
    Returns the standard button enum value corresponding to the given \a button,
    or NoButton if the given \a button isn't a standard button.

    \sa button(), buttons(), standardButtons()
*/
QDialogButtonBox::StandardButton QDialogButtonBox::standardButton(QAbstractButton *button) const
{
    Q_D(const QDialogButtonBox);
    return d->standardButtonHash.value(static_cast<QPushButton *>(button));
}

void QDialogButtonBoxPrivate::_q_handleButtonClicked()
{
    Q_Q(QDialogButtonBox);
    if (QAbstractButton *button = qobject_cast<QAbstractButton *>(q->sender())) {
        emit q->clicked(button);

        switch (q->buttonRole(button)) {
        case AcceptRole:
        case YesRole:
            emit q->accepted();
            break;
        case RejectRole:
        case NoRole:
            emit q->rejected();
            break;
        case HelpRole:
            emit q->helpRequested();
            break;
        default:
            break;
        }
    }
}

void QDialogButtonBoxPrivate::_q_handleButtonDestroyed()
{
    Q_Q(QDialogButtonBox);
    if (QObject *object = q->sender()) {
        QBoolBlocker skippy(internalRemove);
        q->removeButton(static_cast<QAbstractButton *>(object));
    }
}

/*!
    \property QDialogButtonBox::centerButtons
    \brief whether the buttons in the button box are centered

    By default, this property is false. This behavior is appopriate
    for most types of dialogs. A notable exception is message boxes
    on most platforms (e.g. Windows), where the button box is
    centered horizontally.

    \sa QMessageBox
*/
void QDialogButtonBox::setCenterButtons(bool center)
{
    Q_D(QDialogButtonBox);
    if (d->center != center) {
        d->center = center;
        d->resetLayout();
    }
}

bool QDialogButtonBox::centerButtons() const
{
    Q_D(const QDialogButtonBox);
    return d->center;
}

/*!
    \reimp
*/
void QDialogButtonBox::changeEvent(QEvent *event)
{
    typedef QHash<QPushButton *, QDialogButtonBox::StandardButton> StandardButtonHash;

    Q_D(QDialogButtonBox);
    switch (event->type()) {
    case QEvent::StyleChange:  // Propagate style
        if (!d->standardButtonHash.empty()) {
            QStyle *newStyle = style();
            const StandardButtonHash::iterator end = d->standardButtonHash.end();
            for (StandardButtonHash::iterator it = d->standardButtonHash.begin(); it != end; ++it)
                it.key()->setStyle(newStyle);
        }
        // fallthrough intended
#ifdef Q_WS_MAC
    case QEvent::MacSizeChange:
#endif
        d->resetLayout();
        QWidget::changeEvent(event);
        break;
    default:
        QWidget::changeEvent(event);
        break;
    }
}

/*!
    \reimp
*/
bool QDialogButtonBox::event(QEvent *event)
{
    Q_D(QDialogButtonBox);
    if (event->type() == QEvent::Show) {
        QList<QAbstractButton *> acceptRoleList = d->buttonLists[AcceptRole];
        QPushButton *firstAcceptButton = acceptRoleList.isEmpty() ? 0 : qobject_cast<QPushButton *>(acceptRoleList.at(0));
        bool hasDefault = false;
        QWidget *dialog = 0;
        QWidget *p = this;
        while (p && !p->isWindow()) {
            p = p->parentWidget();
            if ((dialog = qobject_cast<QDialog *>(p)))
                break;
        }

        foreach (QPushButton *pb, (dialog ? dialog : this)->findChildren<QPushButton *>()) {
            if (pb->isDefault() && pb != firstAcceptButton) {
                hasDefault = true;
                break;
            }
        }
        if (!hasDefault && firstAcceptButton)
            firstAcceptButton->setDefault(true);
#ifdef QT_SOFTKEYS_ENABLED
        if (dialog)
            setFixedSize(0,0);
#endif
    }else if (event->type() == QEvent::LanguageChange) {
        d->retranslateStrings();
    }
#if defined(QT_SOFTKEYS_ENABLED) && !defined(QT_NO_ACTION)
    else if (event->type() == QEvent::ParentChange) {
        QWidget *dialog = 0;
        QWidget *p = this;
        while (p && !p->isWindow()) {
            p = p->parentWidget();
            if ((dialog = qobject_cast<QDialog *>(p)))
                break;
        }

        // If the parent changes, then move the softkeys
        for (QHash<QAbstractButton *, QAction *>::const_iterator it = d->softKeyActions.constBegin();
            it != d->softKeyActions.constEnd(); ++it) {
            QAction *current = it.value();
            QList<QWidget *> widgets = current->associatedWidgets();
            foreach (QWidget *w, widgets)
                w->removeAction(current);
            if (dialog)
                dialog->addAction(current);
            else
                addAction(current);
        }
    }
#endif

    return QWidget::event(event);
}

QT_END_NAMESPACE

#include "moc_qdialogbuttonbox.cpp"
