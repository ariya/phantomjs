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

#ifndef HistoryItemViewState_h
#define HistoryItemViewState_h

#include <BlackBerryPlatformString.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

struct HistoryItemViewState {

    HistoryItemViewState()
        : orientation(0)
        , scale(1)
        , minimumScale(-1.0)
        , maximumScale(-1.0)
        , isUserScalable(true)
        , isZoomToFitScale(false)
        , shouldReflowBlock(false)
        , shouldSaveViewState(true)
    {
    }

    int orientation;
    double scale;
    double minimumScale;
    double maximumScale;
    bool isUserScalable;
    bool isZoomToFitScale;
    bool shouldReflowBlock;
    bool shouldSaveViewState;
    String networkToken;
    BlackBerry::Platform::String webPageClientState;
};

} // namespace WebCore

#endif // HistoryItemViewState_h
