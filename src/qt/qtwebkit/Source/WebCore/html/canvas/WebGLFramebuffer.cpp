/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#if ENABLE(WEBGL)

#include "WebGLFramebuffer.h"

#include "EXTDrawBuffers.h"
#include "Extensions3D.h"
#include "WebGLContextGroup.h"
#include "WebGLRenderingContext.h"

namespace WebCore {

namespace {

    Platform3DObject objectOrZero(WebGLObject* object)
    {
        return object ? object->object() : 0;
    }

    class WebGLRenderbufferAttachment : public WebGLFramebuffer::WebGLAttachment {
    public:
        static PassRefPtr<WebGLFramebuffer::WebGLAttachment> create(WebGLRenderbuffer*);

    private:
        WebGLRenderbufferAttachment(WebGLRenderbuffer*);
        virtual GC3Dsizei getWidth() const;
        virtual GC3Dsizei getHeight() const;
        virtual GC3Denum getFormat() const;
        virtual WebGLSharedObject* getObject() const;
        virtual bool isSharedObject(WebGLSharedObject*) const;
        virtual bool isValid() const;
        virtual bool isInitialized() const;
        virtual void setInitialized();
        virtual void onDetached(GraphicsContext3D*);
        virtual void attach(GraphicsContext3D*, GC3Denum attachment);
        virtual void unattach(GraphicsContext3D*, GC3Denum attachment);

        WebGLRenderbufferAttachment() { };

        RefPtr<WebGLRenderbuffer> m_renderbuffer;
    };

    PassRefPtr<WebGLFramebuffer::WebGLAttachment> WebGLRenderbufferAttachment::create(WebGLRenderbuffer* renderbuffer)
    {
        return adoptRef(new WebGLRenderbufferAttachment(renderbuffer));
    }

    WebGLRenderbufferAttachment::WebGLRenderbufferAttachment(WebGLRenderbuffer* renderbuffer)
        : m_renderbuffer(renderbuffer)
    {
    }

    GC3Dsizei WebGLRenderbufferAttachment::getWidth() const
    {
        return m_renderbuffer->getWidth();
    }

    GC3Dsizei WebGLRenderbufferAttachment::getHeight() const
    {
        return m_renderbuffer->getHeight();
    }

    GC3Denum WebGLRenderbufferAttachment::getFormat() const
    {
        return m_renderbuffer->getInternalFormat();
    }

    WebGLSharedObject* WebGLRenderbufferAttachment::getObject() const
    {
        return m_renderbuffer->object() ? m_renderbuffer.get() : 0;
    }

    bool WebGLRenderbufferAttachment::isSharedObject(WebGLSharedObject* object) const
    {
        return object == m_renderbuffer;
    }

    bool WebGLRenderbufferAttachment::isValid() const
    {
        return m_renderbuffer->object();
    }

    bool WebGLRenderbufferAttachment::isInitialized() const
    {
        return m_renderbuffer->object() && m_renderbuffer->isInitialized();
    }

    void WebGLRenderbufferAttachment::setInitialized()
    {
        if (m_renderbuffer->object())
            m_renderbuffer->setInitialized();
    }

    void WebGLRenderbufferAttachment::onDetached(GraphicsContext3D* context)
    {
        m_renderbuffer->onDetached(context);
    }

    void WebGLRenderbufferAttachment::attach(GraphicsContext3D* context, GC3Denum attachment)
    {
        Platform3DObject object = objectOrZero(m_renderbuffer.get());
        context->framebufferRenderbuffer(GraphicsContext3D::FRAMEBUFFER, attachment, GraphicsContext3D::RENDERBUFFER, object);
    }

    void WebGLRenderbufferAttachment::unattach(GraphicsContext3D* context, GC3Denum attachment)
    {
        if (attachment == GraphicsContext3D::DEPTH_STENCIL_ATTACHMENT) {
            context->framebufferRenderbuffer(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::DEPTH_ATTACHMENT, GraphicsContext3D::RENDERBUFFER, 0);
            context->framebufferRenderbuffer(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::STENCIL_ATTACHMENT, GraphicsContext3D::RENDERBUFFER, 0);
        } else
            context->framebufferRenderbuffer(GraphicsContext3D::FRAMEBUFFER, attachment, GraphicsContext3D::RENDERBUFFER, 0);
    }

    class WebGLTextureAttachment : public WebGLFramebuffer::WebGLAttachment {
    public:
        static PassRefPtr<WebGLFramebuffer::WebGLAttachment> create(WebGLTexture*, GC3Denum target, GC3Dint level);

    private:
        WebGLTextureAttachment(WebGLTexture*, GC3Denum target, GC3Dint level);
        virtual GC3Dsizei getWidth() const;
        virtual GC3Dsizei getHeight() const;
        virtual GC3Denum getFormat() const;
        virtual WebGLSharedObject* getObject() const;
        virtual bool isSharedObject(WebGLSharedObject*) const;
        virtual bool isValid() const;
        virtual bool isInitialized() const;
        virtual void setInitialized();
        virtual void onDetached(GraphicsContext3D*);
        virtual void attach(GraphicsContext3D*, GC3Denum attachment);
        virtual void unattach(GraphicsContext3D*, GC3Denum attachment);

        WebGLTextureAttachment() { };

        RefPtr<WebGLTexture> m_texture;
        GC3Denum m_target;
        GC3Dint m_level;
    };

    PassRefPtr<WebGLFramebuffer::WebGLAttachment> WebGLTextureAttachment::create(WebGLTexture* texture, GC3Denum target, GC3Dint level)
    {
        return adoptRef(new WebGLTextureAttachment(texture, target, level));
    }

    WebGLTextureAttachment::WebGLTextureAttachment(WebGLTexture* texture, GC3Denum target, GC3Dint level)
        : m_texture(texture)
        , m_target(target)
        , m_level(level)
    {
    }

    GC3Dsizei WebGLTextureAttachment::getWidth() const
    {
        return m_texture->getWidth(m_target, m_level);
    }

    GC3Dsizei WebGLTextureAttachment::getHeight() const
    {
        return m_texture->getHeight(m_target, m_level);
    }

    GC3Denum WebGLTextureAttachment::getFormat() const
    {
        return m_texture->getInternalFormat(m_target, m_level);
    }

    WebGLSharedObject* WebGLTextureAttachment::getObject() const
    {
        return m_texture->object() ? m_texture.get() : 0;
    }

    bool WebGLTextureAttachment::isSharedObject(WebGLSharedObject* object) const
    {
        return object == m_texture;
    }

    bool WebGLTextureAttachment::isValid() const
    {
        return m_texture->object();
    }

    bool WebGLTextureAttachment::isInitialized() const
    {
        // Textures are assumed to be initialized.
        return true;
    }

    void WebGLTextureAttachment::setInitialized()
    {
        // Textures are assumed to be initialized.
    }

    void WebGLTextureAttachment::onDetached(GraphicsContext3D* context)
    {
        m_texture->onDetached(context);
    }

    void WebGLTextureAttachment::attach(GraphicsContext3D* context, GC3Denum attachment)
    {
        Platform3DObject object = objectOrZero(m_texture.get());
        context->framebufferTexture2D(GraphicsContext3D::FRAMEBUFFER, attachment, m_target, object, m_level);
    }

    void WebGLTextureAttachment::unattach(GraphicsContext3D* context, GC3Denum attachment)
    {
        if (attachment == GraphicsContext3D::DEPTH_STENCIL_ATTACHMENT) {
            context->framebufferTexture2D(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::DEPTH_ATTACHMENT, m_target, 0, m_level);
            context->framebufferTexture2D(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::STENCIL_ATTACHMENT, m_target, 0, m_level);
        } else
            context->framebufferTexture2D(GraphicsContext3D::FRAMEBUFFER, attachment, m_target, 0, m_level);
    }

    bool isAttachmentComplete(WebGLFramebuffer::WebGLAttachment* attachedObject, GC3Denum attachment, const char** reason)
    {
        ASSERT(attachedObject && attachedObject->isValid());
        ASSERT(reason);
        GC3Denum format = attachedObject->getFormat();
        unsigned need = GraphicsContext3D::getClearBitsByAttachmentType(attachment);
        unsigned have = GraphicsContext3D::getClearBitsByFormat(format);

        if ((need & have) != need) {
            *reason = "attachment type is not correct for attachment";
            return false;
        }
        if (!attachedObject->getWidth() || !attachedObject->getHeight()) {
            *reason = "attachment has a 0 dimension";
            return false;
        }
        if ((attachment == GraphicsContext3D::DEPTH_ATTACHMENT || attachment == GraphicsContext3D::STENCIL_ATTACHMENT)
            && format == GraphicsContext3D::DEPTH_STENCIL) {
          *reason = "attachment DEPTH_STENCIL not allowed on DEPTH or STENCIL attachment";
          return false;
        }
        return true;
    }

} // anonymous namespace

WebGLFramebuffer::WebGLAttachment::WebGLAttachment()
{
}

WebGLFramebuffer::WebGLAttachment::~WebGLAttachment()
{
}

PassRefPtr<WebGLFramebuffer> WebGLFramebuffer::create(WebGLRenderingContext* ctx)
{
    return adoptRef(new WebGLFramebuffer(ctx));
}

WebGLFramebuffer::WebGLFramebuffer(WebGLRenderingContext* ctx)
    : WebGLContextObject(ctx)
    , m_hasEverBeenBound(false)
{
    setObject(ctx->graphicsContext3D()->createFramebuffer());
}

WebGLFramebuffer::~WebGLFramebuffer()
{
    deleteObject(0);
}

void WebGLFramebuffer::setAttachmentForBoundFramebuffer(GC3Denum attachment, GC3Denum texTarget, WebGLTexture* texture, GC3Dint level)
{
    ASSERT(isBound());
    removeAttachmentFromBoundFramebuffer(attachment);
    if (!object())
        return;
    if (texture && texture->object()) {
        m_attachments.add(attachment, WebGLTextureAttachment::create(texture, texTarget, level));
        drawBuffersIfNecessary(false);
        texture->onAttached();
    }
}

void WebGLFramebuffer::setAttachmentForBoundFramebuffer(GC3Denum attachment, WebGLRenderbuffer* renderbuffer)
{
    ASSERT(isBound());
    removeAttachmentFromBoundFramebuffer(attachment);
    if (!object())
        return;
    if (renderbuffer && renderbuffer->object()) {
        m_attachments.add(attachment, WebGLRenderbufferAttachment::create(renderbuffer));
        drawBuffersIfNecessary(false);
        renderbuffer->onAttached();
    }
}

void WebGLFramebuffer::attach(GC3Denum attachment, GC3Denum attachmentPoint)
{
    ASSERT(isBound());
    WebGLAttachment* attachmentObject = getAttachment(attachment);
    if (attachmentObject)
        attachmentObject->attach(context()->graphicsContext3D(), attachmentPoint);
}

WebGLSharedObject* WebGLFramebuffer::getAttachmentObject(GC3Denum attachment) const
{
    if (!object())
        return 0;
    WebGLAttachment* attachmentObject = getAttachment(attachment);
    return attachmentObject ? attachmentObject->getObject() : 0;
}

WebGLFramebuffer::WebGLAttachment* WebGLFramebuffer::getAttachment(GC3Denum attachment) const
{
    const AttachmentMap::const_iterator it = m_attachments.find(attachment);
    return (it != m_attachments.end()) ? it->value.get() : 0;
}

void WebGLFramebuffer::removeAttachmentFromBoundFramebuffer(GC3Denum attachment)
{
    ASSERT(isBound());
    if (!object())
        return;

    WebGLAttachment* attachmentObject = getAttachment(attachment);
    if (attachmentObject) {
        attachmentObject->onDetached(context()->graphicsContext3D());
        m_attachments.remove(attachment);
        drawBuffersIfNecessary(false);
        switch (attachment) {
        case GraphicsContext3D::DEPTH_STENCIL_ATTACHMENT:
            attach(GraphicsContext3D::DEPTH_ATTACHMENT, GraphicsContext3D::DEPTH_ATTACHMENT);
            attach(GraphicsContext3D::STENCIL_ATTACHMENT, GraphicsContext3D::STENCIL_ATTACHMENT);
            break;
        case GraphicsContext3D::DEPTH_ATTACHMENT:
            attach(GraphicsContext3D::DEPTH_STENCIL_ATTACHMENT, GraphicsContext3D::DEPTH_ATTACHMENT);
            break;
        case GraphicsContext3D::STENCIL_ATTACHMENT:
            attach(GraphicsContext3D::DEPTH_STENCIL_ATTACHMENT, GraphicsContext3D::STENCIL_ATTACHMENT);
            break;
        }
    }
}

void WebGLFramebuffer::removeAttachmentFromBoundFramebuffer(WebGLSharedObject* attachment)
{
    ASSERT(isBound());
    if (!object())
        return;
    if (!attachment)
        return;

    bool checkMore = true;
    while (checkMore) {
        checkMore = false;
        for (AttachmentMap::iterator it = m_attachments.begin(); it != m_attachments.end(); ++it) {
            WebGLAttachment* attachmentObject = it->value.get();
            if (attachmentObject->isSharedObject(attachment)) {
                GC3Denum attachmentType = it->key;
                attachmentObject->unattach(context()->graphicsContext3D(), attachmentType);
                removeAttachmentFromBoundFramebuffer(attachmentType);
                checkMore = true;
                break;
            }
        }
    }
}

GC3Dsizei WebGLFramebuffer::getColorBufferWidth() const
{
    if (!object())
        return 0;
    WebGLAttachment* attachment = getAttachment(GraphicsContext3D::COLOR_ATTACHMENT0);
    if (!attachment)
        return 0;

    return attachment->getWidth();
}

GC3Dsizei WebGLFramebuffer::getColorBufferHeight() const
{
    if (!object())
        return 0;
    WebGLAttachment* attachment = getAttachment(GraphicsContext3D::COLOR_ATTACHMENT0);
    if (!attachment)
        return 0;

    return attachment->getHeight();
}

GC3Denum WebGLFramebuffer::getColorBufferFormat() const
{
    if (!object())
        return 0;
    WebGLAttachment* attachment = getAttachment(GraphicsContext3D::COLOR_ATTACHMENT0);
    if (!attachment)
        return 0;
    return attachment->getFormat();
}

GC3Denum WebGLFramebuffer::checkStatus(const char** reason) const
{
    unsigned int count = 0;
    GC3Dsizei width = 0, height = 0;
    bool haveDepth = false;
    bool haveStencil = false;
    bool haveDepthStencil = false;
    for (AttachmentMap::const_iterator it = m_attachments.begin(); it != m_attachments.end(); ++it) {
        WebGLAttachment* attachment = it->value.get();
        if (!isAttachmentComplete(attachment, it->key, reason))
            return GraphicsContext3D::FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
        if (!attachment->isValid()) {
            *reason = "attachment is not valid";
            return GraphicsContext3D::FRAMEBUFFER_UNSUPPORTED;
        }
        if (!attachment->getFormat()) {
            *reason = "attachment is an unsupported format";
            return GraphicsContext3D::FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
        }
        switch (it->key) {
        case GraphicsContext3D::DEPTH_ATTACHMENT:
            haveDepth = true;
            break;
        case GraphicsContext3D::STENCIL_ATTACHMENT:
            haveStencil = true;
            break;
        case GraphicsContext3D::DEPTH_STENCIL_ATTACHMENT:
            haveDepthStencil = true;
            break;
        }
        if (!count) {
            width = attachment->getWidth();
            height = attachment->getHeight();
        } else {
            if (width != attachment->getWidth() || height != attachment->getHeight()) {
                *reason = "attachments do not have the same dimensions";
                return GraphicsContext3D::FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
            }
        }
        ++count;
    }
    if (!count) {
        *reason = "no attachments";
        return GraphicsContext3D::FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
    }
    if (!width || !height) {
        *reason = "framebuffer has a 0 dimension";
        return GraphicsContext3D::FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
    }
    // WebGL specific: no conflicting DEPTH/STENCIL/DEPTH_STENCIL attachments.
    if ((haveDepthStencil && (haveDepth || haveStencil)) || (haveDepth && haveStencil)) {
        *reason = "conflicting DEPTH/STENCIL/DEPTH_STENCIL attachments";
        return GraphicsContext3D::FRAMEBUFFER_UNSUPPORTED;
    }
    return GraphicsContext3D::FRAMEBUFFER_COMPLETE;
}

bool WebGLFramebuffer::onAccess(GraphicsContext3D* context3d, bool needToInitializeAttachments, const char** reason)
{
    if (checkStatus(reason) != GraphicsContext3D::FRAMEBUFFER_COMPLETE)
        return false;
    if (needToInitializeAttachments)
        return initializeAttachments(context3d, reason);
    return true;
}

bool WebGLFramebuffer::hasStencilBuffer() const
{
    WebGLAttachment* attachment = getAttachment(GraphicsContext3D::STENCIL_ATTACHMENT);
    if (!attachment)
        attachment = getAttachment(GraphicsContext3D::DEPTH_STENCIL_ATTACHMENT);
    return attachment && attachment->isValid();
}

void WebGLFramebuffer::deleteObjectImpl(GraphicsContext3D* context3d, Platform3DObject object)
{
    for (AttachmentMap::iterator it = m_attachments.begin(); it != m_attachments.end(); ++it)
        it->value->onDetached(context3d);

    context3d->deleteFramebuffer(object);
}

bool WebGLFramebuffer::initializeAttachments(GraphicsContext3D* g3d, const char** reason)
{
    ASSERT(object());
    GC3Dbitfield mask = 0;

    for (AttachmentMap::iterator it = m_attachments.begin(); it != m_attachments.end(); ++it) {
        GC3Denum attachmentType = it->key;
        WebGLAttachment* attachment = it->value.get();
        if (!attachment->isInitialized())
           mask |= GraphicsContext3D::getClearBitsByAttachmentType(attachmentType);
    }
    if (!mask)
        return true;

    // We only clear un-initialized renderbuffers when they are ready to be
    // read, i.e., when the framebuffer is complete.
    if (g3d->checkFramebufferStatus(GraphicsContext3D::FRAMEBUFFER) != GraphicsContext3D::FRAMEBUFFER_COMPLETE) {
        *reason = "framebuffer not complete";
        return false;
    }

    bool initColor = mask & GraphicsContext3D::COLOR_BUFFER_BIT;
    bool initDepth = mask & GraphicsContext3D::DEPTH_BUFFER_BIT;
    bool initStencil = mask & GraphicsContext3D::STENCIL_BUFFER_BIT;

    GC3Dfloat colorClearValue[] = {0, 0, 0, 0}, depthClearValue = 0;
    GC3Dint stencilClearValue = 0;
    GC3Dboolean colorMask[] = {0, 0, 0, 0}, depthMask = 0;
    GC3Duint stencilMask = 0xffffffff;
    GC3Dboolean isScissorEnabled = 0;
    GC3Dboolean isDitherEnabled = 0;
    if (initColor) {
        g3d->getFloatv(GraphicsContext3D::COLOR_CLEAR_VALUE, colorClearValue);
        g3d->getBooleanv(GraphicsContext3D::COLOR_WRITEMASK, colorMask);
        g3d->clearColor(0, 0, 0, 0);
        g3d->colorMask(true, true, true, true);
    }
    if (initDepth) {
        g3d->getFloatv(GraphicsContext3D::DEPTH_CLEAR_VALUE, &depthClearValue);
        g3d->getBooleanv(GraphicsContext3D::DEPTH_WRITEMASK, &depthMask);
        g3d->clearDepth(0);
        g3d->depthMask(true);
    }
    if (initStencil) {
        g3d->getIntegerv(GraphicsContext3D::STENCIL_CLEAR_VALUE, &stencilClearValue);
        g3d->getIntegerv(GraphicsContext3D::STENCIL_WRITEMASK, reinterpret_cast<GC3Dint*>(&stencilMask));
        g3d->clearStencil(0);
        g3d->stencilMask(0xffffffff);
    }
    isScissorEnabled = g3d->isEnabled(GraphicsContext3D::SCISSOR_TEST);
    g3d->disable(GraphicsContext3D::SCISSOR_TEST);
    isDitherEnabled = g3d->isEnabled(GraphicsContext3D::DITHER);
    g3d->disable(GraphicsContext3D::DITHER);

    g3d->clear(mask);

    if (initColor) {
        g3d->clearColor(colorClearValue[0], colorClearValue[1], colorClearValue[2], colorClearValue[3]);
        g3d->colorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
    }
    if (initDepth) {
        g3d->clearDepth(depthClearValue);
        g3d->depthMask(depthMask);
    }
    if (initStencil) {
        g3d->clearStencil(stencilClearValue);
        g3d->stencilMask(stencilMask);
    }
    if (isScissorEnabled)
        g3d->enable(GraphicsContext3D::SCISSOR_TEST);
    else
        g3d->disable(GraphicsContext3D::SCISSOR_TEST);
    if (isDitherEnabled)
        g3d->enable(GraphicsContext3D::DITHER);
    else
        g3d->disable(GraphicsContext3D::DITHER);

    for (AttachmentMap::iterator it = m_attachments.begin(); it != m_attachments.end(); ++it) {
        GC3Denum attachmentType = it->key;
        WebGLAttachment* attachment = it->value.get();
        GC3Dbitfield bits = GraphicsContext3D::getClearBitsByAttachmentType(attachmentType);
        if (bits & mask)
            attachment->setInitialized();
    }
    return true;
}

bool WebGLFramebuffer::isBound() const
{
    return (context()->m_framebufferBinding.get() == this);
}

void WebGLFramebuffer::drawBuffers(const Vector<GC3Denum>& bufs)
{
    m_drawBuffers = bufs;
    m_filteredDrawBuffers.resize(m_drawBuffers.size());
    for (size_t i = 0; i < m_filteredDrawBuffers.size(); ++i)
        m_filteredDrawBuffers[i] = GraphicsContext3D::NONE;
    drawBuffersIfNecessary(true);
}

void WebGLFramebuffer::drawBuffersIfNecessary(bool force)
{
    if (!context()->m_extDrawBuffers)
        return;
    bool reset = force;
    // This filtering works around graphics driver bugs on Mac OS X.
    for (size_t i = 0; i < m_drawBuffers.size(); ++i) {
        if (m_drawBuffers[i] != GraphicsContext3D::NONE && getAttachment(m_drawBuffers[i])) {
            if (m_filteredDrawBuffers[i] != m_drawBuffers[i]) {
                m_filteredDrawBuffers[i] = m_drawBuffers[i];
                reset = true;
            }
        } else {
            if (m_filteredDrawBuffers[i] != GraphicsContext3D::NONE) {
                m_filteredDrawBuffers[i] = GraphicsContext3D::NONE;
                reset = true;
            }
        }
    }
    if (reset) {
        context()->graphicsContext3D()->getExtensions()->drawBuffersEXT(
            m_filteredDrawBuffers.size(), m_filteredDrawBuffers.data());
    }
}

GC3Denum WebGLFramebuffer::getDrawBuffer(GC3Denum drawBuffer)
{
    int index = static_cast<int>(drawBuffer - Extensions3D::DRAW_BUFFER0_EXT);
    ASSERT(index >= 0);
    if (index < static_cast<int>(m_drawBuffers.size()))
        return m_drawBuffers[index];
    if (drawBuffer == Extensions3D::DRAW_BUFFER0_EXT)
        return GraphicsContext3D::COLOR_ATTACHMENT0;
    return GraphicsContext3D::NONE;
}

}

#endif // ENABLE(WEBGL)
