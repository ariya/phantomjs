/*
 * Copyright (C) 2011 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef MediaPlayerPrivateBlackBerry_h
#define MediaPlayerPrivateBlackBerry_h

#if ENABLE(VIDEO)
#include "AuthenticationChallengeManager.h"
#include "MediaPlayerPrivate.h"

#include <BlackBerryPlatformPlayer.h>

namespace BlackBerry {

namespace Platform {
class IntRect;

namespace Graphics {
class GLES2Program;
}
}

namespace WebKit {
class WebPageClient;
}
}

namespace WebCore {

class MediaPlayerPrivate : public MediaPlayerPrivateInterface, public AuthenticationChallengeClient, public BlackBerry::Platform::IPlatformPlayerListener {
public:
    virtual ~MediaPlayerPrivate();

    static PassOwnPtr<MediaPlayerPrivateInterface> create(MediaPlayer*);
    static void registerMediaEngine(MediaEngineRegistrar);
    static void getSupportedTypes(HashSet<WTF::String>&);
    static MediaPlayer::SupportsType supportsType(const WTF::String&, const WTF::String&, const KURL&);
    static void notifyAppActivatedEvent(bool);
    static void setCertificatePath(const WTF::String&);

    virtual void load(const WTF::String& url);
    virtual void cancelLoad();

    virtual void prepareToPlay();
#if USE(ACCELERATED_COMPOSITING)
    virtual PlatformMedia platformMedia() const;
    virtual PlatformLayer* platformLayer() const;
    void drawBufferingAnimation(const TransformationMatrix&, const BlackBerry::Platform::Graphics::GLES2Program&);
#endif

    virtual void play();
    virtual void pause();

    virtual bool supportsFullscreen() const;

    virtual IntSize naturalSize() const;

    virtual bool hasVideo() const;
    virtual bool hasAudio() const;

    virtual void setVisible(bool);

    virtual float duration() const;

    virtual float currentTime() const;
    virtual void seek(float time);
    virtual bool seeking() const;

    virtual void setRate(float);

    virtual bool paused() const;
    virtual bool muted() const;
    virtual bool supportsMuting() const { return true; }

    virtual void setVolume(float);
    virtual void setMuted(bool);

    virtual MediaPlayer::NetworkState networkState() const;
    virtual MediaPlayer::ReadyState readyState() const;

    virtual float maxTimeSeekable() const;
    virtual PassRefPtr<TimeRanges> buffered() const;

    virtual bool didLoadingProgress() const;

    virtual void setSize(const IntSize&);

    virtual void paint(GraphicsContext*, const IntRect&);

    virtual void paintCurrentFrameInContext(GraphicsContext*, const IntRect&);

    virtual bool hasAvailableVideoFrame() const;

#if USE(ACCELERATED_COMPOSITING)
    // Whether accelerated rendering is supported by the media engine for the current media.
    virtual bool supportsAcceleratedRendering() const;
    // Called when the rendering system flips the into or out of accelerated rendering mode.
    virtual void acceleratedRenderingStateChanged() { }
#endif

    virtual bool hasSingleSecurityOrigin() const;

    virtual MediaPlayer::MovieLoadType movieLoadType() const;

    virtual void prepareForRendering();

    void resizeSourceDimensions();
    void setFullscreenWebPageClient(BlackBerry::WebKit::WebPageClient*);
    BlackBerry::Platform::Graphics::Window* getWindow();
    BlackBerry::Platform::Graphics::Window* getPeerWindow(const char*) const;
    BlackBerry::Platform::IntRect getWindowScreenRect() const;
    const char* mmrContextName();
    float percentLoaded();
    unsigned sourceWidth();
    unsigned sourceHeight();
    void setAllowPPSVolumeUpdates(bool);

    // IPlatformPlayerListener implementation.
    virtual void onStateChanged(BlackBerry::Platform::PlatformPlayer::MpState);
    virtual void onMediaStatusChanged(BlackBerry::Platform::PlatformPlayer::MMRPlayState);
    virtual void onError(BlackBerry::Platform::PlatformPlayer::Error);
    virtual void onDurationChanged(float);
    virtual void onTimeChanged(float);
    virtual void onRateChanged(float);
    virtual void onVolumeChanged(float);
    virtual void onPauseStateChanged();
    virtual void onRepaint();
    virtual void onSizeChanged();
    virtual void onPlayNotified();
    virtual void onPauseNotified();
    virtual void onWaitMetadataNotified(bool hasFinished, int timeWaited);
#if USE(ACCELERATED_COMPOSITING)
    virtual void onBuffering(bool);
#endif
    virtual void onAuthenticationNeeded(BlackBerry::Platform::MMRAuthChallenge&);
    virtual void onAuthenticationAccepted(const BlackBerry::Platform::MMRAuthChallenge&) const;
    virtual void onConditionallyEnterFullscreen();
    virtual void onExitFullscreen();
    virtual void onCreateHolePunchRect();
    virtual void onDestroyHolePunchRect();

    virtual void notifyChallengeResult(const KURL&, const ProtectionSpace&, AuthenticationChallengeResult, const Credential&);

    virtual bool isProcessingUserGesture() const;
    virtual bool isFullscreen() const;
    virtual bool isElementPaused() const;
    virtual bool isTabVisible() const;
    virtual int onShowErrorDialog(BlackBerry::Platform::PlatformPlayer::Error);
    virtual BlackBerry::Platform::Graphics::Window* platformWindow();
    virtual BlackBerry::Platform::WebMediaStreamDescriptor lookupMediaStream(const BlackBerry::Platform::String& url);

private:
    MediaPlayerPrivate(MediaPlayer*);

    void updateStates();
    WTF::String userAgent(const WTF::String&) const;

    virtual WTF::String engineDescription() const { return "BlackBerry"; }

    MediaPlayer* m_webCorePlayer;
    BlackBerry::Platform::PlatformPlayer* m_platformPlayer;

    mutable MediaPlayer::NetworkState m_networkState;
    MediaPlayer::ReadyState m_readyState;

    BlackBerry::WebKit::WebPageClient* m_fullscreenWebPageClient;
#if USE(ACCELERATED_COMPOSITING)
    void bufferingTimerFired(Timer<MediaPlayerPrivate>*);
    void setBuffering(bool);

    Timer<MediaPlayerPrivate> m_bufferingTimer;
    RefPtr<PlatformLayer> m_platformLayer;
    bool m_showBufferingImage;
#endif

    void userDrivenSeekTimerFired(Timer<MediaPlayerPrivate>*);
    Timer<MediaPlayerPrivate> m_userDrivenSeekTimer;
    float m_lastSeekTime;
    mutable float m_lastLoadingTime;
    bool m_lastSeekTimePending;
    bool m_isAuthenticationChallenging;
    void waitMetadataTimerFired(Timer<MediaPlayerPrivate>*);
    Timer<MediaPlayerPrivate> m_waitMetadataTimer;
    int m_waitMetadataPopDialogCounter;
};

} // namespace WebCore

#endif // ENABLE(VIDEO)
#endif // MediaPlayerPrivateBlackBerry_h
