/*
 * Copyright (C) 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
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

#if USE(ACCELERATED_COMPOSITING) && ENABLE(VIDEO)
#include "VideoLayerWebKitThread.h"

#include "FrameView.h"
#include "LayerCompositingThread.h"
#include "MediaPlayer.h"

#include <BlackBerryPlatformLog.h>

namespace WebCore {

VideoLayerWebKitThread::VideoLayerWebKitThread(MediaPlayer* mediaPlayer)
    : LayerWebKitThread(Layer, 0)
{
    setMediaPlayer(mediaPlayer);
}

VideoLayerWebKitThread::~VideoLayerWebKitThread()
{
}

void VideoLayerWebKitThread::setMediaPlayer(MediaPlayer* mediaPlayer)
{
    m_mediaPlayer = mediaPlayer;

    // This is a bit of a misnomer, since we don't need an actual GL texture.
    // However, the LayerRenderer will only process layers that "need textures"
    setNeedsTexture(isDrawable() && (contents() || drawsContent() || this->mediaPlayer()));

    if (!m_mediaPlayer) {
        // We can't afford to wait until the next commit to set the m_mediaPlayer
        // to 0 in the impl, because it is about to be destroyed.
        layerCompositingThread()->setMediaPlayer(0);

        // Clear hole punch rect.
        setHolePunchRect(IntRect());
    }

    setNeedsCommit();
}

void VideoLayerWebKitThread::setHolePunchRect(const IntRect& rect)
{
    m_holePunchRect = rect;
    setNeedsCommit();
}

void VideoLayerWebKitThread::setNeedsDisplay()
{
    // We don't really need display per se, but some code may depend on
    // setNeedsDisplay() to call setNeedsCommit()
    setNeedsCommit();
}

void VideoLayerWebKitThread::boundsChanged()
{
    if (!mediaPlayer())
        return;

    // For video layers, the entire layer should be hole punched
    // Hole punch rectangle size is equal to the size of this layer.
    // Note that although we know the *size* of this layer, we won't know the position of this layer
    // until the LayerCompositingThread::setDrawTransform method is called from the LayerRenderer.
    setHolePunchRect(IntRect(IntPoint::zero(), m_bounds));
}

void VideoLayerWebKitThread::updateTextureContentsIfNeeded()
{
    // There are no texture contents, video is shown in a special screen window
    // behind the web page.
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING) && ENABLE(VIDEO)
