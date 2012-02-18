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

#include "WebGLRenderingContext.h"

#include "CachedImage.h"
#include "CanvasPixelArray.h"
#include "CheckedInt.h"
#include "WebKitLoseContext.h"
#include "Console.h"
#include "DOMWindow.h"
#include "Extensions3D.h"
#include "FrameView.h"
#include "HTMLCanvasElement.h"
#include "HTMLImageElement.h"
#include "HTMLVideoElement.h"
#include "ImageBuffer.h"
#include "ImageData.h"
#include "IntSize.h"
#include "NotImplemented.h"
#include "OESStandardDerivatives.h"
#include "OESTextureFloat.h"
#include "OESVertexArrayObject.h"
#include "RenderBox.h"
#include "RenderLayer.h"
#include "Settings.h"
#include "Uint16Array.h"
#include "WebGLActiveInfo.h"
#include "WebGLBuffer.h"
#include "WebGLContextAttributes.h"
#include "WebGLContextEvent.h"
#include "WebGLFramebuffer.h"
#include "WebGLProgram.h"
#include "WebGLRenderbuffer.h"
#include "WebGLShader.h"
#include "WebGLTexture.h"
#include "WebGLUniformLocation.h"

#include <wtf/ByteArray.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/PassOwnArrayPtr.h>
#include <wtf/text/StringBuilder.h>

#if PLATFORM(QT)
#undef emit
#endif

namespace WebCore {

const double secondsBetweenRestoreAttempts = 1.0;

namespace {

    Platform3DObject objectOrZero(WebGLObject* object)
    {
        return object ? object->object() : 0;
    }

    void clip1D(GC3Dint start, GC3Dsizei range, GC3Dsizei sourceRange, GC3Dint* clippedStart, GC3Dsizei* clippedRange)
    {
        ASSERT(clippedStart && clippedRange);
        if (start < 0) {
            range += start;
            start = 0;
        }
        GC3Dint end = start + range;
        if (end > sourceRange)
            range -= end - sourceRange;
        *clippedStart = start;
        *clippedRange = range;
    }

    // Returns false if no clipping is necessary, i.e., x, y, width, height stay the same.
    bool clip2D(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height,
                GC3Dsizei sourceWidth, GC3Dsizei sourceHeight,
                GC3Dint* clippedX, GC3Dint* clippedY, GC3Dsizei* clippedWidth, GC3Dsizei*clippedHeight)
    {
        ASSERT(clippedX && clippedY && clippedWidth && clippedHeight);
        clip1D(x, width, sourceWidth, clippedX, clippedWidth);
        clip1D(y, height, sourceHeight, clippedY, clippedHeight);
        return (*clippedX != x || *clippedY != y || *clippedWidth != width || *clippedHeight != height);
    }

    // Return true if a character belongs to the ASCII subset as defined in
    // GLSL ES 1.0 spec section 3.1.
    bool validateCharacter(unsigned char c)
    {
        // Printing characters are valid except " $ ` @ \ ' DEL.
        if (c >= 32 && c <= 126
            && c != '"' && c != '$' && c != '`' && c != '@' && c != '\\' && c != '\'')
            return true;
        // Horizontal tab, line feed, vertical tab, form feed, carriage return
        // are also valid.
        if (c >= 9 && c <= 13)
            return true;
        return false;
    }

    // Strips comments from shader text. This allows non-ASCII characters
    // to be used in comments without potentially breaking OpenGL
    // implementations not expecting characters outside the GLSL ES set.
    class StripComments {
    public:
        StripComments(const String& str)
            : m_parseState(BeginningOfLine)
            , m_sourceString(str)
            , m_length(str.length())
            , m_position(0)
        {
            parse();
        }

        String result()
        {
            return m_builder.toString();
        }

    private:
        bool hasMoreCharacters()
        {
            return (m_position < m_length);
        }

        void parse()
        {
            while (hasMoreCharacters()) {
                process(current());
                // process() might advance the position.
                if (hasMoreCharacters())
                    advance();
            }
        }

        void process(UChar);

        bool peek(UChar& character)
        {
            if (m_position + 1 >= m_length)
                return false;
            character = m_sourceString[m_position + 1];
            return true;
        }

        UChar current()
        {
            ASSERT(m_position < m_length);
            return m_sourceString[m_position];
        }

        void advance()
        {
            ++m_position;
        }

        bool isNewline(UChar character)
        {
            // Don't attempt to canonicalize newline related characters.
            return (character == '\n' || character == '\r');
        }

        void emit(UChar character)
        {
            m_builder.append(character);
        }

        enum ParseState {
            // Have not seen an ASCII non-whitespace character yet on
            // this line. Possible that we might see a preprocessor
            // directive.
            BeginningOfLine,

            // Have seen at least one ASCII non-whitespace character
            // on this line.
            MiddleOfLine,

            // Handling a preprocessor directive. Passes through all
            // characters up to the end of the line. Disables comment
            // processing.
            InPreprocessorDirective,

            // Handling a single-line comment. The comment text is
            // replaced with a single space.
            InSingleLineComment,

            // Handling a multi-line comment. Newlines are passed
            // through to preserve line numbers.
            InMultiLineComment
        };

        ParseState m_parseState;
        String m_sourceString;
        unsigned m_length;
        unsigned m_position;
        StringBuilder m_builder;
    };

    void StripComments::process(UChar c)
    {
        if (isNewline(c)) {
            // No matter what state we are in, pass through newlines
            // so we preserve line numbers.
            emit(c);

            if (m_parseState != InMultiLineComment)
                m_parseState = BeginningOfLine;

            return;
        }

        UChar temp = 0;
        switch (m_parseState) {
        case BeginningOfLine:
            if (WTF::isASCIISpace(c)) {
                emit(c);
                break;
            }

            if (c == '#') {
                m_parseState = InPreprocessorDirective;
                emit(c);
                break;
            }

            // Transition to normal state and re-handle character.
            m_parseState = MiddleOfLine;
            process(c);
            break;

        case MiddleOfLine:
            if (c == '/' && peek(temp)) {
                if (temp == '/') {
                    m_parseState = InSingleLineComment;
                    emit(' ');
                    advance();
                    break;
                }

                if (temp == '*') {
                    m_parseState = InMultiLineComment;
                    // Emit the comment start in case the user has
                    // an unclosed comment and we want to later
                    // signal an error.
                    emit('/');
                    emit('*');
                    advance();
                    break;
                }
            }

            emit(c);
            break;

        case InPreprocessorDirective:
            // No matter what the character is, just pass it
            // through. Do not parse comments in this state. This
            // might not be the right thing to do long term, but it
            // should handle the #error preprocessor directive.
            emit(c);
            break;

        case InSingleLineComment:
            // The newline code at the top of this function takes care
            // of resetting our state when we get out of the
            // single-line comment. Swallow all other characters.
            break;

        case InMultiLineComment:
            if (c == '*' && peek(temp) && temp == '/') {
                emit('*');
                emit('/');
                m_parseState = MiddleOfLine;
                advance();
                break;
            }

            // Swallow all other characters. Unclear whether we may
            // want or need to just emit a space per character to try
            // to preserve column numbers for debugging purposes.
            break;
        }
    }
} // namespace anonymous

class WebGLStateRestorer {
public:
    WebGLStateRestorer(WebGLRenderingContext* context,
                       bool changed)
        : m_context(context)
        , m_changed(changed)
    {
    }

    ~WebGLStateRestorer()
    {
        m_context->cleanupAfterGraphicsCall(m_changed);
    }

private:
    WebGLRenderingContext* m_context;
    bool m_changed;
};

void WebGLRenderingContext::WebGLRenderingContextRestoreTimer::fired()
{
    // Timer is started when m_contextLost is false.  It will first call
    // onLostContext, which will set m_contextLost to true.  Then it will keep
    // calling restoreContext and reschedule itself until m_contextLost is back
    // to false.
    if (!m_context->m_contextLost) {
        m_context->onLostContext();
        startOneShot(secondsBetweenRestoreAttempts);
    } else {
        // The rendering context is not restored if there is no handler for
        // the context restored event.
        if (!m_context->canvas()->hasEventListeners(eventNames().webglcontextrestoredEvent))
            return;

        m_context->restoreContext();
        if (m_context->m_contextLost)
            startOneShot(secondsBetweenRestoreAttempts);
    }
}

class WebGLRenderingContextLostCallback : public GraphicsContext3D::ContextLostCallback {
public:
    WebGLRenderingContextLostCallback(WebGLRenderingContext* cb) : m_contextLostCallback(cb) {}
    virtual void onContextLost() { m_contextLostCallback->forceLostContext(); }
    virtual ~WebGLRenderingContextLostCallback() {}
private:
    WebGLRenderingContext* m_contextLostCallback;
};

PassOwnPtr<WebGLRenderingContext> WebGLRenderingContext::create(HTMLCanvasElement* canvas, WebGLContextAttributes* attrs)
{
    HostWindow* hostWindow = canvas->document()->view()->root()->hostWindow();
    GraphicsContext3D::Attributes attributes = attrs ? attrs->attributes() : GraphicsContext3D::Attributes();

    if (attributes.antialias) {
        Page* p = canvas->document()->page();
        if (p && !p->settings()->openGLMultisamplingEnabled())
            attributes.antialias = false;
    }

    RefPtr<GraphicsContext3D> context(GraphicsContext3D::create(attributes, hostWindow));

    if (!context) {
        canvas->dispatchEvent(WebGLContextEvent::create(eventNames().webglcontextcreationerrorEvent, false, true, "Could not create a WebGL context."));
        return nullptr;
    }

    return adoptPtr(new WebGLRenderingContext(canvas, context, attributes));
}

WebGLRenderingContext::WebGLRenderingContext(HTMLCanvasElement* passedCanvas, PassRefPtr<GraphicsContext3D> context,
                                             GraphicsContext3D::Attributes attributes)
    : CanvasRenderingContext(passedCanvas)
    , m_context(context)
    , m_restoreTimer(this)
    , m_videoCache(4)
    , m_contextLost(false)
    , m_attributes(attributes)
{
    ASSERT(m_context);
    setupFlags();
    initializeNewContext();
}

void WebGLRenderingContext::initializeNewContext()
{
    ASSERT(!m_contextLost);
    m_needsUpdate = true;
    m_markedCanvasDirty = false;
    m_activeTextureUnit = 0;
    m_packAlignment = 4;
    m_unpackAlignment = 4;
    m_unpackFlipY = false;
    m_unpackPremultiplyAlpha = false;
    m_unpackColorspaceConversion = GraphicsContext3D::BROWSER_DEFAULT_WEBGL;
    m_boundArrayBuffer = 0;
    m_currentProgram = 0;
    m_framebufferBinding = 0;
    m_renderbufferBinding = 0;
    m_depthMask = true;
    m_stencilMask = 0xFFFFFFFF;
    m_stencilMaskBack = 0xFFFFFFFF;
    m_stencilFuncRef = 0;
    m_stencilFuncRefBack = 0;
    m_stencilFuncMask = 0xFFFFFFFF;
    m_stencilFuncMaskBack = 0xFFFFFFFF;
    m_layerCleared = false;
    
    m_clearColor[0] = m_clearColor[1] = m_clearColor[2] = m_clearColor[3] = 0;
    m_scissorEnabled = false;
    m_clearDepth = 1;
    m_clearStencil = 0;
    m_colorMask[0] = m_colorMask[1] = m_colorMask[2] = m_colorMask[3] = true;

    GC3Dint numCombinedTextureImageUnits = 0;
    m_context->getIntegerv(GraphicsContext3D::MAX_COMBINED_TEXTURE_IMAGE_UNITS, &numCombinedTextureImageUnits);
    m_textureUnits.clear();
    m_textureUnits.resize(numCombinedTextureImageUnits);

    GC3Dint numVertexAttribs = 0;
    m_context->getIntegerv(GraphicsContext3D::MAX_VERTEX_ATTRIBS, &numVertexAttribs);
    m_maxVertexAttribs = numVertexAttribs;
    
    m_maxTextureSize = 0;
    m_context->getIntegerv(GraphicsContext3D::MAX_TEXTURE_SIZE, &m_maxTextureSize);
    m_maxTextureLevel = WebGLTexture::computeLevelCount(m_maxTextureSize, m_maxTextureSize);
    m_maxCubeMapTextureSize = 0;
    m_context->getIntegerv(GraphicsContext3D::MAX_CUBE_MAP_TEXTURE_SIZE, &m_maxCubeMapTextureSize);
    m_maxCubeMapTextureLevel = WebGLTexture::computeLevelCount(m_maxCubeMapTextureSize, m_maxCubeMapTextureSize);
    
    m_defaultVertexArrayObject = WebGLVertexArrayObjectOES::create(this, WebGLVertexArrayObjectOES::VaoTypeDefault);
    addObject(m_defaultVertexArrayObject.get());
    m_boundVertexArrayObject = m_defaultVertexArrayObject;
    
    m_vertexAttribValue.resize(m_maxVertexAttribs);

    if (!isGLES2NPOTStrict())
        createFallbackBlackTextures1x1();
    if (!isGLES2Compliant())
        initVertexAttrib0();

    m_context->reshape(canvas()->width(), canvas()->height());
    m_context->viewport(0, 0, canvas()->width(), canvas()->height());

    m_context->setContextLostCallback(adoptPtr(new WebGLRenderingContextLostCallback(this)));
}

void WebGLRenderingContext::setupFlags()
{
    ASSERT(m_context);

    m_isGLES2Compliant = m_context->isGLES2Compliant();
    m_isErrorGeneratedOnOutOfBoundsAccesses = m_context->getExtensions()->isEnabled("GL_CHROMIUM_strict_attribs");
    m_isResourceSafe = m_context->getExtensions()->isEnabled("GL_CHROMIUM_resource_safe");
    if (m_isGLES2Compliant) {
        m_isGLES2NPOTStrict = !m_context->getExtensions()->isEnabled("GL_OES_texture_npot");
        m_isDepthStencilSupported = m_context->getExtensions()->isEnabled("GL_OES_packed_depth_stencil");
    } else {
        m_isGLES2NPOTStrict = !m_context->getExtensions()->isEnabled("GL_ARB_texture_non_power_of_two");
        m_isDepthStencilSupported = m_context->getExtensions()->isEnabled("GL_EXT_packed_depth_stencil");
    }
}

WebGLRenderingContext::~WebGLRenderingContext()
{
    detachAndRemoveAllObjects();
    m_context->setContextLostCallback(nullptr);
}

void WebGLRenderingContext::markContextChanged()
{
    if (m_framebufferBinding)
        return;
    m_context->markContextChanged();
    m_layerCleared = false;
#if USE(ACCELERATED_COMPOSITING)
    RenderBox* renderBox = canvas()->renderBox();
    if (renderBox && renderBox->hasLayer() && renderBox->layer()->hasAcceleratedCompositing())
        renderBox->layer()->contentChanged(RenderLayer::CanvasChanged);
    else {
#endif
        if (!m_markedCanvasDirty)
            canvas()->didDraw(FloatRect(0, 0, canvas()->width(), canvas()->height()));
#if USE(ACCELERATED_COMPOSITING)
    }
#endif
    m_markedCanvasDirty = true;
}

bool WebGLRenderingContext::clearIfComposited(GC3Dbitfield mask)
{
    if (isContextLost()) 
        return false;

    if (!m_context->layerComposited() || m_layerCleared
        || m_attributes.preserveDrawingBuffer || (mask && m_framebufferBinding))
        return false;

    RefPtr<WebGLContextAttributes> contextAttributes = getContextAttributes();

    // Determine if it's possible to combine the clear the user asked for and this clear.
    bool combinedClear = mask && !m_scissorEnabled;

    if (m_framebufferBinding)
        m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, 0);
    m_context->disable(GraphicsContext3D::SCISSOR_TEST);
    if (combinedClear && (mask & GraphicsContext3D::COLOR_BUFFER_BIT))
        m_context->clearColor(m_colorMask[0] ? m_clearColor[0] : 0,
                              m_colorMask[1] ? m_clearColor[1] : 0,
                              m_colorMask[2] ? m_clearColor[2] : 0,
                              m_colorMask[3] ? m_clearColor[3] : 0);
    else
        m_context->clearColor(0, 0, 0, 0);
    m_context->colorMask(true, true, true, true);
    GC3Dbitfield clearMask = GraphicsContext3D::COLOR_BUFFER_BIT;
    if (contextAttributes->depth()) {
        if (!combinedClear || !m_depthMask || !(mask & GraphicsContext3D::DEPTH_BUFFER_BIT))
            m_context->clearDepth(1.0f);
        clearMask |= GraphicsContext3D::DEPTH_BUFFER_BIT;
        m_context->depthMask(true);
    }
    if (contextAttributes->stencil()) {
        if (combinedClear && (mask & GraphicsContext3D::STENCIL_BUFFER_BIT))
            m_context->clearStencil(m_clearStencil & m_stencilMask);
        else
            m_context->clearStencil(0);
        clearMask |= GraphicsContext3D::STENCIL_BUFFER_BIT;
        m_context->stencilMaskSeparate(GraphicsContext3D::FRONT, 0xFFFFFFFF);
    }
    m_context->clear(clearMask);

    // Restore the state that the context set.
    if (m_scissorEnabled)
        m_context->enable(GraphicsContext3D::SCISSOR_TEST);
    m_context->clearColor(m_clearColor[0], m_clearColor[1],
                          m_clearColor[2], m_clearColor[3]);
    m_context->colorMask(m_colorMask[0], m_colorMask[1],
                         m_colorMask[2], m_colorMask[3]);
    m_context->clearDepth(m_clearDepth);
    m_context->clearStencil(m_clearStencil);
    m_context->stencilMaskSeparate(GraphicsContext3D::FRONT, m_stencilMask);
    m_context->depthMask(m_depthMask);
    if (m_framebufferBinding)
        m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, objectOrZero(m_framebufferBinding.get()));
    m_layerCleared = true;

    return combinedClear;
}

void WebGLRenderingContext::markLayerComposited()
{
    m_context->markLayerComposited();
}

void WebGLRenderingContext::paintRenderingResultsToCanvas()
{
    // Until the canvas is written to by the application, the clear that
    // happened after it was composited should be ignored by the compositor.
    if (m_context->layerComposited() && !m_attributes.preserveDrawingBuffer)
        canvas()->makePresentationCopy();
    else
        canvas()->clearPresentationCopy();
    clearIfComposited();
    if (!m_markedCanvasDirty && !m_layerCleared)
        return;
    canvas()->clearCopiedImage();
    m_markedCanvasDirty = false;
    m_context->paintRenderingResultsToCanvas(this);
}

PassRefPtr<ImageData> WebGLRenderingContext::paintRenderingResultsToImageData()
{
    clearIfComposited();
    return m_context->paintRenderingResultsToImageData();
}

bool WebGLRenderingContext::paintsIntoCanvasBuffer() const
{
    return m_context->paintsIntoCanvasBuffer();
}

void WebGLRenderingContext::reshape(int width, int height)
{
    if (m_needsUpdate) {
#if USE(ACCELERATED_COMPOSITING)
        RenderBox* renderBox = canvas()->renderBox();
        if (renderBox && renderBox->hasLayer())
            renderBox->layer()->contentChanged(RenderLayer::CanvasChanged);
#endif
        m_needsUpdate = false;
    }

    // We don't have to mark the canvas as dirty, since the newly created image buffer will also start off
    // clear (and this matches what reshape will do).
    m_context->reshape(width, height);
}

unsigned int WebGLRenderingContext::sizeInBytes(GC3Denum type)
{
    switch (type) {
    case GraphicsContext3D::BYTE:
        return sizeof(GC3Dbyte);
    case GraphicsContext3D::UNSIGNED_BYTE:
        return sizeof(GC3Dubyte);
    case GraphicsContext3D::SHORT:
        return sizeof(GC3Dshort);
    case GraphicsContext3D::UNSIGNED_SHORT:
        return sizeof(GC3Dushort);
    case GraphicsContext3D::INT:
        return sizeof(GC3Dint);
    case GraphicsContext3D::UNSIGNED_INT:
        return sizeof(GC3Duint);
    case GraphicsContext3D::FLOAT:
        return sizeof(GC3Dfloat);
    }
    ASSERT_NOT_REACHED();
    return 0;
}

void WebGLRenderingContext::activeTexture(GC3Denum texture, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return;
    if (texture - GraphicsContext3D::TEXTURE0 >= m_textureUnits.size()) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return;
    }
    m_activeTextureUnit = texture - GraphicsContext3D::TEXTURE0;
    m_context->activeTexture(texture);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::attachShader(WebGLProgram* program, WebGLShader* shader, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateWebGLObject(program) || !validateWebGLObject(shader))
        return;
    if (!program->attachShader(shader)) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return;
    }
    m_context->attachShader(objectOrZero(program), objectOrZero(shader));
    shader->onAttached();
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::bindAttribLocation(WebGLProgram* program, GC3Duint index, const String& name, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateWebGLObject(program))
        return;
    if (!validateString(name))
        return;
    m_context->bindAttribLocation(objectOrZero(program), index, name);
    cleanupAfterGraphicsCall(false);
}

bool WebGLRenderingContext::checkObjectToBeBound(WebGLObject* object, bool& deleted)
{
    deleted = false;
    if (isContextLost())
        return false;
    if (object) {
        if (object->context() != this) {
            m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
            return false;
        }
        deleted = !object->object();
    }
    return true;
}

void WebGLRenderingContext::bindBuffer(GC3Denum target, WebGLBuffer* buffer, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    bool deleted;
    if (!checkObjectToBeBound(buffer, deleted))
        return;
    if (deleted)
        buffer = 0;
    if (buffer && buffer->getTarget() && buffer->getTarget() != target) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return;
    }
    if (target == GraphicsContext3D::ARRAY_BUFFER)
        m_boundArrayBuffer = buffer;
    else if (target == GraphicsContext3D::ELEMENT_ARRAY_BUFFER)
        m_boundVertexArrayObject->setElementArrayBuffer(buffer);
    else {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return;
    }

    m_context->bindBuffer(target, objectOrZero(buffer));
    if (buffer)
        buffer->setTarget(target);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::bindFramebuffer(GC3Denum target, WebGLFramebuffer* buffer, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    bool deleted;
    if (!checkObjectToBeBound(buffer, deleted))
        return;
    if (deleted)
        buffer = 0;
    if (target != GraphicsContext3D::FRAMEBUFFER) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return;
    }
    m_framebufferBinding = buffer;
    m_context->bindFramebuffer(target, objectOrZero(buffer));
    if (buffer)
        buffer->setHasEverBeenBound();
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::bindRenderbuffer(GC3Denum target, WebGLRenderbuffer* renderBuffer, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    bool deleted;
    if (!checkObjectToBeBound(renderBuffer, deleted))
        return;
    if (deleted)
        renderBuffer = 0;
    if (target != GraphicsContext3D::RENDERBUFFER) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return;
    }
    m_renderbufferBinding = renderBuffer;
    m_context->bindRenderbuffer(target, objectOrZero(renderBuffer));
    if (renderBuffer)
        renderBuffer->setHasEverBeenBound();
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::bindTexture(GC3Denum target, WebGLTexture* texture, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    bool deleted;
    if (!checkObjectToBeBound(texture, deleted))
        return;
    if (deleted)
        texture = 0;
    if (texture && texture->getTarget() && texture->getTarget() != target) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return;
    }
    GC3Dint maxLevel = 0;
    if (target == GraphicsContext3D::TEXTURE_2D) {
        m_textureUnits[m_activeTextureUnit].m_texture2DBinding = texture;
        maxLevel = m_maxTextureLevel;
    } else if (target == GraphicsContext3D::TEXTURE_CUBE_MAP) {
        m_textureUnits[m_activeTextureUnit].m_textureCubeMapBinding = texture;
        maxLevel = m_maxCubeMapTextureLevel;
    } else {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return;
    }
    m_context->bindTexture(target, objectOrZero(texture));
    if (texture)
        texture->setTarget(target, maxLevel);

    // Note: previously we used to automatically set the TEXTURE_WRAP_R
    // repeat mode to CLAMP_TO_EDGE for cube map textures, because OpenGL
    // ES 2.0 doesn't expose this flag (a bug in the specification) and
    // otherwise the application has no control over the seams in this
    // dimension. However, it appears that supporting this properly on all
    // platforms is fairly involved (will require a HashMap from texture ID
    // in all ports), and we have not had any complaints, so the logic has
    // been removed.

    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::blendColor(GC3Dfloat red, GC3Dfloat green, GC3Dfloat blue, GC3Dfloat alpha)
{
    if (isContextLost())
        return;
    m_context->blendColor(red, green, blue, alpha);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::blendEquation(GC3Denum mode)
{
    if (isContextLost() || !validateBlendEquation(mode))
        return;
    m_context->blendEquation(mode);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::blendEquationSeparate(GC3Denum modeRGB, GC3Denum modeAlpha)
{
    if (isContextLost() || !validateBlendEquation(modeRGB) || !validateBlendEquation(modeAlpha))
        return;
    m_context->blendEquationSeparate(modeRGB, modeAlpha);
    cleanupAfterGraphicsCall(false);
}


void WebGLRenderingContext::blendFunc(GC3Denum sfactor, GC3Denum dfactor)
{
    if (isContextLost() || !validateBlendFuncFactors(sfactor, dfactor))
        return;
    m_context->blendFunc(sfactor, dfactor);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::blendFuncSeparate(GC3Denum srcRGB, GC3Denum dstRGB, GC3Denum srcAlpha, GC3Denum dstAlpha)
{
    if (isContextLost() || !validateBlendFuncFactors(srcRGB, dstRGB))
        return;
    m_context->blendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::bufferData(GC3Denum target, GC3Dsizeiptr size, GC3Denum usage, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return;
    WebGLBuffer* buffer = validateBufferDataParameters(target, usage);
    if (!buffer)
        return;
    if (size < 0) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return;
    }
    if (!isErrorGeneratedOnOutOfBoundsAccesses()) {
        if (!buffer->associateBufferData(size)) {
            m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
            return;
        }
    }

    m_context->bufferData(target, size, usage);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::bufferData(GC3Denum target, ArrayBuffer* data, GC3Denum usage, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return;
    WebGLBuffer* buffer = validateBufferDataParameters(target, usage);
    if (!buffer)
        return;
    if (!data) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return;
    }
    if (!isErrorGeneratedOnOutOfBoundsAccesses()) {
        if (!buffer->associateBufferData(data)) {
            m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
            return;
        }
    }

    m_context->bufferData(target, data->byteLength(), data->data(), usage);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::bufferData(GC3Denum target, ArrayBufferView* data, GC3Denum usage, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return;
    WebGLBuffer* buffer = validateBufferDataParameters(target, usage);
    if (!buffer)
        return;
    if (!data) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return;
    }
    if (!isErrorGeneratedOnOutOfBoundsAccesses()) {
        if (!buffer->associateBufferData(data)) {
            m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
            return;
        }
    }

    m_context->bufferData(target, data->byteLength(), data->baseAddress(), usage);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::bufferSubData(GC3Denum target, GC3Dintptr offset, ArrayBuffer* data, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return;
    WebGLBuffer* buffer = validateBufferDataParameters(target, GraphicsContext3D::STATIC_DRAW);
    if (!buffer)
        return;
    if (offset < 0) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return;
    }
    if (!data)
        return;
    if (!isErrorGeneratedOnOutOfBoundsAccesses()) {
        if (!buffer->associateBufferSubData(offset, data)) {
            m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
            return;
        }
    }

    m_context->bufferSubData(target, offset, data->byteLength(), data->data());
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::bufferSubData(GC3Denum target, GC3Dintptr offset, ArrayBufferView* data, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return;
    WebGLBuffer* buffer = validateBufferDataParameters(target, GraphicsContext3D::STATIC_DRAW);
    if (!buffer)
        return;
    if (offset < 0) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return;
    }
    if (!data)
        return;
    if (!isErrorGeneratedOnOutOfBoundsAccesses()) {
        if (!buffer->associateBufferSubData(offset, data)) {
            m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
            return;
        }
    }

    m_context->bufferSubData(target, offset, data->byteLength(), data->baseAddress());
    cleanupAfterGraphicsCall(false);
}

GC3Denum WebGLRenderingContext::checkFramebufferStatus(GC3Denum target)
{
    if (isContextLost())
        return GraphicsContext3D::FRAMEBUFFER_UNSUPPORTED;
    if (target != GraphicsContext3D::FRAMEBUFFER) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return 0;
    }
    if (!m_framebufferBinding || !m_framebufferBinding->object())
        return GraphicsContext3D::FRAMEBUFFER_COMPLETE;
    if (m_framebufferBinding->isIncomplete(true))
        return GraphicsContext3D::FRAMEBUFFER_UNSUPPORTED;
    unsigned long result = m_context->checkFramebufferStatus(target);
    cleanupAfterGraphicsCall(false);
    return result;
}

void WebGLRenderingContext::clear(GC3Dbitfield mask)
{
    if (isContextLost())
        return;
    if (mask & ~(GraphicsContext3D::COLOR_BUFFER_BIT | GraphicsContext3D::DEPTH_BUFFER_BIT | GraphicsContext3D::STENCIL_BUFFER_BIT)) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return;
    }
    if (m_framebufferBinding && !m_framebufferBinding->onAccess(!isResourceSafe())) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_FRAMEBUFFER_OPERATION);
        return;
    }
    if (!clearIfComposited(mask))
        m_context->clear(mask);
    cleanupAfterGraphicsCall(true);
}

void WebGLRenderingContext::clearColor(GC3Dfloat r, GC3Dfloat g, GC3Dfloat b, GC3Dfloat a)
{
    if (isContextLost())
        return;
    if (isnan(r))
        r = 0;
    if (isnan(g))
        g = 0;
    if (isnan(b))
        b = 0;
    if (isnan(a))
        a = 1;
    m_clearColor[0] = r;
    m_clearColor[1] = g;
    m_clearColor[2] = b;
    m_clearColor[3] = a;
    m_context->clearColor(r, g, b, a);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::clearDepth(GC3Dfloat depth)
{
    if (isContextLost())
        return;
    m_clearDepth = depth;
    m_context->clearDepth(depth);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::clearStencil(GC3Dint s)
{
    if (isContextLost())
        return;
    m_clearStencil = s;
    m_context->clearStencil(s);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::colorMask(GC3Dboolean red, GC3Dboolean green, GC3Dboolean blue, GC3Dboolean alpha)
{
    if (isContextLost())
        return;
    m_colorMask[0] = red;
    m_colorMask[1] = green;
    m_colorMask[2] = blue;
    m_colorMask[3] = alpha;
    m_context->colorMask(red, green, blue, alpha);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::compileShader(WebGLShader* shader, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateWebGLObject(shader))
        return;
    m_context->compileShader(objectOrZero(shader));
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::copyTexImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat, GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height, GC3Dint border)
{
    if (isContextLost())
        return;
    if (!validateTexFuncParameters(target, level, internalformat, width, height, border, internalformat, GraphicsContext3D::UNSIGNED_BYTE))
        return;
    WebGLTexture* tex = validateTextureBinding(target, true);
    if (!tex)
        return;
    if (!isTexInternalFormatColorBufferCombinationValid(internalformat, getBoundFramebufferColorFormat())) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return;
    }
    if (!isGLES2NPOTStrict() && level && WebGLTexture::isNPOT(width, height)) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return;
    }
    if (m_framebufferBinding && !m_framebufferBinding->onAccess(!isResourceSafe())) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_FRAMEBUFFER_OPERATION);
        return;
    }
    clearIfComposited();
    if (isResourceSafe())
        m_context->copyTexImage2D(target, level, internalformat, x, y, width, height, border);
    else {
        GC3Dint clippedX, clippedY;
        GC3Dsizei clippedWidth, clippedHeight;
        if (clip2D(x, y, width, height, getBoundFramebufferWidth(), getBoundFramebufferHeight(), &clippedX, &clippedY, &clippedWidth, &clippedHeight)) {
            m_context->texImage2DResourceSafe(target, level, internalformat, width, height, border,
                                              internalformat, GraphicsContext3D::UNSIGNED_BYTE, m_unpackAlignment);
            if (clippedWidth > 0 && clippedHeight > 0) {
                m_context->copyTexSubImage2D(target, level, clippedX - x, clippedY - y,
                                             clippedX, clippedY, clippedWidth, clippedHeight);
            }
        } else
            m_context->copyTexImage2D(target, level, internalformat, x, y, width, height, border);
    }
    // FIXME: if the framebuffer is not complete, none of the below should be executed.
    tex->setLevelInfo(target, level, internalformat, width, height, GraphicsContext3D::UNSIGNED_BYTE);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::copyTexSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset, GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height)
{
    if (isContextLost())
        return;
    if (!validateTexFuncLevel(target, level))
        return;
    WebGLTexture* tex = validateTextureBinding(target, true);
    if (!tex)
        return;
    if (!validateSize(xoffset, yoffset) || !validateSize(width, height))
        return;
    if (xoffset + width > tex->getWidth(target, level) || yoffset + height > tex->getHeight(target, level)) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return;
    }
    if (!isTexInternalFormatColorBufferCombinationValid(tex->getInternalFormat(target, level), getBoundFramebufferColorFormat())) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return;
    }
    if (m_framebufferBinding && !m_framebufferBinding->onAccess(!isResourceSafe())) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_FRAMEBUFFER_OPERATION);
        return;
    }
    clearIfComposited();
    if (isResourceSafe())
        m_context->copyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
    else {
        GC3Dint clippedX, clippedY;
        GC3Dsizei clippedWidth, clippedHeight;
        if (clip2D(x, y, width, height, getBoundFramebufferWidth(), getBoundFramebufferHeight(), &clippedX, &clippedY, &clippedWidth, &clippedHeight)) {
            GC3Denum format = tex->getInternalFormat(target, level);
            GC3Denum type = tex->getType(target, level);
            OwnArrayPtr<unsigned char> zero;
            if (width && height) {
                unsigned int size;
                GC3Denum error = m_context->computeImageSizeInBytes(format, type, width, height, m_unpackAlignment, &size, 0);
                if (error != GraphicsContext3D::NO_ERROR) {
                    m_context->synthesizeGLError(error);
                    return;
                }
                zero = adoptArrayPtr(new unsigned char[size]);
                if (!zero) {
                    m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
                    return;
                }
                memset(zero.get(), 0, size);
            }
            m_context->texSubImage2D(target, level, xoffset, yoffset, width, height, format, type, zero.get());
            if (clippedWidth > 0 && clippedHeight > 0) {
                m_context->copyTexSubImage2D(target, level, xoffset + clippedX - x, yoffset + clippedY - y,
                                             clippedX, clippedY, clippedWidth, clippedHeight);
            }
        } else
            m_context->copyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
    }
    cleanupAfterGraphicsCall(false);
}

PassRefPtr<WebGLBuffer> WebGLRenderingContext::createBuffer()
{
    if (isContextLost())
        return 0;
    RefPtr<WebGLBuffer> o = WebGLBuffer::create(this);
    addObject(o.get());
    return o;
}

PassRefPtr<WebGLFramebuffer> WebGLRenderingContext::createFramebuffer()
{
    if (isContextLost())
        return 0;
    RefPtr<WebGLFramebuffer> o = WebGLFramebuffer::create(this);
    addObject(o.get());
    return o;
}

PassRefPtr<WebGLTexture> WebGLRenderingContext::createTexture()
{
    if (isContextLost())
        return 0;
    RefPtr<WebGLTexture> o = WebGLTexture::create(this);
    addObject(o.get());
    return o;
}

PassRefPtr<WebGLProgram> WebGLRenderingContext::createProgram()
{
    if (isContextLost())
        return 0;
    RefPtr<WebGLProgram> o = WebGLProgram::create(this);
    addObject(o.get());
    return o;
}

PassRefPtr<WebGLRenderbuffer> WebGLRenderingContext::createRenderbuffer()
{
    if (isContextLost())
        return 0;
    RefPtr<WebGLRenderbuffer> o = WebGLRenderbuffer::create(this);
    addObject(o.get());
    return o;
}

PassRefPtr<WebGLShader> WebGLRenderingContext::createShader(GC3Denum type, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return 0;
    if (type != GraphicsContext3D::VERTEX_SHADER && type != GraphicsContext3D::FRAGMENT_SHADER) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return 0;
    }

    RefPtr<WebGLShader> o = WebGLShader::create(this, type);
    addObject(o.get());
    return o;
}

void WebGLRenderingContext::cullFace(GC3Denum mode)
{
    if (isContextLost())
        return;
    m_context->cullFace(mode);
    cleanupAfterGraphicsCall(false);
}

bool WebGLRenderingContext::deleteObject(WebGLObject* object)
{
    if (isContextLost() || !object)
        return false;
    if (object->context() != this) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return false;
    }
    if (object->object())
        object->deleteObject();
    return true;
}

void WebGLRenderingContext::deleteBuffer(WebGLBuffer* buffer)
{
    if (!deleteObject(buffer))
        return;
    if (m_boundArrayBuffer == buffer)
        m_boundArrayBuffer = 0;
    RefPtr<WebGLBuffer> elementArrayBuffer = m_boundVertexArrayObject->getElementArrayBuffer();
    if (elementArrayBuffer == buffer)
        m_boundVertexArrayObject->setElementArrayBuffer(0);
    if (!isGLES2Compliant()) {
        WebGLVertexArrayObjectOES::VertexAttribState& state = m_boundVertexArrayObject->getVertexAttribState(0);
        if (buffer == state.bufferBinding) {
            state.bufferBinding = m_vertexAttrib0Buffer;
            state.bytesPerElement = 0;
            state.size = 4;
            state.type = GraphicsContext3D::FLOAT;
            state.normalized = false;
            state.stride = 16;
            state.originalStride = 0;
            state.offset = 0;
        }
    }
}

void WebGLRenderingContext::deleteFramebuffer(WebGLFramebuffer* framebuffer)
{
    if (!deleteObject(framebuffer))
        return;
    if (framebuffer == m_framebufferBinding) {
        m_framebufferBinding = 0;
        // Have to call bindFramebuffer here to bind back to internal fbo.
        m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, 0);
    }
}

void WebGLRenderingContext::deleteProgram(WebGLProgram* program)
{
    deleteObject(program);
    // We don't reset m_currentProgram to 0 here because the deletion of the
    // current program is delayed.
}

void WebGLRenderingContext::deleteRenderbuffer(WebGLRenderbuffer* renderbuffer)
{
    if (!deleteObject(renderbuffer))
        return;
    if (renderbuffer == m_renderbufferBinding)
        m_renderbufferBinding = 0;
    if (m_framebufferBinding)
        m_framebufferBinding->removeAttachment(renderbuffer);
}

void WebGLRenderingContext::deleteShader(WebGLShader* shader)
{
    deleteObject(shader);
}

void WebGLRenderingContext::deleteTexture(WebGLTexture* texture)
{
    if (!deleteObject(texture))
        return;
    for (size_t i = 0; i < m_textureUnits.size(); ++i) {
        if (texture == m_textureUnits[i].m_texture2DBinding)
            m_textureUnits[i].m_texture2DBinding = 0;
        if (texture == m_textureUnits[i].m_textureCubeMapBinding)
            m_textureUnits[i].m_textureCubeMapBinding = 0;
    }
    if (m_framebufferBinding)
        m_framebufferBinding->removeAttachment(texture);
}

void WebGLRenderingContext::depthFunc(GC3Denum func)
{
    if (isContextLost())
        return;
    m_context->depthFunc(func);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::depthMask(GC3Dboolean flag)
{
    if (isContextLost())
        return;
    m_depthMask = flag;
    m_context->depthMask(flag);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::depthRange(GC3Dfloat zNear, GC3Dfloat zFar)
{
    if (isContextLost())
        return;
    if (zNear > zFar) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return;
    }
    m_context->depthRange(zNear, zFar);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::detachShader(WebGLProgram* program, WebGLShader* shader, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateWebGLObject(program) || !validateWebGLObject(shader))
        return;
    if (!program->detachShader(shader)) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return;
    }
    m_context->detachShader(objectOrZero(program), objectOrZero(shader));
    shader->onDetached();
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::disable(GC3Denum cap)
{
    if (isContextLost() || !validateCapability(cap))
        return;
    if (cap == GraphicsContext3D::SCISSOR_TEST)
        m_scissorEnabled = false;
    m_context->disable(cap);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::disableVertexAttribArray(GC3Duint index, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return;
    if (index >= m_maxVertexAttribs) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return;
    }

    WebGLVertexArrayObjectOES::VertexAttribState& state = m_boundVertexArrayObject->getVertexAttribState(index);
    state.enabled = false;

    if (index > 0 || isGLES2Compliant()) {
        m_context->disableVertexAttribArray(index);
        cleanupAfterGraphicsCall(false);
    }
}

bool WebGLRenderingContext::validateElementArraySize(GC3Dsizei count, GC3Denum type, GC3Dintptr offset)
{
    RefPtr<WebGLBuffer> elementArrayBuffer = m_boundVertexArrayObject->getElementArrayBuffer();
    
    if (!elementArrayBuffer)
        return false;

    if (offset < 0)
        return false;

    if (type == GraphicsContext3D::UNSIGNED_SHORT) {
        // For an unsigned short array, offset must be divisible by 2 for alignment reasons.
        if (offset % 2)
            return false;

        // Make uoffset an element offset.
        offset /= 2;

        GC3Dsizeiptr n = elementArrayBuffer->byteLength() / 2;
        if (offset > n || count > n - offset)
            return false;
    } else if (type == GraphicsContext3D::UNSIGNED_BYTE) {
        GC3Dsizeiptr n = elementArrayBuffer->byteLength();
        if (offset > n || count > n - offset)
            return false;
    }
    return true;
}

bool WebGLRenderingContext::validateIndexArrayConservative(GC3Denum type, int& numElementsRequired)
{
    // Performs conservative validation by caching a maximum index of
    // the given type per element array buffer. If all of the bound
    // array buffers have enough elements to satisfy that maximum
    // index, skips the expensive per-draw-call iteration in
    // validateIndexArrayPrecise.
    
    RefPtr<WebGLBuffer> elementArrayBuffer = m_boundVertexArrayObject->getElementArrayBuffer();

    if (!elementArrayBuffer)
        return false;

    GC3Dsizeiptr numElements = elementArrayBuffer->byteLength();
    // The case count==0 is already dealt with in drawElements before validateIndexArrayConservative.
    if (!numElements)
        return false;
    const ArrayBuffer* buffer = elementArrayBuffer->elementArrayBuffer();
    ASSERT(buffer);

    int maxIndex = elementArrayBuffer->getCachedMaxIndex(type);
    if (maxIndex < 0) {
        // Compute the maximum index in the entire buffer for the given type of index.
        switch (type) {
        case GraphicsContext3D::UNSIGNED_BYTE: {
            const GC3Dubyte* p = static_cast<const GC3Dubyte*>(buffer->data());
            for (GC3Dsizeiptr i = 0; i < numElements; i++)
                maxIndex = max(maxIndex, static_cast<int>(p[i]));
            break;
        }
        case GraphicsContext3D::UNSIGNED_SHORT: {
            numElements /= sizeof(GC3Dushort);
            const GC3Dushort* p = static_cast<const GC3Dushort*>(buffer->data());
            for (GC3Dsizeiptr i = 0; i < numElements; i++)
                maxIndex = max(maxIndex, static_cast<int>(p[i]));
            break;
        }
        default:
            return false;
        }
        elementArrayBuffer->setCachedMaxIndex(type, maxIndex);
    }

    if (maxIndex >= 0) {
        // The number of required elements is one more than the maximum
        // index that will be accessed.
        numElementsRequired = maxIndex + 1;
        return true;
    }

    return false;
}

bool WebGLRenderingContext::validateIndexArrayPrecise(GC3Dsizei count, GC3Denum type, GC3Dintptr offset, int& numElementsRequired)
{
    ASSERT(count >= 0 && offset >= 0);
    int lastIndex = -1;
    
    RefPtr<WebGLBuffer> elementArrayBuffer = m_boundVertexArrayObject->getElementArrayBuffer();

    if (!elementArrayBuffer)
        return false;

    if (!count) {
        numElementsRequired = 0;
        return true;
    }

    if (!elementArrayBuffer->elementArrayBuffer())
        return false;

    unsigned long uoffset = offset;
    unsigned long n = count;

    if (type == GraphicsContext3D::UNSIGNED_SHORT) {
        // Make uoffset an element offset.
        uoffset /= sizeof(GC3Dushort);
        const GC3Dushort* p = static_cast<const GC3Dushort*>(elementArrayBuffer->elementArrayBuffer()->data()) + uoffset;
        while (n-- > 0) {
            if (*p > lastIndex)
                lastIndex = *p;
            ++p;
        }
    } else if (type == GraphicsContext3D::UNSIGNED_BYTE) {
        const GC3Dubyte* p = static_cast<const GC3Dubyte*>(elementArrayBuffer->elementArrayBuffer()->data()) + uoffset;
        while (n-- > 0) {
            if (*p > lastIndex)
                lastIndex = *p;
            ++p;
        }
    }

    // Then set the last index in the index array and make sure it is valid.
    numElementsRequired = lastIndex + 1;
    return numElementsRequired > 0;
}

bool WebGLRenderingContext::validateRenderingState(int numElementsRequired)
{
    if (!m_currentProgram)
        return false;

    // Look in each enabled vertex attrib and check if they've been bound to a buffer.
    for (unsigned i = 0; i < m_maxVertexAttribs; ++i) {
        const WebGLVertexArrayObjectOES::VertexAttribState& state = m_boundVertexArrayObject->getVertexAttribState(i);
        if (state.enabled
            && (!state.bufferBinding || !state.bufferBinding->object()))
            return false;
    }

    if (numElementsRequired <= 0)
        return true;

    // Look in each consumed vertex attrib (by the current program) and find the smallest buffer size
    int smallestNumElements = INT_MAX;
    int numActiveAttribLocations = m_currentProgram->numActiveAttribLocations();
    for (int i = 0; i < numActiveAttribLocations; ++i) {
        int loc = m_currentProgram->getActiveAttribLocation(i);
        if (loc >= 0 && loc < static_cast<int>(m_maxVertexAttribs)) {
            const WebGLVertexArrayObjectOES::VertexAttribState& state = m_boundVertexArrayObject->getVertexAttribState(loc);
            if (state.enabled) {
                // Avoid off-by-one errors in numElements computation.
                // For the last element, we will only touch the data for the
                // element and nothing beyond it.
                int bytesRemaining = static_cast<int>(state.bufferBinding->byteLength() - state.offset);
                int numElements = 0;
                ASSERT(state.stride > 0);
                if (bytesRemaining >= state.bytesPerElement)
                    numElements = 1 + (bytesRemaining - state.bytesPerElement) / state.stride;
                if (numElements < smallestNumElements)
                    smallestNumElements = numElements;
            }
        }
    }

    if (smallestNumElements == INT_MAX)
        smallestNumElements = 0;

    return numElementsRequired <= smallestNumElements;
}

bool WebGLRenderingContext::validateWebGLObject(WebGLObject* object)
{
    if (!object || !object->object()) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return false;
    }
    if (object->context() != this) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return false;
    }
    return true;
}

void WebGLRenderingContext::drawArrays(GC3Denum mode, GC3Dint first, GC3Dsizei count, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);

    if (isContextLost() || !validateDrawMode(mode))
        return;

    if (!validateStencilSettings())
        return;

    if (first < 0 || count < 0) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return;
    }

    if (!count)
        return;

    if (!isErrorGeneratedOnOutOfBoundsAccesses()) {
        // Ensure we have a valid rendering state
        CheckedInt<GC3Dint> checkedFirst(first);
        CheckedInt<GC3Dint> checkedCount(count);
        CheckedInt<GC3Dint> checkedSum = checkedFirst + checkedCount;
        if (!checkedSum.valid() || !validateRenderingState(checkedSum.value())) {
            m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
            return;
        }
    } else {
        if (!validateRenderingState(0)) {
            m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
            return;
        }
    }

    if (m_framebufferBinding && !m_framebufferBinding->onAccess(!isResourceSafe())) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_FRAMEBUFFER_OPERATION);
        return;
    }

    clearIfComposited();

    bool vertexAttrib0Simulated = false;
    if (!isGLES2Compliant())
        vertexAttrib0Simulated = simulateVertexAttrib0(first + count - 1);
    if (!isGLES2NPOTStrict())
        handleNPOTTextures(true);
    m_context->drawArrays(mode, first, count);
    if (!isGLES2Compliant() && vertexAttrib0Simulated)
        restoreStatesAfterVertexAttrib0Simulation();
    if (!isGLES2NPOTStrict())
        handleNPOTTextures(false);
    cleanupAfterGraphicsCall(true);
}

void WebGLRenderingContext::drawElements(GC3Denum mode, GC3Dsizei count, GC3Denum type, GC3Dintptr offset, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);

    if (isContextLost() || !validateDrawMode(mode))
        return;

    if (!validateStencilSettings())
        return;

    switch (type) {
    case GraphicsContext3D::UNSIGNED_BYTE:
    case GraphicsContext3D::UNSIGNED_SHORT:
        break;
    default:
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return;
    }

    if (count < 0 || offset < 0) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return;
    }

    if (!count)
        return;

    if (!m_boundVertexArrayObject->getElementArrayBuffer()) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return;
    }

    int numElements = 0;
    if (!isErrorGeneratedOnOutOfBoundsAccesses()) {
        // Ensure we have a valid rendering state
        if (!validateElementArraySize(count, type, offset)) {
            m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
            return;
        }
        if (!count)
            return;
        if (!validateIndexArrayConservative(type, numElements) || !validateRenderingState(numElements)) {
            if (!validateIndexArrayPrecise(count, type, offset, numElements) || !validateRenderingState(numElements)) {
                m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
                return;
            }
        }
    } else {
        if (!validateRenderingState(0)) {
            m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
            return;
        }
    }

    if (m_framebufferBinding && !m_framebufferBinding->onAccess(!isResourceSafe())) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_FRAMEBUFFER_OPERATION);
        return;
    }
    clearIfComposited();

    bool vertexAttrib0Simulated = false;
    if (!isGLES2Compliant()) {
        if (!numElements)
            validateIndexArrayPrecise(count, type, offset, numElements);
        vertexAttrib0Simulated = simulateVertexAttrib0(numElements);
    }
    if (!isGLES2NPOTStrict())
        handleNPOTTextures(true);
    m_context->drawElements(mode, count, type, offset);
    if (!isGLES2Compliant() && vertexAttrib0Simulated)
        restoreStatesAfterVertexAttrib0Simulation();
    if (!isGLES2NPOTStrict())
        handleNPOTTextures(false);
    cleanupAfterGraphicsCall(true);
}

void WebGLRenderingContext::enable(GC3Denum cap)
{
    if (isContextLost() || !validateCapability(cap))
        return;
    if (cap == GraphicsContext3D::SCISSOR_TEST)
        m_scissorEnabled = true;
    m_context->enable(cap);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::enableVertexAttribArray(GC3Duint index, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return;
    if (index >= m_maxVertexAttribs) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return;
    }

    WebGLVertexArrayObjectOES::VertexAttribState& state = m_boundVertexArrayObject->getVertexAttribState(index);
    state.enabled = true;

    m_context->enableVertexAttribArray(index);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::finish()
{
    if (isContextLost())
        return;
    m_context->finish();
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::flush()
{
    if (isContextLost())
        return;
    m_context->flush();
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::framebufferRenderbuffer(GC3Denum target, GC3Denum attachment, GC3Denum renderbuffertarget, WebGLRenderbuffer* buffer, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateFramebufferFuncParameters(target, attachment))
        return;
    if (renderbuffertarget != GraphicsContext3D::RENDERBUFFER) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return;
    }
    if (buffer && buffer->context() != this) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return;
    }
    // Don't allow the default framebuffer to be mutated; all current
    // implementations use an FBO internally in place of the default
    // FBO.
    if (!m_framebufferBinding || !m_framebufferBinding->object()) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return;
    }
    Platform3DObject bufferObject = objectOrZero(buffer);
    bool reattachDepth = false;
    bool reattachStencil = false;
    bool reattachDepthStencilDepth = false;
    bool reattachDepthStencilStencil = false;
    switch (attachment) {
    case GraphicsContext3D::DEPTH_STENCIL_ATTACHMENT:
        m_context->framebufferRenderbuffer(target, GraphicsContext3D::DEPTH_ATTACHMENT, renderbuffertarget, bufferObject);
        m_context->framebufferRenderbuffer(target, GraphicsContext3D::STENCIL_ATTACHMENT, renderbuffertarget, bufferObject);
        if (!bufferObject) {
            reattachDepth = true;
            reattachStencil = true;
        }
        break;
    case GraphicsContext3D::DEPTH_ATTACHMENT:
        m_context->framebufferRenderbuffer(target, attachment, renderbuffertarget, objectOrZero(buffer));
        if (!bufferObject)
            reattachDepthStencilDepth = true;
        break;
    case GraphicsContext3D::STENCIL_ATTACHMENT:
        m_context->framebufferRenderbuffer(target, attachment, renderbuffertarget, objectOrZero(buffer));
        if (!bufferObject)
            reattachDepthStencilStencil = true;
        break;
    default:
        m_context->framebufferRenderbuffer(target, attachment, renderbuffertarget, objectOrZero(buffer));
    }
    m_framebufferBinding->setAttachment(attachment, buffer);
    if (reattachDepth) {
        Platform3DObject object = objectOrZero(m_framebufferBinding->getAttachment(GraphicsContext3D::DEPTH_ATTACHMENT));
        if (object)
            m_context->framebufferRenderbuffer(target, GraphicsContext3D::DEPTH_ATTACHMENT, renderbuffertarget, object);
    }
    if (reattachStencil) {
        Platform3DObject object = objectOrZero(m_framebufferBinding->getAttachment(GraphicsContext3D::STENCIL_ATTACHMENT));
        if (object)
            m_context->framebufferRenderbuffer(target, GraphicsContext3D::STENCIL_ATTACHMENT, renderbuffertarget, object);
    }
    if (reattachDepthStencilDepth) {
        Platform3DObject object = objectOrZero(m_framebufferBinding->getAttachment(GraphicsContext3D::DEPTH_STENCIL_ATTACHMENT));
        if (object)
            m_context->framebufferRenderbuffer(target, GraphicsContext3D::DEPTH_ATTACHMENT, renderbuffertarget, object);
    }
    if (reattachDepthStencilStencil) {
        Platform3DObject object = objectOrZero(m_framebufferBinding->getAttachment(GraphicsContext3D::DEPTH_STENCIL_ATTACHMENT));
        if (object)
            m_context->framebufferRenderbuffer(target, GraphicsContext3D::STENCIL_ATTACHMENT, renderbuffertarget, object);
    }
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::framebufferTexture2D(GC3Denum target, GC3Denum attachment, GC3Denum textarget, WebGLTexture* texture, GC3Dint level, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateFramebufferFuncParameters(target, attachment))
        return;
    if (level) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return;
    }
    if (texture && texture->context() != this) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return;
    }
    // Don't allow the default framebuffer to be mutated; all current
    // implementations use an FBO internally in place of the default
    // FBO.
    if (!m_framebufferBinding || !m_framebufferBinding->object()) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return;
    }
    m_context->framebufferTexture2D(target, attachment, textarget, objectOrZero(texture), level);
    m_framebufferBinding->setAttachment(attachment, textarget, texture, level);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::frontFace(GC3Denum mode)
{
    if (isContextLost())
        return;
    m_context->frontFace(mode);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::generateMipmap(GC3Denum target)
{
    if (isContextLost())
        return;
    WebGLTexture* tex = validateTextureBinding(target, false);
    if (!tex)
        return;
    if (!tex->canGenerateMipmaps()) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return;
    }
    // generateMipmap won't work properly if minFilter is not NEAREST_MIPMAP_LINEAR
    // on Mac.  Remove the hack once this driver bug is fixed.
#if OS(DARWIN)
    bool needToResetMinFilter = false;
    if (tex->getMinFilter() != GraphicsContext3D::NEAREST_MIPMAP_LINEAR) {
        m_context->texParameteri(target, GraphicsContext3D::TEXTURE_MIN_FILTER, GraphicsContext3D::NEAREST_MIPMAP_LINEAR);
        needToResetMinFilter = true;
    }
#endif
    m_context->generateMipmap(target);
#if OS(DARWIN)
    if (needToResetMinFilter)
        m_context->texParameteri(target, GraphicsContext3D::TEXTURE_MIN_FILTER, tex->getMinFilter());
#endif
    tex->generateMipmapLevelInfo();
    cleanupAfterGraphicsCall(false);
}

PassRefPtr<WebGLActiveInfo> WebGLRenderingContext::getActiveAttrib(WebGLProgram* program, GC3Duint index, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateWebGLObject(program))
        return 0;
    ActiveInfo info;
    if (!m_context->getActiveAttrib(objectOrZero(program), index, info))
        return 0;
    return WebGLActiveInfo::create(info.name, info.type, info.size);
}

PassRefPtr<WebGLActiveInfo> WebGLRenderingContext::getActiveUniform(WebGLProgram* program, GC3Duint index, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateWebGLObject(program))
        return 0;
    ActiveInfo info;
    if (!m_context->getActiveUniform(objectOrZero(program), index, info))
        return 0;
    if (!isGLES2Compliant())
        if (info.size > 1 && !info.name.endsWith("[0]"))
            info.name.append("[0]");
    return WebGLActiveInfo::create(info.name, info.type, info.size);
}

bool WebGLRenderingContext::getAttachedShaders(WebGLProgram* program, Vector<WebGLShader*>& shaderObjects, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    shaderObjects.clear();
    if (isContextLost() || !validateWebGLObject(program))
        return false;
    GC3Dint numShaders = 0;
    m_context->getProgramiv(objectOrZero(program), GraphicsContext3D::ATTACHED_SHADERS, &numShaders);
    if (numShaders) {
        OwnArrayPtr<Platform3DObject> shaders = adoptArrayPtr(new Platform3DObject[numShaders]);
        GC3Dsizei count = 0;
        m_context->getAttachedShaders(objectOrZero(program), numShaders, &count, shaders.get());
        if (count != numShaders)
            return false;
        shaderObjects.resize(numShaders);
        for (GC3Dint ii = 0; ii < numShaders; ++ii) {
            WebGLShader* shader = findShader(shaders[ii]);
            if (!shader) {
                shaderObjects.clear();
                return false;
            }
            shaderObjects[ii] = shader;
        }
    }
    return true;
}

GC3Dint WebGLRenderingContext::getAttribLocation(WebGLProgram* program, const String& name)
{
    if (isContextLost())
        return -1;
    if (!validateString(name))
        return -1;
    return m_context->getAttribLocation(objectOrZero(program), name);
}

WebGLGetInfo WebGLRenderingContext::getBufferParameter(GC3Denum target, GC3Denum pname, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return WebGLGetInfo();
    if (target != GraphicsContext3D::ARRAY_BUFFER && target != GraphicsContext3D::ELEMENT_ARRAY_BUFFER) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return WebGLGetInfo();
    }

    if (pname != GraphicsContext3D::BUFFER_SIZE && pname != GraphicsContext3D::BUFFER_USAGE) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return WebGLGetInfo();
    }

    WebGLStateRestorer(this, false);
    GC3Dint value = 0;
    m_context->getBufferParameteriv(target, pname, &value);
    if (pname == GraphicsContext3D::BUFFER_SIZE)
        return WebGLGetInfo(value);
    return WebGLGetInfo(static_cast<unsigned int>(value));
}

PassRefPtr<WebGLContextAttributes> WebGLRenderingContext::getContextAttributes()
{
    if (isContextLost())
        return 0;
    // We always need to return a new WebGLContextAttributes object to
    // prevent the user from mutating any cached version.
    return WebGLContextAttributes::create(m_context->getContextAttributes());
}

GC3Denum WebGLRenderingContext::getError()
{
    return m_context->getError();
}

WebGLExtension* WebGLRenderingContext::getExtension(const String& name)
{
    if (isContextLost())
        return 0;

    if (equalIgnoringCase(name, "OES_standard_derivatives")
        && m_context->getExtensions()->supports("GL_OES_standard_derivatives")) {
        if (!m_oesStandardDerivatives) {
            m_context->getExtensions()->ensureEnabled("GL_OES_standard_derivatives");
            m_oesStandardDerivatives = OESStandardDerivatives::create(this);
        }
        return m_oesStandardDerivatives.get();
    }
    if (equalIgnoringCase(name, "OES_texture_float")
        && m_context->getExtensions()->supports("GL_OES_texture_float")) {
        if (!m_oesTextureFloat) {
            m_context->getExtensions()->ensureEnabled("GL_OES_texture_float");
            m_oesTextureFloat = OESTextureFloat::create(this);
        }
        return m_oesTextureFloat.get();
    }
    if (equalIgnoringCase(name, "OES_vertex_array_object")
        && m_context->getExtensions()->supports("GL_OES_vertex_array_object")) {
        if (!m_oesVertexArrayObject) {
            m_context->getExtensions()->ensureEnabled("GL_OES_vertex_array_object");
            m_oesVertexArrayObject = OESVertexArrayObject::create(this);
        }
        return m_oesVertexArrayObject.get();
    }
    if (equalIgnoringCase(name, "WEBKIT_lose_context")) {
        if (!m_webkitLoseContext)
            m_webkitLoseContext = WebKitLoseContext::create(this);
        return m_webkitLoseContext.get();
    }

    return 0;
}

WebGLGetInfo WebGLRenderingContext::getFramebufferAttachmentParameter(GC3Denum target, GC3Denum attachment, GC3Denum pname, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateFramebufferFuncParameters(target, attachment))
        return WebGLGetInfo();
    switch (pname) {
    case GraphicsContext3D::FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
    case GraphicsContext3D::FRAMEBUFFER_ATTACHMENT_OBJECT_NAME:
    case GraphicsContext3D::FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL:
    case GraphicsContext3D::FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE:
        break;
    default:
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return WebGLGetInfo();
    }

    if (!m_framebufferBinding || !m_framebufferBinding->object() || m_framebufferBinding->isIncomplete(false)) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return WebGLGetInfo();
    }

    if (pname != GraphicsContext3D::FRAMEBUFFER_ATTACHMENT_OBJECT_NAME) {
        WebGLStateRestorer(this, false);
        GC3Dint value = 0;
        m_context->getFramebufferAttachmentParameteriv(target, attachment, pname, &value);
        if (pname == GraphicsContext3D::FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE)
            return WebGLGetInfo(static_cast<unsigned int>(value));
        return WebGLGetInfo(value);
    }

    WebGLStateRestorer(this, false);
    GC3Dint type = 0;
    m_context->getFramebufferAttachmentParameteriv(target, attachment, GraphicsContext3D::FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);
    if (!type)
        return WebGLGetInfo();
    GC3Dint value = 0;
    m_context->getFramebufferAttachmentParameteriv(target, attachment, GraphicsContext3D::FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &value);
    switch (type) {
    case GraphicsContext3D::RENDERBUFFER:
        return WebGLGetInfo(PassRefPtr<WebGLRenderbuffer>(findRenderbuffer(static_cast<Platform3DObject>(value))));
    case GraphicsContext3D::TEXTURE:
        return WebGLGetInfo(PassRefPtr<WebGLTexture>(findTexture(static_cast<Platform3DObject>(value))));
    default:
        // FIXME: raise exception?
        return WebGLGetInfo();
    }
}

WebGLGetInfo WebGLRenderingContext::getParameter(GC3Denum pname, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return WebGLGetInfo();
    WebGLStateRestorer(this, false);
    switch (pname) {
    case GraphicsContext3D::ACTIVE_TEXTURE:
        return getUnsignedIntParameter(pname);
    case GraphicsContext3D::ALIASED_LINE_WIDTH_RANGE:
        return getWebGLFloatArrayParameter(pname);
    case GraphicsContext3D::ALIASED_POINT_SIZE_RANGE:
        return getWebGLFloatArrayParameter(pname);
    case GraphicsContext3D::ALPHA_BITS:
        return getIntParameter(pname);
    case GraphicsContext3D::ARRAY_BUFFER_BINDING:
        return WebGLGetInfo(PassRefPtr<WebGLBuffer>(m_boundArrayBuffer));
    case GraphicsContext3D::BLEND:
        return getBooleanParameter(pname);
    case GraphicsContext3D::BLEND_COLOR:
        return getWebGLFloatArrayParameter(pname);
    case GraphicsContext3D::BLEND_DST_ALPHA:
        return getUnsignedIntParameter(pname);
    case GraphicsContext3D::BLEND_DST_RGB:
        return getUnsignedIntParameter(pname);
    case GraphicsContext3D::BLEND_EQUATION_ALPHA:
        return getUnsignedIntParameter(pname);
    case GraphicsContext3D::BLEND_EQUATION_RGB:
        return getUnsignedIntParameter(pname);
    case GraphicsContext3D::BLEND_SRC_ALPHA:
        return getUnsignedIntParameter(pname);
    case GraphicsContext3D::BLEND_SRC_RGB:
        return getUnsignedIntParameter(pname);
    case GraphicsContext3D::BLUE_BITS:
        return getIntParameter(pname);
    case GraphicsContext3D::COLOR_CLEAR_VALUE:
        return getWebGLFloatArrayParameter(pname);
    case GraphicsContext3D::COLOR_WRITEMASK:
        return getBooleanArrayParameter(pname);
    case GraphicsContext3D::COMPRESSED_TEXTURE_FORMATS:
        // Defined as null in the spec
        return WebGLGetInfo();
    case GraphicsContext3D::CULL_FACE:
        return getBooleanParameter(pname);
    case GraphicsContext3D::CULL_FACE_MODE:
        return getUnsignedIntParameter(pname);
    case GraphicsContext3D::CURRENT_PROGRAM:
        return WebGLGetInfo(PassRefPtr<WebGLProgram>(m_currentProgram));
    case GraphicsContext3D::DEPTH_BITS:
        return getIntParameter(pname);
    case GraphicsContext3D::DEPTH_CLEAR_VALUE:
        return getFloatParameter(pname);
    case GraphicsContext3D::DEPTH_FUNC:
        return getUnsignedIntParameter(pname);
    case GraphicsContext3D::DEPTH_RANGE:
        return getWebGLFloatArrayParameter(pname);
    case GraphicsContext3D::DEPTH_TEST:
        return getBooleanParameter(pname);
    case GraphicsContext3D::DEPTH_WRITEMASK:
        return getBooleanParameter(pname);
    case GraphicsContext3D::DITHER:
        return getBooleanParameter(pname);
    case GraphicsContext3D::ELEMENT_ARRAY_BUFFER_BINDING:
        return WebGLGetInfo(PassRefPtr<WebGLBuffer>(m_boundVertexArrayObject->getElementArrayBuffer()));
    case GraphicsContext3D::FRAMEBUFFER_BINDING:
        return WebGLGetInfo(PassRefPtr<WebGLFramebuffer>(m_framebufferBinding));
    case GraphicsContext3D::FRONT_FACE:
        return getUnsignedIntParameter(pname);
    case GraphicsContext3D::GENERATE_MIPMAP_HINT:
        return getUnsignedIntParameter(pname);
    case GraphicsContext3D::GREEN_BITS:
        return getIntParameter(pname);
    case GraphicsContext3D::LINE_WIDTH:
        return getFloatParameter(pname);
    case GraphicsContext3D::MAX_COMBINED_TEXTURE_IMAGE_UNITS:
        return getIntParameter(pname);
    case GraphicsContext3D::MAX_CUBE_MAP_TEXTURE_SIZE:
        return getIntParameter(pname);
    case GraphicsContext3D::MAX_FRAGMENT_UNIFORM_VECTORS:
        return getIntParameter(pname);
    case GraphicsContext3D::MAX_RENDERBUFFER_SIZE:
        return getIntParameter(pname);
    case GraphicsContext3D::MAX_TEXTURE_IMAGE_UNITS:
        return getIntParameter(pname);
    case GraphicsContext3D::MAX_TEXTURE_SIZE:
        return getIntParameter(pname);
    case GraphicsContext3D::MAX_VARYING_VECTORS:
        return getIntParameter(pname);
    case GraphicsContext3D::MAX_VERTEX_ATTRIBS:
        return getIntParameter(pname);
    case GraphicsContext3D::MAX_VERTEX_TEXTURE_IMAGE_UNITS:
        return getIntParameter(pname);
    case GraphicsContext3D::MAX_VERTEX_UNIFORM_VECTORS:
        return getIntParameter(pname);
    case GraphicsContext3D::MAX_VIEWPORT_DIMS:
        return getWebGLIntArrayParameter(pname);
    case GraphicsContext3D::NUM_COMPRESSED_TEXTURE_FORMATS:
        // WebGL 1.0 specifies that there are no compressed texture formats.
        return WebGLGetInfo(static_cast<int>(0));
    case GraphicsContext3D::NUM_SHADER_BINARY_FORMATS:
        // FIXME: should we always return 0 for this?
        return getIntParameter(pname);
    case GraphicsContext3D::PACK_ALIGNMENT:
        return getIntParameter(pname);
    case GraphicsContext3D::POLYGON_OFFSET_FACTOR:
        return getFloatParameter(pname);
    case GraphicsContext3D::POLYGON_OFFSET_FILL:
        return getBooleanParameter(pname);
    case GraphicsContext3D::POLYGON_OFFSET_UNITS:
        return getFloatParameter(pname);
    case GraphicsContext3D::RED_BITS:
        return getIntParameter(pname);
    case GraphicsContext3D::RENDERBUFFER_BINDING:
        return WebGLGetInfo(PassRefPtr<WebGLRenderbuffer>(m_renderbufferBinding));
    case GraphicsContext3D::RENDERER:
        return WebGLGetInfo(m_context->getString(GraphicsContext3D::RENDERER));
    case GraphicsContext3D::SAMPLE_BUFFERS:
        return getIntParameter(pname);
    case GraphicsContext3D::SAMPLE_COVERAGE_INVERT:
        return getBooleanParameter(pname);
    case GraphicsContext3D::SAMPLE_COVERAGE_VALUE:
        return getFloatParameter(pname);
    case GraphicsContext3D::SAMPLES:
        return getIntParameter(pname);
    case GraphicsContext3D::SCISSOR_BOX:
        return getWebGLIntArrayParameter(pname);
    case GraphicsContext3D::SCISSOR_TEST:
        return getBooleanParameter(pname);
    case GraphicsContext3D::SHADING_LANGUAGE_VERSION:
        return WebGLGetInfo("WebGL GLSL ES 1.0 (" + m_context->getString(GraphicsContext3D::SHADING_LANGUAGE_VERSION) + ")");
    case GraphicsContext3D::STENCIL_BACK_FAIL:
        return getUnsignedIntParameter(pname);
    case GraphicsContext3D::STENCIL_BACK_FUNC:
        return getUnsignedIntParameter(pname);
    case GraphicsContext3D::STENCIL_BACK_PASS_DEPTH_FAIL:
        return getUnsignedIntParameter(pname);
    case GraphicsContext3D::STENCIL_BACK_PASS_DEPTH_PASS:
        return getUnsignedIntParameter(pname);
    case GraphicsContext3D::STENCIL_BACK_REF:
        return getIntParameter(pname);
    case GraphicsContext3D::STENCIL_BACK_VALUE_MASK:
        return getUnsignedIntParameter(pname);
    case GraphicsContext3D::STENCIL_BACK_WRITEMASK:
        return getUnsignedIntParameter(pname);
    case GraphicsContext3D::STENCIL_BITS:
        return getIntParameter(pname);
    case GraphicsContext3D::STENCIL_CLEAR_VALUE:
        return getIntParameter(pname);
    case GraphicsContext3D::STENCIL_FAIL:
        return getUnsignedIntParameter(pname);
    case GraphicsContext3D::STENCIL_FUNC:
        return getUnsignedIntParameter(pname);
    case GraphicsContext3D::STENCIL_PASS_DEPTH_FAIL:
        return getUnsignedIntParameter(pname);
    case GraphicsContext3D::STENCIL_PASS_DEPTH_PASS:
        return getUnsignedIntParameter(pname);
    case GraphicsContext3D::STENCIL_REF:
        return getIntParameter(pname);
    case GraphicsContext3D::STENCIL_TEST:
        return getBooleanParameter(pname);
    case GraphicsContext3D::STENCIL_VALUE_MASK:
        return getUnsignedIntParameter(pname);
    case GraphicsContext3D::STENCIL_WRITEMASK:
        return getUnsignedIntParameter(pname);
    case GraphicsContext3D::SUBPIXEL_BITS:
        return getIntParameter(pname);
    case GraphicsContext3D::TEXTURE_BINDING_2D:
        return WebGLGetInfo(PassRefPtr<WebGLTexture>(m_textureUnits[m_activeTextureUnit].m_texture2DBinding));
    case GraphicsContext3D::TEXTURE_BINDING_CUBE_MAP:
        return WebGLGetInfo(PassRefPtr<WebGLTexture>(m_textureUnits[m_activeTextureUnit].m_textureCubeMapBinding));
    case GraphicsContext3D::UNPACK_ALIGNMENT:
        return getIntParameter(pname);
    case GraphicsContext3D::UNPACK_FLIP_Y_WEBGL:
        return WebGLGetInfo(m_unpackFlipY);
    case GraphicsContext3D::UNPACK_PREMULTIPLY_ALPHA_WEBGL:
        return WebGLGetInfo(m_unpackPremultiplyAlpha);
    case GraphicsContext3D::UNPACK_COLORSPACE_CONVERSION_WEBGL:
        return WebGLGetInfo(m_unpackColorspaceConversion);
    case GraphicsContext3D::VENDOR:
        return WebGLGetInfo("Webkit (" + m_context->getString(GraphicsContext3D::VENDOR) + ")");
    case GraphicsContext3D::VERSION:
        return WebGLGetInfo("WebGL 1.0 (" + m_context->getString(GraphicsContext3D::VERSION) + ")");
    case GraphicsContext3D::VIEWPORT:
        return getWebGLIntArrayParameter(pname);
    case Extensions3D::FRAGMENT_SHADER_DERIVATIVE_HINT_OES: // OES_standard_derivatives
        if (m_oesStandardDerivatives)
            return getUnsignedIntParameter(Extensions3D::FRAGMENT_SHADER_DERIVATIVE_HINT_OES);
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return WebGLGetInfo();
    case Extensions3D::VERTEX_ARRAY_BINDING_OES: // OES_vertex_array_object
        if (m_oesVertexArrayObject) {
            if (!m_boundVertexArrayObject->isDefaultObject())
                return WebGLGetInfo(PassRefPtr<WebGLVertexArrayObjectOES>(m_boundVertexArrayObject));
            return WebGLGetInfo();
        }
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return WebGLGetInfo();
    default:
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return WebGLGetInfo();
    }
}

WebGLGetInfo WebGLRenderingContext::getProgramParameter(WebGLProgram* program, GC3Denum pname, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateWebGLObject(program))
        return WebGLGetInfo();

    WebGLStateRestorer(this, false);
    GC3Dint value = 0;
    switch (pname) {
    case GraphicsContext3D::DELETE_STATUS:
        return WebGLGetInfo(program->isDeleted());
    case GraphicsContext3D::VALIDATE_STATUS:
        m_context->getProgramiv(objectOrZero(program), pname, &value);
        return WebGLGetInfo(static_cast<bool>(value));
    case GraphicsContext3D::LINK_STATUS:
        return WebGLGetInfo(program->getLinkStatus());
    case GraphicsContext3D::ATTACHED_SHADERS:
    case GraphicsContext3D::ACTIVE_ATTRIBUTES:
    case GraphicsContext3D::ACTIVE_UNIFORMS:
        m_context->getProgramiv(objectOrZero(program), pname, &value);
        return WebGLGetInfo(value);
    default:
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return WebGLGetInfo();
    }
}

String WebGLRenderingContext::getProgramInfoLog(WebGLProgram* program, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return String();
    if (!validateWebGLObject(program))
        return "";
    WebGLStateRestorer(this, false);
    return m_context->getProgramInfoLog(objectOrZero(program));
}

WebGLGetInfo WebGLRenderingContext::getRenderbufferParameter(GC3Denum target, GC3Denum pname, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return WebGLGetInfo();
    if (target != GraphicsContext3D::RENDERBUFFER) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return WebGLGetInfo();
    }
    if (!m_renderbufferBinding || !m_renderbufferBinding->object()) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return WebGLGetInfo();
    }

    if (m_renderbufferBinding->getInternalFormat() == GraphicsContext3D::DEPTH_STENCIL
        && !m_renderbufferBinding->isValid()) {
        ASSERT(!isDepthStencilSupported());
        int value = 0;
        switch (pname) {
        case GraphicsContext3D::RENDERBUFFER_WIDTH:
            value = m_renderbufferBinding->getWidth();
            break;
        case GraphicsContext3D::RENDERBUFFER_HEIGHT:
            value = m_renderbufferBinding->getHeight();
            break;
        case GraphicsContext3D::RENDERBUFFER_RED_SIZE:
        case GraphicsContext3D::RENDERBUFFER_GREEN_SIZE:
        case GraphicsContext3D::RENDERBUFFER_BLUE_SIZE:
        case GraphicsContext3D::RENDERBUFFER_ALPHA_SIZE:
            value = 0;
            break;
        case GraphicsContext3D::RENDERBUFFER_DEPTH_SIZE:
            value = 24;
            break;
        case GraphicsContext3D::RENDERBUFFER_STENCIL_SIZE:
            value = 8;
            break;
        case GraphicsContext3D::RENDERBUFFER_INTERNAL_FORMAT:
            return WebGLGetInfo(m_renderbufferBinding->getInternalFormat());
        default:
            m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
            return WebGLGetInfo();
        }
        return WebGLGetInfo(value);
    }

    WebGLStateRestorer(this, false);
    GC3Dint value = 0;
    switch (pname) {
    case GraphicsContext3D::RENDERBUFFER_WIDTH:
    case GraphicsContext3D::RENDERBUFFER_HEIGHT:
    case GraphicsContext3D::RENDERBUFFER_RED_SIZE:
    case GraphicsContext3D::RENDERBUFFER_GREEN_SIZE:
    case GraphicsContext3D::RENDERBUFFER_BLUE_SIZE:
    case GraphicsContext3D::RENDERBUFFER_ALPHA_SIZE:
    case GraphicsContext3D::RENDERBUFFER_DEPTH_SIZE:
    case GraphicsContext3D::RENDERBUFFER_STENCIL_SIZE:
        m_context->getRenderbufferParameteriv(target, pname, &value);
        return WebGLGetInfo(value);
    case GraphicsContext3D::RENDERBUFFER_INTERNAL_FORMAT:
        return WebGLGetInfo(m_renderbufferBinding->getInternalFormat());
    default:
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return WebGLGetInfo();
    }
}

WebGLGetInfo WebGLRenderingContext::getShaderParameter(WebGLShader* shader, GC3Denum pname, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateWebGLObject(shader))
        return WebGLGetInfo();
    WebGLStateRestorer(this, false);
    GC3Dint value = 0;
    switch (pname) {
    case GraphicsContext3D::DELETE_STATUS:
        return WebGLGetInfo(shader->isDeleted());
    case GraphicsContext3D::COMPILE_STATUS:
        m_context->getShaderiv(objectOrZero(shader), pname, &value);
        return WebGLGetInfo(static_cast<bool>(value));
    case GraphicsContext3D::SHADER_TYPE:
        m_context->getShaderiv(objectOrZero(shader), pname, &value);
        return WebGLGetInfo(static_cast<unsigned int>(value));
    default:
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return WebGLGetInfo();
    }
}

String WebGLRenderingContext::getShaderInfoLog(WebGLShader* shader, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return String();
    if (!validateWebGLObject(shader))
        return "";
    WebGLStateRestorer(this, false);
    return m_context->getShaderInfoLog(objectOrZero(shader));
}

String WebGLRenderingContext::getShaderSource(WebGLShader* shader, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return String();
    if (!validateWebGLObject(shader))
        return "";
    return shader->getSource();
}

Vector<String> WebGLRenderingContext::getSupportedExtensions()
{
    Vector<String> result;
    if (m_context->getExtensions()->supports("GL_OES_texture_float"))
        result.append("OES_texture_float");
    if (m_context->getExtensions()->supports("GL_OES_standard_derivatives"))
        result.append("OES_standard_derivatives");
    if (m_context->getExtensions()->supports("GL_OES_vertex_array_object"))
        result.append("OES_vertex_array_object");
    result.append("WEBKIT_lose_context");
    return result;
}

WebGLGetInfo WebGLRenderingContext::getTexParameter(GC3Denum target, GC3Denum pname, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return WebGLGetInfo();
    WebGLTexture* tex = validateTextureBinding(target, false);
    if (!tex)
        return WebGLGetInfo();
    WebGLStateRestorer(this, false);
    GC3Dint value = 0;
    switch (pname) {
    case GraphicsContext3D::TEXTURE_MAG_FILTER:
    case GraphicsContext3D::TEXTURE_MIN_FILTER:
    case GraphicsContext3D::TEXTURE_WRAP_S:
    case GraphicsContext3D::TEXTURE_WRAP_T:
        m_context->getTexParameteriv(target, pname, &value);
        return WebGLGetInfo(static_cast<unsigned int>(value));
    default:
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return WebGLGetInfo();
    }
}

WebGLGetInfo WebGLRenderingContext::getUniform(WebGLProgram* program, const WebGLUniformLocation* uniformLocation, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateWebGLObject(program))
        return WebGLGetInfo();
    if (!uniformLocation || uniformLocation->program() != program) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return WebGLGetInfo();
    }
    GC3Dint location = uniformLocation->location();

    WebGLStateRestorer(this, false);
    // FIXME: make this more efficient using WebGLUniformLocation and caching types in it
    GC3Dint activeUniforms = 0;
    m_context->getProgramiv(objectOrZero(program), GraphicsContext3D::ACTIVE_UNIFORMS, &activeUniforms);
    for (GC3Dint i = 0; i < activeUniforms; i++) {
        ActiveInfo info;
        if (!m_context->getActiveUniform(objectOrZero(program), i, info))
            return WebGLGetInfo();
        // Strip "[0]" from the name if it's an array.
        if (info.size > 1 && info.name.endsWith("[0]"))
            info.name = info.name.left(info.name.length() - 3);
        // If it's an array, we need to iterate through each element, appending "[index]" to the name.
        for (GC3Dint index = 0; index < info.size; ++index) {
            String name = info.name;
            if (info.size > 1 && index >= 1) {
                name.append('[');
                name.append(String::number(index));
                name.append(']');
            }
            // Now need to look this up by name again to find its location
            GC3Dint loc = m_context->getUniformLocation(objectOrZero(program), name);
            if (loc == location) {
                // Found it. Use the type in the ActiveInfo to determine the return type.
                GC3Denum baseType;
                unsigned int length;
                switch (info.type) {
                case GraphicsContext3D::BOOL:
                    baseType = GraphicsContext3D::BOOL;
                    length = 1;
                    break;
                case GraphicsContext3D::BOOL_VEC2:
                    baseType = GraphicsContext3D::BOOL;
                    length = 2;
                    break;
                case GraphicsContext3D::BOOL_VEC3:
                    baseType = GraphicsContext3D::BOOL;
                    length = 3;
                    break;
                case GraphicsContext3D::BOOL_VEC4:
                    baseType = GraphicsContext3D::BOOL;
                    length = 4;
                    break;
                case GraphicsContext3D::INT:
                    baseType = GraphicsContext3D::INT;
                    length = 1;
                    break;
                case GraphicsContext3D::INT_VEC2:
                    baseType = GraphicsContext3D::INT;
                    length = 2;
                    break;
                case GraphicsContext3D::INT_VEC3:
                    baseType = GraphicsContext3D::INT;
                    length = 3;
                    break;
                case GraphicsContext3D::INT_VEC4:
                    baseType = GraphicsContext3D::INT;
                    length = 4;
                    break;
                case GraphicsContext3D::FLOAT:
                    baseType = GraphicsContext3D::FLOAT;
                    length = 1;
                    break;
                case GraphicsContext3D::FLOAT_VEC2:
                    baseType = GraphicsContext3D::FLOAT;
                    length = 2;
                    break;
                case GraphicsContext3D::FLOAT_VEC3:
                    baseType = GraphicsContext3D::FLOAT;
                    length = 3;
                    break;
                case GraphicsContext3D::FLOAT_VEC4:
                    baseType = GraphicsContext3D::FLOAT;
                    length = 4;
                    break;
                case GraphicsContext3D::FLOAT_MAT2:
                    baseType = GraphicsContext3D::FLOAT;
                    length = 4;
                    break;
                case GraphicsContext3D::FLOAT_MAT3:
                    baseType = GraphicsContext3D::FLOAT;
                    length = 9;
                    break;
                case GraphicsContext3D::FLOAT_MAT4:
                    baseType = GraphicsContext3D::FLOAT;
                    length = 16;
                    break;
                case GraphicsContext3D::SAMPLER_2D:
                case GraphicsContext3D::SAMPLER_CUBE:
                    baseType = GraphicsContext3D::INT;
                    length = 1;
                    break;
                default:
                    // Can't handle this type
                    m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
                    return WebGLGetInfo();
                }
                switch (baseType) {
                case GraphicsContext3D::FLOAT: {
                    GC3Dfloat value[16] = {0};
                    m_context->getUniformfv(objectOrZero(program), location, value);
                    if (length == 1)
                        return WebGLGetInfo(value[0]);
                    return WebGLGetInfo(Float32Array::create(value, length));
                }
                case GraphicsContext3D::INT: {
                    GC3Dint value[4] = {0};
                    m_context->getUniformiv(objectOrZero(program), location, value);
                    if (length == 1)
                        return WebGLGetInfo(value[0]);
                    return WebGLGetInfo(Int32Array::create(value, length));
                }
                case GraphicsContext3D::BOOL: {
                    GC3Dint value[4] = {0};
                    m_context->getUniformiv(objectOrZero(program), location, value);
                    if (length > 1) {
                        bool boolValue[16] = {0};
                        for (unsigned j = 0; j < length; j++)
                            boolValue[j] = static_cast<bool>(value[j]);
                        return WebGLGetInfo(boolValue, length);
                    }
                    return WebGLGetInfo(static_cast<bool>(value[0]));
                }
                default:
                    notImplemented();
                }
            }
        }
    }
    // If we get here, something went wrong in our unfortunately complex logic above
    m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
    return WebGLGetInfo();
}

PassRefPtr<WebGLUniformLocation> WebGLRenderingContext::getUniformLocation(WebGLProgram* program, const String& name, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateWebGLObject(program))
        return 0;
    if (!validateString(name))
        return 0;
    WebGLStateRestorer(this, false);
    GC3Dint uniformLocation = m_context->getUniformLocation(objectOrZero(program), name);
    if (uniformLocation == -1)
        return 0;
    return WebGLUniformLocation::create(program, uniformLocation);
}

WebGLGetInfo WebGLRenderingContext::getVertexAttrib(GC3Duint index, GC3Denum pname, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return WebGLGetInfo();
    WebGLStateRestorer(this, false);
    if (index >= m_maxVertexAttribs) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return WebGLGetInfo();
    }
    const WebGLVertexArrayObjectOES::VertexAttribState& state = m_boundVertexArrayObject->getVertexAttribState(index);
    switch (pname) {
    case GraphicsContext3D::VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
        if ((!isGLES2Compliant() && !index && m_boundVertexArrayObject->getVertexAttribState(0).bufferBinding == m_vertexAttrib0Buffer)
            || !state.bufferBinding
            || !state.bufferBinding->object())
            return WebGLGetInfo();
        return WebGLGetInfo(PassRefPtr<WebGLBuffer>(state.bufferBinding));
    case GraphicsContext3D::VERTEX_ATTRIB_ARRAY_ENABLED:
        return WebGLGetInfo(state.enabled);
    case GraphicsContext3D::VERTEX_ATTRIB_ARRAY_NORMALIZED:
        return WebGLGetInfo(state.normalized);
    case GraphicsContext3D::VERTEX_ATTRIB_ARRAY_SIZE:
        return WebGLGetInfo(state.size);
    case GraphicsContext3D::VERTEX_ATTRIB_ARRAY_STRIDE:
        return WebGLGetInfo(state.originalStride);
    case GraphicsContext3D::VERTEX_ATTRIB_ARRAY_TYPE:
        return WebGLGetInfo(state.type);
    case GraphicsContext3D::CURRENT_VERTEX_ATTRIB:
        return WebGLGetInfo(Float32Array::create(m_vertexAttribValue[index].value, 4));
    default:
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return WebGLGetInfo();
    }
}

GC3Dsizeiptr WebGLRenderingContext::getVertexAttribOffset(GC3Duint index, GC3Denum pname)
{
    if (isContextLost())
        return 0;
    GC3Dsizeiptr result = m_context->getVertexAttribOffset(index, pname);
    cleanupAfterGraphicsCall(false);
    return result;
}

void WebGLRenderingContext::hint(GC3Denum target, GC3Denum mode)
{
    if (isContextLost())
        return;
    bool isValid = false;
    switch (target) {
    case GraphicsContext3D::GENERATE_MIPMAP_HINT:
        isValid = true;
        break;
    case Extensions3D::FRAGMENT_SHADER_DERIVATIVE_HINT_OES: // OES_standard_derivatives
        if (m_oesStandardDerivatives)
            isValid = true;
        break;
    }
    if (!isValid) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return;
    }
    m_context->hint(target, mode);
    cleanupAfterGraphicsCall(false);
}

GC3Dboolean WebGLRenderingContext::isBuffer(WebGLBuffer* buffer)
{
    if (!buffer || isContextLost())
        return 0;

    if (!buffer->hasEverBeenBound())
        return 0;

    return m_context->isBuffer(buffer->object());
}

bool WebGLRenderingContext::isContextLost()
{
    if (m_restoreTimer.isActive())
        return true;

    bool newContextLost = m_context->getExtensions()->getGraphicsResetStatusARB() != GraphicsContext3D::NO_ERROR;

    if (newContextLost != m_contextLost)
        m_restoreTimer.startOneShot(secondsBetweenRestoreAttempts);

    return m_contextLost;
}

GC3Dboolean WebGLRenderingContext::isEnabled(GC3Denum cap)
{
    if (!validateCapability(cap) || isContextLost())
        return 0;
    return m_context->isEnabled(cap);
}

GC3Dboolean WebGLRenderingContext::isFramebuffer(WebGLFramebuffer* framebuffer)
{
    if (!framebuffer || isContextLost())
        return 0;

    if (!framebuffer->hasEverBeenBound())
        return 0;

    return m_context->isFramebuffer(framebuffer->object());
}

GC3Dboolean WebGLRenderingContext::isProgram(WebGLProgram* program)
{
    if (!program || isContextLost())
        return 0;

    return m_context->isProgram(program->object());
}

GC3Dboolean WebGLRenderingContext::isRenderbuffer(WebGLRenderbuffer* renderbuffer)
{
    if (!renderbuffer || isContextLost())
        return 0;

    if (!renderbuffer->hasEverBeenBound())
        return 0;

    return m_context->isRenderbuffer(renderbuffer->object());
}

GC3Dboolean WebGLRenderingContext::isShader(WebGLShader* shader)
{
    if (!shader || isContextLost())
        return 0;

    return m_context->isShader(shader->object());
}

GC3Dboolean WebGLRenderingContext::isTexture(WebGLTexture* texture)
{
    if (!texture || isContextLost())
        return 0;

    if (!texture->hasEverBeenBound())
        return 0;

    return m_context->isTexture(texture->object());
}

void WebGLRenderingContext::lineWidth(GC3Dfloat width)
{
    if (isContextLost())
        return;
    m_context->lineWidth(width);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::linkProgram(WebGLProgram* program, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateWebGLObject(program))
        return;
    if (!isGLES2Compliant()) {
        if (!program->getAttachedShader(GraphicsContext3D::VERTEX_SHADER) || !program->getAttachedShader(GraphicsContext3D::FRAGMENT_SHADER)) {
            program->setLinkStatus(false);
            return;
        }
    }

    m_context->linkProgram(objectOrZero(program));
    program->increaseLinkCount();
    // cache link status
    GC3Dint value = 0;
    m_context->getProgramiv(objectOrZero(program), GraphicsContext3D::LINK_STATUS, &value);
    program->setLinkStatus(static_cast<bool>(value));
    // Need to cache link status before caching active attribute locations.
    program->cacheActiveAttribLocations();
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::pixelStorei(GC3Denum pname, GC3Dint param)
{
    if (isContextLost())
        return;
    switch (pname) {
    case GraphicsContext3D::UNPACK_FLIP_Y_WEBGL:
        m_unpackFlipY = param;
        break;
    case GraphicsContext3D::UNPACK_PREMULTIPLY_ALPHA_WEBGL:
        m_unpackPremultiplyAlpha = param;
        break;
    case GraphicsContext3D::UNPACK_COLORSPACE_CONVERSION_WEBGL:
        if (param == GraphicsContext3D::BROWSER_DEFAULT_WEBGL || param == GraphicsContext3D::NONE)
            m_unpackColorspaceConversion = static_cast<GC3Denum>(param);
        else {
            m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
            return;
        }
        break;
    case GraphicsContext3D::PACK_ALIGNMENT:
    case GraphicsContext3D::UNPACK_ALIGNMENT:
        if (param == 1 || param == 2 || param == 4 || param == 8) {
            if (pname == GraphicsContext3D::PACK_ALIGNMENT)
                m_packAlignment = param;
            else // GraphicsContext3D::UNPACK_ALIGNMENT:
                m_unpackAlignment = param;
            m_context->pixelStorei(pname, param);
            cleanupAfterGraphicsCall(false);
        } else {
            m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
            return;
        }
        break;
    default:
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return;
    }
}

void WebGLRenderingContext::polygonOffset(GC3Dfloat factor, GC3Dfloat units)
{
    if (isContextLost())
        return;
    m_context->polygonOffset(factor, units);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::readPixels(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height, GC3Denum format, GC3Denum type, ArrayBufferView* pixels, ExceptionCode& ec)
{
    if (isContextLost())
        return;
    if (!canvas()->originClean()) {
        ec = SECURITY_ERR;
        return;
    }
    // Validate input parameters.
    if (!pixels) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return;
    }
    switch (format) {
    case GraphicsContext3D::ALPHA:
    case GraphicsContext3D::RGB:
    case GraphicsContext3D::RGBA:
        break;
    default:
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return;
    }
    switch (type) {
    case GraphicsContext3D::UNSIGNED_BYTE:
    case GraphicsContext3D::UNSIGNED_SHORT_5_6_5:
    case GraphicsContext3D::UNSIGNED_SHORT_4_4_4_4:
    case GraphicsContext3D::UNSIGNED_SHORT_5_5_5_1:
        break;
    default:
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return;
    }
    if (format != GraphicsContext3D::RGBA || type != GraphicsContext3D::UNSIGNED_BYTE) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return;
    }
    // Validate array type against pixel type.
    if (!pixels->isUnsignedByteArray()) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return;
    }
    if (m_framebufferBinding && !m_framebufferBinding->onAccess(!isResourceSafe())) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_FRAMEBUFFER_OPERATION);
        return;
    }
    // Calculate array size, taking into consideration of PACK_ALIGNMENT.
    unsigned int totalBytesRequired;
    unsigned int padding;
    GC3Denum error = m_context->computeImageSizeInBytes(format, type, width, height, m_packAlignment, &totalBytesRequired, &padding);
    if (error != GraphicsContext3D::NO_ERROR) {
        m_context->synthesizeGLError(error);
        return;
    }
    if (pixels->byteLength() < totalBytesRequired) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return;
    }
    clearIfComposited();
    void* data = pixels->baseAddress();
    m_context->readPixels(x, y, width, height, format, type, data);
#if OS(DARWIN)
    // FIXME: remove this section when GL driver bug on Mac is fixed, i.e.,
    // when alpha is off, readPixels should set alpha to 255 instead of 0.
    if (!m_context->getContextAttributes().alpha) {
        unsigned char* pixels = reinterpret_cast<unsigned char*>(data);
        for (GC3Dsizei iy = 0; iy < height; ++iy) {
            for (GC3Dsizei ix = 0; ix < width; ++ix) {
                pixels[3] = 255;
                pixels += 4;
            }
            pixels += padding;
        }
    }
#endif
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::releaseShaderCompiler()
{
    if (isContextLost())
        return;
    m_context->releaseShaderCompiler();
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::renderbufferStorage(GC3Denum target, GC3Denum internalformat, GC3Dsizei width, GC3Dsizei height)
{
    if (isContextLost())
        return;
    if (target != GraphicsContext3D::RENDERBUFFER) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return;
    }
    if (!m_renderbufferBinding || !m_renderbufferBinding->object()) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return;
    }
    if (!validateSize(width, height))
        return;
    switch (internalformat) {
    case GraphicsContext3D::DEPTH_COMPONENT16:
    case GraphicsContext3D::RGBA4:
    case GraphicsContext3D::RGB5_A1:
    case GraphicsContext3D::RGB565:
    case GraphicsContext3D::STENCIL_INDEX8:
        m_context->renderbufferStorage(target, internalformat, width, height);
        m_renderbufferBinding->setInternalFormat(internalformat);
        m_renderbufferBinding->setIsValid(true);
        m_renderbufferBinding->setSize(width, height);
        cleanupAfterGraphicsCall(false);
        break;
    case GraphicsContext3D::DEPTH_STENCIL:
        if (isDepthStencilSupported()) {
            m_context->renderbufferStorage(target, Extensions3D::DEPTH24_STENCIL8, width, height);
            cleanupAfterGraphicsCall(false);
        }
        m_renderbufferBinding->setSize(width, height);
        m_renderbufferBinding->setIsValid(isDepthStencilSupported());
        m_renderbufferBinding->setInternalFormat(internalformat);
        break;
    default:
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
    }
}

void WebGLRenderingContext::sampleCoverage(GC3Dfloat value, GC3Dboolean invert)
{
    if (isContextLost())
        return;
    m_context->sampleCoverage(value, invert);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::scissor(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height)
{
    if (isContextLost())
        return;
    if (!validateSize(width, height))
        return;
    m_context->scissor(x, y, width, height);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::shaderSource(WebGLShader* shader, const String& string, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateWebGLObject(shader))
        return;
    String stringWithoutComments = StripComments(string).result();
    if (!validateString(stringWithoutComments))
        return;
    shader->setSource(string);
    m_context->shaderSource(objectOrZero(shader), stringWithoutComments);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::stencilFunc(GC3Denum func, GC3Dint ref, GC3Duint mask)
{
    if (isContextLost())
        return;
    if (!validateStencilFunc(func))
        return;
    m_stencilFuncRef = ref;
    m_stencilFuncRefBack = ref;
    m_stencilFuncMask = mask;
    m_stencilFuncMaskBack = mask;
    m_context->stencilFunc(func, ref, mask);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::stencilFuncSeparate(GC3Denum face, GC3Denum func, GC3Dint ref, GC3Duint mask)
{
    if (isContextLost())
        return;
    if (!validateStencilFunc(func))
        return;
    switch (face) {
    case GraphicsContext3D::FRONT_AND_BACK:
        m_stencilFuncRef = ref;
        m_stencilFuncRefBack = ref;
        m_stencilFuncMask = mask;
        m_stencilFuncMaskBack = mask;
        break;
    case GraphicsContext3D::FRONT:
        m_stencilFuncRef = ref;
        m_stencilFuncMask = mask;
        break;
    case GraphicsContext3D::BACK:
        m_stencilFuncRefBack = ref;
        m_stencilFuncMaskBack = mask;
        break;
    default:
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return;
    }
    m_context->stencilFuncSeparate(face, func, ref, mask);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::stencilMask(GC3Duint mask)
{
    if (isContextLost())
        return;
    m_stencilMask = mask;
    m_stencilMaskBack = mask;
    m_context->stencilMask(mask);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::stencilMaskSeparate(GC3Denum face, GC3Duint mask)
{
    if (isContextLost())
        return;
    switch (face) {
    case GraphicsContext3D::FRONT_AND_BACK:
        m_stencilMask = mask;
        m_stencilMaskBack = mask;
        break;
    case GraphicsContext3D::FRONT:
        m_stencilMask = mask;
        break;
    case GraphicsContext3D::BACK:
        m_stencilMaskBack = mask;
        break;
    default:
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return;
    }
    m_context->stencilMaskSeparate(face, mask);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::stencilOp(GC3Denum fail, GC3Denum zfail, GC3Denum zpass)
{
    if (isContextLost())
        return;
    m_context->stencilOp(fail, zfail, zpass);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::stencilOpSeparate(GC3Denum face, GC3Denum fail, GC3Denum zfail, GC3Denum zpass)
{
    if (isContextLost())
        return;
    m_context->stencilOpSeparate(face, fail, zfail, zpass);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::texImage2DBase(GC3Denum target, GC3Dint level, GC3Denum internalformat,
                                           GC3Dsizei width, GC3Dsizei height, GC3Dint border,
                                           GC3Denum format, GC3Denum type, void* pixels, ExceptionCode& ec)
{
    // FIXME: For now we ignore any errors returned
    ec = 0;
    if (!validateTexFuncParameters(target, level, internalformat, width, height, border, format, type))
        return;
    WebGLTexture* tex = validateTextureBinding(target, true);
    if (!tex)
        return;
    if (!isGLES2NPOTStrict()) {
        if (level && WebGLTexture::isNPOT(width, height)) {
            m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
            return;
        }
    }
    if (!pixels && !isResourceSafe()) {
        bool succeed = m_context->texImage2DResourceSafe(target, level, internalformat, width, height,
                                                         border, format, type, m_unpackAlignment);
        if (!succeed)
            return;
    } else {
        m_context->texImage2D(target, level, internalformat, width, height,
                              border, format, type, pixels);
    }
    tex->setLevelInfo(target, level, internalformat, width, height, type);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::texImage2DImpl(GC3Denum target, GC3Dint level, GC3Denum internalformat,
                                           GC3Denum format, GC3Denum type, Image* image,
                                           bool flipY, bool premultiplyAlpha, ExceptionCode& ec)
{
    ec = 0;
    Vector<uint8_t> data;
    if (!m_context->extractImageData(image, format, type, flipY, premultiplyAlpha, m_unpackColorspaceConversion == GraphicsContext3D::NONE, data)) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return;
    }
    if (m_unpackAlignment != 1)
        m_context->pixelStorei(GraphicsContext3D::UNPACK_ALIGNMENT, 1);
    texImage2DBase(target, level, internalformat, image->width(), image->height(), 0,
                   format, type, data.data(), ec);
    if (m_unpackAlignment != 1)
        m_context->pixelStorei(GraphicsContext3D::UNPACK_ALIGNMENT, m_unpackAlignment);
}

void WebGLRenderingContext::texImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat,
                                       GC3Dsizei width, GC3Dsizei height, GC3Dint border,
                                       GC3Denum format, GC3Denum type, ArrayBufferView* pixels, ExceptionCode& ec)
{
    if (isContextLost() || !validateTexFuncData(width, height, format, type, pixels))
        return;
    void* data = pixels ? pixels->baseAddress() : 0;
    Vector<uint8_t> tempData;
    bool changeUnpackAlignment = false;
    if (data && (m_unpackFlipY || m_unpackPremultiplyAlpha)) {
        if (!m_context->extractTextureData(width, height, format, type,
                                           m_unpackAlignment,
                                           m_unpackFlipY, m_unpackPremultiplyAlpha,
                                           data,
                                           tempData))
            return;
        data = tempData.data();
        changeUnpackAlignment = true;
    }
    if (changeUnpackAlignment)
        m_context->pixelStorei(GraphicsContext3D::UNPACK_ALIGNMENT, 1);
    texImage2DBase(target, level, internalformat, width, height, border,
                   format, type, data, ec);
    if (changeUnpackAlignment)
        m_context->pixelStorei(GraphicsContext3D::UNPACK_ALIGNMENT, m_unpackAlignment);
}

void WebGLRenderingContext::texImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat,
                                       GC3Denum format, GC3Denum type, ImageData* pixels, ExceptionCode& ec)
{
    ec = 0;
    if (isContextLost())
        return;
    Vector<uint8_t> data;
    if (!m_context->extractImageData(pixels, format, type, m_unpackFlipY, m_unpackPremultiplyAlpha, data)) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return;
    }
    if (m_unpackAlignment != 1)
        m_context->pixelStorei(GraphicsContext3D::UNPACK_ALIGNMENT, 1);
    texImage2DBase(target, level, internalformat, pixels->width(), pixels->height(), 0,
                   format, type, data.data(), ec);
    if (m_unpackAlignment != 1)
        m_context->pixelStorei(GraphicsContext3D::UNPACK_ALIGNMENT, m_unpackAlignment);
}

void WebGLRenderingContext::texImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat,
                                       GC3Denum format, GC3Denum type, HTMLImageElement* image, ExceptionCode& ec)
{
    ec = 0;
    if (isContextLost())
        return;
    if (!validateHTMLImageElement(image))
        return;
    checkOrigin(image);
    texImage2DImpl(target, level, internalformat, format, type, image->cachedImage()->image(),
                   m_unpackFlipY, m_unpackPremultiplyAlpha, ec);
}

void WebGLRenderingContext::texImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat,
                                       GC3Denum format, GC3Denum type, HTMLCanvasElement* canvas, ExceptionCode& ec)
{
    ec = 0;
    if (isContextLost())
        return;
    if (!canvas || !canvas->buffer()) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return;
    }
    checkOrigin(canvas);
    RefPtr<ImageData> imageData = canvas->getImageData();
    if (imageData)
        texImage2D(target, level, internalformat, format, type, imageData.get(), ec);
    else
        texImage2DImpl(target, level, internalformat, format, type, canvas->copiedImage(),
                       m_unpackFlipY, m_unpackPremultiplyAlpha, ec);
}

#if ENABLE(VIDEO)
PassRefPtr<Image> WebGLRenderingContext::videoFrameToImage(HTMLVideoElement* video)
{
    if (!video || !video->videoWidth() || !video->videoHeight()) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return 0;
    }
    IntSize size(video->videoWidth(), video->videoHeight());
    ImageBuffer* buf = m_videoCache.imageBuffer(size);
    if (!buf) {
        m_context->synthesizeGLError(GraphicsContext3D::OUT_OF_MEMORY);
        return 0;
    }
    checkOrigin(video);
    IntRect destRect(0, 0, size.width(), size.height());
    // FIXME: Turn this into a GPU-GPU texture copy instead of CPU readback.
    video->paintCurrentFrameInContext(buf->context(), destRect);
    return buf->copyImage();
}

void WebGLRenderingContext::texImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat,
                                       GC3Denum format, GC3Denum type, HTMLVideoElement* video, ExceptionCode& ec)
{
    ec = 0;
    if (isContextLost())
        return;
    RefPtr<Image> image = videoFrameToImage(video);
    if (!video)
        return;
    texImage2DImpl(target, level, internalformat, format, type, image.get(), m_unpackFlipY, m_unpackPremultiplyAlpha, ec);
}
#endif

void WebGLRenderingContext::texParameter(GC3Denum target, GC3Denum pname, GC3Dfloat paramf, GC3Dint parami, bool isFloat)
{
    if (isContextLost())
        return;
    WebGLTexture* tex = validateTextureBinding(target, false);
    if (!tex)
        return;
    switch (pname) {
    case GraphicsContext3D::TEXTURE_MIN_FILTER:
    case GraphicsContext3D::TEXTURE_MAG_FILTER:
        break;
    case GraphicsContext3D::TEXTURE_WRAP_S:
    case GraphicsContext3D::TEXTURE_WRAP_T:
        if ((isFloat && paramf != GraphicsContext3D::CLAMP_TO_EDGE && paramf != GraphicsContext3D::MIRRORED_REPEAT && paramf != GraphicsContext3D::REPEAT)
            || (!isFloat && parami != GraphicsContext3D::CLAMP_TO_EDGE && parami != GraphicsContext3D::MIRRORED_REPEAT && parami != GraphicsContext3D::REPEAT)) {
            m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
            return;
        }
        break;
    default:
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return;
    }
    if (isFloat) {
        tex->setParameterf(pname, paramf);
        m_context->texParameterf(target, pname, paramf);
    } else {
        tex->setParameteri(pname, parami);
        m_context->texParameteri(target, pname, parami);
    }
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::texParameterf(GC3Denum target, GC3Denum pname, GC3Dfloat param)
{
    texParameter(target, pname, param, 0, true);
}

void WebGLRenderingContext::texParameteri(GC3Denum target, GC3Denum pname, GC3Dint param)
{
    texParameter(target, pname, 0, param, false);
}

void WebGLRenderingContext::texSubImage2DBase(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset,
                                              GC3Dsizei width, GC3Dsizei height,
                                              GC3Denum format, GC3Denum type, void* pixels, ExceptionCode& ec)
{
    // FIXME: For now we ignore any errors returned
    ec = 0;
    if (isContextLost())
        return;
    if (!validateTexFuncParameters(target, level, format, width, height, 0, format, type))
        return;
    if (!validateSize(xoffset, yoffset))
        return;
    WebGLTexture* tex = validateTextureBinding(target, true);
    if (!tex)
        return;
    if (xoffset + width > tex->getWidth(target, level) || yoffset + height > tex->getHeight(target, level)) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return;
    }
    if (tex->getInternalFormat(target, level) != format || tex->getType(target, level) != type) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return;
    }
    m_context->texSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::texSubImage2DImpl(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset,
                                              GC3Denum format, GC3Denum type,
                                              Image* image, bool flipY, bool premultiplyAlpha, ExceptionCode& ec)
{
    ec = 0;
    if (isContextLost())
        return;
    Vector<uint8_t> data;
    if (!m_context->extractImageData(image, format, type, flipY, premultiplyAlpha, m_unpackColorspaceConversion == GraphicsContext3D::NONE, data)) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return;
    }
    texSubImage2DBase(target, level, xoffset, yoffset, image->width(), image->height(),
                      format, type, data.data(), ec);
}

void WebGLRenderingContext::texSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset,
                                          GC3Dsizei width, GC3Dsizei height,
                                          GC3Denum format, GC3Denum type, ArrayBufferView* pixels, ExceptionCode& ec)
{
    if (isContextLost() || !validateTexFuncData(width, height, format, type, pixels))
        return;
    void* data = pixels ? pixels->baseAddress() : 0;
    Vector<uint8_t> tempData;
    bool changeUnpackAlignment = false;
    if (data && (m_unpackFlipY || m_unpackPremultiplyAlpha)) {
        if (!m_context->extractTextureData(width, height, format, type,
                                           m_unpackAlignment,
                                           m_unpackFlipY, m_unpackPremultiplyAlpha,
                                           data,
                                           tempData))
            return;
        data = tempData.data();
        changeUnpackAlignment = true;
    }
    if (changeUnpackAlignment)
        m_context->pixelStorei(GraphicsContext3D::UNPACK_ALIGNMENT, 1);
    texSubImage2DBase(target, level, xoffset, yoffset, width, height, format, type, data, ec);
    if (changeUnpackAlignment)
        m_context->pixelStorei(GraphicsContext3D::UNPACK_ALIGNMENT, m_unpackAlignment);
}

void WebGLRenderingContext::texSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset,
                                          GC3Denum format, GC3Denum type, ImageData* pixels, ExceptionCode& ec)
{
    ec = 0;
    if (isContextLost())
        return;
    Vector<uint8_t> data;
    if (!m_context->extractImageData(pixels, format, type, m_unpackFlipY, m_unpackPremultiplyAlpha, data)) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return;
    }
    texSubImage2DBase(target, level, xoffset, yoffset, pixels->width(), pixels->height(),
                      format, type, data.data(), ec);
}

void WebGLRenderingContext::texSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset,
                                          GC3Denum format, GC3Denum type, HTMLImageElement* image, ExceptionCode& ec)
{
    ec = 0;
    if (isContextLost())
        return;
    if (!validateHTMLImageElement(image))
        return;
    checkOrigin(image);
    texSubImage2DImpl(target, level, xoffset, yoffset, format, type, image->cachedImage()->image(),
                      m_unpackFlipY, m_unpackPremultiplyAlpha, ec);
}

void WebGLRenderingContext::texSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset,
                                          GC3Denum format, GC3Denum type, HTMLCanvasElement* canvas, ExceptionCode& ec)
{
    ec = 0;
    if (isContextLost())
        return;
    if (!canvas || !canvas->buffer()) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return;
    }
    checkOrigin(canvas);
    RefPtr<ImageData> imageData = canvas->getImageData();
    if (imageData)
        texSubImage2D(target, level, xoffset, yoffset, format, type, imageData.get(), ec);
    else
        texSubImage2DImpl(target, level, xoffset, yoffset, format, type, canvas->copiedImage(),
                          m_unpackFlipY, m_unpackPremultiplyAlpha, ec);
}

#if ENABLE(VIDEO)
void WebGLRenderingContext::texSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset,
                                          GC3Denum format, GC3Denum type, HTMLVideoElement* video, ExceptionCode& ec)
{
    ec = 0;
    if (isContextLost())
        return;
    RefPtr<Image> image = videoFrameToImage(video);
    if (!video)
        return;
    texSubImage2DImpl(target, level, xoffset, yoffset, format, type, image.get(), m_unpackFlipY, m_unpackPremultiplyAlpha, ec);
}
#endif

void WebGLRenderingContext::uniform1f(const WebGLUniformLocation* location, GC3Dfloat x, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !location)
        return;

    if (location->program() != m_currentProgram) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return;
    }

    m_context->uniform1f(location->location(), x);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform1fv(const WebGLUniformLocation* location, Float32Array* v, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters(location, v, 1))
        return;

    m_context->uniform1fv(location->location(), v->data(), v->length());
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform1fv(const WebGLUniformLocation* location, GC3Dfloat* v, GC3Dsizei size, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters(location, v, size, 1))
        return;

    m_context->uniform1fv(location->location(), v, size);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform1i(const WebGLUniformLocation* location, GC3Dint x, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !location)
        return;

    if (location->program() != m_currentProgram) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return;
    }

    m_context->uniform1i(location->location(), x);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform1iv(const WebGLUniformLocation* location, Int32Array* v, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters(location, v, 1))
        return;

    m_context->uniform1iv(location->location(), v->data(), v->length());
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform1iv(const WebGLUniformLocation* location, GC3Dint* v, GC3Dsizei size, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters(location, v, size, 1))
        return;

    m_context->uniform1iv(location->location(), v, size);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform2f(const WebGLUniformLocation* location, GC3Dfloat x, GC3Dfloat y, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !location)
        return;

    if (location->program() != m_currentProgram) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return;
    }

    m_context->uniform2f(location->location(), x, y);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform2fv(const WebGLUniformLocation* location, Float32Array* v, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters(location, v, 2))
        return;

    m_context->uniform2fv(location->location(), v->data(), v->length() / 2);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform2fv(const WebGLUniformLocation* location, GC3Dfloat* v, GC3Dsizei size, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters(location, v, size, 2))
        return;

    m_context->uniform2fv(location->location(), v, size / 2);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform2i(const WebGLUniformLocation* location, GC3Dint x, GC3Dint y, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !location)
        return;

    if (location->program() != m_currentProgram) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return;
    }

    m_context->uniform2i(location->location(), x, y);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform2iv(const WebGLUniformLocation* location, Int32Array* v, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters(location, v, 2))
        return;

    m_context->uniform2iv(location->location(), v->data(), v->length() / 2);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform2iv(const WebGLUniformLocation* location, GC3Dint* v, GC3Dsizei size, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters(location, v, size, 2))
        return;

    m_context->uniform2iv(location->location(), v, size / 2);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform3f(const WebGLUniformLocation* location, GC3Dfloat x, GC3Dfloat y, GC3Dfloat z, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !location)
        return;

    if (location->program() != m_currentProgram) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return;
    }

    m_context->uniform3f(location->location(), x, y, z);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform3fv(const WebGLUniformLocation* location, Float32Array* v, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters(location, v, 3))
        return;

    m_context->uniform3fv(location->location(), v->data(), v->length() / 3);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform3fv(const WebGLUniformLocation* location, GC3Dfloat* v, GC3Dsizei size, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters(location, v, size, 3))
        return;

    m_context->uniform3fv(location->location(), v, size / 3);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform3i(const WebGLUniformLocation* location, GC3Dint x, GC3Dint y, GC3Dint z, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !location)
        return;

    if (location->program() != m_currentProgram) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return;
    }

    m_context->uniform3i(location->location(), x, y, z);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform3iv(const WebGLUniformLocation* location, Int32Array* v, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters(location, v, 3))
        return;

    m_context->uniform3iv(location->location(), v->data(), v->length() / 3);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform3iv(const WebGLUniformLocation* location, GC3Dint* v, GC3Dsizei size, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters(location, v, size, 3))
        return;

    m_context->uniform3iv(location->location(), v, size / 3);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform4f(const WebGLUniformLocation* location, GC3Dfloat x, GC3Dfloat y, GC3Dfloat z, GC3Dfloat w, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !location)
        return;

    if (location->program() != m_currentProgram) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return;
    }

    m_context->uniform4f(location->location(), x, y, z, w);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform4fv(const WebGLUniformLocation* location, Float32Array* v, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters(location, v, 4))
        return;

    m_context->uniform4fv(location->location(), v->data(), v->length() / 4);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform4fv(const WebGLUniformLocation* location, GC3Dfloat* v, GC3Dsizei size, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters(location, v, size, 4))
        return;

    m_context->uniform4fv(location->location(), v, size / 4);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform4i(const WebGLUniformLocation* location, GC3Dint x, GC3Dint y, GC3Dint z, GC3Dint w, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !location)
        return;

    if (location->program() != m_currentProgram) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return;
    }

    m_context->uniform4i(location->location(), x, y, z, w);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform4iv(const WebGLUniformLocation* location, Int32Array* v, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters(location, v, 4))
        return;

    m_context->uniform4iv(location->location(), v->data(), v->length() / 4);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform4iv(const WebGLUniformLocation* location, GC3Dint* v, GC3Dsizei size, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters(location, v, size, 4))
        return;

    m_context->uniform4iv(location->location(), v, size / 4);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniformMatrix2fv(const WebGLUniformLocation* location, GC3Dboolean transpose, Float32Array* v, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformMatrixParameters(location, transpose, v, 4))
        return;
    m_context->uniformMatrix2fv(location->location(), transpose, v->data(), v->length() / 4);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniformMatrix2fv(const WebGLUniformLocation* location, GC3Dboolean transpose, GC3Dfloat* v, GC3Dsizei size, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformMatrixParameters(location, transpose, v, size, 4))
        return;
    m_context->uniformMatrix2fv(location->location(), transpose, v, size / 4);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniformMatrix3fv(const WebGLUniformLocation* location, GC3Dboolean transpose, Float32Array* v, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformMatrixParameters(location, transpose, v, 9))
        return;
    m_context->uniformMatrix3fv(location->location(), transpose, v->data(), v->length() / 9);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniformMatrix3fv(const WebGLUniformLocation* location, GC3Dboolean transpose, GC3Dfloat* v, GC3Dsizei size, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformMatrixParameters(location, transpose, v, size, 9))
        return;
    m_context->uniformMatrix3fv(location->location(), transpose, v, size / 9);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniformMatrix4fv(const WebGLUniformLocation* location, GC3Dboolean transpose, Float32Array* v, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformMatrixParameters(location, transpose, v, 16))
        return;
    m_context->uniformMatrix4fv(location->location(), transpose, v->data(), v->length() / 16);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniformMatrix4fv(const WebGLUniformLocation* location, GC3Dboolean transpose, GC3Dfloat* v, GC3Dsizei size, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformMatrixParameters(location, transpose, v, size, 16))
        return;
    m_context->uniformMatrix4fv(location->location(), transpose, v, size / 16);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::useProgram(WebGLProgram* program, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    bool deleted;
    if (!checkObjectToBeBound(program, deleted))
        return;
    if (deleted)
        program = 0;
    if (program && !program->getLinkStatus()) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        cleanupAfterGraphicsCall(false);
        return;
    }
    if (m_currentProgram != program) {
        if (m_currentProgram)
            m_currentProgram->onDetached();
        m_currentProgram = program;
        m_context->useProgram(objectOrZero(program));
        if (program)
            program->onAttached();
    }
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::validateProgram(WebGLProgram* program, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateWebGLObject(program))
        return;
    m_context->validateProgram(objectOrZero(program));
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::vertexAttrib1f(GC3Duint index, GC3Dfloat v0)
{
    vertexAttribfImpl(index, 1, v0, 0.0f, 0.0f, 1.0f);
}

void WebGLRenderingContext::vertexAttrib1fv(GC3Duint index, Float32Array* v)
{
    vertexAttribfvImpl(index, v, 1);
}

void WebGLRenderingContext::vertexAttrib1fv(GC3Duint index, GC3Dfloat* v, GC3Dsizei size)
{
    vertexAttribfvImpl(index, v, size, 1);
}

void WebGLRenderingContext::vertexAttrib2f(GC3Duint index, GC3Dfloat v0, GC3Dfloat v1)
{
    vertexAttribfImpl(index, 2, v0, v1, 0.0f, 1.0f);
}

void WebGLRenderingContext::vertexAttrib2fv(GC3Duint index, Float32Array* v)
{
    vertexAttribfvImpl(index, v, 2);
}

void WebGLRenderingContext::vertexAttrib2fv(GC3Duint index, GC3Dfloat* v, GC3Dsizei size)
{
    vertexAttribfvImpl(index, v, size, 2);
}

void WebGLRenderingContext::vertexAttrib3f(GC3Duint index, GC3Dfloat v0, GC3Dfloat v1, GC3Dfloat v2)
{
    vertexAttribfImpl(index, 3, v0, v1, v2, 1.0f);
}

void WebGLRenderingContext::vertexAttrib3fv(GC3Duint index, Float32Array* v)
{
    vertexAttribfvImpl(index, v, 3);
}

void WebGLRenderingContext::vertexAttrib3fv(GC3Duint index, GC3Dfloat* v, GC3Dsizei size)
{
    vertexAttribfvImpl(index, v, size, 3);
}

void WebGLRenderingContext::vertexAttrib4f(GC3Duint index, GC3Dfloat v0, GC3Dfloat v1, GC3Dfloat v2, GC3Dfloat v3)
{
    vertexAttribfImpl(index, 4, v0, v1, v2, v3);
}

void WebGLRenderingContext::vertexAttrib4fv(GC3Duint index, Float32Array* v)
{
    vertexAttribfvImpl(index, v, 4);
}

void WebGLRenderingContext::vertexAttrib4fv(GC3Duint index, GC3Dfloat* v, GC3Dsizei size)
{
    vertexAttribfvImpl(index, v, size, 4);
}

void WebGLRenderingContext::vertexAttribPointer(GC3Duint index, GC3Dint size, GC3Denum type, GC3Dboolean normalized, GC3Dsizei stride, GC3Dintptr offset, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return;
    switch (type) {
    case GraphicsContext3D::BYTE:
    case GraphicsContext3D::UNSIGNED_BYTE:
    case GraphicsContext3D::SHORT:
    case GraphicsContext3D::UNSIGNED_SHORT:
    case GraphicsContext3D::FLOAT:
        break;
    default:
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return;
    }
    if (index >= m_maxVertexAttribs) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return;
    }
    if (size < 1 || size > 4 || stride < 0 || stride > 255 || offset < 0) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return;
    }
    if (!m_boundArrayBuffer) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return;
    }
    // Determine the number of elements the bound buffer can hold, given the offset, size, type and stride
    unsigned int typeSize = sizeInBytes(type);
    if (!typeSize) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return;
    }
    if ((stride % typeSize) || (offset % typeSize)) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return;
    }
    GC3Dsizei bytesPerElement = size * typeSize;

    GC3Dsizei validatedStride = stride ? stride : bytesPerElement;

    WebGLVertexArrayObjectOES::VertexAttribState& state = m_boundVertexArrayObject->getVertexAttribState(index);
    state.bufferBinding = m_boundArrayBuffer;
    state.bytesPerElement = bytesPerElement;
    state.size = size;
    state.type = type;
    state.normalized = normalized;
    state.stride = validatedStride;
    state.originalStride = stride;
    state.offset = offset;
    m_context->vertexAttribPointer(index, size, type, normalized, stride, offset);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::viewport(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height)
{
    if (isContextLost())
        return;
    if (isnan(x))
        x = 0;
    if (isnan(y))
        y = 0;
    if (isnan(width))
        width = 100;
    if (isnan(height))
        height = 100;
    if (!validateSize(width, height))
        return;
    m_context->viewport(x, y, width, height);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::forceLostContext()
{
    if (isContextLost()) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return;
    }

    m_restoreTimer.startOneShot(0);
}

void WebGLRenderingContext::onLostContext()
{
    m_contextLost = true;

    detachAndRemoveAllObjects();

    // There is no direct way to clear errors from a GL implementation and
    // looping until getError() becomes NO_ERROR might cause an infinite loop if
    // the driver or context implementation had a bug.  So, loop a reasonably
    // large number of times to clear any existing errors.
    for (int i = 0; i < 100; ++i) {
        if (m_context->getError() == GraphicsContext3D::NO_ERROR)
            break;
    }
    m_context->synthesizeGLError(GraphicsContext3D::CONTEXT_LOST_WEBGL);

    canvas()->dispatchEvent(WebGLContextEvent::create(eventNames().webglcontextlostEvent, false, true, ""));
}

void WebGLRenderingContext::restoreContext()
{
    RefPtr<GraphicsContext3D> context(GraphicsContext3D::create(m_attributes, canvas()->document()->view()->root()->hostWindow()));
    if (!context)
        return;

    m_context = context;
    m_contextLost = false;
    initializeNewContext();
    canvas()->dispatchEvent(WebGLContextEvent::create(eventNames().webglcontextrestoredEvent, false, true, ""));
}

void WebGLRenderingContext::removeObject(WebGLObject* object)
{
    m_canvasObjects.remove(object);
}

void WebGLRenderingContext::addObject(WebGLObject* object)
{
    ASSERT(!isContextLost());
    removeObject(object);
    m_canvasObjects.add(object);
}

void WebGLRenderingContext::detachAndRemoveAllObjects()
{
    HashSet<RefPtr<WebGLObject> >::iterator pend = m_canvasObjects.end();
    for (HashSet<RefPtr<WebGLObject> >::iterator it = m_canvasObjects.begin(); it != pend; ++it)
        (*it)->detachContext();

    m_canvasObjects.clear();
}

WebGLTexture* WebGLRenderingContext::findTexture(Platform3DObject obj)
{
    if (!obj)
        return 0;
    HashSet<RefPtr<WebGLObject> >::iterator pend = m_canvasObjects.end();
    for (HashSet<RefPtr<WebGLObject> >::iterator it = m_canvasObjects.begin(); it != pend; ++it) {
        if ((*it)->isTexture() && (*it)->object() == obj)
            return reinterpret_cast<WebGLTexture*>((*it).get());
    }
    return 0;
}

WebGLRenderbuffer* WebGLRenderingContext::findRenderbuffer(Platform3DObject obj)
{
    if (!obj)
        return 0;
    HashSet<RefPtr<WebGLObject> >::iterator pend = m_canvasObjects.end();
    for (HashSet<RefPtr<WebGLObject> >::iterator it = m_canvasObjects.begin(); it != pend; ++it) {
        if ((*it)->isRenderbuffer() && (*it)->object() == obj)
            return reinterpret_cast<WebGLRenderbuffer*>((*it).get());
    }
    return 0;
}

WebGLBuffer* WebGLRenderingContext::findBuffer(Platform3DObject obj)
{
    if (!obj)
        return 0;
    HashSet<RefPtr<WebGLObject> >::iterator pend = m_canvasObjects.end();
    for (HashSet<RefPtr<WebGLObject> >::iterator it = m_canvasObjects.begin(); it != pend; ++it) {
        if ((*it)->isBuffer() && (*it)->object() == obj)
            return reinterpret_cast<WebGLBuffer*>((*it).get());
    }
    return 0;
}

WebGLShader* WebGLRenderingContext::findShader(Platform3DObject obj)
{
    if (!obj)
        return 0;
    HashSet<RefPtr<WebGLObject> >::iterator pend = m_canvasObjects.end();
    for (HashSet<RefPtr<WebGLObject> >::iterator it = m_canvasObjects.begin(); it != pend; ++it) {
        if ((*it)->isShader() && (*it)->object() == obj)
            return reinterpret_cast<WebGLShader*>((*it).get());
    }
    return 0;
}

WebGLGetInfo WebGLRenderingContext::getBooleanParameter(GC3Denum pname)
{
    GC3Dboolean value = 0;
    m_context->getBooleanv(pname, &value);
    return WebGLGetInfo(static_cast<bool>(value));
}

WebGLGetInfo WebGLRenderingContext::getBooleanArrayParameter(GC3Denum pname)
{
    if (pname != GraphicsContext3D::COLOR_WRITEMASK) {
        notImplemented();
        return WebGLGetInfo(0, 0);
    }
    GC3Dboolean value[4] = {0};
    m_context->getBooleanv(pname, value);
    bool boolValue[4];
    for (int ii = 0; ii < 4; ++ii)
        boolValue[ii] = static_cast<bool>(value[ii]);
    return WebGLGetInfo(boolValue, 4);
}

WebGLGetInfo WebGLRenderingContext::getFloatParameter(GC3Denum pname)
{
    GC3Dfloat value = 0;
    m_context->getFloatv(pname, &value);
    return WebGLGetInfo(value);
}

WebGLGetInfo WebGLRenderingContext::getIntParameter(GC3Denum pname)
{
    GC3Dint value = 0;
    m_context->getIntegerv(pname, &value);
    return WebGLGetInfo(value);
}

WebGLGetInfo WebGLRenderingContext::getUnsignedIntParameter(GC3Denum pname)
{
    GC3Dint value = 0;
    m_context->getIntegerv(pname, &value);
    return WebGLGetInfo(static_cast<unsigned int>(value));
}

WebGLGetInfo WebGLRenderingContext::getWebGLFloatArrayParameter(GC3Denum pname)
{
    GC3Dfloat value[4] = {0};
    m_context->getFloatv(pname, value);
    unsigned length = 0;
    switch (pname) {
    case GraphicsContext3D::ALIASED_POINT_SIZE_RANGE:
    case GraphicsContext3D::ALIASED_LINE_WIDTH_RANGE:
    case GraphicsContext3D::DEPTH_RANGE:
        length = 2;
        break;
    case GraphicsContext3D::BLEND_COLOR:
    case GraphicsContext3D::COLOR_CLEAR_VALUE:
        length = 4;
        break;
    default:
        notImplemented();
    }
    return WebGLGetInfo(Float32Array::create(value, length));
}

WebGLGetInfo WebGLRenderingContext::getWebGLIntArrayParameter(GC3Denum pname)
{
    GC3Dint value[4] = {0};
    m_context->getIntegerv(pname, value);
    unsigned length = 0;
    switch (pname) {
    case GraphicsContext3D::MAX_VIEWPORT_DIMS:
        length = 2;
        break;
    case GraphicsContext3D::SCISSOR_BOX:
    case GraphicsContext3D::VIEWPORT:
        length = 4;
        break;
    default:
        notImplemented();
    }
    return WebGLGetInfo(Int32Array::create(value, length));
}

void WebGLRenderingContext::handleNPOTTextures(bool prepareToDraw)
{
    bool resetActiveUnit = false;
    for (unsigned ii = 0; ii < m_textureUnits.size(); ++ii) {
        if ((m_textureUnits[ii].m_texture2DBinding && m_textureUnits[ii].m_texture2DBinding->needToUseBlackTexture())
            || (m_textureUnits[ii].m_textureCubeMapBinding && m_textureUnits[ii].m_textureCubeMapBinding->needToUseBlackTexture())) {
            if (ii != m_activeTextureUnit) {
                m_context->activeTexture(ii);
                resetActiveUnit = true;
            } else if (resetActiveUnit) {
                m_context->activeTexture(ii);
                resetActiveUnit = false;
            }
            WebGLTexture* tex2D;
            WebGLTexture* texCubeMap;
            if (prepareToDraw) {
                tex2D = m_blackTexture2D.get();
                texCubeMap = m_blackTextureCubeMap.get();
            } else {
                tex2D = m_textureUnits[ii].m_texture2DBinding.get();
                texCubeMap = m_textureUnits[ii].m_textureCubeMapBinding.get();
            }
            if (m_textureUnits[ii].m_texture2DBinding && m_textureUnits[ii].m_texture2DBinding->needToUseBlackTexture())
                m_context->bindTexture(GraphicsContext3D::TEXTURE_2D, objectOrZero(tex2D));
            if (m_textureUnits[ii].m_textureCubeMapBinding && m_textureUnits[ii].m_textureCubeMapBinding->needToUseBlackTexture())
                m_context->bindTexture(GraphicsContext3D::TEXTURE_CUBE_MAP, objectOrZero(texCubeMap));
        }
    }
    if (resetActiveUnit)
        m_context->activeTexture(m_activeTextureUnit);
}

void WebGLRenderingContext::createFallbackBlackTextures1x1()
{
    unsigned char black[] = {0, 0, 0, 255};
    m_blackTexture2D = createTexture();
    m_context->bindTexture(GraphicsContext3D::TEXTURE_2D, m_blackTexture2D->object());
    m_context->texImage2D(GraphicsContext3D::TEXTURE_2D, 0, GraphicsContext3D::RGBA, 1, 1,
                          0, GraphicsContext3D::RGBA, GraphicsContext3D::UNSIGNED_BYTE, black);
    m_context->bindTexture(GraphicsContext3D::TEXTURE_2D, 0);
    m_blackTextureCubeMap = createTexture();
    m_context->bindTexture(GraphicsContext3D::TEXTURE_CUBE_MAP, m_blackTextureCubeMap->object());
    m_context->texImage2D(GraphicsContext3D::TEXTURE_CUBE_MAP_POSITIVE_X, 0, GraphicsContext3D::RGBA, 1, 1,
                          0, GraphicsContext3D::RGBA, GraphicsContext3D::UNSIGNED_BYTE, black);
    m_context->texImage2D(GraphicsContext3D::TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GraphicsContext3D::RGBA, 1, 1,
                          0, GraphicsContext3D::RGBA, GraphicsContext3D::UNSIGNED_BYTE, black);
    m_context->texImage2D(GraphicsContext3D::TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GraphicsContext3D::RGBA, 1, 1,
                          0, GraphicsContext3D::RGBA, GraphicsContext3D::UNSIGNED_BYTE, black);
    m_context->texImage2D(GraphicsContext3D::TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GraphicsContext3D::RGBA, 1, 1,
                          0, GraphicsContext3D::RGBA, GraphicsContext3D::UNSIGNED_BYTE, black);
    m_context->texImage2D(GraphicsContext3D::TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GraphicsContext3D::RGBA, 1, 1,
                          0, GraphicsContext3D::RGBA, GraphicsContext3D::UNSIGNED_BYTE, black);
    m_context->texImage2D(GraphicsContext3D::TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GraphicsContext3D::RGBA, 1, 1,
                          0, GraphicsContext3D::RGBA, GraphicsContext3D::UNSIGNED_BYTE, black);
    m_context->bindTexture(GraphicsContext3D::TEXTURE_CUBE_MAP, 0);
}

bool WebGLRenderingContext::isTexInternalFormatColorBufferCombinationValid(GC3Denum texInternalFormat,
                                                                           GC3Denum colorBufferFormat)
{
    switch (colorBufferFormat) {
    case GraphicsContext3D::ALPHA:
        if (texInternalFormat == GraphicsContext3D::ALPHA)
            return true;
        break;
    case GraphicsContext3D::RGB:
        if (texInternalFormat == GraphicsContext3D::LUMINANCE
            || texInternalFormat == GraphicsContext3D::RGB)
            return true;
        break;
    case GraphicsContext3D::RGBA:
        return true;
    }
    return false;
}

GC3Denum WebGLRenderingContext::getBoundFramebufferColorFormat()
{
    if (m_framebufferBinding && m_framebufferBinding->object())
        return m_framebufferBinding->getColorBufferFormat();
    if (m_attributes.alpha)
        return GraphicsContext3D::RGBA;
    return GraphicsContext3D::RGB;
}

int WebGLRenderingContext::getBoundFramebufferWidth()
{
    if (m_framebufferBinding && m_framebufferBinding->object())
        return m_framebufferBinding->getWidth();
    return m_context->getInternalFramebufferSize().width();
}

int WebGLRenderingContext::getBoundFramebufferHeight()
{
    if (m_framebufferBinding && m_framebufferBinding->object())
        return m_framebufferBinding->getHeight();
    return m_context->getInternalFramebufferSize().height();
}

WebGLTexture* WebGLRenderingContext::validateTextureBinding(GC3Denum target, bool useSixEnumsForCubeMap)
{
    WebGLTexture* tex = 0;
    switch (target) {
    case GraphicsContext3D::TEXTURE_2D:
        tex = m_textureUnits[m_activeTextureUnit].m_texture2DBinding.get();
        break;
    case GraphicsContext3D::TEXTURE_CUBE_MAP_POSITIVE_X:
    case GraphicsContext3D::TEXTURE_CUBE_MAP_NEGATIVE_X:
    case GraphicsContext3D::TEXTURE_CUBE_MAP_POSITIVE_Y:
    case GraphicsContext3D::TEXTURE_CUBE_MAP_NEGATIVE_Y:
    case GraphicsContext3D::TEXTURE_CUBE_MAP_POSITIVE_Z:
    case GraphicsContext3D::TEXTURE_CUBE_MAP_NEGATIVE_Z:
        if (!useSixEnumsForCubeMap) {
            m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
            return 0;
        }
        tex = m_textureUnits[m_activeTextureUnit].m_textureCubeMapBinding.get();
        break;
    case GraphicsContext3D::TEXTURE_CUBE_MAP:
        if (useSixEnumsForCubeMap) {
            m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
            return 0;
        }
        tex = m_textureUnits[m_activeTextureUnit].m_textureCubeMapBinding.get();
        break;
    default:
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return 0;
    }
    if (!tex)
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
    return tex;
}

bool WebGLRenderingContext::validateSize(GC3Dint x, GC3Dint y)
{
    if (x < 0 || y < 0) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return false;
    }
    return true;
}

bool WebGLRenderingContext::validateString(const String& string)
{
    for (size_t i = 0; i < string.length(); ++i) {
        if (!validateCharacter(string[i])) {
            m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
            return false;
        }
    }
    return true;
}

bool WebGLRenderingContext::validateTexFuncFormatAndType(GC3Denum format, GC3Denum type)
{
    switch (format) {
    case GraphicsContext3D::ALPHA:
    case GraphicsContext3D::LUMINANCE:
    case GraphicsContext3D::LUMINANCE_ALPHA:
    case GraphicsContext3D::RGB:
    case GraphicsContext3D::RGBA:
        break;
    default:
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return false;
    }

    switch (type) {
    case GraphicsContext3D::UNSIGNED_BYTE:
    case GraphicsContext3D::UNSIGNED_SHORT_5_6_5:
    case GraphicsContext3D::UNSIGNED_SHORT_4_4_4_4:
    case GraphicsContext3D::UNSIGNED_SHORT_5_5_5_1:
        break;
    case GraphicsContext3D::FLOAT:
        if (m_oesTextureFloat)
            break;
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return false;
    default:
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return false;
    }

    // Verify that the combination of format and type is supported.
    switch (format) {
    case GraphicsContext3D::ALPHA:
    case GraphicsContext3D::LUMINANCE:
    case GraphicsContext3D::LUMINANCE_ALPHA:
        if (type != GraphicsContext3D::UNSIGNED_BYTE
            && type != GraphicsContext3D::FLOAT) {
            m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
            return false;
        }
        break;
    case GraphicsContext3D::RGB:
        if (type != GraphicsContext3D::UNSIGNED_BYTE
            && type != GraphicsContext3D::UNSIGNED_SHORT_5_6_5
            && type != GraphicsContext3D::FLOAT) {
            m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
            return false;
        }
        break;
    case GraphicsContext3D::RGBA:
        if (type != GraphicsContext3D::UNSIGNED_BYTE
            && type != GraphicsContext3D::UNSIGNED_SHORT_4_4_4_4
            && type != GraphicsContext3D::UNSIGNED_SHORT_5_5_5_1
            && type != GraphicsContext3D::FLOAT) {
            m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
            return false;
        }
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    return true;
}

bool WebGLRenderingContext::validateTexFuncLevel(GC3Denum target, GC3Dint level)
{
    if (level < 0) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return false;
    }
    switch (target) {
    case GraphicsContext3D::TEXTURE_2D:
        if (level > m_maxTextureLevel) {
            m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
            return false;
        }
        break;
    case GraphicsContext3D::TEXTURE_CUBE_MAP_POSITIVE_X:
    case GraphicsContext3D::TEXTURE_CUBE_MAP_NEGATIVE_X:
    case GraphicsContext3D::TEXTURE_CUBE_MAP_POSITIVE_Y:
    case GraphicsContext3D::TEXTURE_CUBE_MAP_NEGATIVE_Y:
    case GraphicsContext3D::TEXTURE_CUBE_MAP_POSITIVE_Z:
    case GraphicsContext3D::TEXTURE_CUBE_MAP_NEGATIVE_Z:
        if (level > m_maxCubeMapTextureLevel) {
            m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
            return false;
        }
        break;
    }
    // This function only checks if level is legal, so we return true and don't
    // generate INVALID_ENUM if target is illegal.
    return true;
}

bool WebGLRenderingContext::validateTexFuncParameters(GC3Denum target, GC3Dint level,
                                                      GC3Denum internalformat,
                                                      GC3Dsizei width, GC3Dsizei height, GC3Dint border,
                                                      GC3Denum format, GC3Denum type)
{
    // We absolutely have to validate the format and type combination.
    // The texImage2D entry points taking HTMLImage, etc. will produce
    // temporary data based on this combination, so it must be legal.
    if (!validateTexFuncFormatAndType(format, type) || !validateTexFuncLevel(target, level))
        return false;

    if (width < 0 || height < 0) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return false;
    }

    switch (target) {
    case GraphicsContext3D::TEXTURE_2D:
        if (width > m_maxTextureSize || height > m_maxTextureSize) {
            m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
            return false;
        }
        break;
    case GraphicsContext3D::TEXTURE_CUBE_MAP_POSITIVE_X:
    case GraphicsContext3D::TEXTURE_CUBE_MAP_NEGATIVE_X:
    case GraphicsContext3D::TEXTURE_CUBE_MAP_POSITIVE_Y:
    case GraphicsContext3D::TEXTURE_CUBE_MAP_NEGATIVE_Y:
    case GraphicsContext3D::TEXTURE_CUBE_MAP_POSITIVE_Z:
    case GraphicsContext3D::TEXTURE_CUBE_MAP_NEGATIVE_Z:
        if (width != height || width > m_maxCubeMapTextureSize) {
            m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
            return false;
        }
        break;
    default:
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return false;
    }

    if (format != internalformat) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return false;
    }

    if (border) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return false;
    }

    return true;
}

bool WebGLRenderingContext::validateTexFuncData(GC3Dsizei width, GC3Dsizei height,
                                                GC3Denum format, GC3Denum type,
                                                ArrayBufferView* pixels)
{
    if (!pixels)
        return true;

    if (!validateTexFuncFormatAndType(format, type))
        return false;

    switch (type) {
    case GraphicsContext3D::UNSIGNED_BYTE:
        if (!pixels->isUnsignedByteArray()) {
            m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
            return false;
        }
        break;
    case GraphicsContext3D::UNSIGNED_SHORT_5_6_5:
    case GraphicsContext3D::UNSIGNED_SHORT_4_4_4_4:
    case GraphicsContext3D::UNSIGNED_SHORT_5_5_5_1:
        if (!pixels->isUnsignedShortArray()) {
            m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
            return false;
        }
        break;
    case GraphicsContext3D::FLOAT: // OES_texture_float
        if (!pixels->isFloatArray()) {
            m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
            return false;
        }
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    unsigned int totalBytesRequired;
    GC3Denum error = m_context->computeImageSizeInBytes(format, type, width, height, m_unpackAlignment, &totalBytesRequired, 0);
    if (error != GraphicsContext3D::NO_ERROR) {
        m_context->synthesizeGLError(error);
        return false;
    }
    if (pixels->byteLength() < totalBytesRequired) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return false;
    }
    return true;
}

bool WebGLRenderingContext::validateDrawMode(GC3Denum mode)
{
    switch (mode) {
    case GraphicsContext3D::POINTS:
    case GraphicsContext3D::LINE_STRIP:
    case GraphicsContext3D::LINE_LOOP:
    case GraphicsContext3D::LINES:
    case GraphicsContext3D::TRIANGLE_STRIP:
    case GraphicsContext3D::TRIANGLE_FAN:
    case GraphicsContext3D::TRIANGLES:
        return true;
    default:
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return false;
    }
}

bool WebGLRenderingContext::validateStencilSettings()
{
    if (m_stencilMask != m_stencilMaskBack || m_stencilFuncRef != m_stencilFuncRefBack || m_stencilFuncMask != m_stencilFuncMaskBack) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return false;
    }
    return true;
}

bool WebGLRenderingContext::validateStencilFunc(GC3Denum func)
{
    switch (func) {
    case GraphicsContext3D::NEVER:
    case GraphicsContext3D::LESS:
    case GraphicsContext3D::LEQUAL:
    case GraphicsContext3D::GREATER:
    case GraphicsContext3D::GEQUAL:
    case GraphicsContext3D::EQUAL:
    case GraphicsContext3D::NOTEQUAL:
    case GraphicsContext3D::ALWAYS:
        return true;
    default:
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return false;
    }
}

void WebGLRenderingContext::printWarningToConsole(const String& message)
{
    canvas()->document()->frame()->domWindow()->console()->addMessage(HTMLMessageSource, LogMessageType, WarningMessageLevel,
                                                                      message, 0, canvas()->document()->url().string());
}

bool WebGLRenderingContext::validateFramebufferFuncParameters(GC3Denum target, GC3Denum attachment)
{
    if (target != GraphicsContext3D::FRAMEBUFFER) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return false;
    }
    switch (attachment) {
    case GraphicsContext3D::COLOR_ATTACHMENT0:
    case GraphicsContext3D::DEPTH_ATTACHMENT:
    case GraphicsContext3D::STENCIL_ATTACHMENT:
    case GraphicsContext3D::DEPTH_STENCIL_ATTACHMENT:
        break;
    default:
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return false;
    }
    return true;
}

bool WebGLRenderingContext::validateBlendEquation(GC3Denum mode)
{
    switch (mode) {
    case GraphicsContext3D::FUNC_ADD:
    case GraphicsContext3D::FUNC_SUBTRACT:
    case GraphicsContext3D::FUNC_REVERSE_SUBTRACT:
        return true;
    default:
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return false;
    }
}

bool WebGLRenderingContext::validateBlendFuncFactors(GC3Denum src, GC3Denum dst)
{
    if (((src == GraphicsContext3D::CONSTANT_COLOR || src == GraphicsContext3D::ONE_MINUS_CONSTANT_COLOR)
         && (dst == GraphicsContext3D::CONSTANT_ALPHA || dst == GraphicsContext3D::ONE_MINUS_CONSTANT_ALPHA))
        || ((dst == GraphicsContext3D::CONSTANT_COLOR || dst == GraphicsContext3D::ONE_MINUS_CONSTANT_COLOR)
            && (src == GraphicsContext3D::CONSTANT_ALPHA || src == GraphicsContext3D::ONE_MINUS_CONSTANT_ALPHA))) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return false;
    }
    return true;
}

bool WebGLRenderingContext::validateCapability(GC3Denum cap)
{
    switch (cap) {
    case GraphicsContext3D::BLEND:
    case GraphicsContext3D::CULL_FACE:
    case GraphicsContext3D::DEPTH_TEST:
    case GraphicsContext3D::DITHER:
    case GraphicsContext3D::POLYGON_OFFSET_FILL:
    case GraphicsContext3D::SAMPLE_ALPHA_TO_COVERAGE:
    case GraphicsContext3D::SAMPLE_COVERAGE:
    case GraphicsContext3D::SCISSOR_TEST:
    case GraphicsContext3D::STENCIL_TEST:
        return true;
    default:
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return false;
    }
}

bool WebGLRenderingContext::validateUniformParameters(const WebGLUniformLocation* location, Float32Array* v, GC3Dsizei requiredMinSize)
{
    if (!v) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return false;
    }
    return validateUniformMatrixParameters(location, false, v->data(), v->length(), requiredMinSize);
}

bool WebGLRenderingContext::validateUniformParameters(const WebGLUniformLocation* location, Int32Array* v, GC3Dsizei requiredMinSize)
{
    if (!v) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return false;
    }
    return validateUniformMatrixParameters(location, false, v->data(), v->length(), requiredMinSize);
}

bool WebGLRenderingContext::validateUniformParameters(const WebGLUniformLocation* location, void* v, GC3Dsizei size, GC3Dsizei requiredMinSize)
{
    return validateUniformMatrixParameters(location, false, v, size, requiredMinSize);
}

bool WebGLRenderingContext::validateUniformMatrixParameters(const WebGLUniformLocation* location, GC3Dboolean transpose, Float32Array* v, GC3Dsizei requiredMinSize)
{
    if (!v) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return false;
    }
    return validateUniformMatrixParameters(location, transpose, v->data(), v->length(), requiredMinSize);
}

bool WebGLRenderingContext::validateUniformMatrixParameters(const WebGLUniformLocation* location, GC3Dboolean transpose, void* v, GC3Dsizei size, GC3Dsizei requiredMinSize)
{
    if (!location)
        return false;
    if (location->program() != m_currentProgram) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return false;
    }
    if (!v) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return false;
    }
    if (transpose) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return false;
    }
    if (size < requiredMinSize || (size % requiredMinSize)) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return false;
    }
    return true;
}

WebGLBuffer* WebGLRenderingContext::validateBufferDataParameters(GC3Denum target, GC3Denum usage)
{
    WebGLBuffer* buffer = 0;
    switch (target) {
    case GraphicsContext3D::ELEMENT_ARRAY_BUFFER:
        buffer = m_boundVertexArrayObject->getElementArrayBuffer().get();
        break;
    case GraphicsContext3D::ARRAY_BUFFER:
        buffer = m_boundArrayBuffer.get();
        break;
    default:
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
        return 0;
    }
    if (!buffer) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_OPERATION);
        return 0;
    }
    switch (usage) {
    case GraphicsContext3D::STREAM_DRAW:
    case GraphicsContext3D::STATIC_DRAW:
    case GraphicsContext3D::DYNAMIC_DRAW:
        return buffer;
    }
    m_context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
    return 0;
}

bool WebGLRenderingContext::validateHTMLImageElement(HTMLImageElement* image)
{
    if (!image || !image->cachedImage()) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return false;
    }
    const KURL& url = image->cachedImage()->response().url();
    if (url.isNull() || url.isEmpty() || !url.isValid()) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return false;
    }
    return true;
}

void WebGLRenderingContext::vertexAttribfImpl(GC3Duint index, GC3Dsizei expectedSize, GC3Dfloat v0, GC3Dfloat v1, GC3Dfloat v2, GC3Dfloat v3)
{
    if (isContextLost())
        return;
    if (index >= m_maxVertexAttribs) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return;
    }
    // In GL, we skip setting vertexAttrib0 values.
    if (index || isGLES2Compliant()) {
        switch (expectedSize) {
        case 1:
            m_context->vertexAttrib1f(index, v0);
            break;
        case 2:
            m_context->vertexAttrib2f(index, v0, v1);
            break;
        case 3:
            m_context->vertexAttrib3f(index, v0, v1, v2);
            break;
        case 4:
            m_context->vertexAttrib4f(index, v0, v1, v2, v3);
            break;
        }
        cleanupAfterGraphicsCall(false);
    }
    VertexAttribValue& attribValue = m_vertexAttribValue[index];
    attribValue.value[0] = v0;
    attribValue.value[1] = v1;
    attribValue.value[2] = v2;
    attribValue.value[3] = v3;
}

void WebGLRenderingContext::vertexAttribfvImpl(GC3Duint index, Float32Array* v, GC3Dsizei expectedSize)
{
    if (isContextLost())
        return;
    if (!v) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return;
    }
    vertexAttribfvImpl(index, v->data(), v->length(), expectedSize);
}

void WebGLRenderingContext::vertexAttribfvImpl(GC3Duint index, GC3Dfloat* v, GC3Dsizei size, GC3Dsizei expectedSize)
{
    if (isContextLost())
        return;
    if (!v) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return;
    }
    if (size < expectedSize) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return;
    }
    if (index >= m_maxVertexAttribs) {
        m_context->synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return;
    }
    // In GL, we skip setting vertexAttrib0 values.
    if (index || isGLES2Compliant()) {
        switch (expectedSize) {
        case 1:
            m_context->vertexAttrib1fv(index, v);
            break;
        case 2:
            m_context->vertexAttrib2fv(index, v);
            break;
        case 3:
            m_context->vertexAttrib3fv(index, v);
            break;
        case 4:
            m_context->vertexAttrib4fv(index, v);
            break;
        }
        cleanupAfterGraphicsCall(false);
    }
    VertexAttribValue& attribValue = m_vertexAttribValue[index];
    attribValue.initValue();
    for (int ii = 0; ii < expectedSize; ++ii)
        attribValue.value[ii] = v[ii];
}

void WebGLRenderingContext::initVertexAttrib0()
{
    WebGLVertexArrayObjectOES::VertexAttribState& state = m_boundVertexArrayObject->getVertexAttribState(0);
    
    m_vertexAttrib0Buffer = createBuffer();
    m_context->bindBuffer(GraphicsContext3D::ARRAY_BUFFER, m_vertexAttrib0Buffer->object());
    m_context->bufferData(GraphicsContext3D::ARRAY_BUFFER, 0, GraphicsContext3D::DYNAMIC_DRAW);
    m_context->vertexAttribPointer(0, 4, GraphicsContext3D::FLOAT, false, 0, 0);
    state.bufferBinding = m_vertexAttrib0Buffer;
    m_context->bindBuffer(GraphicsContext3D::ARRAY_BUFFER, 0);
    m_context->enableVertexAttribArray(0);
    m_vertexAttrib0BufferSize = 0;
    m_vertexAttrib0BufferValue[0] = 0.0f;
    m_vertexAttrib0BufferValue[1] = 0.0f;
    m_vertexAttrib0BufferValue[2] = 0.0f;
    m_vertexAttrib0BufferValue[3] = 1.0f;
    m_forceAttrib0BufferRefill = false;
    m_vertexAttrib0UsedBefore = false;
}

bool WebGLRenderingContext::simulateVertexAttrib0(GC3Dsizei numVertex)
{
    const WebGLVertexArrayObjectOES::VertexAttribState& state = m_boundVertexArrayObject->getVertexAttribState(0);
    const VertexAttribValue& attribValue = m_vertexAttribValue[0];
    if (!m_currentProgram)
        return false;
    bool usingVertexAttrib0 = m_currentProgram->isUsingVertexAttrib0();
    if (usingVertexAttrib0)
        m_vertexAttrib0UsedBefore = true;
    if (state.enabled && usingVertexAttrib0)
        return false;
    if (!usingVertexAttrib0 && !m_vertexAttrib0UsedBefore)
        return false;
    m_vertexAttrib0UsedBefore = true;
    m_context->bindBuffer(GraphicsContext3D::ARRAY_BUFFER, m_vertexAttrib0Buffer->object());
    GC3Dsizeiptr bufferDataSize = (numVertex + 1) * 4 * sizeof(GC3Dfloat);
    if (bufferDataSize > m_vertexAttrib0BufferSize) {
        m_context->bufferData(GraphicsContext3D::ARRAY_BUFFER, bufferDataSize, 0, GraphicsContext3D::DYNAMIC_DRAW);
        m_vertexAttrib0BufferSize = bufferDataSize;
        m_forceAttrib0BufferRefill = true;
    }
    if (usingVertexAttrib0
        && (m_forceAttrib0BufferRefill
            || attribValue.value[0] != m_vertexAttrib0BufferValue[0]
            || attribValue.value[1] != m_vertexAttrib0BufferValue[1]
            || attribValue.value[2] != m_vertexAttrib0BufferValue[2]
            || attribValue.value[3] != m_vertexAttrib0BufferValue[3])) {
        OwnArrayPtr<GC3Dfloat> bufferData = adoptArrayPtr(new GC3Dfloat[(numVertex + 1) * 4]);
        for (GC3Dsizei ii = 0; ii < numVertex + 1; ++ii) {
            bufferData[ii * 4] = attribValue.value[0];
            bufferData[ii * 4 + 1] = attribValue.value[1];
            bufferData[ii * 4 + 2] = attribValue.value[2];
            bufferData[ii * 4 + 3] = attribValue.value[3];
        }
        m_vertexAttrib0BufferValue[0] = attribValue.value[0];
        m_vertexAttrib0BufferValue[1] = attribValue.value[1];
        m_vertexAttrib0BufferValue[2] = attribValue.value[2];
        m_vertexAttrib0BufferValue[3] = attribValue.value[3];
        m_forceAttrib0BufferRefill = false;
        m_context->bufferSubData(GraphicsContext3D::ARRAY_BUFFER, 0, bufferDataSize, bufferData.get());
    }
    m_context->vertexAttribPointer(0, 4, GraphicsContext3D::FLOAT, 0, 0, 0);
    return true;
}

void WebGLRenderingContext::restoreStatesAfterVertexAttrib0Simulation()
{
    const WebGLVertexArrayObjectOES::VertexAttribState& state = m_boundVertexArrayObject->getVertexAttribState(0);
    if (state.bufferBinding != m_vertexAttrib0Buffer) {
        m_context->bindBuffer(GraphicsContext3D::ARRAY_BUFFER, objectOrZero(state.bufferBinding.get()));
        m_context->vertexAttribPointer(0, state.size, state.type, state.normalized, state.originalStride, state.offset);
    }
    m_context->bindBuffer(GraphicsContext3D::ARRAY_BUFFER, objectOrZero(m_boundArrayBuffer.get()));
}

WebGLRenderingContext::LRUImageBufferCache::LRUImageBufferCache(int capacity)
    : m_buffers(adoptArrayPtr(new OwnPtr<ImageBuffer>[capacity]))
    , m_capacity(capacity)
{
}

ImageBuffer* WebGLRenderingContext::LRUImageBufferCache::imageBuffer(const IntSize& size)
{
    int i;
    for (i = 0; i < m_capacity; ++i) {
        ImageBuffer* buf = m_buffers[i].get();
        if (!buf)
            break;
        if (buf->size() != size)
            continue;
        bubbleToFront(i);
        return buf;
    }

    OwnPtr<ImageBuffer> temp = ImageBuffer::create(size);
    if (!temp)
        return 0;
    i = std::min(m_capacity - 1, i);
    m_buffers[i] = temp.release();

    ImageBuffer* buf = m_buffers[i].get();
    bubbleToFront(i);
    return buf;
}

void WebGLRenderingContext::LRUImageBufferCache::bubbleToFront(int idx)
{
    for (int i = idx; i > 0; --i)
        m_buffers[i].swap(m_buffers[i-1]);
}

} // namespace WebCore

#endif // ENABLE(WEBGL)
