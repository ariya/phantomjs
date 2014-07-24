/*
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "Cursor.h"

#include "BitmapInfo.h"
#include "HWndDC.h"
#include "Image.h"
#include "IntPoint.h"
#include "SystemInfo.h"

#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

#include <windows.h>

#define ALPHA_CURSORS

namespace WebCore {

static PassRefPtr<SharedCursor> createSharedCursor(Image* img, const IntPoint& hotSpot)
{
    RefPtr<SharedCursor> impl;

    IntPoint effectiveHotSpot = determineHotSpot(img, hotSpot);
    static bool doAlpha = windowsVersion() >= WindowsXP;
    BitmapInfo cursorImage = BitmapInfo::create(IntSize(img->width(), img->height()));

    HWndDC dc(0);
    HDC workingDC = CreateCompatibleDC(dc);
    if (doAlpha) {
        OwnPtr<HBITMAP> hCursor = adoptPtr(CreateDIBSection(dc, (BITMAPINFO *)&cursorImage, DIB_RGB_COLORS, 0, 0, 0));
        ASSERT(hCursor);

        img->getHBITMAP(hCursor.get()); 
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(workingDC, hCursor.get());
        SetBkMode(workingDC, TRANSPARENT);
        SelectObject(workingDC, hOldBitmap);

        Vector<unsigned char, 128> maskBits;
        maskBits.fill(0xff, (img->width() + 7) / 8 * img->height());
        OwnPtr<HBITMAP> hMask = adoptPtr(CreateBitmap(img->width(), img->height(), 1, 1, maskBits.data()));

        ICONINFO ii;
        ii.fIcon = FALSE;
        ii.xHotspot = effectiveHotSpot.x();
        ii.yHotspot = effectiveHotSpot.y();
        ii.hbmMask = hMask.get();
        ii.hbmColor = hCursor.get();

        impl = SharedCursor::create(CreateIconIndirect(&ii));
    } else {
        // Platform doesn't support alpha blended cursors, so we need
        // to create the mask manually
        HDC andMaskDC = CreateCompatibleDC(dc);
        HDC xorMaskDC = CreateCompatibleDC(dc);
        OwnPtr<HBITMAP> hCursor = adoptPtr(CreateDIBSection(dc, &cursorImage, DIB_RGB_COLORS, 0, 0, 0));
        ASSERT(hCursor);
        img->getHBITMAP(hCursor.get()); 
        BITMAP cursor;
        GetObject(hCursor.get(), sizeof(BITMAP), &cursor);
        OwnPtr<HBITMAP> andMask = adoptPtr(CreateBitmap(cursor.bmWidth, cursor.bmHeight, 1, 1, NULL));
        OwnPtr<HBITMAP> xorMask = adoptPtr(CreateCompatibleBitmap(dc, cursor.bmWidth, cursor.bmHeight));
        HBITMAP oldCursor = (HBITMAP)SelectObject(workingDC, hCursor.get());
        HBITMAP oldAndMask = (HBITMAP)SelectObject(andMaskDC, andMask.get());
        HBITMAP oldXorMask = (HBITMAP)SelectObject(xorMaskDC, xorMask.get());

        SetBkColor(workingDC, RGB(0,0,0));  
        BitBlt(andMaskDC, 0, 0, cursor.bmWidth, cursor.bmHeight, workingDC, 0, 0, SRCCOPY);
    
        SetBkColor(xorMaskDC, RGB(255, 255, 255));
        SetTextColor(xorMaskDC, RGB(255, 255, 255));
        BitBlt(xorMaskDC, 0, 0, cursor.bmWidth, cursor.bmHeight, andMaskDC, 0, 0, SRCCOPY);
        BitBlt(xorMaskDC, 0, 0, cursor.bmWidth, cursor.bmHeight, workingDC, 0,0, SRCAND);

        SelectObject(workingDC, oldCursor);
        SelectObject(andMaskDC, oldAndMask);
        SelectObject(xorMaskDC, oldXorMask);

        ICONINFO icon = {0};
        icon.fIcon = FALSE;
        icon.xHotspot = effectiveHotSpot.x();
        icon.yHotspot = effectiveHotSpot.y();
        icon.hbmMask = andMask.get();
        icon.hbmColor = xorMask.get();
        impl = SharedCursor::create(CreateIconIndirect(&icon));

        DeleteDC(xorMaskDC);
        DeleteDC(andMaskDC);
    }
    DeleteDC(workingDC);

    return impl.release();
}

static PassRefPtr<SharedCursor> loadSharedCursor(HINSTANCE hInstance, LPCWSTR lpCursorName)
{
    return SharedCursor::create(::LoadCursorW(hInstance, lpCursorName));
}

static PassRefPtr<SharedCursor> loadCursorByName(char* name, int x, int y)
{
    IntPoint hotSpot(x, y);
    RefPtr<Image> cursorImage(Image::loadPlatformResource(name));
    if (cursorImage && !cursorImage->isNull())
        return createSharedCursor(cursorImage.get(), hotSpot);
    return loadSharedCursor(0, IDC_ARROW);
}

void Cursor::ensurePlatformCursor() const
{
    if (m_platformCursor)
        return;

    switch (m_type) {
    case Cursor::Pointer:
    case Cursor::Cell:
    case Cursor::ContextMenu:
    case Cursor::Alias:
    case Cursor::Copy:
    case Cursor::None:
    case Cursor::Grab:
    case Cursor::Grabbing:
        m_platformCursor = loadSharedCursor(0, IDC_ARROW);
        break;
    case Cursor::Cross:
        m_platformCursor = loadSharedCursor(0, IDC_CROSS);
        break;
    case Cursor::Hand:
        m_platformCursor = loadSharedCursor(0, IDC_HAND);
        break;
    case Cursor::IBeam:
        m_platformCursor = loadSharedCursor(0, IDC_IBEAM);
        break;
    case Cursor::Wait:
        m_platformCursor = loadSharedCursor(0, IDC_WAIT);
        break;
    case Cursor::Help:
        m_platformCursor = loadSharedCursor(0, IDC_HELP);
        break;
    case Cursor::Move:
        m_platformCursor = loadSharedCursor(0, IDC_SIZEALL);
        break;
    case Cursor::MiddlePanning:
        m_platformCursor = loadCursorByName("panIcon", 8, 8);
        break;
    case Cursor::EastResize:
        m_platformCursor = loadSharedCursor(0, IDC_SIZEWE);
        break;
    case Cursor::EastPanning:
        m_platformCursor = loadCursorByName("panEastCursor", 7, 7);
        break;
    case Cursor::NorthResize:
        m_platformCursor = loadSharedCursor(0, IDC_SIZENS);
        break;
    case Cursor::NorthPanning:
        m_platformCursor = loadCursorByName("panNorthCursor", 7, 7);
        break;
    case Cursor::NorthEastResize:
        m_platformCursor = loadSharedCursor(0, IDC_SIZENESW);
        break;
    case Cursor::NorthEastPanning:
        m_platformCursor = loadCursorByName("panNorthEastCursor", 7, 7);
        break;
    case Cursor::NorthWestResize:
        m_platformCursor = loadSharedCursor(0, IDC_SIZENWSE);
        break;
    case Cursor::NorthWestPanning:
        m_platformCursor = loadCursorByName("panNorthWestCursor", 7, 7);
        break;
    case Cursor::SouthResize:
        m_platformCursor = loadSharedCursor(0, IDC_SIZENS);
        break;
    case Cursor::SouthPanning:
        m_platformCursor = loadCursorByName("panSouthCursor", 7, 7);
        break;
    case Cursor::SouthEastResize:
        m_platformCursor = loadSharedCursor(0, IDC_SIZENWSE);
        break;
    case Cursor::SouthEastPanning:
        m_platformCursor = loadCursorByName("panSouthEastCursor", 7, 7);
        break;
    case Cursor::SouthWestResize:
        m_platformCursor = loadSharedCursor(0, IDC_SIZENESW);
        break;
    case Cursor::SouthWestPanning:
        m_platformCursor = loadCursorByName("panSouthWestCursor", 7, 7);
        break;
    case Cursor::WestResize:
        m_platformCursor = loadSharedCursor(0, IDC_SIZEWE);
        break;
    case Cursor::NorthSouthResize:
        m_platformCursor = loadSharedCursor(0, IDC_SIZENS);
        break;
    case Cursor::EastWestResize:
        m_platformCursor = loadSharedCursor(0, IDC_SIZEWE);
        break;
    case Cursor::WestPanning:
        m_platformCursor = loadCursorByName("panWestCursor", 7, 7);
        break;
    case Cursor::NorthEastSouthWestResize:
        m_platformCursor = loadSharedCursor(0, IDC_SIZENESW);
        break;
    case Cursor::NorthWestSouthEastResize:
        m_platformCursor = loadSharedCursor(0, IDC_SIZENWSE);
        break;
    case Cursor::ColumnResize:
        // FIXME: Windows does not have a standard column resize cursor <rdar://problem/5018591>
        m_platformCursor = loadSharedCursor(0, IDC_SIZEWE);
        break;
    case Cursor::RowResize:
        // FIXME: Windows does not have a standard row resize cursor <rdar://problem/5018591>
        m_platformCursor = loadSharedCursor(0, IDC_SIZENS);
        break;
    case Cursor::VerticalText:
        m_platformCursor = loadCursorByName("verticalTextCursor", 7, 7);
        break;
    case Cursor::Progress:
        m_platformCursor = loadSharedCursor(0, IDC_APPSTARTING);
        break;
    case Cursor::NoDrop:
    case Cursor::NotAllowed:
        m_platformCursor = loadSharedCursor(0, IDC_NO);
        break;
    case Cursor::ZoomIn:
        m_platformCursor = loadCursorByName("zoomInCursor", 7, 7);
        break;
    case Cursor::ZoomOut:
        m_platformCursor = loadCursorByName("zoomOutCursor", 7, 7);
        break;
    case Cursor::Custom:
        m_platformCursor = createSharedCursor(m_image.get(), m_hotSpot);
        break;
    default:
        ASSERT_NOT_REACHED();
        m_platformCursor = loadSharedCursor(0, IDC_ARROW);
        break;
    }
}

SharedCursor::~SharedCursor()
{
    DestroyIcon(m_nativeCursor);
}

Cursor::Cursor(const Cursor& other)
    : m_type(other.m_type)
    , m_image(other.m_image)
    , m_hotSpot(other.m_hotSpot)
    , m_platformCursor(other.m_platformCursor)
{
}

Cursor& Cursor::operator=(const Cursor& other)
{
    m_type = other.m_type;
    m_image = other.m_image;
    m_hotSpot = other.m_hotSpot;
    m_platformCursor = other.m_platformCursor;
    return *this;
}

Cursor::~Cursor()
{
}

} // namespace WebCore
