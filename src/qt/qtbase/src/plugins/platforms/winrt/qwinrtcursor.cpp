/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qwinrtcursor.h"
#include "qwinrtscreen.h"

#include <QtCore/qfunctions_winrt.h>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>

#include <wrl.h>
#include <windows.ui.core.h>
#include <windows.foundation.h>
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::UI::Core;
using namespace ABI::Windows::Foundation;

QT_USE_NAMESPACE

class QWinRTCursorPrivate
{
public:
    ComPtr<ICoreCursorFactory> cursorFactory;
};

QWinRTCursor::QWinRTCursor()
  : d_ptr(new QWinRTCursorPrivate)
{
    Q_D(QWinRTCursor);

    HRESULT hr;
    hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_UI_Core_CoreCursor).Get(),
                                IID_PPV_ARGS(&d->cursorFactory));
    Q_ASSERT_SUCCEEDED(hr);
}

QWinRTCursor::~QWinRTCursor()
{
}

#ifndef QT_NO_CURSOR
void QWinRTCursor::changeCursor(QCursor *windowCursor, QWindow *window)
{
    Q_D(QWinRTCursor);

    ICoreWindow *coreWindow = static_cast<QWinRTScreen *>(window->screen()->handle())->coreWindow();

    CoreCursorType type;
    switch (windowCursor ? windowCursor->shape() : Qt::ArrowCursor) {
    case Qt::BlankCursor:
        coreWindow->put_PointerCursor(Q_NULLPTR);
        return;
    default:
    case Qt::OpenHandCursor:
    case Qt::ClosedHandCursor:
    case Qt::DragCopyCursor:
    case Qt::DragMoveCursor:
    case Qt::DragLinkCursor:
        // (unavailable)
    case Qt::ArrowCursor:
        type = CoreCursorType_Arrow;
        break;
    case Qt::UpArrowCursor:
        type = CoreCursorType_UpArrow;
        break;
    case Qt::CrossCursor:
        type = CoreCursorType_Cross;
        break;
    case Qt::WaitCursor:
    case Qt::BusyCursor:
        type = CoreCursorType_Wait;
        break;
    case Qt::IBeamCursor:
        type = CoreCursorType_IBeam;
        break;
    case Qt::SizeVerCursor:
    case Qt::SplitVCursor:
        type = CoreCursorType_SizeNorthSouth;
        break;
    case Qt::SizeHorCursor:
    case Qt::SplitHCursor:
        type = CoreCursorType_SizeWestEast;
        break;
    case Qt::SizeBDiagCursor:
        type = CoreCursorType_SizeNortheastSouthwest;
        break;
    case Qt::SizeFDiagCursor:
        type = CoreCursorType_SizeNorthwestSoutheast;
        break;
    case Qt::SizeAllCursor:
        type = CoreCursorType_SizeAll;
        break;
    case Qt::PointingHandCursor:
        type = CoreCursorType_Hand;
        break;
    case Qt::ForbiddenCursor:
        type = CoreCursorType_UniversalNo;
        break;
    case Qt::WhatsThisCursor:
        type = CoreCursorType_Help;
        break;
    case Qt::BitmapCursor:
    case Qt::CustomCursor:
        // TODO: figure out if arbitrary bitmaps can be made into resource IDs
        // For now, we don't get enough info from QCursor to set a custom cursor
        type = CoreCursorType_Custom;
        break;
    }

    ComPtr<ICoreCursor> cursor;
    HRESULT hr = d->cursorFactory->CreateCursor(type, 0, &cursor);
    RETURN_VOID_IF_FAILED("Failed to create native cursor.");

    hr = coreWindow->put_PointerCursor(cursor.Get());
    RETURN_VOID_IF_FAILED("Failed to set native cursor.");
}
#endif // QT_NO_CURSOR

QPoint QWinRTCursor::pos() const
{
    ICoreWindow *coreWindow =
            static_cast<QWinRTScreen *>(QGuiApplication::primaryScreen()->handle())->coreWindow();
    Point point;
    coreWindow->get_PointerPosition(&point);
    return QPoint(point.X, point.Y);
}
