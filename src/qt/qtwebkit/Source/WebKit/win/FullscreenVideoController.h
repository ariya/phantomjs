/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef FullscreenVideoController_h
#define FullscreenVideoController_h

#if ENABLE(VIDEO)

#include "MediaPlayerPrivateFullscreenWindow.h"

#include <WebCore/HTMLMediaElement.h>
#include <WebCore/Image.h>
#include <WebCore/IntPoint.h>
#include <WebCore/IntSize.h>
#include <wtf/RefPtr.h>

namespace WebCore {
class GraphicsContext;
#if USE(ACCELERATED_COMPOSITING)
class PlatformCALayer;
#endif
}

class HUDWidget {
public:
    HUDWidget(const WebCore::IntRect& rect) : m_rect(rect) { }
    
    virtual ~HUDWidget() { }

    virtual void draw(WebCore::GraphicsContext&) = 0;
    virtual void drag(const WebCore::IntPoint&, bool start) = 0;
    bool hitTest(const WebCore::IntPoint& point) const { return m_rect.contains(point); }

protected:
    WebCore::IntRect m_rect;
};

class HUDButton : public HUDWidget {
public:
    enum HUDButtonType {
        NoButton,
        PlayPauseButton,
        TimeSliderButton,
        VolumeUpButton,
        VolumeSliderButton,
        VolumeDownButton,
        ExitFullscreenButton
    };

    HUDButton(HUDButtonType, const WebCore::IntPoint&);
    ~HUDButton() { }

    virtual void draw(WebCore::GraphicsContext&);
    virtual void drag(const WebCore::IntPoint&, bool start) { }
    void setShowAltButton(bool b)  { m_showAltButton = b; }

private:
    RefPtr<WebCore::Image> m_buttonImage;
    RefPtr<WebCore::Image> m_buttonImageAlt;
    HUDButtonType m_type;
    bool m_showAltButton;
};

class HUDSlider : public HUDWidget {
public:
    enum HUDSliderButtonShape { RoundButton, DiamondButton };

    HUDSlider(HUDSliderButtonShape, int buttonSize, const WebCore::IntRect& rect);
    ~HUDSlider() { }

    virtual void draw(WebCore::GraphicsContext&);
    virtual void drag(const WebCore::IntPoint&, bool start);
    float value() const { return static_cast<float>(m_buttonPosition) / (m_rect.width() - m_buttonSize); }
    void setValue(float value) { m_buttonPosition = static_cast<int>(value * (m_rect.width() - m_buttonSize)); }

private:
    HUDSliderButtonShape m_buttonShape;
    int m_buttonSize;
    int m_buttonPosition;
    int m_dragStartOffset;
};

class FullscreenVideoController : WebCore::MediaPlayerPrivateFullscreenClient {
    WTF_MAKE_NONCOPYABLE(FullscreenVideoController);
public:
    FullscreenVideoController();
    virtual ~FullscreenVideoController();

    void setMediaElement(WebCore::HTMLMediaElement*);
    WebCore::HTMLMediaElement* mediaElement() const { return m_mediaElement.get(); }

    void enterFullscreen();
    void exitFullscreen();

private:
    // MediaPlayerPrivateFullscreenWindowClient
    virtual LRESULT fullscreenClientWndProc(HWND, UINT message, WPARAM, LPARAM);
    
    void ensureWindow();
    
    bool canPlay() const;
    void play();
    void pause();
    float volume() const;
    void setVolume(float);
    float currentTime() const;
    void setCurrentTime(float);
    float duration() const;
    void beginScrubbing();
    void endScrubbing();

    WebCore::IntPoint fullscreenToHUDCoordinates(const WebCore::IntPoint& point) const
    {
        return WebCore::IntPoint(point.x()- m_hudPosition.x(), point.y() - m_hudPosition.y());
    }

    static void registerHUDWindowClass();
    static LRESULT CALLBACK hudWndProc(HWND, UINT message, WPARAM, LPARAM);
    void createHUDWindow();
    void timerFired(WebCore::Timer<FullscreenVideoController>*);

    void togglePlay();
    void draw();

    void onChar(int c);
    void onMouseDown(const WebCore::IntPoint&);
    void onMouseMove(const WebCore::IntPoint&);
    void onMouseUp(const WebCore::IntPoint&);
    void onKeyDown(int virtualKey);

    RefPtr<WebCore::HTMLMediaElement> m_mediaElement;

    HWND m_hudWindow;
    OwnPtr<HBITMAP> m_bitmap;
    WebCore::IntSize m_fullscreenSize;
    WebCore::IntPoint m_hudPosition;
    OwnPtr<WebCore::MediaPlayerPrivateFullscreenWindow> m_fullscreenWindow;
#if USE(ACCELERATED_COMPOSITING)
    class LayerClient;
    friend class LayerClient;
    OwnPtr<LayerClient> m_layerClient;
    RefPtr<WebCore::PlatformCALayer> m_rootChild;
#endif

    HUDButton m_playPauseButton;
    HUDButton m_timeSliderButton;
    HUDButton m_volumeUpButton;
    HUDButton m_volumeSliderButton;
    HUDButton m_volumeDownButton;
    HUDButton m_exitFullscreenButton;
    HUDSlider m_volumeSlider;
    HUDSlider m_timeSlider;

    HUDWidget* m_hitWidget;
    WebCore::IntPoint m_moveOffset;
    bool m_movingWindow;
    WebCore::Timer<FullscreenVideoController> m_timer;
};

#endif

#endif // FullscreenVideoController_h
