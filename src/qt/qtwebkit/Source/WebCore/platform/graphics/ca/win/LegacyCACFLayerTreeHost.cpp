/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
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
#include "LegacyCACFLayerTreeHost.h"

#if USE(ACCELERATED_COMPOSITING)

#include "PlatformCALayer.h"
#include <QuartzCore/CABase.h>
#include <WebKitSystemInterface/WebKitSystemInterface.h>

#ifndef NDEBUG
#define D3D_DEBUG_INFO
#endif

#include <d3d9.h>
#include <d3dx9.h>

#pragma comment(lib, "d3d9")
#pragma comment(lib, "d3dx9")

using namespace std;

namespace WebCore {

static IDirect3D9* s_d3d = 0;
static IDirect3D9* d3d()
{
    if (s_d3d)
        return s_d3d;

    if (!LoadLibrary(TEXT("d3d9.dll")))
        return 0;

    s_d3d = Direct3DCreate9(D3D_SDK_VERSION);

    return s_d3d;
}

static D3DPRESENT_PARAMETERS initialPresentationParameters()
{
    D3DPRESENT_PARAMETERS parameters = {0};
    parameters.Windowed = TRUE;
    parameters.SwapEffect = D3DSWAPEFFECT_COPY;
    parameters.BackBufferCount = 1;
    parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    parameters.MultiSampleType = D3DMULTISAMPLE_NONE;

    return parameters;
}

// FIXME: <rdar://6507851> Share this code with CoreAnimation.
static bool hardwareCapabilitiesIndicateCoreAnimationSupport(const D3DCAPS9& caps)
{
    // CoreAnimation needs two or more texture units.
    if (caps.MaxTextureBlendStages < 2)
        return false;

    // CoreAnimation needs non-power-of-two textures.
    if ((caps.TextureCaps & D3DPTEXTURECAPS_POW2) && !(caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL))
        return false;

    // CoreAnimation needs vertex shader 2.0 or greater.
    if (D3DSHADER_VERSION_MAJOR(caps.VertexShaderVersion) < 2)
        return false;

    // CoreAnimation needs pixel shader 2.0 or greater.
    if (D3DSHADER_VERSION_MAJOR(caps.PixelShaderVersion) < 2)
        return false;

    return true;
}

PassRefPtr<LegacyCACFLayerTreeHost> LegacyCACFLayerTreeHost::create()
{
    return adoptRef(new LegacyCACFLayerTreeHost);
}

LegacyCACFLayerTreeHost::LegacyCACFLayerTreeHost()
    : m_renderTimer(this, &LegacyCACFLayerTreeHost::renderTimerFired)
    , m_context(wkCACFContextCreate())
    , m_mightBeAbleToCreateDeviceLater(true)
    , m_mustResetLostDeviceBeforeRendering(false)
{
#ifndef NDEBUG
    char* printTreeFlag = getenv("CA_PRINT_TREE");
    m_printTree = printTreeFlag && atoi(printTreeFlag);
#endif
}

LegacyCACFLayerTreeHost::~LegacyCACFLayerTreeHost()
{
    wkCACFContextDestroy(m_context);
}

void LegacyCACFLayerTreeHost::initializeContext(void* userData, PlatformCALayer* layer)
{
    wkCACFContextSetUserData(m_context, userData);
    wkCACFContextSetLayer(m_context, layer->platformLayer());
}

bool LegacyCACFLayerTreeHost::createRenderer()
{
    if (m_d3dDevice || !m_mightBeAbleToCreateDeviceLater)
        return m_d3dDevice;

    m_mightBeAbleToCreateDeviceLater = false;
    D3DPRESENT_PARAMETERS parameters = initialPresentationParameters();

    if (!d3d() || !::IsWindow(window()))
        return false;

    // D3D doesn't like to make back buffers for 0 size windows. We skirt this problem if we make the
    // passed backbuffer width and height non-zero. The window will necessarily get set to a non-zero
    // size eventually, and then the backbuffer size will get reset.
    RECT rect;
    GetClientRect(window(), &rect);

    if (rect.left-rect.right == 0 || rect.bottom-rect.top == 0) {
        parameters.BackBufferWidth = 1;
        parameters.BackBufferHeight = 1;
    }

    D3DCAPS9 d3dCaps;
    if (FAILED(d3d()->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &d3dCaps)))
        return false;

    DWORD behaviorFlags = D3DCREATE_FPU_PRESERVE;
    if ((d3dCaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) && d3dCaps.VertexProcessingCaps)
        behaviorFlags |= D3DCREATE_HARDWARE_VERTEXPROCESSING;
    else
        behaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;

    COMPtr<IDirect3DDevice9> device;
    if (FAILED(d3d()->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window(), behaviorFlags, &parameters, &device))) {
        // In certain situations (e.g., shortly after waking from sleep), Direct3DCreate9() will
        // return an IDirect3D9 for which IDirect3D9::CreateDevice will always fail. In case we
        // have one of these bad IDirect3D9s, get rid of it so we'll fetch a new one the next time
        // we want to call CreateDevice.
        s_d3d->Release();
        s_d3d = 0;

        // Even if we don't have a bad IDirect3D9, in certain situations (e.g., shortly after
        // waking from sleep), CreateDevice will fail, but will later succeed if called again.
        m_mightBeAbleToCreateDeviceLater = true;

        return false;
    }

    // Now that we've created the IDirect3DDevice9 based on the capabilities we
    // got from the IDirect3D9 global object, we requery the device for its
    // actual capabilities. The capabilities returned by the device can
    // sometimes be more complete, for example when using software vertex
    // processing.
    D3DCAPS9 deviceCaps;
    if (FAILED(device->GetDeviceCaps(&deviceCaps)))
        return false;

    if (!hardwareCapabilitiesIndicateCoreAnimationSupport(deviceCaps))
        return false;

    m_d3dDevice = device;

    initD3DGeometry();

    wkCACFContextSetD3DDevice(m_context, m_d3dDevice.get());

    if (IsWindow(window())) {
        rootLayer()->setBounds(bounds());
        flushContext();
    }

    return true;
}

void LegacyCACFLayerTreeHost::destroyRenderer()
{
    wkCACFContextSetLayer(m_context, 0);

    wkCACFContextSetD3DDevice(m_context, 0);
    m_d3dDevice = 0;
    if (s_d3d)
        s_d3d->Release();

    s_d3d = 0;
    m_mightBeAbleToCreateDeviceLater = true;

    CACFLayerTreeHost::destroyRenderer();
}

void LegacyCACFLayerTreeHost::resize()
{
    if (!m_d3dDevice)
        return;

    // Resetting the device might fail here. But that's OK, because if it does it we will attempt to
    // reset the device the next time we try to render.
    resetDevice(ChangedWindowSize);

    if (rootLayer()) {
        rootLayer()->setBounds(bounds());
        flushContext();
    }
}

void LegacyCACFLayerTreeHost::renderTimerFired(Timer<LegacyCACFLayerTreeHost>*)
{
    paint();
}

void LegacyCACFLayerTreeHost::paint()
{
    createRenderer();
    if (!m_d3dDevice) {
        if (m_mightBeAbleToCreateDeviceLater)
            renderSoon();
        return;
    }

    CACFLayerTreeHost::paint();
}

void LegacyCACFLayerTreeHost::render(const Vector<CGRect>& windowDirtyRects)
{
    ASSERT(m_d3dDevice);

    if (m_mustResetLostDeviceBeforeRendering && !resetDevice(LostDevice)) {
        // We can't reset the device right now. Try again soon.
        renderSoon();
        return;
    }

    CGRect bounds = this->bounds();

    // Give the renderer some space to use. This needs to be valid until the
    // wkCACFContextFinishUpdate() call below.
    char space[4096];
    if (!wkCACFContextBeginUpdate(m_context, space, sizeof(space), CACurrentMediaTime(), bounds, windowDirtyRects.data(), windowDirtyRects.size()))
        return;

    HRESULT err = S_OK;
    CFTimeInterval timeToNextRender = numeric_limits<CFTimeInterval>::infinity();

    do {
        // FIXME: don't need to clear dirty region if layer tree is opaque.

        WKCACFUpdateRectEnumerator* e = wkCACFContextCopyUpdateRectEnumerator(m_context);
        if (!e)
            break;

        Vector<D3DRECT, 64> rects;
        for (const CGRect* r = wkCACFUpdateRectEnumeratorNextRect(e); r; r = wkCACFUpdateRectEnumeratorNextRect(e)) {
            D3DRECT rect;
            rect.x1 = r->origin.x;
            rect.x2 = rect.x1 + r->size.width;
            rect.y1 = bounds.origin.y + bounds.size.height - (r->origin.y + r->size.height);
            rect.y2 = rect.y1 + r->size.height;

            rects.append(rect);
        }
        wkCACFUpdateRectEnumeratorRelease(e);

        timeToNextRender = wkCACFContextGetNextUpdateTime(m_context);

        if (rects.isEmpty())
            break;

        m_d3dDevice->Clear(rects.size(), rects.data(), D3DCLEAR_TARGET, 0, 1.0f, 0);

        m_d3dDevice->BeginScene();
        wkCACFContextRenderUpdate(m_context);
        m_d3dDevice->EndScene();

        err = m_d3dDevice->Present(0, 0, 0, 0);

        if (err == D3DERR_DEVICELOST) {
            wkCACFContextAddUpdateRect(m_context, bounds);
            if (!resetDevice(LostDevice)) {
                // We can't reset the device right now. Try again soon.
                renderSoon();
                return;
            }
        }
    } while (err == D3DERR_DEVICELOST);

    wkCACFContextFinishUpdate(m_context);

#ifndef NDEBUG
    if (m_printTree)
        rootLayer()->printTree();
#endif

    // If timeToNextRender is not infinity, it means animations are running, so queue up to render again
    if (timeToNextRender != numeric_limits<CFTimeInterval>::infinity())
        renderSoon();
}

void LegacyCACFLayerTreeHost::renderSoon()
{
    if (!m_renderTimer.isActive())
        m_renderTimer.startOneShot(0);
}

void LegacyCACFLayerTreeHost::flushContext()
{
    wkCACFContextFlush(m_context);
    contextDidChange();
}

void LegacyCACFLayerTreeHost::contextDidChange()
{
    renderSoon();
    CACFLayerTreeHost::contextDidChange();
}

CFTimeInterval LegacyCACFLayerTreeHost::lastCommitTime() const
{
    return wkCACFContextGetLastCommitTime(m_context);
}

void LegacyCACFLayerTreeHost::initD3DGeometry()
{
    ASSERT(m_d3dDevice);

    CGRect bounds = this->bounds();

    float x0 = bounds.origin.x;
    float y0 = bounds.origin.y;
    float x1 = x0 + bounds.size.width;
    float y1 = y0 + bounds.size.height;

    D3DXMATRIXA16 projection;
    D3DXMatrixOrthoOffCenterRH(&projection, x0, x1, y0, y1, -1.0f, 1.0f);

    m_d3dDevice->SetTransform(D3DTS_PROJECTION, &projection);
}

bool LegacyCACFLayerTreeHost::resetDevice(ResetReason reason)
{
    ASSERT(m_d3dDevice);
    ASSERT(m_context);

    HRESULT hr = m_d3dDevice->TestCooperativeLevel();

    if (hr == D3DERR_DEVICELOST || hr == D3DERR_DRIVERINTERNALERROR) {
        // The device cannot be reset at this time. Try again soon.
        m_mustResetLostDeviceBeforeRendering = true;
        return false;
    }

    m_mustResetLostDeviceBeforeRendering = false;

    if (reason == LostDevice && hr == D3D_OK) {
        // The device wasn't lost after all.
        return true;
    }

    // We can reset the device.

    // We have to release the context's D3D resrouces whenever we reset the IDirect3DDevice9 in order to
    // destroy any D3DPOOL_DEFAULT resources that Core Animation has allocated (e.g., textures used
    // for mask layers). See <http://msdn.microsoft.com/en-us/library/bb174425(v=VS.85).aspx>.
    wkCACFContextReleaseD3DResources(m_context);

    D3DPRESENT_PARAMETERS parameters = initialPresentationParameters();
    hr = m_d3dDevice->Reset(&parameters);

    // TestCooperativeLevel told us the device may be reset now, so we should
    // not be told here that the device is lost.
    ASSERT(hr != D3DERR_DEVICELOST);

    initD3DGeometry();

    return true;
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)
