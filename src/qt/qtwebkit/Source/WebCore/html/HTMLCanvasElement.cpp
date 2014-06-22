/*
 * Copyright (C) 2004, 2006, 2007 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2010 Torch Mobile (Beijing) Co. Ltd. All rights reserved.
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
#include "HTMLCanvasElement.h"

#include "Attribute.h"
#include "CanvasContextAttributes.h"
#include "CanvasGradient.h"
#include "CanvasPattern.h"
#include "CanvasRenderingContext2D.h"
#include "Chrome.h"
#include "Document.h"
#include "ExceptionCode.h"
#include "Frame.h"
#include "GraphicsContext.h"
#include "HTMLNames.h"
#include "ImageBuffer.h"
#include "ImageData.h"
#include "MIMETypeRegistry.h"
#include "Page.h"
#include "RenderHTMLCanvas.h"
#include "ScriptController.h"
#include "Settings.h"
#include <math.h>

#include <runtime/JSLock.h>
#include <runtime/Operations.h>

#if ENABLE(WEBGL)    
#include "WebGLContextAttributes.h"
#include "WebGLRenderingContext.h"
#endif

namespace WebCore {

using namespace HTMLNames;

// These values come from the WhatWG spec.
static const int DefaultWidth = 300;
static const int DefaultHeight = 150;

// Firefox limits width/height to 32767 pixels, but slows down dramatically before it
// reaches that limit. We limit by area instead, giving us larger maximum dimensions,
// in exchange for a smaller maximum canvas size.
static const float MaxCanvasArea = 32768 * 8192; // Maximum canvas area in CSS pixels

HTMLCanvasElement::HTMLCanvasElement(const QualifiedName& tagName, Document* document)
    : HTMLElement(tagName, document)
    , m_size(DefaultWidth, DefaultHeight)
    , m_rendererIsCanvas(false)
    , m_ignoreReset(false)
    , m_deviceScaleFactor(targetDeviceScaleFactor())
    , m_originClean(true)
    , m_hasCreatedImageBuffer(false)
    , m_didClearImageBuffer(false)
{
    ASSERT(hasTagName(canvasTag));
}

PassRefPtr<HTMLCanvasElement> HTMLCanvasElement::create(Document* document)
{
    return adoptRef(new HTMLCanvasElement(canvasTag, document));
}

PassRefPtr<HTMLCanvasElement> HTMLCanvasElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLCanvasElement(tagName, document));
}

HTMLCanvasElement::~HTMLCanvasElement()
{
    HashSet<CanvasObserver*>::iterator end = m_observers.end();
    for (HashSet<CanvasObserver*>::iterator it = m_observers.begin(); it != end; ++it)
        (*it)->canvasDestroyed(this);

    m_context.clear(); // Ensure this goes away before the ImageBuffer.
}

void HTMLCanvasElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == widthAttr || name == heightAttr)
        reset();
    HTMLElement::parseAttribute(name, value);
}

RenderObject* HTMLCanvasElement::createRenderer(RenderArena* arena, RenderStyle* style)
{
    Frame* frame = document()->frame();
    if (frame && frame->script()->canExecuteScripts(NotAboutToExecuteScript)) {
        m_rendererIsCanvas = true;
        return new (arena) RenderHTMLCanvas(this);
    }

    m_rendererIsCanvas = false;
    return HTMLElement::createRenderer(arena, style);
}

void HTMLCanvasElement::attach(const AttachContext& context)
{
    setIsInCanvasSubtree(true);
    HTMLElement::attach(context);
}

bool HTMLCanvasElement::areAuthorShadowsAllowed() const
{
    return false;
}

bool HTMLCanvasElement::canContainRangeEndPoint() const
{
    return false;
}

bool HTMLCanvasElement::canStartSelection() const
{
    return false;
}

void HTMLCanvasElement::addObserver(CanvasObserver* observer)
{
    m_observers.add(observer);
}

void HTMLCanvasElement::removeObserver(CanvasObserver* observer)
{
    m_observers.remove(observer);
}

void HTMLCanvasElement::setHeight(int value)
{
    setAttribute(heightAttr, String::number(value));
}

void HTMLCanvasElement::setWidth(int value)
{
    setAttribute(widthAttr, String::number(value));
}

#if ENABLE(WEBGL)
static bool requiresAcceleratedCompositingForWebGL()
{
#if PLATFORM(GTK) || PLATFORM(EFL) || PLATFORM(QT)
    return false;
#else
    return true;
#endif

}
static bool shouldEnableWebGL(Settings* settings)
{
    if (!settings)
        return false;

    if (!settings->webGLEnabled())
        return false;

    if (!requiresAcceleratedCompositingForWebGL())
        return true;

    return settings->acceleratedCompositingEnabled();
}
#endif

CanvasRenderingContext* HTMLCanvasElement::getContext(const String& type, CanvasContextAttributes* attrs)
{
    // A Canvas can either be "2D" or "webgl" but never both. If you request a 2D canvas and the existing
    // context is already 2D, just return that. If the existing context is WebGL, then destroy it
    // before creating a new 2D context. Vice versa when requesting a WebGL canvas. Requesting a
    // context with any other type string will destroy any existing context.
    
    // FIXME: The code depends on the context not going away once created, to prevent JS from
    // seeing a dangling pointer. So for now we will disallow the context from being changed
    // once it is created. https://bugs.webkit.org/show_bug.cgi?id=117095
    if (is2dType(type)) {
        if (m_context && !m_context->is2d())
            return 0;
        if (!m_context) {
            bool usesDashbardCompatibilityMode = false;
#if ENABLE(DASHBOARD_SUPPORT)
            if (Settings* settings = document()->settings())
                usesDashbardCompatibilityMode = settings->usesDashboardBackwardCompatibilityMode();
#endif
            m_context = CanvasRenderingContext2D::create(this, document()->inQuirksMode(), usesDashbardCompatibilityMode);
#if USE(IOSURFACE_CANVAS_BACKING_STORE) || (ENABLE(ACCELERATED_2D_CANVAS) && USE(ACCELERATED_COMPOSITING))
            // Need to make sure a RenderLayer and compositing layer get created for the Canvas
            setNeedsStyleRecalc(SyntheticStyleChange);
#endif
        }
        return m_context.get();
    }
#if ENABLE(WEBGL)
    if (shouldEnableWebGL(document()->settings())) {

        if (is3dType(type)) {
            if (m_context && !m_context->is3d())
                return 0;
            if (!m_context) {
                m_context = WebGLRenderingContext::create(this, static_cast<WebGLContextAttributes*>(attrs));
                if (m_context) {
                    // Need to make sure a RenderLayer and compositing layer get created for the Canvas
                    setNeedsStyleRecalc(SyntheticStyleChange);
                }
            }
            return m_context.get();
        }
    }
#else
    UNUSED_PARAM(attrs);
#endif
    return 0;
}
    
bool HTMLCanvasElement::supportsContext(const String& type, CanvasContextAttributes*)
{
    // FIXME: Provide implementation that accounts for attributes. Bugzilla bug 117093
    // https://bugs.webkit.org/show_bug.cgi?id=117093

    // FIXME: The code depends on the context not going away once created (as getContext
    // is implemented under this assumption) https://bugs.webkit.org/show_bug.cgi?id=117095
    if (is2dType(type))
        return !m_context || m_context->is2d();

#if ENABLE(WEBGL)
    if (shouldEnableWebGL(document()->settings())) {
        if (is3dType(type))
            return !m_context || m_context->is3d();
    }
#endif
    return false;
}

bool HTMLCanvasElement::is2dType(const String& type)
{
    return type == "2d";
}

#if ENABLE(WEBGL)
bool HTMLCanvasElement::is3dType(const String& type)
{
    // Accept the legacy "webkit-3d" name as well as the provisional "experimental-webgl" name.
    return type == "webkit-3d" || type == "experimental-webgl";
}
#endif

void HTMLCanvasElement::didDraw(const FloatRect& rect)
{
    clearCopiedImage();

    if (RenderBox* ro = renderBox()) {
        FloatRect destRect = ro->contentBoxRect();
        FloatRect r = mapRect(rect, FloatRect(0, 0, size().width(), size().height()), destRect);
        r.intersect(destRect);
        if (r.isEmpty() || m_dirtyRect.contains(r))
            return;

        m_dirtyRect.unite(r);
        ro->repaintRectangle(enclosingIntRect(m_dirtyRect));
    }

    notifyObserversCanvasChanged(rect);
}

void HTMLCanvasElement::notifyObserversCanvasChanged(const FloatRect& rect)
{
    HashSet<CanvasObserver*>::iterator end = m_observers.end();
    for (HashSet<CanvasObserver*>::iterator it = m_observers.begin(); it != end; ++it)
        (*it)->canvasChanged(this, rect);
}

void HTMLCanvasElement::reset()
{
    if (m_ignoreReset)
        return;

    bool ok;
    bool hadImageBuffer = hasCreatedImageBuffer();

    int w = getAttribute(widthAttr).toInt(&ok);
    if (!ok || w < 0)
        w = DefaultWidth;

    int h = getAttribute(heightAttr).toInt(&ok);
    if (!ok || h < 0)
        h = DefaultHeight;

    if (m_contextStateSaver) {
        // Reset to the initial graphics context state.
        m_contextStateSaver->restore();
        m_contextStateSaver->save();
    }

    if (m_context && m_context->is2d()) {
        CanvasRenderingContext2D* context2D = static_cast<CanvasRenderingContext2D*>(m_context.get());
        context2D->reset();
    }

    IntSize oldSize = size();
    IntSize newSize(w, h);
    float newDeviceScaleFactor = targetDeviceScaleFactor();

    // If the size of an existing buffer matches, we can just clear it instead of reallocating.
    // This optimization is only done for 2D canvases for now.
    if (m_hasCreatedImageBuffer && oldSize == newSize && m_deviceScaleFactor == newDeviceScaleFactor && m_context && m_context->is2d()) {
        if (!m_didClearImageBuffer)
            clearImageBuffer();
        return;
    }

    m_deviceScaleFactor = newDeviceScaleFactor;

    setSurfaceSize(newSize);

#if ENABLE(WEBGL)
    if (m_context && m_context->is3d() && oldSize != size())
        static_cast<WebGLRenderingContext*>(m_context.get())->reshape(width(), height());
#endif

    if (RenderObject* renderer = this->renderer()) {
        if (m_rendererIsCanvas) {
            if (oldSize != size()) {
                toRenderHTMLCanvas(renderer)->canvasSizeChanged();
#if USE(ACCELERATED_COMPOSITING)
                if (renderBox() && renderBox()->hasAcceleratedCompositing())
                    renderBox()->contentChanged(CanvasChanged);
#endif
            }
            if (hadImageBuffer)
                renderer->repaint();
        }
    }

    HashSet<CanvasObserver*>::iterator end = m_observers.end();
    for (HashSet<CanvasObserver*>::iterator it = m_observers.begin(); it != end; ++it)
        (*it)->canvasResized(this);
}

float HTMLCanvasElement::targetDeviceScaleFactor() const
{
#if ENABLE(HIGH_DPI_CANVAS)
    return document()->frame() ? document()->frame()->page()->deviceScaleFactor() : 1;
#else
    return 1;
#endif
}

bool HTMLCanvasElement::paintsIntoCanvasBuffer() const
{
    ASSERT(m_context);
#if USE(IOSURFACE_CANVAS_BACKING_STORE)
    if (m_context->is2d())
        return true;
#endif

#if USE(ACCELERATED_COMPOSITING)
    if (!m_context->isAccelerated())
        return true;

    if (renderBox() && renderBox()->hasAcceleratedCompositing())
        return false;
#endif
    return true;
}


void HTMLCanvasElement::paint(GraphicsContext* context, const LayoutRect& r, bool useLowQualityScale)
{
    // Clear the dirty rect
    m_dirtyRect = FloatRect();

    if (context->paintingDisabled())
        return;
    
    if (m_context) {
        if (!paintsIntoCanvasBuffer() && !document()->printing())
            return;
        m_context->paintRenderingResultsToCanvas();
    }

    if (hasCreatedImageBuffer()) {
        ImageBuffer* imageBuffer = buffer();
        if (imageBuffer) {
            if (m_presentedImage)
                context->drawImage(m_presentedImage.get(), ColorSpaceDeviceRGB, pixelSnappedIntRect(r), CompositeSourceOver, DoNotRespectImageOrientation, useLowQualityScale);
            else
                context->drawImageBuffer(imageBuffer, ColorSpaceDeviceRGB, pixelSnappedIntRect(r), CompositeSourceOver, BlendModeNormal, useLowQualityScale);
        }
    }

#if ENABLE(WEBGL)    
    if (is3D())
        static_cast<WebGLRenderingContext*>(m_context.get())->markLayerComposited();
#endif
}

#if ENABLE(WEBGL)
bool HTMLCanvasElement::is3D() const
{
    return m_context && m_context->is3d();
}
#endif

void HTMLCanvasElement::makeRenderingResultsAvailable()
{
    if (m_context)
        m_context->paintRenderingResultsToCanvas();
}

void HTMLCanvasElement::makePresentationCopy()
{
    if (!m_presentedImage) {
        // The buffer contains the last presented data, so save a copy of it.
        m_presentedImage = buffer()->copyImage(CopyBackingStore, Unscaled);
    }
}

void HTMLCanvasElement::clearPresentationCopy()
{
    m_presentedImage.clear();
}

void HTMLCanvasElement::setSurfaceSize(const IntSize& size)
{
    m_size = size;
    m_hasCreatedImageBuffer = false;
    m_contextStateSaver.clear();
    m_imageBuffer.clear();
    clearCopiedImage();
}

String HTMLCanvasElement::toEncodingMimeType(const String& mimeType)
{
    String lowercaseMimeType = mimeType.lower();

    // FIXME: Make isSupportedImageMIMETypeForEncoding threadsafe (to allow this method to be used on a worker thread).
    if (mimeType.isNull() || !MIMETypeRegistry::isSupportedImageMIMETypeForEncoding(lowercaseMimeType))
        lowercaseMimeType = "image/png";

    return lowercaseMimeType;
}

String HTMLCanvasElement::toDataURL(const String& mimeType, const double* quality, ExceptionCode& ec)
{
    if (!m_originClean) {
        ec = SECURITY_ERR;
        return String();
    }

    if (m_size.isEmpty() || !buffer())
        return String("data:,");

    String encodingMimeType = toEncodingMimeType(mimeType);

#if USE(CG)
    // Try to get ImageData first, as that may avoid lossy conversions.
    RefPtr<ImageData> imageData = getImageData();

    if (imageData)
        return ImageDataToDataURL(*imageData, encodingMimeType, quality);
#endif

    makeRenderingResultsAvailable();

    return buffer()->toDataURL(encodingMimeType, quality);
}

PassRefPtr<ImageData> HTMLCanvasElement::getImageData()
{
    if (!m_context || !m_context->is3d())
       return 0;

#if ENABLE(WEBGL)    
    WebGLRenderingContext* ctx = static_cast<WebGLRenderingContext*>(m_context.get());

    return ctx->paintRenderingResultsToImageData();
#else
    return 0;
#endif
}

FloatRect HTMLCanvasElement::convertLogicalToDevice(const FloatRect& logicalRect) const
{
    FloatRect deviceRect(logicalRect);
    deviceRect.scale(m_deviceScaleFactor);

    float x = floorf(deviceRect.x());
    float y = floorf(deviceRect.y());
    float w = ceilf(deviceRect.maxX() - x);
    float h = ceilf(deviceRect.maxY() - y);
    deviceRect.setX(x);
    deviceRect.setY(y);
    deviceRect.setWidth(w);
    deviceRect.setHeight(h);

    return deviceRect;
}

FloatSize HTMLCanvasElement::convertLogicalToDevice(const FloatSize& logicalSize) const
{
    float width = ceilf(logicalSize.width() * m_deviceScaleFactor);
    float height = ceilf(logicalSize.height() * m_deviceScaleFactor);
    return FloatSize(width, height);
}

FloatSize HTMLCanvasElement::convertDeviceToLogical(const FloatSize& deviceSize) const
{
    float width = ceilf(deviceSize.width() / m_deviceScaleFactor);
    float height = ceilf(deviceSize.height() / m_deviceScaleFactor);
    return FloatSize(width, height);
}

SecurityOrigin* HTMLCanvasElement::securityOrigin() const
{
    return document()->securityOrigin();
}

bool HTMLCanvasElement::shouldAccelerate(const IntSize& size) const
{
#if USE(IOSURFACE_CANVAS_BACKING_STORE)
    UNUSED_PARAM(size);
    return document()->settings() && document()->settings()->canvasUsesAcceleratedDrawing();
#elif ENABLE(ACCELERATED_2D_CANVAS)
    if (m_context && !m_context->is2d())
        return false;

    Settings* settings = document()->settings();
    if (!settings || !settings->accelerated2dCanvasEnabled())
        return false;

    // Do not use acceleration for small canvas.
    if (size.width() * size.height() < settings->minimumAccelerated2dCanvasSize())
        return false;

    return true;
#else
    UNUSED_PARAM(size);
    return false;
#endif
}

void HTMLCanvasElement::createImageBuffer() const
{
    ASSERT(!m_imageBuffer);

    m_hasCreatedImageBuffer = true;
    m_didClearImageBuffer = true;

    FloatSize logicalSize = size();
    FloatSize deviceSize = convertLogicalToDevice(logicalSize);
    if (!deviceSize.isExpressibleAsIntSize())
        return;

    if (deviceSize.width() * deviceSize.height() > MaxCanvasArea)
        return;

    IntSize bufferSize(deviceSize.width(), deviceSize.height());
    if (!bufferSize.width() || !bufferSize.height())
        return;

    RenderingMode renderingMode = shouldAccelerate(bufferSize) ? Accelerated : Unaccelerated;
    m_imageBuffer = ImageBuffer::create(size(), m_deviceScaleFactor, ColorSpaceDeviceRGB, renderingMode);
    if (!m_imageBuffer)
        return;
    m_imageBuffer->context()->setShadowsIgnoreTransforms(true);
    m_imageBuffer->context()->setImageInterpolationQuality(DefaultInterpolationQuality);
    if (document()->settings() && !document()->settings()->antialiased2dCanvasEnabled())
        m_imageBuffer->context()->setShouldAntialias(false);
    m_imageBuffer->context()->setStrokeThickness(1);
    m_contextStateSaver = adoptPtr(new GraphicsContextStateSaver(*m_imageBuffer->context()));

    JSC::JSLockHolder lock(scriptExecutionContext()->vm());
    size_t numBytes = 4 * m_imageBuffer->internalSize().width() * m_imageBuffer->internalSize().height();
    scriptExecutionContext()->vm()->heap.reportExtraMemoryCost(numBytes);

#if USE(IOSURFACE_CANVAS_BACKING_STORE) || (ENABLE(ACCELERATED_2D_CANVAS) && USE(ACCELERATED_COMPOSITING))
    if (m_context && m_context->is2d())
        // Recalculate compositing requirements if acceleration state changed.
        const_cast<HTMLCanvasElement*>(this)->setNeedsStyleRecalc(SyntheticStyleChange);
#endif
}

GraphicsContext* HTMLCanvasElement::drawingContext() const
{
    return buffer() ? m_imageBuffer->context() : 0;
}

GraphicsContext* HTMLCanvasElement::existingDrawingContext() const
{
    if (!m_hasCreatedImageBuffer)
        return 0;

    return drawingContext();
}

ImageBuffer* HTMLCanvasElement::buffer() const
{
    if (!m_hasCreatedImageBuffer)
        createImageBuffer();
    return m_imageBuffer.get();
}

Image* HTMLCanvasElement::copiedImage() const
{
    if (!m_copiedImage && buffer()) {
        if (m_context)
            m_context->paintRenderingResultsToCanvas();
        m_copiedImage = buffer()->copyImage(CopyBackingStore, Unscaled);
    }
    return m_copiedImage.get();
}

void HTMLCanvasElement::clearImageBuffer() const
{
    ASSERT(m_hasCreatedImageBuffer);
    ASSERT(!m_didClearImageBuffer);
    ASSERT(m_context);

    m_didClearImageBuffer = true;

    if (m_context->is2d()) {
        CanvasRenderingContext2D* context2D = static_cast<CanvasRenderingContext2D*>(m_context.get());
        // No need to undo transforms/clip/etc. because we are called right after the context is reset.
        context2D->clearRect(0, 0, width(), height());
    }
}

void HTMLCanvasElement::clearCopiedImage()
{
    m_copiedImage.clear();
    m_didClearImageBuffer = false;
}

AffineTransform HTMLCanvasElement::baseTransform() const
{
    ASSERT(m_hasCreatedImageBuffer);
    FloatSize unscaledSize = size();
    FloatSize deviceSize = convertLogicalToDevice(unscaledSize);
    IntSize size(deviceSize.width(), deviceSize.height());
    AffineTransform transform;
    if (size.width() && size.height())
        transform.scaleNonUniform(size.width() / unscaledSize.width(), size.height() / unscaledSize.height());
    return m_imageBuffer->baseTransform() * transform;
}

}
