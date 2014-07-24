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

#ifndef PluginLayerWebKitThread_h
#define PluginLayerWebKitThread_h

#if USE(ACCELERATED_COMPOSITING)

#include "LayerWebKitThread.h"

namespace WebCore {

class PluginView;

class PluginLayerWebKitThread : public LayerWebKitThread {
public:
    static PassRefPtr<PluginLayerWebKitThread> create(PluginView* pluginView)
    {
        return adoptRef(new PluginLayerWebKitThread(pluginView));
    }

    virtual ~PluginLayerWebKitThread();

    void setPluginView(PluginView*);
    void setHolePunchRect(const IntRect&);

    virtual void setNeedsDisplay();

protected:
    virtual void updateTextureContentsIfNeeded();

private:
    PluginLayerWebKitThread(PluginView*);
    bool m_needsDisplay;
};

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)

#endif // PluginLayerWebKitThread_h
