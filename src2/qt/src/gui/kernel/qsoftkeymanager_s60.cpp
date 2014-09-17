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

#include "qapplication.h"
#include "qevent.h"
#include "qbitmap.h"
#include "qstyle.h"
#include "qmenubar.h"
#include "private/qt_s60_p.h"
#include "private/qmenu_p.h"
#include "private/qaction_p.h"
#include "private/qsoftkeymanager_p.h"
#include "private/qsoftkeymanager_s60_p.h"
#include "private/qobject_p.h"
#include <eiksoftkeyimage.h>
#include <eikcmbut.h>

#ifndef QT_NO_STYLE_S60
#include <qs60style.h>
#endif

#ifndef QT_NO_SOFTKEYMANAGER
QT_BEGIN_NAMESPACE

const int S60_COMMAND_START = 6000;
const int LSK_POSITION = 0;
const int MSK_POSITION = 3;
const int RSK_POSITION = 2;

QSoftKeyManagerPrivateS60::QSoftKeyManagerPrivateS60() : cbaHasImage(4) // 4 since MSK position index is 3
{
    cachedCbaIconSize[0] = QSize(0,0);
    cachedCbaIconSize[1] = QSize(0,0);
    cachedCbaIconSize[2] = QSize(0,0);
    cachedCbaIconSize[3] = QSize(0,0);
}

bool QSoftKeyManagerPrivateS60::skipCbaUpdate()
{
    // Lets not update softkeys if
    // 1. We don't have application panes, i.e. cba
    // 2. Our CBA is not active, i.e. S60 native dialog or menu with custom CBA is shown
    //    2.1. Except if thre is no current CBA at all and WindowSoftkeysRespondHint is set

    // Note: Cannot use IsDisplayingMenuOrDialog since CBA update can be triggered before
    // menu/dialog CBA is actually displayed i.e. it is being costructed.
    CEikButtonGroupContainer *appUiCba = S60->buttonGroupContainer();
    if (!appUiCba)
        return true;
    // CEikButtonGroupContainer::Current returns 0 if CBA is not visible at all
    CEikButtonGroupContainer *currentCba = CEikButtonGroupContainer::Current();
    // Check if softkey need to be update even they are not visible
    bool cbaRespondsWhenInvisible = false;
    QWidget *window = QApplication::activeWindow();
    if (window && (window->windowFlags() & Qt::WindowSoftkeysRespondHint))
        cbaRespondsWhenInvisible = true;

    if (QApplication::testAttribute(Qt::AA_S60DontConstructApplicationPanes)
            || (appUiCba != currentCba && !cbaRespondsWhenInvisible)) {
        return true;
    }
    return false;
}

void QSoftKeyManagerPrivateS60::ensureCbaVisibilityAndResponsiviness(CEikButtonGroupContainer &cba)
{
    RDrawableWindow *cbaWindow = cba.DrawableWindow();
    Q_ASSERT_X(cbaWindow, Q_FUNC_INFO, "Native CBA does not have window!");
    // CBA comes on top of new option menu
    int pos = 0;
   
    if(cba.ButtonGroupType()== SLafButtonGroupContainer::ECba)
        pos = 1;
    
    cbaWindow->SetOrdinalPosition(pos);
    // Qt shares same CBA instance between top-level widgets,
    // make sure we are not faded by underlying window.
    cbaWindow->SetFaded(EFalse, RWindowTreeNode::EFadeIncludeChildren);
    // Modal dialogs capture pointer events, but shared cba instance
    // shall stay responsive. Raise pointer capture priority to keep
    // softkeys responsive in modal dialogs
    cbaWindow->SetPointerCapturePriority(1);
}

void QSoftKeyManagerPrivateS60::clearSoftkeys(CEikButtonGroupContainer &cba)
{
#if defined(Q_WS_S60) && !defined(SYMBIAN_VERSION_9_4) && !defined(SYMBIAN_VERSION_9_3) && !defined(SYMBIAN_VERSION_9_2)
    QT_TRAP_THROWING(
        //EAknSoftkeyEmpty is used, because using -1 adds softkeys without actions on Symbian3
        cba.SetCommandL(0, EAknSoftkeyEmpty, KNullDesC);
        cba.SetCommandL(2, EAknSoftkeyEmpty, KNullDesC);
    );
#else
    QT_TRAP_THROWING(
        //Using -1 instead of EAknSoftkeyEmpty to avoid flickering.
        cba.SetCommandL(0, -1, KNullDesC);
        // TODO: Should we clear also middle SK?
        cba.SetCommandL(2, -1, KNullDesC);
    );
#endif
    realSoftKeyActions.clear();
}

QString QSoftKeyManagerPrivateS60::softkeyText(QAction &softkeyAction)
{
    // In S60 softkeys and menu items do not support key accelerators (i.e.
    // CTRL+X). Therefore, removing the accelerator characters from both softkey
    // and menu item texts.
    const int underlineShortCut = QApplication::style()->styleHint(QStyle::SH_UnderlineShortcut);
    QString iconText = softkeyAction.iconText();
    return underlineShortCut ? softkeyAction.text() : iconText;
}

QAction *QSoftKeyManagerPrivateS60::highestPrioritySoftkey(QAction::SoftKeyRole role)
{
    QAction *ret = NULL;
    // Priority look up is two level
    // 1. First widget with softkeys always has highest priority
    for (int level = 0; !ret; level++) {
        // 2. Highest priority action within widget
        QList<QAction*> actions = requestedSoftKeyActions.values(level);
        if (actions.isEmpty())
            break;
        qSort(actions.begin(), actions.end(), QSoftKeyManagerPrivateS60::actionPriorityMoreThan);
        foreach (QAction *action, actions) {
            if (action->softKeyRole() == role) {
                ret = action;
                break;
            }
        }
    }
    return ret;
}

bool QSoftKeyManagerPrivateS60::actionPriorityMoreThan(const QAction *firstItem, const QAction *secondItem)
{
    return firstItem->priority() > secondItem->priority();
}

void QSoftKeyManagerPrivateS60::setNativeSoftkey(CEikButtonGroupContainer &cba,
                                              TInt position, TInt command, const TDesC &text)
{
    // Calling SetCommandL causes CBA redraw
    QT_TRAP_THROWING(cba.SetCommandL(position, command, text));
}

QPoint QSoftKeyManagerPrivateS60::softkeyIconPosition(int position, QSize sourceSize, QSize targetSize)
{
    QPoint iconPosition(0,0);

    // Prior to S60 5.3 icons need to be properly positioned to buttons, but starting with 5.3
    // positioning is done on Avkon side.
    if (QSysInfo::s60Version() < QSysInfo::SV_S60_5_3) {
        switch (AknLayoutUtils::CbaLocation())
            {
            case AknLayoutUtils::EAknCbaLocationBottom:
                // RSK must be moved to right, LSK in on correct position by default
                if (position == RSK_POSITION)
                    iconPosition.setX(targetSize.width() - sourceSize.width());
                break;
            case AknLayoutUtils::EAknCbaLocationRight:
            case AknLayoutUtils::EAknCbaLocationLeft:
                // Already in correct position
            default:
                break;
            }

        // Align horizontally to center
        iconPosition.setY((targetSize.height() - sourceSize.height()) >> 1);
    }
    return iconPosition;
}

QPixmap QSoftKeyManagerPrivateS60::prepareSoftkeyPixmap(QPixmap src, int position, QSize targetSize)
{
    QPixmap target(targetSize);
    target.fill(Qt::transparent);
    QPainter p;
    p.begin(&target);
    p.drawPixmap(softkeyIconPosition(position, src.size(), targetSize), src);
    p.end();
    return target;
}

bool QSoftKeyManagerPrivateS60::isOrientationLandscape()
{
    // Hard to believe that there is no public API in S60 to
    // get current orientation. This workaround works with currently supported resolutions
    return  S60->screenHeightInPixels <  S60->screenWidthInPixels;
}

QSize QSoftKeyManagerPrivateS60::cbaIconSize(CEikButtonGroupContainer *cba, int position)
{
    int index = position;
    index += isOrientationLandscape() ? 0 : 1;
    if(cachedCbaIconSize[index].isNull()) {
        if (QSysInfo::s60Version() >= QSysInfo::SV_S60_5_3) {
            // S60 5.3 and later have fixed icon size on softkeys, while the button
            // itself is bigger, so the automatic check doesn't work.
            // Use custom pixel metrics to deduce the CBA icon size
            int iconHeight = 30;
            int iconWidth = 30;
#ifndef QT_NO_STYLE_S60
            QS60Style *s60Style = 0;
            s60Style = qobject_cast<QS60Style *>(QApplication::style());
            if (s60Style) {
                iconWidth = s60Style->pixelMetric((QStyle::PixelMetric)PM_CbaIconWidth);
                iconHeight = s60Style->pixelMetric((QStyle::PixelMetric)PM_CbaIconHeight);
            }
#endif
            cachedCbaIconSize[index] = QSize(iconWidth, iconHeight);
        } else {
            // Only way I figured out to get CBA icon size without RnD SDK, was
            // to set some dummy icon to CBA first and then ask CBA button CCoeControl::Size()
            // The returned value is cached to avoid unnecessary icon setting every time.
            const bool left = (position == LSK_POSITION);
            if (position == LSK_POSITION || position == RSK_POSITION) {
                CEikImage* tmpImage = NULL;
                QT_TRAP_THROWING(tmpImage = new (ELeave) CEikImage);
                EikSoftkeyImage::SetImage(cba, *tmpImage, left); // Takes tmpImage ownership
                int command = S60_COMMAND_START + position;
                setNativeSoftkey(*cba, position, command, KNullDesC());
                cachedCbaIconSize[index] = qt_TSize2QSize(cba->ControlOrNull(command)->Size());
                EikSoftkeyImage::SetLabel(cba, left);

                if (cachedCbaIconSize[index] == QSize(138,72)) {
                    // Hack for S60 5.0 landscape orientation, which return wrong icon size
                    cachedCbaIconSize[index] = QSize(60,60);
                }
            }
        }
    }

    return cachedCbaIconSize[index];
}

bool QSoftKeyManagerPrivateS60::setSoftkeyImage(CEikButtonGroupContainer *cba,
                                            QAction &action, int position)
{
    bool ret = false;

    const bool left = (position == LSK_POSITION);
    if(position == LSK_POSITION || position == RSK_POSITION) {
        QIcon icon = action.icon();
        if (!icon.isNull()) {
            // Get size of CBA icon area based on button position and orientation
            QSize requiredIconSize = cbaIconSize(cba, position);
            // Get pixmap out of icon based on preferred size, the aspect ratio is kept
            QPixmap pmWihtAspectRatio = icon.pixmap(requiredIconSize);
            // Native softkeys require that pixmap size is exactly the same as requiredIconSize
            // prepareSoftkeyPixmap creates a new pixmap with requiredIconSize and blits the 'pmWihtAspectRatio'
            // to correct location of it
            QPixmap softkeyPixmap = prepareSoftkeyPixmap(pmWihtAspectRatio, position, requiredIconSize);

            QPixmap softkeyAlpha = softkeyPixmap.alphaChannel();
            // Alpha channel in 5.1 and older devices need to be inverted
            // TODO: Switch to use toSymbianCFbsBitmap with invert when available
            if(QSysInfo::s60Version() <= QSysInfo::SV_S60_5_1) {
                QImage alphaImage = softkeyAlpha.toImage();
                alphaImage.invertPixels();
                softkeyAlpha = QPixmap::fromImage(alphaImage);
            }

            CFbsBitmap* nBitmap = softkeyPixmap.toSymbianCFbsBitmap();
            CFbsBitmap* nMask = softkeyAlpha.toSymbianCFbsBitmap();

            CEikImage* myimage = new (ELeave) CEikImage;
            myimage->SetPicture( nBitmap, nMask ); // nBitmap and nMask ownership transferred

            EikSoftkeyImage::SetImage(cba, *myimage, left); // Takes myimage ownership
            cbaHasImage[position] = true;
            ret = true;
        }
    }
    return ret;
}

bool QSoftKeyManagerPrivateS60::setSoftkey(CEikButtonGroupContainer &cba,
                                        QAction::SoftKeyRole role, int position)
{
    QAction *action = highestPrioritySoftkey(role);
    if (action) {
        bool hasImage = setSoftkeyImage(&cba, *action, position);
        QString text = softkeyText(*action);
        TPtrC nativeText = qt_QString2TPtrC(text);
        int command = S60_COMMAND_START + position;
#if defined(Q_WS_S60) && !defined(SYMBIAN_VERSION_9_4) && !defined(SYMBIAN_VERSION_9_3) && !defined(SYMBIAN_VERSION_9_2)
        if (softKeyCommandActions.contains(action))
            command = softKeyCommandActions.value(action);
#endif
        setNativeSoftkey(cba, position, command, nativeText);
        if (!hasImage && cbaHasImage[position]) {
            EikSoftkeyImage::SetLabel(&cba, (position == LSK_POSITION));
            cbaHasImage[position] = false;
        }

        const bool dimmed = !action->isEnabled() && !QSoftKeyManager::isForceEnabledInSofkeys(action);
        cba.DimCommand(command, dimmed);
        realSoftKeyActions.insert(command, action);
        return true;
    }
    return false;
}

bool QSoftKeyManagerPrivateS60::setLeftSoftkey(CEikButtonGroupContainer &cba)
{
    if (!setSoftkey(cba, QAction::PositiveSoftKey, LSK_POSITION)) {
        if (cbaHasImage[LSK_POSITION]) {
            // Clear any residual icon if LSK has no action. A real softkey
            // is needed for SetLabel command to work, so do a temporary dummy
            setNativeSoftkey(cba, LSK_POSITION, EAknSoftkeyExit, KNullDesC);
            EikSoftkeyImage::SetLabel(&cba, true);
            setNativeSoftkey(cba, LSK_POSITION, EAknSoftkeyEmpty, KNullDesC);
            cbaHasImage[LSK_POSITION] = false;
        }
        return false;
    }
    return true;
}

bool QSoftKeyManagerPrivateS60::setMiddleSoftkey(CEikButtonGroupContainer &cba)
{
    // Note: In order to get MSK working, application has to have EAknEnableMSK flag set
    // Currently it is not possible very easily)
    // For more information see: http://wiki.forum.nokia.com/index.php/Middle_softkey_usage
    return setSoftkey(cba, QAction::SelectSoftKey, MSK_POSITION);
}

bool QSoftKeyManagerPrivateS60::setRightSoftkey(CEikButtonGroupContainer &cba)
{
    if (!setSoftkey(cba, QAction::NegativeSoftKey, RSK_POSITION)) {
        const Qt::WindowType windowType = initialSoftKeySource
            ? initialSoftKeySource->window()->windowType() : Qt::Window;
        if (windowType != Qt::Dialog && windowType != Qt::Popup) {
            QString text(QSoftKeyManager::tr("Exit"));
            TPtrC nativeText = qt_QString2TPtrC(text);
            setNativeSoftkey(cba, RSK_POSITION, EAknSoftkeyExit, nativeText);
            if (cbaHasImage[RSK_POSITION]) {
                EikSoftkeyImage::SetLabel(&cba, false);
                cbaHasImage[RSK_POSITION] = false;
            }
            cba.DimCommand(EAknSoftkeyExit, false);
            return true;
        } else {
            if (cbaHasImage[RSK_POSITION]) {
                // Clear any residual icon if RSK has no action. A real softkey
                // is needed for SetLabel command to work, so do a temporary dummy
                setNativeSoftkey(cba, RSK_POSITION, EAknSoftkeyExit, KNullDesC);
                EikSoftkeyImage::SetLabel(&cba, false);
                setNativeSoftkey(cba, RSK_POSITION, EAknSoftkeyEmpty, KNullDesC);
                cbaHasImage[RSK_POSITION] = false;
            }
            return false;
        }
    }
    return true;
}

void QSoftKeyManagerPrivateS60::setSoftkeys(CEikButtonGroupContainer &cba)
{
    int requestedSoftkeyCount = requestedSoftKeyActions.count();
    const int maxSoftkeyCount = 2; // TODO: differs based on orientation ans S60 versions (some have MSK)
    if (requestedSoftkeyCount > maxSoftkeyCount) {
        // We have more softkeys than available slots
        // Put highest priority negative action to RSK and Options menu with rest of softkey actions to LSK
        // TODO: Build menu
        setLeftSoftkey(cba);
        if(AknLayoutUtils::MSKEnabled())
            setMiddleSoftkey(cba);
        setRightSoftkey(cba);
    } else {
        // We have less softkeys than available slots
        // Put softkeys to request slots based on role
        setLeftSoftkey(cba);
        if(AknLayoutUtils::MSKEnabled())
            setMiddleSoftkey(cba);
        setRightSoftkey(cba);
    }
}

void QSoftKeyManagerPrivateS60::updateSoftKeys_sys()
{
    if (skipCbaUpdate())
        return;

    CEikButtonGroupContainer *nativeContainer = S60->buttonGroupContainer();
    Q_ASSERT_X(nativeContainer, Q_FUNC_INFO, "Native CBA does not exist!");
    ensureCbaVisibilityAndResponsiviness(*nativeContainer);
    clearSoftkeys(*nativeContainer);
    setSoftkeys(*nativeContainer);

    nativeContainer->DrawDeferred(); // 3.1 needs an extra invitation
}

static void resetMenuBeingConstructed(TAny* /*aAny*/)
{
    S60->menuBeingConstructed = false;
}

void QSoftKeyManagerPrivateS60::tryDisplayMenuBarL()
{
    CleanupStack::PushL(TCleanupItem(resetMenuBeingConstructed, NULL));
    S60->menuBeingConstructed = true;
    S60->menuBar()->TryDisplayMenuBarL();
    CleanupStack::PopAndDestroy(); // Reset menuBeingConstructed to false in all cases
}

bool QSoftKeyManagerPrivateS60::handleCommand(int command)
{
    QAction *action = realSoftKeyActions.value(command);
    if (action) {
        bool property = QActionPrivate::get(action)->menuActionSoftkeys;
        if (property) {
            QT_TRAP_THROWING(tryDisplayMenuBarL());
        } else if (action->menu()) {
            // TODO: This is hack, in order to use exising QMenuBar implementation for Symbian
            // menubar needs to have widget to which it is associated. Since we want to associate
            // menubar to action (which is inherited from QObject), we create and associate QWidget
            // to action and pass that for QMenuBar. This associates the menubar to action, and we
            // can have own menubar for each action.
            QWidget *actionContainer = action->property("_q_action_widget").value<QWidget*>();
            if(!actionContainer) {
                actionContainer = new QWidget(action->parentWidget());
                QMenuBar *menuBar = new QMenuBar(actionContainer);
                foreach(QAction *menuAction, action->menu()->actions()) {
                    QMenu *menu = menuAction->menu();
                    if(menu)
                        menuBar->addMenu(menu);
                    else
                        menuBar->addAction(menuAction);
                }
                QVariant v;
                v.setValue(actionContainer);
                action->setProperty("_q_action_widget", v);
            }
            qt_symbian_next_menu_from_action(actionContainer);
            QT_TRAP_THROWING(tryDisplayMenuBarL());
        }

        Q_ASSERT(action->softKeyRole() != QAction::NoSoftKey);
        QWidget *actionParent = action->parentWidget();
        Q_ASSERT_X(actionParent, Q_FUNC_INFO, "No parent set for softkey action!");
        if (actionParent->isEnabled()) {
            action->activate(QAction::Trigger);
            return true;
        }
    }
    return false;
}

QT_END_NAMESPACE
#endif //QT_NO_SOFTKEYMANAGER
