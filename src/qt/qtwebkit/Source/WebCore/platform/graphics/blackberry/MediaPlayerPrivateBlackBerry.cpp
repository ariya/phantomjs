/*
 * Copyright (C) 2011, 2012 Research In Motion Limited. All rights reserved.
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

#include "config.h"

#if ENABLE(VIDEO)
#include "MediaPlayerPrivateBlackBerry.h"

#include "CookieManager.h"
#include "CredentialStorage.h"
#include "DOMFileSystemBase.h"
#include "HostWindow.h"
#include "MediaStreamDescriptor.h"
#include "MediaStreamRegistry.h"
#include "SecurityOrigin.h"

#include <BlackBerryPlatformDeviceInfo.h>
#include <BlackBerryPlatformPrimitives.h>
#include <BlackBerryPlatformSettings.h>
#include <BlackBerryPlatformWebFileSystem.h>
#include <FrameLoaderClientBlackBerry.h>

#if USE(ACCELERATED_COMPOSITING)
#include "VideoLayerWebKitThread.h"
#include <BlackBerryPlatformGLES2Program.h>
#include <GLES2/gl2.h>
#endif

#include <TiledImage.h>

using namespace std;
using namespace BlackBerry::Platform;

namespace WebCore {

// Static callback functions for factory
PassOwnPtr<MediaPlayerPrivateInterface> MediaPlayerPrivate::create(MediaPlayer* player)
{
    return adoptPtr(new MediaPlayerPrivate(player));
}

void MediaPlayerPrivate::registerMediaEngine(MediaEngineRegistrar registrar)
{
    registrar(create, getSupportedTypes, supportsType, 0, 0, 0);
}

void MediaPlayerPrivate::getSupportedTypes(HashSet<WTF::String>& types)
{
    set<BlackBerry::Platform::String> supported = PlatformPlayer::allSupportedMimeTypes();
    set<BlackBerry::Platform::String>::iterator i = supported.begin();
    for (; i != supported.end(); i++)
        types.add(*i);
}

MediaPlayer::SupportsType MediaPlayerPrivate::supportsType(const WTF::String& type, const WTF::String& codecs, const KURL& url)
{
    bool isRTSP = url.protocolIs("rtsp");

    if (!isRTSP && (type.isNull() || type.isEmpty())) {
        LOG(Media, "MediaPlayer does not support type; type is null or empty.");
        return MediaPlayer::IsNotSupported;
    }

    // spec says we should not return "probably" if the codecs string is empty
    if (isRTSP || PlatformPlayer::mimeTypeSupported(type.ascii().data())) {
        LOG(Media, "MediaPlayer supports type %s.", isRTSP ? "rtsp" : type.ascii().data());
        return codecs.isEmpty() ? MediaPlayer::MayBeSupported : MediaPlayer::IsSupported;
    }
    LOG(Media, "MediaPlayer does not support type %s.", type.ascii().data());
    return MediaPlayer::IsNotSupported;
}

void MediaPlayerPrivate::notifyAppActivatedEvent(bool activated)
{
    PlatformPlayer::notifyAppActivatedEvent(activated);
}

void MediaPlayerPrivate::setCertificatePath(const WTF::String& caPath)
{
    PlatformPlayer::setCertificatePath(caPath);
}

MediaPlayerPrivate::MediaPlayerPrivate(MediaPlayer* player)
    : m_webCorePlayer(player)
    , m_platformPlayer(0)
    , m_networkState(MediaPlayer::Empty)
    , m_readyState(MediaPlayer::HaveNothing)
    , m_fullscreenWebPageClient(0)
#if USE(ACCELERATED_COMPOSITING)
    , m_bufferingTimer(this, &MediaPlayerPrivate::bufferingTimerFired)
    , m_showBufferingImage(false)
#endif
    , m_userDrivenSeekTimer(this, &MediaPlayerPrivate::userDrivenSeekTimerFired)
    , m_lastSeekTime(0)
    , m_lastLoadingTime(0)
    , m_lastSeekTimePending(false)
    , m_isAuthenticationChallenging(false)
    , m_waitMetadataTimer(this, &MediaPlayerPrivate::waitMetadataTimerFired)
    , m_waitMetadataPopDialogCounter(0)
{
}

MediaPlayerPrivate::~MediaPlayerPrivate()
{
    if (m_isAuthenticationChallenging)
        AuthenticationChallengeManager::instance()->cancelAuthenticationChallenge(this);

    if (isFullscreen())
        m_webCorePlayer->mediaPlayerClient()->mediaPlayerExitFullscreen();
#if USE(ACCELERATED_COMPOSITING)
    // Remove media player from platform layer.
    if (m_platformLayer)
        static_cast<VideoLayerWebKitThread*>(m_platformLayer.get())->setMediaPlayer(0);
#endif

    if (m_platformPlayer) {
        if (m_platformPlayer->dialogState() == PlatformPlayer::DialogShown) {
            m_platformPlayer->setDialogState(PlatformPlayer::MediaPlayerPrivateDestroyed);
            m_platformPlayer->stop();
        } else
            deleteGuardedObject(m_platformPlayer);
    }
}

void MediaPlayerPrivate::load(const WTF::String& url)
{
    WTF::String modifiedUrl(url);

    if (modifiedUrl.startsWith("local://")) {
        KURL kurl = KURL(KURL(), modifiedUrl);
        kurl.setProtocol("file");
        WTF::String tempPath(BlackBerry::Platform::Settings::instance()->applicationLocalDirectory().c_str());
        tempPath.append(kurl.path());
        kurl.setPath(tempPath);
        modifiedUrl = kurl.string();
    }
    // filesystem: URLs refer to entities in the Web File System (WFS) and are
    // intended to be useable by HTML 5 media elements directly. Unfortunately
    // the loader for our media player is implemented in a separate process and
    // does not have access to WFS, so we translate to a file:// URL. Normally
    // this would be a security violation, but since the MediaElement has
    // already done a security check on the filesystem: URL as part of the
    // media resource selection algorithm, we should be OK here.
    if (modifiedUrl.startsWith("filesystem:")) {
        KURL kurl = KURL(KURL(), modifiedUrl);
        KURL mediaURL;
        WTF::String fsPath;
        FileSystemType fsType;
        WebFileSystem::Type type;

        // Extract the root and file paths from WFS
        DOMFileSystemBase::crackFileSystemURL(kurl, fsType, fsPath);
        if (fsType == FileSystemTypeTemporary)
            type = WebFileSystem::Temporary;
        else
            type = WebFileSystem::Persistent;
        WTF::String fsRoot = BlackBerry::Platform::WebFileSystem::rootPathForWebFileSystem(type);

        // Build a BlackBerry::Platform::SecurityOrigin from the document's 
        // WebCore::SecurityOrigin and serialize it to build the last
        // path component
        WebCore::SecurityOrigin* wkOrigin = m_webCorePlayer->mediaPlayerClient()->mediaPlayerOwningDocument()->securityOrigin();
        BlackBerry::Platform::SecurityOrigin bbOrigin(wkOrigin->protocol(), wkOrigin->host(), wkOrigin->port());
        WTF::String secOrigin(bbOrigin.serialize('_'));

        // Build a file:// URL from the path components and extract it to 
        // a string for further processing
        mediaURL.setProtocol("file");
        mediaURL.setPath(fsRoot + "/" + secOrigin + "/" + fsPath);
        modifiedUrl = mediaURL.string();
    } else if (modifiedUrl.startsWith("file://") || modifiedUrl.startsWith("data:")) {
        // The QNX Multimedia Framework cannot handle filenames or data containing URL escape sequences.
        modifiedUrl = decodeURLEscapeSequences(modifiedUrl);
    }

    void* tabId = m_webCorePlayer->mediaPlayerClient()->mediaPlayerHostWindow()->platformPageClient();
    int playerID = m_webCorePlayer->mediaPlayerClient()->mediaPlayerHostWindow()->platformPageClient()->playerID();
    bool isVideo = m_webCorePlayer->mediaPlayerClient()->mediaPlayerIsVideo();

    deleteGuardedObject(m_platformPlayer);
#if USE(ACCELERATED_COMPOSITING)
    m_platformPlayer = PlatformPlayer::create(this, tabId, isVideo, true, modifiedUrl);
#else
    m_platformPlayer = PlatformPlayer::create(this, tabId, isVideo, false, modifiedUrl);
#endif

    WTF::String cookiePairs;
    if (!url.isEmpty())
        cookiePairs = cookieManager().getCookie(KURL(ParsedURLString, url), WithHttpOnlyCookies);
    m_platformPlayer->load(playerID, modifiedUrl, m_webCorePlayer->userAgent(), cookiePairs);
}

void MediaPlayerPrivate::cancelLoad()
{
    if (m_platformPlayer)
        m_platformPlayer->cancelLoad();
}

void MediaPlayerPrivate::prepareToPlay()
{
    if (m_platformPlayer)
        m_platformPlayer->prepareToPlay();
}

void MediaPlayerPrivate::play()
{
    if (m_platformPlayer)
        m_platformPlayer->play();
}

void MediaPlayerPrivate::pause()
{
    if (m_platformPlayer)
        m_platformPlayer->pause();
}

bool MediaPlayerPrivate::supportsFullscreen() const
{
    return hasVideo();
}

IntSize MediaPlayerPrivate::naturalSize() const
{
    if (!m_platformPlayer)
        return IntSize();

    // Cannot return empty size, otherwise paint() will never get called.
    // Also, the values here will affect the aspect ratio of the output rectangle that will
    // be used for renderering the video, so we must take PAR into account.
    // Now, hope that metadata has been provided before this gets called if this is a video.
    double pixelWidth = static_cast<double>(m_platformPlayer->pixelWidth());
    double pixelHeight = static_cast<double>(m_platformPlayer->pixelHeight());
    if (!m_platformPlayer->pixelWidth() || !m_platformPlayer->pixelHeight())
        pixelWidth = pixelHeight = 1.0;

    // Use floating point arithmetic to eliminate the chance of integer
    // overflow. PAR numbers can be 5 digits or more.
    double adjustedSourceWidth = static_cast<double>(m_platformPlayer->sourceWidth()) * pixelWidth / pixelHeight;
    return IntSize(static_cast<int>(adjustedSourceWidth + 0.5), m_platformPlayer->sourceHeight());
}

bool MediaPlayerPrivate::hasVideo() const
{
    if (m_platformPlayer)
        return m_platformPlayer->hasVideo();
    return false;
}

bool MediaPlayerPrivate::hasAudio() const
{
    if (m_platformPlayer)
        return m_platformPlayer->hasAudio();
    return false;
}

void MediaPlayerPrivate::setVisible(bool)
{
    notImplemented();
}

float MediaPlayerPrivate::duration() const
{
    if (m_platformPlayer)
        return m_platformPlayer->duration();
    return 0.0f;
}

static const double SeekSubmissionDelay = 0.1; // Reasonable throttling value.
static const double ShortMediaThreshold = SeekSubmissionDelay * 2.0;

float MediaPlayerPrivate::currentTime() const
{
    if (!m_platformPlayer)
        return 0.0f;

    // For very short media on the order of SeekSubmissionDelay we get
    // unwanted repeats if we don't return the most up-to-date currentTime().
    bool shortMedia = m_platformPlayer->duration() < ShortMediaThreshold;
    return m_userDrivenSeekTimer.isActive() && !shortMedia ? m_lastSeekTime: m_platformPlayer->currentTime();
}

void MediaPlayerPrivate::seek(float time)
{
    if (!m_platformPlayer)
        return;

    m_lastSeekTime = time;
    m_lastSeekTimePending = true;
    if (!m_userDrivenSeekTimer.isActive())
        userDrivenSeekTimerFired(0);
}

void MediaPlayerPrivate::userDrivenSeekTimerFired(Timer<MediaPlayerPrivate>*)
{
    if (m_lastSeekTimePending) {
        m_platformPlayer->seek(m_lastSeekTime);
        m_lastSeekTimePending = false;
        m_userDrivenSeekTimer.startOneShot(SeekSubmissionDelay);
    }
}

bool MediaPlayerPrivate::seeking() const
{
    return false;
}

void MediaPlayerPrivate::setRate(float rate)
{
    if (m_platformPlayer)
        m_platformPlayer->setRate(rate);
}

bool MediaPlayerPrivate::paused() const
{
    if (m_platformPlayer)
        return m_platformPlayer->paused();
    return false;
}

void MediaPlayerPrivate::setVolume(float volume)
{
    if (m_platformPlayer)
        m_platformPlayer->setVolume(volume);
}

void MediaPlayerPrivate::setMuted(bool muted)
{
    if (m_platformPlayer)
        m_platformPlayer->setMuted(muted);
}

bool MediaPlayerPrivate::muted() const
{
    if (m_platformPlayer)
        return m_platformPlayer->muted();
    return false;
}

MediaPlayer::NetworkState MediaPlayerPrivate::networkState() const
{
    return m_networkState;
}

MediaPlayer::ReadyState MediaPlayerPrivate::readyState() const
{
    return m_readyState;
}

float MediaPlayerPrivate::maxTimeSeekable() const
{
    if (m_platformPlayer)
        return m_platformPlayer->maxTimeSeekable();
    return 0.0f;
}

PassRefPtr<TimeRanges> MediaPlayerPrivate::buffered() const
{
    if (!m_platformPlayer)
        return TimeRanges::create();

    RefPtr<TimeRanges> timeRanges = TimeRanges::create();
    if (float bufferLoaded = m_platformPlayer->bufferLoaded())
        timeRanges->add(0, bufferLoaded);
    return timeRanges.release();
}

bool MediaPlayerPrivate::didLoadingProgress() const
{
    if (!m_platformPlayer)
        return false;

    float bufferLoaded = m_platformPlayer->bufferLoaded();
    if (bufferLoaded == m_lastLoadingTime)
        return false;

    m_lastLoadingTime = bufferLoaded;
    return true;
}

void MediaPlayerPrivate::setSize(const IntSize&)
{
    notImplemented();
}

void MediaPlayerPrivate::paint(GraphicsContext* context, const IntRect& rect)
{
    if (!m_platformPlayer)
        return;

#if USE(ACCELERATED_COMPOSITING)
    if (supportsAcceleratedRendering()) {
        // Only process paint calls coming via the accelerated compositing code
        // path, where we get called with a null graphics context. See
        // LayerCompositingThread::drawTextures(). Ignore calls from the regular
        // rendering path.
        if (!context)
            m_platformPlayer->notifyOutputUpdate(BlackBerry::Platform::IntRect(rect.x(), rect.y(), rect.width(), rect.height()));

        return;
    }
#endif

    paintCurrentFrameInContext(context, rect);
}

void MediaPlayerPrivate::paintCurrentFrameInContext(GraphicsContext* context, const IntRect& rect)
{
    if (!hasVideo() || context->paintingDisabled() || !m_webCorePlayer->visible())
        return;

    BlackBerry::Platform::Graphics::Drawable* dst = context->platformContext();

    BlackBerry::Platform::IntRect platformRect(rect.x(), rect.y(), rect.width(), rect.height());
    IntRect clippedRect = m_webCorePlayer->mediaPlayerClient()->mediaPlayerWindowClipRect();
    BlackBerry::Platform::IntRect platformWindowClipRect(clippedRect.x(), clippedRect.y(), clippedRect.width(), clippedRect.height());
    m_platformPlayer->paint(dst, platformRect, platformWindowClipRect);
}

bool MediaPlayerPrivate::hasAvailableVideoFrame() const
{
    if (m_platformPlayer)
        return m_platformPlayer->hasAvailableVideoFrame();
    return false;
}

bool MediaPlayerPrivate::hasSingleSecurityOrigin() const
{
    return true;
}

MediaPlayer::MovieLoadType MediaPlayerPrivate::movieLoadType() const
{
    if (m_platformPlayer)
        return static_cast<MediaPlayer::MovieLoadType>(m_platformPlayer->movieLoadType());
    return MediaPlayer::Unknown;
}

void MediaPlayerPrivate::prepareForRendering()
{
    if (m_platformPlayer)
        m_platformPlayer->prepareForRendering();
}

void MediaPlayerPrivate::resizeSourceDimensions()
{
    if (!m_webCorePlayer)
        return;

    if (!m_webCorePlayer->mediaPlayerClient()->mediaPlayerIsVideo())
        return;

    // If we don't know what the width and height of the video source is, then we need to set it to something sane.
    if (m_platformPlayer->sourceWidth() && m_platformPlayer->sourceHeight())
        return;

    LayoutRect rect = m_webCorePlayer->mediaPlayerClient()->mediaPlayerContentBoxRect();
    m_platformPlayer->setSourceDimension(rect.width().toUnsigned(), rect.height().toUnsigned());
}

void MediaPlayerPrivate::setFullscreenWebPageClient(BlackBerry::WebKit::WebPageClient* client)
{
    if (m_fullscreenWebPageClient == client)
        return;

    m_fullscreenWebPageClient = client;
    m_platformPlayer->toggleFullscreen(client);

    // The following repaint is needed especially if video is paused and
    // fullscreen is exiting, so that a MediaPlayerPrivate::paint() is
    // triggered and the code in outputUpdate() sets the correct window
    // rectangle.
    if (!client)
        m_webCorePlayer->repaint();
}

BlackBerry::Platform::Graphics::Window* MediaPlayerPrivate::getWindow()
{
    return m_platformPlayer->getWindow();
}

BlackBerry::Platform::Graphics::Window* MediaPlayerPrivate::getPeerWindow(const char* uniqueID) const
{
    return m_platformPlayer->getPeerWindow(uniqueID);
}

BlackBerry::Platform::IntRect MediaPlayerPrivate::getWindowScreenRect() const
{
    unsigned x, y, width, height;
    m_platformPlayer->getWindowPosition(x, y, width, height);
    return BlackBerry::Platform::IntRect(x, y, width, height);
}

const char* MediaPlayerPrivate::mmrContextName()
{
    return m_platformPlayer->mmrContextName();
}

float MediaPlayerPrivate::percentLoaded()
{
    if (!m_platformPlayer->duration())
        return 0;

    float buffered = 0;
    RefPtr<TimeRanges> timeRanges = this->buffered();
    for (unsigned i = 0; i < timeRanges->length(); ++i) {
        ExceptionCode ignoredException;
        float start = timeRanges->start(i, ignoredException);
        float end = timeRanges->end(i, ignoredException);
        buffered += end - start;
    }

    float loaded = buffered / m_platformPlayer->duration();
    return loaded;
}

unsigned MediaPlayerPrivate::sourceWidth()
{
    return m_platformPlayer->sourceWidth();
}

unsigned MediaPlayerPrivate::sourceHeight()
{
    return m_platformPlayer->sourceHeight();
}

void MediaPlayerPrivate::setAllowPPSVolumeUpdates(bool allow)
{
    if (m_platformPlayer)
        return m_platformPlayer->setAllowPPSVolumeUpdates(allow);
}

void MediaPlayerPrivate::updateStates()
{
    MediaPlayer::NetworkState oldNetworkState = m_networkState;
    MediaPlayer::ReadyState oldReadyState = m_readyState;

    PlatformPlayer::Error currentError = m_platformPlayer->error();

    if (currentError != PlatformPlayer::MediaOK) {
        m_readyState = MediaPlayer::HaveNothing;
        if (currentError == PlatformPlayer::MediaDecodeError)
            m_networkState = MediaPlayer::DecodeError;
        else if (currentError == PlatformPlayer::MediaMetaDataError
            || currentError == PlatformPlayer::MediaAudioReceiveError
            || currentError == PlatformPlayer::MediaVideoReceiveError)
            m_networkState = MediaPlayer::NetworkError;
    } else {
        switch (m_platformPlayer->mediaState()) {
        case PlatformPlayer::MMRPlayStateIdle:
            m_networkState = MediaPlayer::Idle;
            break;
        case PlatformPlayer::MMRPlayStatePlaying:
            m_networkState = MediaPlayer::Loading;
            break;
        case PlatformPlayer::MMRPlayStateStopped:
            m_networkState = MediaPlayer::Idle;
            break;
        case PlatformPlayer::MMRPlayStateUnknown:
        default:
            break;
        }

        switch (m_platformPlayer->state()) {
        case PlatformPlayer::MP_STATE_IDLE:
            if (isFullscreen())
                m_webCorePlayer->mediaPlayerClient()->mediaPlayerExitFullscreen();
            break;
        case PlatformPlayer::MP_STATE_ACTIVE:
            break;
        case PlatformPlayer::MP_STATE_UNSUPPORTED:
            break;
        default:
            break;
        }
        if ((duration() || movieLoadType() == MediaPlayer::LiveStream)
            && m_readyState != MediaPlayer::HaveEnoughData)
            m_readyState = MediaPlayer::HaveEnoughData;
    }

    if (m_readyState != oldReadyState)
        m_webCorePlayer->readyStateChanged();
    if (m_networkState != oldNetworkState)
        m_webCorePlayer->networkStateChanged();
}

// IPlatformPlayerListener callbacks implementation
void MediaPlayerPrivate::onStateChanged(PlatformPlayer::MpState)
{
    updateStates();
}

void MediaPlayerPrivate::onMediaStatusChanged(PlatformPlayer::MMRPlayState)
{
    updateStates();
}

void MediaPlayerPrivate::onError(PlatformPlayer::Error)
{
    updateStates();
}

void MediaPlayerPrivate::onDurationChanged(float)
{
    updateStates();
    m_webCorePlayer->durationChanged();
}

void MediaPlayerPrivate::onTimeChanged(float)
{
    m_webCorePlayer->timeChanged();
}

void MediaPlayerPrivate::onPauseStateChanged()
{
    if (!isFullscreen())
        return;

    // Paused state change not due to local controller.
    if (m_platformPlayer->isPaused())
        m_webCorePlayer->mediaPlayerClient()->mediaPlayerPause();
    else {
        // The HMI fullscreen widget has resumed play. Check if the
        // pause timeout occurred.
        m_platformPlayer->processPauseTimeoutIfNecessary();
        m_webCorePlayer->mediaPlayerClient()->mediaPlayerPlay();
    }
}

void MediaPlayerPrivate::onRateChanged(float)
{
    m_webCorePlayer->rateChanged();
}

void MediaPlayerPrivate::onVolumeChanged(float volume)
{
    m_webCorePlayer->volumeChanged(volume);
}

void MediaPlayerPrivate::onRepaint()
{
    m_webCorePlayer->repaint();
}

void MediaPlayerPrivate::onSizeChanged()
{
    resizeSourceDimensions();
    if (hasVideo())
        m_webCorePlayer->sizeChanged();
}

void MediaPlayerPrivate::onPlayNotified()
{
    m_webCorePlayer->mediaPlayerClient()->mediaPlayerPlay();
}

void MediaPlayerPrivate::onPauseNotified()
{
    m_webCorePlayer->mediaPlayerClient()->mediaPlayerPause();
}

static const int popupDialogInterval = 10;
static const double checkMetadataReadyInterval = 0.5;
void MediaPlayerPrivate::onWaitMetadataNotified(bool hasFinished, int timeWaited)
{
    if (!hasFinished) {
        if (!m_waitMetadataTimer.isActive()) {
            // Make sure to popup dialog every 10 seconds after metadata start to load.
            // This should be set only once at the first time when user press the play button.
            m_waitMetadataPopDialogCounter = static_cast<int>(timeWaited / checkMetadataReadyInterval);
            m_waitMetadataTimer.startOneShot(checkMetadataReadyInterval);
        }
    } else if (m_waitMetadataTimer.isActive()) {
        m_waitMetadataTimer.stop();
        m_waitMetadataPopDialogCounter = 0;
    }
}

void MediaPlayerPrivate::waitMetadataTimerFired(Timer<MediaPlayerPrivate>*)
{
    if (m_platformPlayer->isMetadataReady()) {
        m_waitMetadataPopDialogCounter = 0;
        m_platformPlayer->playWithMetadataReady();
        return;
    }

    static const int hitTimeToPopupDialog = static_cast<int>(popupDialogInterval / checkMetadataReadyInterval);
    m_waitMetadataPopDialogCounter++;
    if (m_waitMetadataPopDialogCounter < hitTimeToPopupDialog) {
        m_waitMetadataTimer.startOneShot(checkMetadataReadyInterval);
        return;
    }
    m_waitMetadataPopDialogCounter = 0;

    PlatformPlayer::DialogResult wait = m_platformPlayer->showErrorDialog(PlatformPlayer::MediaMetaDataTimeoutError);
    if (wait == PlatformPlayer::DialogEmergencyExit)
        return;
    if (wait == PlatformPlayer::DialogResponse0)
        onPauseNotified();
    else {
        if (m_platformPlayer->isMetadataReady())
            m_platformPlayer->playWithMetadataReady();
        else
            m_waitMetadataTimer.startOneShot(checkMetadataReadyInterval);
    }
}

#if USE(ACCELERATED_COMPOSITING)
void MediaPlayerPrivate::onBuffering(bool flag)
{
    setBuffering(flag);
}
#endif

static ProtectionSpace generateProtectionSpaceFromMMRAuthChallenge(const MMRAuthChallenge& authChallenge)
{
    KURL url(ParsedURLString, WTF::String(authChallenge.url().c_str()));
    ASSERT(url.isValid());

    return ProtectionSpace(url.host(), url.port(),
        static_cast<ProtectionSpaceServerType>(authChallenge.serverType()),
        authChallenge.realm().c_str(),
        static_cast<ProtectionSpaceAuthenticationScheme>(authChallenge.authScheme()));
}

void MediaPlayerPrivate::onAuthenticationNeeded(MMRAuthChallenge& authChallenge)
{
    KURL url(ParsedURLString, WTF::String(authChallenge.url().c_str()));
    if (!url.isValid())
        return;

    ProtectionSpace protectionSpace = generateProtectionSpaceFromMMRAuthChallenge(authChallenge);
    Credential credential = CredentialStorage::get(protectionSpace);
    if (!credential.isEmpty()) {
        notifyChallengeResult(url, protectionSpace, AuthenticationChallengeSuccess, credential);
        return;
    }

    m_isAuthenticationChallenging = true;
    AuthenticationChallengeManager::instance()->authenticationChallenge(url, protectionSpace, credential,
        this, m_webCorePlayer->mediaPlayerClient()->mediaPlayerHostWindow()->platformPageClient());
}

void MediaPlayerPrivate::notifyChallengeResult(const KURL& url, const ProtectionSpace&, AuthenticationChallengeResult result, const Credential& credential)
{
    m_isAuthenticationChallenging = false;

    if (result != AuthenticationChallengeSuccess || !url.isValid())
        return;

    m_platformPlayer->reloadWithCredential(credential.user(), credential.password(), static_cast<MMRAuthChallenge::CredentialPersistence>(credential.persistence()));
}

void MediaPlayerPrivate::onAuthenticationAccepted(const MMRAuthChallenge& authChallenge) const
{
    KURL url(ParsedURLString, WTF::String(authChallenge.url().c_str()));
    if (!url.isValid())
        return;

    ProtectionSpace protectionSpace = generateProtectionSpaceFromMMRAuthChallenge(authChallenge);
    Credential savedCredential = CredentialStorage::get(protectionSpace);
    if (savedCredential.isEmpty())
        CredentialStorage::set(Credential(authChallenge.username().c_str(), authChallenge.password().c_str(), static_cast<CredentialPersistence>(authChallenge.persistence())), protectionSpace, url);
}

int MediaPlayerPrivate::onShowErrorDialog(PlatformPlayer::Error type)
{
    using namespace BlackBerry::WebKit;

    WebPageClient::AlertType atype;
    switch (type) {
    case PlatformPlayer::MediaOK:
        atype = WebPageClient::MediaOK;
        break;
    case PlatformPlayer::MediaDecodeError:
        atype = WebPageClient::MediaDecodeError;
        break;
    case PlatformPlayer::MediaMetaDataError:
        atype = WebPageClient::MediaMetaDataError;
        break;
    case PlatformPlayer::MediaMetaDataTimeoutError:
        atype = WebPageClient::MediaMetaDataTimeoutError;
        break;
    case PlatformPlayer::MediaNoMetaDataError:
        atype = WebPageClient::MediaNoMetaDataError;
        break;
    case PlatformPlayer::MediaVideoReceiveError:
        atype = WebPageClient::MediaVideoReceiveError;
        break;
    case PlatformPlayer::MediaAudioReceiveError:
        atype = WebPageClient::MediaAudioReceiveError;
        break;
    case PlatformPlayer::MediaInvalidError:
        atype = WebPageClient::MediaInvalidError;
        break;
    default:
        LOG(Media, "Alert type does not exist.");
        return -1;
    }
    return m_webCorePlayer->mediaPlayerClient()->mediaPlayerHostWindow()->platformPageClient()->showAlertDialog(atype);
}

static WebMediaStreamSource toWebMediaStreamSource(MediaStreamSource* src)
{
    return WebMediaStreamSource(src->id(), static_cast<WebMediaStreamSource::Type>(src->type()), src->name());
}

static WebMediaStreamDescriptor toWebMediaStreamDescriptor(MediaStreamDescriptor* d)
{
    vector<WebMediaStreamSource> audioSources;
    for (size_t i = 0; i < d->numberOfAudioComponents(); i++)
        audioSources.push_back(toWebMediaStreamSource(d->audioComponent(i)->source()));

    vector<WebMediaStreamSource> videoSources;
    for (size_t i = 0; i < d->numberOfVideoComponents(); i++)
        videoSources.push_back(toWebMediaStreamSource(d->videoComponent(i)->source()));

    return WebMediaStreamDescriptor(d->id(), audioSources, videoSources);
}

WebMediaStreamDescriptor MediaPlayerPrivate::lookupMediaStream(const BlackBerry::Platform::String& url)
{
    MediaStreamDescriptor* descriptor = MediaStreamRegistry::registry().lookupMediaStreamDescriptor(WTF::String::fromUTF8(url.c_str()));
    if (!descriptor)
        return WebMediaStreamDescriptor();

    return toWebMediaStreamDescriptor(descriptor);
}

BlackBerry::Platform::Graphics::Window* MediaPlayerPrivate::platformWindow()
{
    return m_webCorePlayer->mediaPlayerClient()->mediaPlayerHostWindow()->platformPageClient()->platformWindow();
}

bool MediaPlayerPrivate::isProcessingUserGesture() const
{
    return m_webCorePlayer->mediaPlayerClient()->mediaPlayerIsProcessingUserGesture();
}

bool MediaPlayerPrivate::isFullscreen() const
{
    return m_fullscreenWebPageClient;
}

bool MediaPlayerPrivate::isElementPaused() const
{
    return m_webCorePlayer->mediaPlayerClient()->mediaPlayerIsPaused();
}

bool MediaPlayerPrivate::isTabVisible() const
{
    return m_webCorePlayer->mediaPlayerClient()->mediaPlayerHostWindow()->platformPageClient()->isVisible();
}

bool MediaPlayerPrivate::supportsAcceleratedRendering() const
{
    if (m_platformPlayer)
        return m_platformPlayer->supportsAcceleratedRendering();
    return false;
}

#if USE(ACCELERATED_COMPOSITING)
static const double BufferingAnimationDelay = 1.0 / 24;
static unsigned* s_bufferingImageData = 0;
static IntSize s_bufferingImageSize;

PlatformMedia MediaPlayerPrivate::platformMedia() const
{
    PlatformMedia pm;
    pm.type = PlatformMedia::QNXMediaPlayerType;
    pm.media.qnxMediaPlayer = const_cast<MediaPlayerPrivate*>(this);
    return pm;
}

PlatformLayer* MediaPlayerPrivate::platformLayer() const
{
    if (m_platformLayer)
        return m_platformLayer.get();
    return 0;
}

static void loadBufferingImageData()
{
    static bool loaded = false;
    if (!loaded) {
        static Image* bufferingIcon = Image::loadPlatformResource("vidbuffer").leakRef();

        NativeImagePtr nativeImage = bufferingIcon->nativeImageForCurrentFrame();
        if (!nativeImage)
            return;

        loaded = true;
        s_bufferingImageSize = bufferingIcon->size();
        int bufSize = bufferingIcon->decodedSize();
        s_bufferingImageData = static_cast<unsigned*>(malloc(bufSize));

        nativeImage->readPixels(s_bufferingImageData, s_bufferingImageSize.width() * s_bufferingImageSize.height());

        bufferingIcon->deref();
    }
}

void MediaPlayerPrivate::bufferingTimerFired(Timer<MediaPlayerPrivate>*)
{
    if (m_showBufferingImage) {
        if (!isFullscreen() && m_platformLayer)
            m_platformLayer->setNeedsDisplay();
        m_bufferingTimer.startOneShot(BufferingAnimationDelay);
    }
}

void MediaPlayerPrivate::setBuffering(bool buffering)
{
    if (!m_webCorePlayer || !m_webCorePlayer->mediaPlayerClient() || !m_webCorePlayer->mediaPlayerClient()->mediaPlayerIsVideo())
        buffering = false; // Buffering animation not visible for audio.

    if (buffering != m_showBufferingImage) {
        m_showBufferingImage = buffering;
        if (buffering) {
            loadBufferingImageData();
            m_bufferingTimer.startOneShot(BufferingAnimationDelay);
        } else
            m_bufferingTimer.stop();

        if (m_platformLayer)
            m_platformLayer->setNeedsDisplay();
    }
}

static unsigned allocateTextureId()
{
    unsigned texid;
    glGenTextures(1, &texid);
    glBindTexture(GL_TEXTURE_2D, texid);
    // Do basic linear filtering on resize.
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // NPOT textures in GL ES only work when the wrap mode is set to GL_CLAMP_TO_EDGE.
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return texid;
}

void MediaPlayerPrivate::drawBufferingAnimation(const TransformationMatrix& matrix, const Graphics::GLES2Program& program)
{
    if (m_showBufferingImage && s_bufferingImageData && !isFullscreen()) {
        TransformationMatrix renderMatrix = matrix;

        // Rotate the buffering indicator so that it takes 1 second to do 1 revolution.
        timespec time;
        clock_gettime(CLOCK_MONOTONIC, &time);
        renderMatrix.rotate(time.tv_nsec / 1000000000.0 * 360.0);

        static bool initialized = false;
        static unsigned texId = allocateTextureId();
        glBindTexture(GL_TEXTURE_2D, texId);
        if (!initialized) {
            initialized = true;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, s_bufferingImageSize.width(), s_bufferingImageSize.height(),
                0, GL_RGBA, GL_UNSIGNED_BYTE, s_bufferingImageData);
            free(s_bufferingImageData);
        }

        float texcoords[] = { 0, 0,  1, 0,  1, 1,  0, 1 };
        FloatRect bufferingImageRect(FloatPoint(-s_bufferingImageSize.width() / 2.0f, -s_bufferingImageSize.height() / 2.0f), s_bufferingImageSize);
        FloatQuad transformedQuad = renderMatrix.mapQuad(bufferingImageRect);

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glUniform1f(program.opacityLocation(), 1.0);
        glVertexAttribPointer(program.positionLocation(), 2, GL_FLOAT, GL_FALSE, 0, &transformedQuad);
        glVertexAttribPointer(program.texCoordLocation(), 2, GL_FLOAT, GL_FALSE, 0, texcoords);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
}
#endif

void MediaPlayerPrivate::onConditionallyEnterFullscreen()
{
    Document* owningDocument = m_webCorePlayer->mediaPlayerClient()->mediaPlayerOwningDocument();
    BlackBerry::Platform::DeviceInfo* info = BlackBerry::Platform::DeviceInfo::instance();

    // Don't allow video in <embed> and <object> containers to go fullscreen
    // on play because this does not currently work. Detect this by checking
    // for MediaDocument with a parent document.
    if (owningDocument->isMediaDocument() && owningDocument->parentDocument())
        return;

    if (info->isMobile()) {
        // This is a mobile device (small screen), not a tablet, so we
        // enter fullscreen video on user-initiated plays.
        bool nothingIsFullscreen = !m_webCorePlayer->mediaPlayerClient()->mediaPlayerIsFullscreen();
#if ENABLE(FULLSCREEN_API)
        if (owningDocument->webkitIsFullScreen())
            nothingIsFullscreen = false;
#endif
        if (nothingIsFullscreen)
            m_webCorePlayer->mediaPlayerClient()->mediaPlayerEnterFullscreen();
    }
}

void MediaPlayerPrivate::onExitFullscreen()
{
    if (m_webCorePlayer->mediaPlayerClient()->mediaPlayerIsFullscreen())
        m_webCorePlayer->mediaPlayerClient()->mediaPlayerExitFullscreen();
}

void MediaPlayerPrivate::onCreateHolePunchRect()
{
#if USE(ACCELERATED_COMPOSITING)
    // Create platform layer for video (create hole punch rect).
    if (!m_platformLayer && supportsAcceleratedRendering()) {
        m_showBufferingImage = false;
        m_platformLayer = VideoLayerWebKitThread::create(m_webCorePlayer);
    }
#endif
}

void MediaPlayerPrivate::onDestroyHolePunchRect()
{
#if USE(ACCELERATED_COMPOSITING)
    setBuffering(false);
    // Remove media player from platform layer (remove hole punch rect).
    if (m_platformLayer) {
        static_cast<VideoLayerWebKitThread*>(m_platformLayer.get())->setMediaPlayer(0);
        m_platformLayer.clear();
    }
#endif
}

} // namespace WebCore

#endif // ENABLE(VIDEO)
