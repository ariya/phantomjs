/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include <private/qcursor_p.h>
#include <private/qwidget_p.h>
#include <private/qapplication_p.h>
#include <coecntrl.h>
#include <qcursor.h>
#include <private/qt_s60_p.h>
#include <qbitmap.h>
#include <w32std.h>
#include <qapplication.h>
#include <qwidget.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_CURSOR
static QCursor cursorSprite;
static int cursorSpriteVisible;
#endif

//pos and setpos are required whether cursors are configured or not.
QPoint QCursor::pos()
{
    return S60->lastCursorPos;
}

void QCursor::setPos(int x, int y)
{
    //clip to screen size (window server allows a sprite hotspot to be outside the screen)
    if (x < 0)
        x=0;
    else if (x >= S60->screenWidthInPixels)
        x = S60->screenWidthInPixels - 1;
    if (y < 0)
        y = 0;
    else if (y >= S60->screenHeightInPixels)
        y = S60->screenHeightInPixels - 1;

#ifndef QT_NO_CURSOR
#ifndef Q_SYMBIAN_FIXED_POINTER_CURSORS
    if (S60->brokenPointerCursors && cursorSpriteVisible)
        cursorSprite.d->scurs.SetPosition(TPoint(x,y));
    else
#endif
        S60->wsSession().SetPointerCursorPosition(TPoint(x, y));
#endif
    S60->lastCursorPos = QPoint(x, y);
    //send a fake mouse move event, so that enter/leave events go to the widget hierarchy
    QWidget *w = QApplication::topLevelAt(S60->lastCursorPos);
    if (w) {
        CCoeControl* ctrl = w->effectiveWinId();
        TPoint epos(x, y);
        TPoint cpos = epos - ctrl->PositionRelativeToScreen();
        TPointerEvent fakeEvent;
        fakeEvent.iType = TPointerEvent::EMove;
        fakeEvent.iModifiers = 0U;
        fakeEvent.iPosition = cpos;
        fakeEvent.iParentPosition = epos;
        ctrl->HandlePointerEventL(fakeEvent);
    }
}

#ifndef QT_NO_CURSOR
/*
 * Request cursor to be turned on or off.
 * Reference counted, so 2 on + 1 off = on, for example
 */
void qt_symbian_set_cursor_visible(bool visible) {
    if (visible)
        cursorSpriteVisible++;
    else
        cursorSpriteVisible--;
    Q_ASSERT(cursorSpriteVisible >=0);

    if (cursorSpriteVisible && !S60->mouseInteractionEnabled) {
#ifndef Q_SYMBIAN_FIXED_POINTER_CURSORS
        if (S60->brokenPointerCursors)
            qt_symbian_show_pointer_sprite();
        else
#endif
            S60->wsSession().SetPointerCursorMode(EPointerCursorNormal);
    } else if (!cursorSpriteVisible && S60->mouseInteractionEnabled) {
#ifndef Q_SYMBIAN_FIXED_POINTER_CURSORS
        if (S60->brokenPointerCursors)
            qt_symbian_hide_pointer_sprite();
        else
#endif
            S60->wsSession().SetPointerCursorMode(EPointerCursorNone);
    }
    S60->mouseInteractionEnabled = ((cursorSpriteVisible > 0) ? true : false);
}

/*
 * Check if the cursor is on or off
 */
bool qt_symbian_is_cursor_visible() {
    return S60->mouseInteractionEnabled;
}

QCursorData::QCursorData(Qt::CursorShape s) :
    cshape(s), bm(0), bmm(0), hx(0), hy(0), pcurs()
{
    ref = 1;
}

QCursorData::~QCursorData()
{
    for(int i=0;i<nativeSpriteMembers.Count();i++) {
        delete nativeSpriteMembers[i]->iBitmap;
        delete nativeSpriteMembers[i]->iMaskBitmap;
    }
    nativeSpriteMembers.ResetAndDestroy();
    pcurs.Close();
    delete bm;
    delete bmm;
}

/* Create a bitmap cursor, this is called by public constructors in the
 * generic QCursor code.
 */
QCursorData *QCursorData::setBitmap(const QBitmap &bitmap, const QBitmap &mask, int hotX, int hotY)
{
    if (!QCursorData::initialized)
        QCursorData::initialize();
    if (bitmap.depth() != 1 || mask.depth() != 1 || bitmap.size() != mask.size()) {
        qWarning("QCursor: Cannot create bitmap cursor; invalid bitmap(s)");
        QCursorData *c = qt_cursorTable[0];
        c->ref.ref();
        return c;
    }
    QCursorData *d = new QCursorData;
    d->bm = new QBitmap(bitmap);
    d->bmm = new QBitmap(mask);
    d->cshape = Qt::BitmapCursor;
    d->hx = hotX >= 0 ? hotX : bitmap.width() / 2;
    d->hy = hotY >= 0 ? hotY : bitmap.height() / 2;
    return d;
}

/*
 * returns an opaque native handle to a cursor.
 * It happens to be the address of the native handle, as window server handles
 * are not POD types. Note there is no QCursor(HANDLE) constructor on Symbian,
 * Mac or QWS.
 */
Qt::HANDLE QCursor::handle() const
{
    if (d->pcurs.WsHandle())
        return reinterpret_cast<Qt::HANDLE> (&(d->pcurs));

#ifdef Q_SYMBIAN_HAS_SYSTEM_CURSORS
    // don't construct shape cursors, QApplication_s60 will use the system cursor instead
    if (!(d->bm))
        return 0;
#endif

    d->pcurs = RWsPointerCursor(S60->wsSession());
    d->pcurs.Construct(0);
    d->constructCursorSprite(d->pcurs);
    d->pcurs.Activate();

    return reinterpret_cast<Qt::HANDLE> (&(d->pcurs));
}

#ifndef Q_SYMBIAN_HAS_SYSTEM_CURSORS
/*
 * Loads a single cursor shape from resources and appends it to a native sprite.
 * Animated cursors (e.g. the busy cursor) have multiple members.
 */
void QCursorData::loadShapeFromResource(RWsSpriteBase& target, QString resource, int hx, int hy, int interval)
{
    QPixmap pix;
    CFbsBitmap* native;
    QScopedPointer<TSpriteMember> member(new TSpriteMember);
    member->iInterval = interval;
    member->iInvertMask = false;
    member->iMaskBitmap = 0; // all shapes are RGBA
    member->iDrawMode = CGraphicsContext::EDrawModePEN;
    member->iOffset = TPoint(-hx, -hy);
    QString res(QLatin1String(":/trolltech/symbian/cursors/images/%1.png"));
    pix.load(res.arg(resource));
    native = pix.toSymbianCFbsBitmap();
    member->iBitmap = native;
    qt_symbian_throwIfError(nativeSpriteMembers.Append(member.data()));
    target.AppendMember(*(member.take()));
}

//TODO: after 4.6, connect with style & skins?
/*
 * Constructs the native cursor from resources compiled into QtGui
 * This is needed only when the platform doesn't have system cursors.
 *
 * System cursors are higher performance, since they are constructed once
 * and shared by all applications by specifying the shape number.
 * Due to symbian platform security considerations, and the fact most
 * existing phones have a broken RWsPointerCursor, system cursors are not
 * being used.
 */
void QCursorData::constructShapeSprite(RWsSpriteBase& target)
{
    int i;
    switch (cshape) {
    default:
        qWarning("QCursorData::constructShapeSprite unknown shape %d", cshape);
        //fall through and give arrow cursor
    case Qt::ArrowCursor:
        loadShapeFromResource(target, QLatin1String("pointer"), 1, 1);
        break;
    case Qt::UpArrowCursor:
        loadShapeFromResource(target, QLatin1String("uparrow"), 4, 0);
        break;
    case Qt::CrossCursor:
        loadShapeFromResource(target, QLatin1String("cross"), 7, 7);
        break;
    case Qt::WaitCursor:
        for (i = 1; i <= 12; i++) {
            loadShapeFromResource(target, QString(QLatin1String("wait%1")).arg(i), 7, 7, 1000000);
        }
        break;
    case Qt::IBeamCursor:
        loadShapeFromResource(target, QLatin1String("ibeam"), 3, 10);
        break;
    case Qt::SizeVerCursor:
        loadShapeFromResource(target, QLatin1String("sizever"), 4, 8);
        break;
    case Qt::SizeHorCursor:
        loadShapeFromResource(target, QLatin1String("sizehor"), 8, 4);
        break;
    case Qt::SizeBDiagCursor:
        loadShapeFromResource(target, QLatin1String("sizebdiag"), 8, 8);
        break;
    case Qt::SizeFDiagCursor:
        loadShapeFromResource(target, QLatin1String("sizefdiag"), 8, 8);
        break;
    case Qt::SizeAllCursor:
        loadShapeFromResource(target, QLatin1String("sizeall"), 7, 7);
        break;
    case Qt::BlankCursor:
        loadShapeFromResource(target, QLatin1String("blank"), 0, 0);
        break;
    case Qt::SplitVCursor:
        loadShapeFromResource(target, QLatin1String("splitv"), 7, 7);
        break;
    case Qt::SplitHCursor:
        loadShapeFromResource(target, QLatin1String("splith"), 7, 7);
        break;
    case Qt::PointingHandCursor:
        loadShapeFromResource(target, QLatin1String("handpoint"), 5, 0);
        break;
    case Qt::ForbiddenCursor:
        loadShapeFromResource(target, QLatin1String("forbidden"), 7, 7);
        break;
    case Qt::WhatsThisCursor:
        loadShapeFromResource(target, QLatin1String("whatsthis"), 1, 1);
        break;
    case Qt::BusyCursor:
        loadShapeFromResource(target, QLatin1String("busy3"), 1, 1, 1000000);
        loadShapeFromResource(target, QLatin1String("busy6"), 1, 1, 1000000);
        loadShapeFromResource(target, QLatin1String("busy9"), 1, 1, 1000000);
        loadShapeFromResource(target, QLatin1String("busy12"), 1, 1, 1000000);
        break;
    case Qt::OpenHandCursor:
        loadShapeFromResource(target, QLatin1String("openhand"), 7, 7);
        break;
    case Qt::ClosedHandCursor:
        loadShapeFromResource(target, QLatin1String("closehand"), 7, 7);
        break;
    }
}
#endif

/*
 * Common code between the sprite workaround and standard modes of operation.
 * RWsSpriteBase is the base class for both RWsSprite and RWsPointerCursor.
 * It is called from both handle() and qt_s60_show_pointer_sprite()
 */
void QCursorData::constructCursorSprite(RWsSpriteBase& target)
{
    int count = nativeSpriteMembers.Count();
    if (count) {
        // already constructed
        for (int i = 0; i < count; i++)
            target.AppendMember(*(nativeSpriteMembers[i]));

        return;
    }
    if (pixmap.isNull() && !bm) {
#ifndef Q_SYMBIAN_HAS_SYSTEM_CURSORS
        //shape cursor
        constructShapeSprite(target);
#endif
        return;
    }
    QScopedPointer<TSpriteMember> member(new TSpriteMember);
    if (pixmap.isNull()) {
        //construct mono cursor
        member->iBitmap = bm->toSymbianCFbsBitmap();
        member->iMaskBitmap = bmm->toSymbianCFbsBitmap();
    }
    else {
        //construct normal cursor
        member->iBitmap = pixmap.toSymbianCFbsBitmap();
        if (pixmap.hasAlphaChannel()) {
            member->iMaskBitmap = 0; //use alpha blending
        }
        else if (pixmap.hasAlpha()) {
            member->iMaskBitmap = pixmap.mask().toSymbianCFbsBitmap();
        }
        else {
            member->iMaskBitmap = 0; //opaque rectangle cursor (due to EDrawModePEN)
        }
    }

    member->iDrawMode = CGraphicsContext::EDrawModePEN;
    member->iInvertMask = EFalse;
    member->iInterval = 0;
    member->iOffset = TPoint(-(hx), -(hy)); //Symbian hotspot coordinates are negative
    qt_symbian_throwIfError(nativeSpriteMembers.Append(member.data()));
    target.AppendMember(*(member.take()));
}

/*
 * shows the pointer sprite by constructing a native handle, and registering
 * it with the window server.
 * Only used when the sprite workaround is in use.
 */
void qt_symbian_show_pointer_sprite()
{
    if (cursorSprite.d) {
        if (cursorSprite.d->scurs.WsHandle())
            cursorSprite.d->scurs.Close();
    } else {
        cursorSprite = QCursor(Qt::ArrowCursor);
    }

    cursorSprite.d->scurs = RWsSprite(S60->wsSession());
    QPoint pos = QCursor::pos();
    cursorSprite.d->scurs.Construct(S60->windowGroup(), TPoint(pos.x(), pos.y()), ESpriteNoChildClip | ESpriteNoShadows);

    cursorSprite.d->constructCursorSprite(cursorSprite.d->scurs);
    cursorSprite.d->scurs.Activate();
}

/*
 * hides the pointer sprite by closing the native handle.
 * Only used when the sprite workaround is in use.
 */
void qt_symbian_hide_pointer_sprite()
{
    if (cursorSprite.d) {
        cursorSprite.d->scurs.Close();
    }
}

/*
 * Changes the cursor sprite to the cursor specified.
 * Only used when the sprite workaround is in use.
 */
void qt_symbian_set_pointer_sprite(const QCursor& cursor)
{
    if (S60->mouseInteractionEnabled)
        qt_symbian_hide_pointer_sprite();
    cursorSprite = cursor;
    if (S60->mouseInteractionEnabled)
        qt_symbian_show_pointer_sprite();
}

/*
 * When using sprites as a workaround on phones that have a broken
 * RWsPointerCursor, this function is called in response to pointer events
 * and when QCursor::setPos() is called.
 * Performance is worse than a real pointer cursor, due to extra context
 * switches vs. the window server moving the cursor by itself.
 */
void qt_symbian_move_cursor_sprite()
{
    if (S60->mouseInteractionEnabled) {
        cursorSprite.d->scurs.SetPosition(TPoint(S60->lastCursorPos.x(), S60->lastCursorPos.y()));
    }
}

/*
 * Translate from Qt::CursorShape to OS system pointer cursor list index.
 * Currently we control the implementation of the system pointer cursor list,
 * so this function is trivial. That may not always be the case.
 */
TInt qt_symbian_translate_cursor_shape(Qt::CursorShape shape)
{
    return (TInt) shape;
}

/*
  Internal function called from QWidget::setCursor()
   force is true if this function is called from dispatchEnterLeave, it means that the
   mouse is actually directly under this widget.
*/
void qt_symbian_set_cursor(QWidget *w, bool force)
{
    static QPointer<QWidget> lastUnderMouse = 0;
    if (force) {
        lastUnderMouse = w;
    }
    else if (w->testAttribute(Qt::WA_WState_Created) && lastUnderMouse
        && lastUnderMouse->effectiveWinId() == w->effectiveWinId()) {
        w = lastUnderMouse;
    }

    if (!S60->curWin && w && w->internalWinId())
        return;
    QWidget* cW = w && !w->internalWinId() ? w : QWidget::find(S60->curWin);
    if (!cW || cW->window() != w->window() || !cW->isVisible() || !cW->underMouse()
        || QApplication::overrideCursor())
        return;

#ifndef Q_SYMBIAN_FIXED_POINTER_CURSORS
    if (S60->brokenPointerCursors)
        qt_symbian_set_pointer_sprite(cW->cursor());
    else
#endif
        qt_symbian_setWindowCursor(cW->cursor(), w->effectiveWinId());
}

/*
 * Makes the specified cursor appear above a specific native window group
 * Called from QSymbianControl and QApplication::restoreOverrideCursor
 *
 * Window server is needed for this, so there is no equivalent when using
 * the sprite workaround.
 */
void qt_symbian_setWindowGroupCursor(const QCursor &cursor, RWindowTreeNode &node)
{
    Qt::HANDLE handle = cursor.handle();
    if (handle) {
        RWsPointerCursor *pcurs = reinterpret_cast<RWsPointerCursor *> (handle);
        node.SetCustomPointerCursor(*pcurs);
    } else
#ifdef Q_SYMBIAN_HAS_SYSTEM_CURSORS
    {
        TInt shape = qt_symbian_translate_cursor_shape(cursor.shape());
        node.SetPointerCursor(shape);
    }
#else
    qWarning("qt_s60_setWindowGroupCursor - null handle");
#endif
}

/*
 * Makes the specified cursor appear above a specific native window
 * Called from QSymbianControl and QApplication::restoreOverrideCursor
 *
 * Window server is needed for this, so there is no equivalent when using
 * the sprite workaround.
 */
void qt_symbian_setWindowCursor(const QCursor &cursor, const CCoeControl* wid)
{
    //find the window for this control
    while (!wid->OwnsWindow()) {
        wid = wid->Parent();
        if (!wid)
            return;
    }
    RWindowTreeNode *node = wid->DrawableWindow();
    qt_symbian_setWindowGroupCursor(cursor, *node);
}

/*
 * Makes the specified cursor appear everywhere.
 * Called from QApplication::setOverrideCursor
 */
void qt_symbian_setGlobalCursor(const QCursor &cursor)
{
#ifndef Q_SYMBIAN_FIXED_POINTER_CURSORS
    if (S60->brokenPointerCursors) {
        qt_symbian_set_pointer_sprite(cursor);
    } else
#endif
    {
        //because of the internals of window server, we need to force the cursor
        //to be set in all child windows too, otherwise when the cursor is over
        //the child window it may show a widget cursor or arrow cursor instead,
        //depending on construction order.
        QListIterator<WId> iter(QWidgetPrivate::mapper->uniqueKeys());
        while(iter.hasNext())
        {
            CCoeControl *ctrl = iter.next();
            if(ctrl->OwnsWindow()) {
                RWindowTreeNode *node = ctrl->DrawableWindow();
                qt_symbian_setWindowGroupCursor(cursor, *node);
            }
        }
    }
}
QT_END_NAMESPACE
#endif // QT_NO_CURSOR
