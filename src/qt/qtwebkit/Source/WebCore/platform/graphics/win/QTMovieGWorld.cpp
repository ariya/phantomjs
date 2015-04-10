/*
 * Copyright (C) 2007, 2008, 2009, 2010 Apple, Inc.  All rights reserved.
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

#include "QTMovieGWorld.h"

#include "QTMovieTask.h"
#include <GXMath.h>
#include <Movies.h>
#include <QTML.h>
#include <QuickTimeComponents.h>
#include <wtf/Assertions.h>
#include <wtf/HashSet.h>
#include <wtf/Noncopyable.h>
#include <wtf/Vector.h>

using namespace std;

static const long minimumQuickTimeVersion = 0x07300000; // 7.3

static LPCWSTR fullscreenQTMovieGWorldPointerProp = L"fullscreenQTMovieGWorldPointer";

// Resizing GWorlds is slow, give them a minimum size so size of small 
// videos can be animated smoothly
static const int cGWorldMinWidth = 640;
static const int cGWorldMinHeight = 360;

static const float cNonContinuousTimeChange = 0.2f;

union UppParam {
    long longValue;
    void* ptr;
};

static MovieDrawingCompleteUPP gMovieDrawingCompleteUPP = 0;
static HashSet<QTMovieGWorldPrivate*>* gTaskList;
static Vector<CFStringRef>* gSupportedTypes = 0;
static SInt32 quickTimeVersion = 0;

class QTMovieGWorldPrivate : public QTMovieClient {
public:
    QTMovieGWorldPrivate(QTMovieGWorld* movieWin);
    virtual ~QTMovieGWorldPrivate();

    void registerDrawingCallback();
    void unregisterDrawingCallback();
    void drawingComplete();
    void updateGWorld();
    void createGWorld();
    void deleteGWorld();
    void clearGWorld();
    void updateMovieSize();

    void setSize(int, int);

    virtual void movieEnded(QTMovie*);
    virtual void movieLoadStateChanged(QTMovie*);
    virtual void movieTimeChanged(QTMovie*);

    QTMovieGWorld* m_movieWin;
    RefPtr<QTMovie> m_qtMovie;
    Movie m_movie;
    QTMovieGWorldClient* m_client;
    long m_loadState;
    int m_width;
    int m_height;
    bool m_visible;
    GWorldPtr m_gWorld;
    int m_gWorldWidth;
    int m_gWorldHeight;
    GWorldPtr m_savedGWorld;
    float m_widthScaleFactor;
    float m_heightScaleFactor;
#if !ASSERT_DISABLED
    bool m_scaleCached;
#endif
    WindowPtr m_fullscreenWindow;
    GWorldPtr m_fullscreenOrigGWorld;
    Rect m_fullscreenRect;
    QTMovieGWorldFullscreenClient* m_fullscreenClient;
    char* m_fullscreenRestoreState;
    bool m_disabled;
};

QTMovieGWorldPrivate::QTMovieGWorldPrivate(QTMovieGWorld* movieWin)
    : m_movieWin(movieWin)
    , m_movie(0)
    , m_client(0)
    , m_loadState(0)
    , m_width(0)
    , m_height(0)
    , m_visible(false)
    , m_gWorld(0)
    , m_gWorldWidth(0)
    , m_gWorldHeight(0)
    , m_savedGWorld(0)
    , m_widthScaleFactor(1)
    , m_heightScaleFactor(1)
#if !ASSERT_DISABLED
    , m_scaleCached(false)
#endif
    , m_fullscreenWindow(0)
    , m_fullscreenOrigGWorld(0)
    , m_fullscreenClient(0)
    , m_fullscreenRestoreState(0)
    , m_disabled(false)
    , m_qtMovie(0)
{
    Rect rect = { 0, 0, 0, 0 };
    m_fullscreenRect = rect;
}

QTMovieGWorldPrivate::~QTMovieGWorldPrivate()
{
    ASSERT(!m_fullscreenWindow);

    if (m_gWorld)
        deleteGWorld();
}

pascal OSErr movieDrawingCompleteProc(Movie movie, long data)
{
    UppParam param;
    param.longValue = data;
    QTMovieGWorldPrivate* mp = static_cast<QTMovieGWorldPrivate*>(param.ptr);
    if (mp)
        mp->drawingComplete();
    return 0;
}

void QTMovieGWorldPrivate::registerDrawingCallback()
{
    if (!gMovieDrawingCompleteUPP)
        gMovieDrawingCompleteUPP = NewMovieDrawingCompleteUPP(movieDrawingCompleteProc);

    UppParam param;
    param.ptr = this;
    SetMovieDrawingCompleteProc(m_movie, movieDrawingCallWhenChanged, gMovieDrawingCompleteUPP, param.longValue);
}

void QTMovieGWorldPrivate::unregisterDrawingCallback()
{
    SetMovieDrawingCompleteProc(m_movie, movieDrawingCallWhenChanged, 0, 0);
}

void QTMovieGWorldPrivate::drawingComplete()
{
    if (!m_gWorld || m_movieWin->m_private->m_disabled || m_loadState < QTMovieLoadStateLoaded)
        return;
    m_client->movieNewImageAvailable(m_movieWin);
}

void QTMovieGWorldPrivate::updateGWorld()
{
    bool shouldBeVisible = m_visible;
    if (!m_height || !m_width)
        shouldBeVisible = false;

    if (shouldBeVisible && !m_gWorld)
        createGWorld();
    else if (!shouldBeVisible && m_gWorld)
        deleteGWorld();
    else if (m_gWorld && (m_width > m_gWorldWidth || m_height > m_gWorldHeight)) {
        // need a bigger, better gWorld
        deleteGWorld();
        createGWorld();
    }
}

void QTMovieGWorldPrivate::createGWorld()
{
    ASSERT(!m_gWorld);
    if (!m_movie || m_loadState < QTMovieLoadStateLoaded)
        return;

    m_gWorldWidth = max(cGWorldMinWidth, m_width);
    m_gWorldHeight = max(cGWorldMinHeight, m_height);
    Rect bounds; 
    bounds.top = 0;
    bounds.left = 0; 
    bounds.right = m_gWorldWidth;
    bounds.bottom = m_gWorldHeight;
    OSErr err = QTNewGWorld(&m_gWorld, k32BGRAPixelFormat, &bounds, 0, 0, 0); 
    if (err) 
        return;
    GetMovieGWorld(m_movie, &m_savedGWorld, 0);
    SetMovieGWorld(m_movie, m_gWorld, 0);
    bounds.right = m_width;
    bounds.bottom = m_height;
    SetMovieBox(m_movie, &bounds);
}

void QTMovieGWorldPrivate::clearGWorld()
{
    if (!m_movie || !m_gWorld)
        return;

    GrafPtr savePort;
    GetPort(&savePort); 
    MacSetPort((GrafPtr)m_gWorld);

    Rect bounds; 
    bounds.top = 0;
    bounds.left = 0; 
    bounds.right = m_gWorldWidth;
    bounds.bottom = m_gWorldHeight;
    EraseRect(&bounds);

    MacSetPort(savePort);
}

void QTMovieGWorldPrivate::setSize(int width, int height)
{
    if (m_width == width && m_height == height)
        return;
    m_width = width;
    m_height = height;

    // Do not change movie box before reaching load state loaded as we grab
    // the initial size when task() sees that state for the first time, and
    // we need the initial size to be able to scale movie properly. 
    if (!m_movie || m_loadState < QTMovieLoadStateLoaded)
        return;

#if !ASSERT_DISABLED
    ASSERT(m_scaleCached);
#endif

    updateMovieSize();
}

void QTMovieGWorldPrivate::updateMovieSize()
{
    if (!m_movie || m_loadState < QTMovieLoadStateLoaded)
        return;

    Rect bounds; 
    bounds.top = 0;
    bounds.left = 0; 
    bounds.right = m_width;
    bounds.bottom = m_height;
    SetMovieBox(m_movie, &bounds);
    updateGWorld();
}


void QTMovieGWorldPrivate::deleteGWorld()
{
    ASSERT(m_gWorld);
    if (m_movie)
        SetMovieGWorld(m_movie, m_savedGWorld, 0);
    m_savedGWorld = 0;
    DisposeGWorld(m_gWorld); 
    m_gWorld = 0;
    m_gWorldWidth = 0;
    m_gWorldHeight = 0;
}

void QTMovieGWorldPrivate::movieEnded(QTMovie*)
{
    // Do nothing
}

void QTMovieGWorldPrivate::movieLoadStateChanged(QTMovie* movie)
{
    long loadState = GetMovieLoadState(movie->getMovieHandle());
    if (loadState != m_loadState) {

        // we only need to erase the movie gworld when the load state changes to loaded while it
        //  is visible as the gworld is destroyed/created when visibility changes
        bool movieNewlyPlayable = loadState >= QTMovieLoadStateLoaded && m_loadState < QTMovieLoadStateLoaded;
        m_loadState = loadState;

        if (movieNewlyPlayable) {
            updateMovieSize();
            if (m_visible)
                clearGWorld();
        }
    }
}

void QTMovieGWorldPrivate::movieTimeChanged(QTMovie*)
{
    // Do nothing
}

QTMovieGWorld::QTMovieGWorld(QTMovieGWorldClient* client)
    : m_private(new QTMovieGWorldPrivate(this))
{
    m_private->m_client = client;
}

QTMovieGWorld::~QTMovieGWorld()
{
    delete m_private;
}

void QTMovieGWorld::setSize(int width, int height)
{
    m_private->setSize(width, height);
    QTMovieTask::sharedTask()->updateTaskTimer();
}

void QTMovieGWorld::setVisible(bool b)
{
    m_private->m_visible = b;
    m_private->updateGWorld();
}

void QTMovieGWorld::getCurrentFrameInfo(void*& buffer, unsigned& bitsPerPixel, unsigned& rowBytes, unsigned& width, unsigned& height)
{
    if (!m_private->m_gWorld) {
        buffer = 0;
        bitsPerPixel = 0;
        rowBytes = 0;
        width = 0;
        height = 0;
        return;
    }
    PixMapHandle offscreenPixMap = GetGWorldPixMap(m_private->m_gWorld);
    buffer = (*offscreenPixMap)->baseAddr;
    bitsPerPixel = (*offscreenPixMap)->pixelSize;
    rowBytes = (*offscreenPixMap)->rowBytes & 0x3FFF;
    width = m_private->m_width;
    height = m_private->m_height;
}

void QTMovieGWorld::paint(HDC hdc, int x, int y)
{
    if (!m_private->m_gWorld)
        return;

    HDC hdcSrc = static_cast<HDC>(GetPortHDC(reinterpret_cast<GrafPtr>(m_private->m_gWorld))); 
    if (!hdcSrc)
        return;

    // FIXME: If we could determine the movie has no alpha, we could use BitBlt for those cases, which might be faster.
    BLENDFUNCTION blendFunction; 
    blendFunction.BlendOp = AC_SRC_OVER;
    blendFunction.BlendFlags = 0;
    blendFunction.SourceConstantAlpha = 255;
    blendFunction.AlphaFormat = AC_SRC_ALPHA;
    AlphaBlend(hdc, x, y, m_private->m_width, m_private->m_height, hdcSrc, 
         0, 0, m_private->m_width, m_private->m_height, blendFunction);
}

void QTMovieGWorld::setDisabled(bool b)
{
    m_private->m_disabled = b;
}

bool QTMovieGWorld::isDisabled() const
{
    return m_private->m_disabled;
}

LRESULT QTMovieGWorld::fullscreenWndProc(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    QTMovieGWorld* movie = static_cast<QTMovieGWorld*>(GetPropW(wnd, fullscreenQTMovieGWorldPointerProp));

    if (message == WM_DESTROY)
        RemovePropW(wnd, fullscreenQTMovieGWorldPointerProp);

    if (!movie)
        return DefWindowProc(wnd, message, wParam, lParam);

    return movie->m_private->m_fullscreenClient->fullscreenClientWndProc(wnd, message, wParam, lParam);
}

HWND QTMovieGWorld::enterFullscreen(QTMovieGWorldFullscreenClient* client)
{
    m_private->m_fullscreenClient = client;
    
    BeginFullScreen(&m_private->m_fullscreenRestoreState, 0, 0, 0, &m_private->m_fullscreenWindow, 0, fullScreenAllowEvents); 
    QTMLSetWindowWndProc(m_private->m_fullscreenWindow, fullscreenWndProc);
    CreatePortAssociation(GetPortNativeWindow(m_private->m_fullscreenWindow), 0, 0);

    GetMovieBox(m_private->m_movie, &m_private->m_fullscreenRect);
    GetMovieGWorld(m_private->m_movie, &m_private->m_fullscreenOrigGWorld, 0);
    SetMovieGWorld(m_private->m_movie, reinterpret_cast<CGrafPtr>(m_private->m_fullscreenWindow), GetGWorldDevice(reinterpret_cast<CGrafPtr>(m_private->m_fullscreenWindow)));

    // Set the size of the box to preserve aspect ratio
    Rect rect = m_private->m_fullscreenWindow->portRect;

    float movieRatio = static_cast<float>(m_private->m_width) / m_private->m_height;
    int windowWidth =  rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;
    float windowRatio = static_cast<float>(windowWidth) / windowHeight;
    int actualWidth = (windowRatio > movieRatio) ? (windowHeight * movieRatio) : windowWidth;
    int actualHeight = (windowRatio < movieRatio) ? (windowWidth / movieRatio) : windowHeight;
    int offsetX = (windowWidth - actualWidth) / 2;
    int offsetY = (windowHeight - actualHeight) / 2;

    rect.left = offsetX;
    rect.right = offsetX + actualWidth;
    rect.top = offsetY;
    rect.bottom = offsetY + actualHeight;

    SetMovieBox(m_private->m_movie, &rect);
    ShowHideTaskBar(true);

    // Set the 'this' pointer on the HWND
    HWND wnd = static_cast<HWND>(GetPortNativeWindow(m_private->m_fullscreenWindow));
    SetPropW(wnd, fullscreenQTMovieGWorldPointerProp, static_cast<HANDLE>(this));

    return wnd;
}

void QTMovieGWorld::exitFullscreen()
{
    if (!m_private->m_fullscreenWindow)
        return;

    HWND wnd = static_cast<HWND>(GetPortNativeWindow(m_private->m_fullscreenWindow));
    DestroyPortAssociation(reinterpret_cast<CGrafPtr>(m_private->m_fullscreenWindow));
    SetMovieGWorld(m_private->m_movie, m_private->m_fullscreenOrigGWorld, 0);
    EndFullScreen(m_private->m_fullscreenRestoreState, 0L);
    SetMovieBox(m_private->m_movie, &m_private->m_fullscreenRect);
    m_private->m_fullscreenWindow = 0;
}

void QTMovieGWorld::setMovie(PassRefPtr<QTMovie> movie)
{
    if (m_private->m_qtMovie) {
        m_private->unregisterDrawingCallback();
        m_private->m_qtMovie->removeClient(m_private);
        m_private->m_qtMovie = 0;
        m_private->m_movie = 0;
    }

    m_private->m_qtMovie = movie;

    if (m_private->m_qtMovie) {
        m_private->m_qtMovie->addClient(m_private);
        m_private->m_movie = m_private->m_qtMovie->getMovieHandle();
        m_private->registerDrawingCallback();
    }
}

QTMovie* QTMovieGWorld::movie() const 
{
    return m_private->m_qtMovie.get();
}
