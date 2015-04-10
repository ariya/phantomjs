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
#include "DOMWindow.h"
#include "EXTDrawBuffers.h"
#include "EXTTextureFilterAnisotropic.h"
#include "ExceptionCode.h"
#include "Extensions3D.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "FrameView.h"
#include "HTMLCanvasElement.h"
#include "HTMLImageElement.h"
#include "HTMLVideoElement.h"
#include "ImageBuffer.h"
#include "ImageData.h"
#include "IntSize.h"
#include "NotImplemented.h"
#include "OESElementIndexUint.h"
#include "OESStandardDerivatives.h"
#include "OESTextureFloat.h"
#include "OESTextureHalfFloat.h"
#include "OESVertexArrayObject.h"
#include "Page.h"
#include "RenderBox.h"
#include "Settings.h"
#include "WebGLActiveInfo.h"
#include "WebGLBuffer.h"
#include "WebGLCompressedTextureATC.h"
#include "WebGLCompressedTexturePVRTC.h"
#include "WebGLCompressedTextureS3TC.h"
#include "WebGLContextAttributes.h"
#include "WebGLContextEvent.h"
#include "WebGLContextGroup.h"
#include "WebGLDebugRendererInfo.h"
#include "WebGLDebugShaders.h"
#include "WebGLDepthTexture.h"
#include "WebGLFramebuffer.h"
#include "WebGLLoseContext.h"
#include "WebGLProgram.h"
#include "WebGLRenderbuffer.h"
#include "WebGLShader.h"
#include "WebGLShaderPrecisionFormat.h"
#include "WebGLTexture.h"
#include "WebGLUniformLocation.h"

#include <wtf/OwnArrayPtr.h>
#include <wtf/PassOwnArrayPtr.h>
#include <wtf/Uint32Array.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>

#if PLATFORM(QT)
#undef emit
#endif

namespace WebCore {

const double secondsBetweenRestoreAttempts = 1.0;
const int maxGLErrorsAllowedToConsole = 256;

namespace {

    class ScopedDrawingBufferBinder {
    public:
        ScopedDrawingBufferBinder(DrawingBuffer* drawingBuffer, WebGLFramebuffer* framebufferBinding)
            : m_drawingBuffer(drawingBuffer)
            , m_framebufferBinding(framebufferBinding)
        {
            // Commit DrawingBuffer if needed (e.g., for multisampling)
            if (!m_framebufferBinding && m_drawingBuffer)
                m_drawingBuffer->commit();
        }

        ~ScopedDrawingBufferBinder()
        {
            // Restore DrawingBuffer if needed
            if (!m_framebufferBinding && m_drawingBuffer)
                m_drawingBuffer->bind();
        }

    private:
        DrawingBuffer* m_drawingBuffer;
        WebGLFramebuffer* m_framebufferBinding;
    };

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

    GC3Dint clamp(GC3Dint value, GC3Dint min, GC3Dint max)
    {
        if (value < min)
            value = min;
        if (value > max)
            value = max;
        return value;
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

    bool isPrefixReserved(const String& name)
    {
        if (name.startsWith("gl_") || name.startsWith("webgl_") || name.startsWith("_webgl_"))
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
        bool hasMoreCharacters() const
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

        bool peek(UChar& character) const
        {
            if (m_position + 1 >= m_length)
                return false;
            character = m_sourceString[m_position + 1];
            return true;
        }

        UChar current() const
        {
            ASSERT_WITH_SECURITY_IMPLICATION(m_position < m_length);
            return m_sourceString[m_position];
        }

        void advance()
        {
            ++m_position;
        }

        bool isNewline(UChar character) const
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

class WebGLRenderingContextLostCallback : public GraphicsContext3D::ContextLostCallback {
    WTF_MAKE_FAST_ALLOCATED;
public:
    explicit WebGLRenderingContextLostCallback(WebGLRenderingContext* cb) : m_context(cb) { }
    virtual void onContextLost() { m_context->forceLostContext(WebGLRenderingContext::RealLostContext); }
    virtual ~WebGLRenderingContextLostCallback() {}
private:
    WebGLRenderingContext* m_context;
};

class WebGLRenderingContextErrorMessageCallback : public GraphicsContext3D::ErrorMessageCallback {
    WTF_MAKE_FAST_ALLOCATED;
public:
    explicit WebGLRenderingContextErrorMessageCallback(WebGLRenderingContext* cb) : m_context(cb) { }
    virtual void onErrorMessage(const String& message, GC3Dint)
    {
        if (m_context->m_synthesizedErrorsToConsole)
            m_context->printGLErrorToConsole(message);
    }
    virtual ~WebGLRenderingContextErrorMessageCallback() { }
private:
    WebGLRenderingContext* m_context;
};

PassOwnPtr<WebGLRenderingContext> WebGLRenderingContext::create(HTMLCanvasElement* canvas, WebGLContextAttributes* attrs)
{
    Document* document = canvas->document();
    Frame* frame = document->frame();
    if (!frame)
        return nullptr;
    Settings* settings = frame->settings();

    // The FrameLoaderClient might creation of a new WebGL context despite the page settings; in
    // particular, if WebGL contexts were lost one or more times via the GL_ARB_robustness extension.
    if (!frame->loader()->client()->allowWebGL(settings && settings->webGLEnabled())) {
        canvas->dispatchEvent(WebGLContextEvent::create(eventNames().webglcontextcreationerrorEvent, false, true, "Web page was not allowed to create a WebGL context."));
        return nullptr;
    }

    HostWindow* hostWindow = document->view()->root()->hostWindow();
    GraphicsContext3D::Attributes attributes = attrs ? attrs->attributes() : GraphicsContext3D::Attributes();

    if (attributes.antialias) {
        if (settings && !settings->openGLMultisamplingEnabled())
            attributes.antialias = false;
    }

    attributes.noExtensions = true;
    attributes.shareResources = false;
    attributes.preferDiscreteGPU = true;

    RefPtr<GraphicsContext3D> context(GraphicsContext3D::create(attributes, hostWindow));

    if (!context || !context->makeContextCurrent()) {
        canvas->dispatchEvent(WebGLContextEvent::create(eventNames().webglcontextcreationerrorEvent, false, true, "Could not create a WebGL context."));
        return nullptr;
    }

    Extensions3D* extensions = context->getExtensions();
    if (extensions->supports("GL_EXT_debug_marker"))
        extensions->pushGroupMarkerEXT("WebGLRenderingContext");

    OwnPtr<WebGLRenderingContext> renderingContext = adoptPtr(new WebGLRenderingContext(canvas, context, attributes));
    renderingContext->suspendIfNeeded();

    return renderingContext.release();
}

WebGLRenderingContext::WebGLRenderingContext(HTMLCanvasElement* passedCanvas, PassRefPtr<GraphicsContext3D> context,
                                             GraphicsContext3D::Attributes attributes)
    : CanvasRenderingContext(passedCanvas)
    , ActiveDOMObject(passedCanvas->document())
    , m_context(context)
    , m_drawingBuffer(0)
    , m_dispatchContextLostEventTimer(this, &WebGLRenderingContext::dispatchContextLostEvent)
    , m_restoreAllowed(false)
    , m_restoreTimer(this, &WebGLRenderingContext::maybeRestoreContext)
    , m_generatedImageCache(4)
    , m_contextLost(false)
    , m_contextLostMode(SyntheticLostContext)
    , m_attributes(attributes)
    , m_synthesizedErrorsToConsole(true)
    , m_numGLErrorsToConsoleAllowed(maxGLErrorsAllowedToConsole)
{
    ASSERT(m_context);
    m_contextGroup = WebGLContextGroup::create();
    m_contextGroup->addContext(this);

    m_maxViewportDims[0] = m_maxViewportDims[1] = 0;
    m_context->getIntegerv(GraphicsContext3D::MAX_VIEWPORT_DIMS, m_maxViewportDims);

    if (m_drawingBuffer)
        m_drawingBuffer->bind();

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
    m_stencilEnabled = false;
    m_stencilMask = 0xFFFFFFFF;
    m_stencilMaskBack = 0xFFFFFFFF;
    m_stencilFuncRef = 0;
    m_stencilFuncRefBack = 0;
    m_stencilFuncMask = 0xFFFFFFFF;
    m_stencilFuncMaskBack = 0xFFFFFFFF;
    m_layerCleared = false;
    m_numGLErrorsToConsoleAllowed = maxGLErrorsAllowedToConsole;
    
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
    m_maxRenderbufferSize = 0;
    m_context->getIntegerv(GraphicsContext3D::MAX_RENDERBUFFER_SIZE, &m_maxRenderbufferSize);

    // These two values from EXT_draw_buffers are lazily queried.
    m_maxDrawBuffers = 0;
    m_maxColorAttachments = 0;

    m_backDrawBuffer = GraphicsContext3D::BACK;
    m_drawBuffersWebGLRequirementsChecked = false;
    m_drawBuffersSupported = false;

    m_defaultVertexArrayObject = WebGLVertexArrayObjectOES::create(this, WebGLVertexArrayObjectOES::VaoTypeDefault);
    addContextObject(m_defaultVertexArrayObject.get());
    m_boundVertexArrayObject = m_defaultVertexArrayObject;
    
    m_vertexAttribValue.resize(m_maxVertexAttribs);

    if (!isGLES2NPOTStrict())
        createFallbackBlackTextures1x1();
    if (!isGLES2Compliant())
        initVertexAttrib0();

    IntSize canvasSize = clampedCanvasSize();
    if (m_drawingBuffer)
        m_drawingBuffer->reset(canvasSize);

    m_context->reshape(canvasSize.width(), canvasSize.height());
    m_context->viewport(0, 0, canvasSize.width(), canvasSize.height());
    m_context->scissor(0, 0, canvasSize.width(), canvasSize.height());

    m_context->setContextLostCallback(adoptPtr(new WebGLRenderingContextLostCallback(this)));
    m_context->setErrorMessageCallback(adoptPtr(new WebGLRenderingContextErrorMessageCallback(this)));
}

void WebGLRenderingContext::setupFlags()
{
    ASSERT(m_context);

    Page* p = canvas()->document()->page();
    if (p)
        m_synthesizedErrorsToConsole = p->settings()->webGLErrorsToConsoleEnabled();

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
    m_isRobustnessEXTSupported = m_context->getExtensions()->isEnabled("GL_EXT_robustness");
}

bool WebGLRenderingContext::allowPrivilegedExtensions() const
{
    Page* p = canvas()->document()->page();
    if (p && p->settings())
        return p->settings()->privilegedWebGLExtensionsEnabled();
    return false;
}

void WebGLRenderingContext::addCompressedTextureFormat(GC3Denum format)
{
    if (!m_compressedTextureFormats.contains(format))
        m_compressedTextureFormats.append(format);
}

WebGLRenderingContext::~WebGLRenderingContext()
{
    // Remove all references to WebGLObjects so if they are the last reference
    // they will be freed before the last context is removed from the context group.
    m_boundArrayBuffer = 0;
    m_defaultVertexArrayObject = 0;
    m_boundVertexArrayObject = 0;
    m_vertexAttrib0Buffer = 0;
    m_currentProgram = 0;
    m_framebufferBinding = 0;
    m_renderbufferBinding = 0;

    for (size_t i = 0; i < m_textureUnits.size(); ++i)
        m_textureUnits[i].m_textureBinding = 0;

    m_blackTexture2D = 0;
    m_blackTextureCubeMap = 0;

    detachAndRemoveAllObjects();
    destroyGraphicsContext3D();
    m_contextGroup->removeContext(this);
}

void WebGLRenderingContext::destroyGraphicsContext3D()
{
    // The drawing buffer holds a context reference. It must also be destroyed
    // in order for the context to be released.
    if (m_drawingBuffer)
        m_drawingBuffer.clear();

    if (m_context) {
        m_context->setContextLostCallback(nullptr);
        m_context->setErrorMessageCallback(nullptr);
        m_context.clear();
    }
}

void WebGLRenderingContext::markContextChanged()
{
    if (m_framebufferBinding)
        return;

    m_context->markContextChanged();

    if (m_drawingBuffer)
        m_drawingBuffer->markContentsChanged();

    m_layerCleared = false;
#if USE(ACCELERATED_COMPOSITING)
    RenderBox* renderBox = canvas()->renderBox();
    if (renderBox && renderBox->hasAcceleratedCompositing()) {
        m_markedCanvasDirty = true;
        canvas()->clearCopiedImage();
        renderBox->contentChanged(CanvasChanged);
    } else {
#endif
        if (!m_markedCanvasDirty) {
            m_markedCanvasDirty = true;
            canvas()->didDraw(FloatRect(FloatPoint(0, 0), clampedCanvasSize()));
        }
#if USE(ACCELERATED_COMPOSITING)
    }
#endif
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
    if (m_drawingBuffer)
        m_drawingBuffer->clearFramebuffers(clearMask);
    else {
        if (m_framebufferBinding)
            m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, 0);
        m_context->clear(clearMask);
    }

    restoreStateAfterClear();
    if (m_framebufferBinding)
        m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, objectOrZero(m_framebufferBinding.get()));
    m_layerCleared = true;

    return combinedClear;
}

void WebGLRenderingContext::restoreStateAfterClear()
{
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
}

void WebGLRenderingContext::markLayerComposited()
{
    m_context->markLayerComposited();
}

void WebGLRenderingContext::paintRenderingResultsToCanvas()
{
    if (canvas()->document()->printing())
        canvas()->clearPresentationCopy();

    // Until the canvas is written to by the application, the clear that
    // happened after it was composited should be ignored by the compositor.
    if (m_context->layerComposited() && !m_attributes.preserveDrawingBuffer) {
        m_context->paintCompositedResultsToCanvas(canvas()->buffer());

        canvas()->makePresentationCopy();
    } else
        canvas()->clearPresentationCopy();
    clearIfComposited();

    if (!m_markedCanvasDirty && !m_layerCleared)
        return;

    canvas()->clearCopiedImage();
    m_markedCanvasDirty = false;

    if (m_drawingBuffer)
        m_drawingBuffer->commit();
    m_context->paintRenderingResultsToCanvas(canvas()->buffer(), m_drawingBuffer.get());

    if (m_drawingBuffer) {
        if (m_framebufferBinding)
            m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, objectOrZero(m_framebufferBinding.get()));
        else
            m_drawingBuffer->bind();
    }
}

PassRefPtr<ImageData> WebGLRenderingContext::paintRenderingResultsToImageData()
{
    clearIfComposited();
    if (m_drawingBuffer)
        m_drawingBuffer->commit();
    RefPtr<ImageData> imageData = m_context->paintRenderingResultsToImageData(m_drawingBuffer.get());

    if (m_drawingBuffer) {
        if (m_framebufferBinding)
            m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, objectOrZero(m_framebufferBinding.get()));
        else
            m_drawingBuffer->bind();
    }

    return imageData;
}

void WebGLRenderingContext::reshape(int width, int height)
{
    // This is an approximation because at WebGLRenderingContext level we don't
    // know if the underlying FBO uses textures or renderbuffers.
    GC3Dint maxSize = std::min(m_maxTextureSize, m_maxRenderbufferSize);
    // Limit drawing buffer size to 4k to avoid memory exhaustion.
    const int sizeUpperLimit = 4096;
    maxSize = std::min(maxSize, sizeUpperLimit);
    GC3Dint maxWidth = std::min(maxSize, m_maxViewportDims[0]);
    GC3Dint maxHeight = std::min(maxSize, m_maxViewportDims[1]);
    width = clamp(width, 1, maxWidth);
    height = clamp(height, 1, maxHeight);

    if (m_needsUpdate) {
#if USE(ACCELERATED_COMPOSITING)
        RenderBox* renderBox = canvas()->renderBox();
        if (renderBox && renderBox->hasAcceleratedCompositing())
            renderBox->contentChanged(CanvasChanged);
#endif
        m_needsUpdate = false;
    }

    // We don't have to mark the canvas as dirty, since the newly created image buffer will also start off
    // clear (and this matches what reshape will do).
    if (m_drawingBuffer) {
        m_drawingBuffer->reset(IntSize(width, height));
        restoreStateAfterClear();
    } else
        m_context->reshape(width, height);

    m_context->bindTexture(GraphicsContext3D::TEXTURE_2D, objectOrZero(m_textureUnits[m_activeTextureUnit].m_textureBinding.get()));
    m_context->bindRenderbuffer(GraphicsContext3D::RENDERBUFFER, objectOrZero(m_renderbufferBinding.get()));
    if (m_framebufferBinding)
      m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, objectOrZero(m_framebufferBinding.get()));
}

int WebGLRenderingContext::drawingBufferWidth() const
{
    if (m_drawingBuffer)
        return m_drawingBuffer->size().width();

    return m_context->getInternalFramebufferSize().width();
}

int WebGLRenderingContext::drawingBufferHeight() const
{
    if (m_drawingBuffer)
        return m_drawingBuffer->size().height();

    return m_context->getInternalFramebufferSize().height();
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
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "activeTexture", "texture unit out of range");
        return;
    }
    m_activeTextureUnit = texture - GraphicsContext3D::TEXTURE0;
    m_context->activeTexture(texture);

    if (m_drawingBuffer)
        m_drawingBuffer->setActiveTextureUnit(texture);

    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::attachShader(WebGLProgram* program, WebGLShader* shader, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateWebGLObject("attachShader", program) || !validateWebGLObject("attachShader", shader))
        return;
    if (!program->attachShader(shader)) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "attachShader", "shader attachment already has shader");
        return;
    }
    m_context->attachShader(objectOrZero(program), objectOrZero(shader));
    shader->onAttached();
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::bindAttribLocation(WebGLProgram* program, GC3Duint index, const String& name, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateWebGLObject("bindAttribLocation", program))
        return;
    if (!validateLocationLength("bindAttribLocation", name))
        return;
    if (!validateString("bindAttribLocation", name))
        return;
    if (isPrefixReserved(name)) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "bindAttribLocation", "reserved prefix");
        return;
    }
    if (index >= m_maxVertexAttribs) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "bindAttribLocation", "index out of range");
        return;
    }
    m_context->bindAttribLocation(objectOrZero(program), index, name);
    cleanupAfterGraphicsCall(false);
}

bool WebGLRenderingContext::checkObjectToBeBound(const char* functionName, WebGLObject* object, bool& deleted)
{
    deleted = false;
    if (isContextLost())
        return false;
    if (object) {
        if (!object->validate(contextGroup(), this)) {
            synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, functionName, "object not from this context");
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
    if (!checkObjectToBeBound("bindBuffer", buffer, deleted))
        return;
    if (deleted)
        buffer = 0;
    if (buffer && buffer->getTarget() && buffer->getTarget() != target) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "bindBuffer", "buffers can not be used with multiple targets");
        return;
    }
    if (target == GraphicsContext3D::ARRAY_BUFFER)
        m_boundArrayBuffer = buffer;
    else if (target == GraphicsContext3D::ELEMENT_ARRAY_BUFFER)
        m_boundVertexArrayObject->setElementArrayBuffer(buffer);
    else {
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "bindBuffer", "invalid target");
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
    if (!checkObjectToBeBound("bindFramebuffer", buffer, deleted))
        return;
    if (deleted)
        buffer = 0;
    if (target != GraphicsContext3D::FRAMEBUFFER) {
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "bindFramebuffer", "invalid target");
        return;
    }
    m_framebufferBinding = buffer;
    if (m_drawingBuffer)
        m_drawingBuffer->setFramebufferBinding(objectOrZero(m_framebufferBinding.get()));
    if (!m_framebufferBinding && m_drawingBuffer) {
        // Instead of binding fb 0, bind the drawing buffer.
        m_drawingBuffer->bind();
    } else
        m_context->bindFramebuffer(target, objectOrZero(buffer));
    if (buffer)
        buffer->setHasEverBeenBound();
    applyStencilTest();
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::bindRenderbuffer(GC3Denum target, WebGLRenderbuffer* renderBuffer, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    bool deleted;
    if (!checkObjectToBeBound("bindRenderbuffer", renderBuffer, deleted))
        return;
    if (deleted)
        renderBuffer = 0;
    if (target != GraphicsContext3D::RENDERBUFFER) {
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "bindRenderbuffer", "invalid target");
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
    if (!checkObjectToBeBound("bindTexture", texture, deleted))
        return;
    if (deleted)
        texture = 0;
    if (texture && texture->getTarget() && texture->getTarget() != target) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "bindTexture", "textures can not be used with multiple targets");
        return;
    }
    GC3Dint maxLevel = 0;
    if (target == GraphicsContext3D::TEXTURE_2D) {
        m_textureUnits[m_activeTextureUnit].m_textureBinding = texture;
        maxLevel = m_maxTextureLevel;

        if (m_drawingBuffer && !m_activeTextureUnit)
            m_drawingBuffer->setTexture2DBinding(objectOrZero(texture));

    } else if (target == GraphicsContext3D::TEXTURE_CUBE_MAP) {
        m_textureUnits[m_activeTextureUnit].m_textureBinding = texture;
        maxLevel = m_maxCubeMapTextureLevel;
    } else {
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "bindTexture", "invalid target");
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
    if (isContextLost() || !validateBlendEquation("blendEquation", mode))
        return;
    m_context->blendEquation(mode);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::blendEquationSeparate(GC3Denum modeRGB, GC3Denum modeAlpha)
{
    if (isContextLost() || !validateBlendEquation("blendEquation", modeRGB) || !validateBlendEquation("blendEquation", modeAlpha))
        return;
    m_context->blendEquationSeparate(modeRGB, modeAlpha);
    cleanupAfterGraphicsCall(false);
}


void WebGLRenderingContext::blendFunc(GC3Denum sfactor, GC3Denum dfactor)
{
    if (isContextLost() || !validateBlendFuncFactors("blendFunc", sfactor, dfactor))
        return;
    m_context->blendFunc(sfactor, dfactor);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::blendFuncSeparate(GC3Denum srcRGB, GC3Denum dstRGB, GC3Denum srcAlpha, GC3Denum dstAlpha)
{
    // Note: Alpha does not have the same restrictions as RGB.
    if (isContextLost() || !validateBlendFuncFactors("blendFunc", srcRGB, dstRGB))
        return;
    m_context->blendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::bufferData(GC3Denum target, long long size, GC3Denum usage, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return;
    WebGLBuffer* buffer = validateBufferDataParameters("bufferData", target, usage);
    if (!buffer)
        return;
    if (size < 0) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "bufferData", "size < 0");
        return;
    }
    if (!size) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "bufferData", "size == 0");
        return;
    }
    if (!isErrorGeneratedOnOutOfBoundsAccesses()) {
        if (!buffer->associateBufferData(static_cast<GC3Dsizeiptr>(size))) {
            synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "bufferData", "invalid buffer");
            return;
        }
    }

    m_context->bufferData(target, static_cast<GC3Dsizeiptr>(size), usage);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::bufferData(GC3Denum target, ArrayBuffer* data, GC3Denum usage, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return;
    WebGLBuffer* buffer = validateBufferDataParameters("bufferData", target, usage);
    if (!buffer)
        return;
    if (!data) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "bufferData", "no data");
        return;
    }
    if (!isErrorGeneratedOnOutOfBoundsAccesses()) {
        if (!buffer->associateBufferData(data)) {
            synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "bufferData", "invalid buffer");
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
    WebGLBuffer* buffer = validateBufferDataParameters("bufferData", target, usage);
    if (!buffer)
        return;
    if (!data) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "bufferData", "no data");
        return;
    }
    if (!isErrorGeneratedOnOutOfBoundsAccesses()) {
        if (!buffer->associateBufferData(data)) {
            synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "bufferData", "invalid buffer");
            return;
        }
    }

    m_context->bufferData(target, data->byteLength(), data->baseAddress(), usage);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::bufferSubData(GC3Denum target, long long offset, ArrayBuffer* data, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return;
    WebGLBuffer* buffer = validateBufferDataParameters("bufferSubData", target, GraphicsContext3D::STATIC_DRAW);
    if (!buffer)
        return;
    if (offset < 0) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "bufferSubData", "offset < 0");
        return;
    }
    if (!data)
        return;
    if (!isErrorGeneratedOnOutOfBoundsAccesses()) {
        if (!buffer->associateBufferSubData(static_cast<GC3Dintptr>(offset), data)) {
            synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "bufferSubData", "offset out of range");
            return;
        }
    }

    m_context->bufferSubData(target, static_cast<GC3Dintptr>(offset), data->byteLength(), data->data());
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::bufferSubData(GC3Denum target, long long offset, ArrayBufferView* data, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return;
    WebGLBuffer* buffer = validateBufferDataParameters("bufferSubData", target, GraphicsContext3D::STATIC_DRAW);
    if (!buffer)
        return;
    if (offset < 0) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "bufferSubData", "offset < 0");
        return;
    }
    if (!data)
        return;
    if (!isErrorGeneratedOnOutOfBoundsAccesses()) {
        if (!buffer->associateBufferSubData(static_cast<GC3Dintptr>(offset), data)) {
            synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "bufferSubData", "offset out of range");
            return;
        }
    }

    m_context->bufferSubData(target, static_cast<GC3Dintptr>(offset), data->byteLength(), data->baseAddress());
    cleanupAfterGraphicsCall(false);
}

GC3Denum WebGLRenderingContext::checkFramebufferStatus(GC3Denum target)
{
    if (isContextLost())
        return GraphicsContext3D::FRAMEBUFFER_UNSUPPORTED;
    if (target != GraphicsContext3D::FRAMEBUFFER) {
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "checkFramebufferStatus", "invalid target");
        return 0;
    }
    if (!m_framebufferBinding || !m_framebufferBinding->object())
        return GraphicsContext3D::FRAMEBUFFER_COMPLETE;
    const char* reason = "framebuffer incomplete";
    GC3Denum result = m_framebufferBinding->checkStatus(&reason);
    if (result != GraphicsContext3D::FRAMEBUFFER_COMPLETE) {
        printGLWarningToConsole("checkFramebufferStatus", reason);
        return result;
    }
    result = m_context->checkFramebufferStatus(target);
    cleanupAfterGraphicsCall(false);
    return result;
}

void WebGLRenderingContext::clear(GC3Dbitfield mask)
{
    if (isContextLost())
        return;
    if (mask & ~(GraphicsContext3D::COLOR_BUFFER_BIT | GraphicsContext3D::DEPTH_BUFFER_BIT | GraphicsContext3D::STENCIL_BUFFER_BIT)) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "clear", "invalid mask");
        return;
    }
    const char* reason = "framebuffer incomplete";
    if (m_framebufferBinding && !m_framebufferBinding->onAccess(graphicsContext3D(), !isResourceSafe(), &reason)) {
        synthesizeGLError(GraphicsContext3D::INVALID_FRAMEBUFFER_OPERATION, "clear", reason);
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
    if (std::isnan(r))
        r = 0;
    if (std::isnan(g))
        g = 0;
    if (std::isnan(b))
        b = 0;
    if (std::isnan(a))
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
    if (isContextLost() || !validateWebGLObject("compileShader", shader))
        return;
    m_context->compileShader(objectOrZero(shader));
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::compressedTexImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat, GC3Dsizei width,
                                                 GC3Dsizei height, GC3Dint border, ArrayBufferView* data)
{
    if (isContextLost())
        return;
    if (!validateTexFuncLevel("compressedTexImage2D", target, level))
        return;

    if (!validateCompressedTexFormat(internalformat)) {
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "compressedTexImage2D", "invalid internalformat");
        return;
    }
    if (border) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "compressedTexImage2D", "border not 0");
        return;
    }
    if (!validateCompressedTexDimensions("compressedTexImage2D", level, width, height, internalformat))
        return;
    if (!validateCompressedTexFuncData("compressedTexImage2D", width, height, internalformat, data))
        return;

    WebGLTexture* tex = validateTextureBinding("compressedTexImage2D", target, true);
    if (!tex)
        return;
    if (!isGLES2NPOTStrict()) {
        if (level && WebGLTexture::isNPOT(width, height)) {
            synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "compressedTexImage2D", "level > 0 not power of 2");
            return;
        }
    }
    graphicsContext3D()->compressedTexImage2D(target, level, internalformat, width, height,
                                              border, data->byteLength(), data->baseAddress());
    tex->setLevelInfo(target, level, internalformat, width, height, GraphicsContext3D::UNSIGNED_BYTE);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::compressedTexSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset,
                                                    GC3Dsizei width, GC3Dsizei height, GC3Denum format, ArrayBufferView* data)
{
    if (isContextLost())
        return;
    if (!validateTexFuncLevel("compressedTexSubImage2D", target, level))
        return;
    if (!validateCompressedTexFormat(format)) {
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "compressedTexSubImage2D", "invalid format");
        return;
    }
    if (!validateCompressedTexFuncData("compressedTexSubImage2D", width, height, format, data))
        return;

    WebGLTexture* tex = validateTextureBinding("compressedTexSubImage2D", target, true);
    if (!tex)
        return;

    if (format != tex->getInternalFormat(target, level)) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "compressedTexSubImage2D", "format does not match texture format");
        return;
    }

    if (!validateCompressedTexSubDimensions("compressedTexSubImage2D", target, level, xoffset, yoffset, width, height, format, tex))
        return;

    graphicsContext3D()->compressedTexSubImage2D(target, level, xoffset, yoffset,
                                                 width, height, format, data->byteLength(), data->baseAddress());
    cleanupAfterGraphicsCall(false);
}

bool WebGLRenderingContext::validateSettableTexFormat(const char* functionName, GC3Denum format)
{
    if (GraphicsContext3D::getClearBitsByFormat(format) & (GraphicsContext3D::DEPTH_BUFFER_BIT | GraphicsContext3D::STENCIL_BUFFER_BIT)) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, functionName, "format can not be set, only rendered to");
        return false;
    }
    return true;
}

void WebGLRenderingContext::copyTexImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat, GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height, GC3Dint border)
{
    if (isContextLost())
        return;
    if (!validateTexFuncParameters("copyTexImage2D", NotTexSubImage2D, target, level, internalformat, width, height, border, internalformat, GraphicsContext3D::UNSIGNED_BYTE))
        return;
    if (!validateSettableTexFormat("copyTexImage2D", internalformat))
        return;
    WebGLTexture* tex = validateTextureBinding("copyTexImage2D", target, true);
    if (!tex)
        return;
    if (!isTexInternalFormatColorBufferCombinationValid(internalformat, getBoundFramebufferColorFormat())) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "copyTexImage2D", "framebuffer is incompatible format");
        return;
    }
    if (!isGLES2NPOTStrict() && level && WebGLTexture::isNPOT(width, height)) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "copyTexImage2D", "level > 0 not power of 2");
        return;
    }
    const char* reason = "framebuffer incomplete";
    if (m_framebufferBinding && !m_framebufferBinding->onAccess(graphicsContext3D(), !isResourceSafe(), &reason)) {
        synthesizeGLError(GraphicsContext3D::INVALID_FRAMEBUFFER_OPERATION, "copyTexImage2D", reason);
        return;
    }
    clearIfComposited();
    if (isResourceSafe()) {
        ScopedDrawingBufferBinder binder(m_drawingBuffer.get(), m_framebufferBinding.get());
        m_context->copyTexImage2D(target, level, internalformat, x, y, width, height, border);
    } else {
        ScopedDrawingBufferBinder binder(m_drawingBuffer.get(), m_framebufferBinding.get());
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
    if (!validateTexFuncLevel("copyTexSubImage2D", target, level))
        return;
    WebGLTexture* tex = validateTextureBinding("copyTexSubImage2D", target, true);
    if (!tex)
        return;
    if (!validateSize("copyTexSubImage2D", xoffset, yoffset) || !validateSize("copyTexSubImage2D", width, height))
        return;
    // Before checking if it is in the range, check if overflow happens first.
    if (xoffset + width < 0 || yoffset + height < 0) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "copyTexSubImage2D", "bad dimensions");
        return;
    }
    if (xoffset + width > tex->getWidth(target, level) || yoffset + height > tex->getHeight(target, level)) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "copyTexSubImage2D", "rectangle out of range");
        return;
    }
    GC3Denum internalformat = tex->getInternalFormat(target, level);
    if (!validateSettableTexFormat("copyTexSubImage2D", internalformat))
        return;
    if (!isTexInternalFormatColorBufferCombinationValid(internalformat, getBoundFramebufferColorFormat())) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "copyTexSubImage2D", "framebuffer is incompatible format");
        return;
    }
    const char* reason = "framebuffer incomplete";
    if (m_framebufferBinding && !m_framebufferBinding->onAccess(graphicsContext3D(), !isResourceSafe(), &reason)) {
        synthesizeGLError(GraphicsContext3D::INVALID_FRAMEBUFFER_OPERATION, "copyTexSubImage2D", reason);
        return;
    }
    clearIfComposited();
    if (isResourceSafe()) {
        ScopedDrawingBufferBinder binder(m_drawingBuffer.get(), m_framebufferBinding.get());
        m_context->copyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
    } else {
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
                    synthesizeGLError(error, "copyTexSubImage2D", "bad dimensions");
                    return;
                }
                zero = adoptArrayPtr(new unsigned char[size]);
                if (!zero) {
                    synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "copyTexSubImage2D", "out of memory");
                    return;
                }
                memset(zero.get(), 0, size);
            }
            m_context->texSubImage2D(target, level, xoffset, yoffset, width, height, format, type, zero.get());
            if (clippedWidth > 0 && clippedHeight > 0) {
                ScopedDrawingBufferBinder binder(m_drawingBuffer.get(), m_framebufferBinding.get());
                m_context->copyTexSubImage2D(target, level, xoffset + clippedX - x, yoffset + clippedY - y,
                                             clippedX, clippedY, clippedWidth, clippedHeight);
            }
        } else {
            ScopedDrawingBufferBinder binder(m_drawingBuffer.get(), m_framebufferBinding.get());
            m_context->copyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
        }
    }
    cleanupAfterGraphicsCall(false);
}

PassRefPtr<WebGLBuffer> WebGLRenderingContext::createBuffer()
{
    if (isContextLost())
        return 0;
    RefPtr<WebGLBuffer> o = WebGLBuffer::create(this);
    addSharedObject(o.get());
    return o;
}

PassRefPtr<WebGLFramebuffer> WebGLRenderingContext::createFramebuffer()
{
    if (isContextLost())
        return 0;
    RefPtr<WebGLFramebuffer> o = WebGLFramebuffer::create(this);
    addContextObject(o.get());
    return o;
}

PassRefPtr<WebGLTexture> WebGLRenderingContext::createTexture()
{
    if (isContextLost())
        return 0;
    RefPtr<WebGLTexture> o = WebGLTexture::create(this);
    addSharedObject(o.get());
    return o;
}

PassRefPtr<WebGLProgram> WebGLRenderingContext::createProgram()
{
    if (isContextLost())
        return 0;
    RefPtr<WebGLProgram> o = WebGLProgram::create(this);
    addSharedObject(o.get());
    return o;
}

PassRefPtr<WebGLRenderbuffer> WebGLRenderingContext::createRenderbuffer()
{
    if (isContextLost())
        return 0;
    RefPtr<WebGLRenderbuffer> o = WebGLRenderbuffer::create(this);
    addSharedObject(o.get());
    return o;
}

PassRefPtr<WebGLShader> WebGLRenderingContext::createShader(GC3Denum type, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return 0;
    if (type != GraphicsContext3D::VERTEX_SHADER && type != GraphicsContext3D::FRAGMENT_SHADER) {
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "createShader", "invalid shader type");
        return 0;
    }

    RefPtr<WebGLShader> o = WebGLShader::create(this, type);
    addSharedObject(o.get());
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
    if (!object->validate(contextGroup(), this)) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "delete", "object does not belong to this context");
        return false;
    }
    if (object->object())
        // We need to pass in context here because we want
        // things in this context unbound.
        object->deleteObject(graphicsContext3D());
    return true;
}

void WebGLRenderingContext::deleteBuffer(WebGLBuffer* buffer)
{
    if (!deleteObject(buffer))
        return;
    if (m_boundArrayBuffer == buffer)
        m_boundArrayBuffer = 0;

    m_boundVertexArrayObject->unbindBuffer(buffer);
}

void WebGLRenderingContext::deleteFramebuffer(WebGLFramebuffer* framebuffer)
{
    if (!deleteObject(framebuffer))
        return;
    if (framebuffer == m_framebufferBinding) {
        m_framebufferBinding = 0;
        if (m_drawingBuffer) {
            m_drawingBuffer->setFramebufferBinding(0);
            // Have to call bindFramebuffer here to bind back to internal fbo.
            m_drawingBuffer->bind();
        } else
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
        m_framebufferBinding->removeAttachmentFromBoundFramebuffer(renderbuffer);
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
        if (texture == m_textureUnits[i].m_textureBinding)
            m_textureUnits[i].m_textureBinding = 0;
    }
    if (m_framebufferBinding)
        m_framebufferBinding->removeAttachmentFromBoundFramebuffer(texture);
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
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "depthRange", "zNear > zFar");
        return;
    }
    m_context->depthRange(zNear, zFar);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::detachShader(WebGLProgram* program, WebGLShader* shader, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateWebGLObject("detachShader", program) || !validateWebGLObject("detachShader", shader))
        return;
    if (!program->detachShader(shader)) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "detachShader", "shader not attached");
        return;
    }
    m_context->detachShader(objectOrZero(program), objectOrZero(shader));
    shader->onDetached(graphicsContext3D());
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::disable(GC3Denum cap)
{
    if (isContextLost() || !validateCapability("disable", cap))
        return;
    if (cap == GraphicsContext3D::STENCIL_TEST) {
        m_stencilEnabled = false;
        applyStencilTest();
        cleanupAfterGraphicsCall(false);
        return;
    }
    if (cap == GraphicsContext3D::SCISSOR_TEST) {
        m_scissorEnabled = false;
        if (m_drawingBuffer)
            m_drawingBuffer->setScissorEnabled(m_scissorEnabled);
    }
    m_context->disable(cap);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::disableVertexAttribArray(GC3Duint index, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return;
    if (index >= m_maxVertexAttribs) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "disableVertexAttribArray", "index out of range");
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

    if (type == GraphicsContext3D::UNSIGNED_INT) {
        // For an unsigned int array, offset must be divisible by 4 for alignment reasons.
        if (offset % 4)
            return false;

        // Make uoffset an element offset.
        offset /= 4;

        GC3Dsizeiptr n = elementArrayBuffer->byteLength() / 4;
        if (offset > n || count > n - offset)
            return false;
    } else if (type == GraphicsContext3D::UNSIGNED_SHORT) {
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

bool WebGLRenderingContext::validateIndexArrayConservative(GC3Denum type, unsigned& numElementsRequired)
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
        case GraphicsContext3D::UNSIGNED_INT: {
            if (!m_oesElementIndexUint)
                return false;
            numElements /= sizeof(GC3Duint);
            const GC3Duint* p = static_cast<const GC3Duint*>(buffer->data());
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

bool WebGLRenderingContext::validateIndexArrayPrecise(GC3Dsizei count, GC3Denum type, GC3Dintptr offset, unsigned& numElementsRequired)
{
    ASSERT(count >= 0 && offset >= 0);
    unsigned lastIndex = 0;
    
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

    if (type == GraphicsContext3D::UNSIGNED_INT) {
        // Make uoffset an element offset.
        uoffset /= sizeof(GC3Duint);
        const GC3Duint* p = static_cast<const GC3Duint*>(elementArrayBuffer->elementArrayBuffer()->data()) + uoffset;
        while (n-- > 0) {
            if (*p > lastIndex)
                lastIndex = *p;
            ++p;
        }
    } else if (type == GraphicsContext3D::UNSIGNED_SHORT) {
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

bool WebGLRenderingContext::validateRenderingState(unsigned numElementsRequired)
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
    unsigned smallestNumElements = UINT_MAX;
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
                unsigned numElements = 0;
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

bool WebGLRenderingContext::validateWebGLObject(const char* functionName, WebGLObject* object)
{
    if (!object || !object->object()) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "no object or object deleted");
        return false;
    }
    if (!object->validate(contextGroup(), this)) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, functionName, "object does not belong to this context");
        return false;
    }
    return true;
}

void WebGLRenderingContext::drawArrays(GC3Denum mode, GC3Dint first, GC3Dsizei count, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);

    if (isContextLost() || !validateDrawMode("drawArrays", mode))
        return;

    if (!validateStencilSettings("drawArrays"))
        return;

    if (first < 0 || count < 0) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "drawArrays", "first or count < 0");
        return;
    }

    if (!count) {
        cleanupAfterGraphicsCall(true);
        return;
    }

    if (!isErrorGeneratedOnOutOfBoundsAccesses()) {
        // Ensure we have a valid rendering state
        Checked<GC3Dint, RecordOverflow> checkedFirst(first);
        Checked<GC3Dint, RecordOverflow> checkedCount(count);
        Checked<GC3Dint, RecordOverflow> checkedSum = checkedFirst + checkedCount;
        if (checkedSum.hasOverflowed() || !validateRenderingState(checkedSum.unsafeGet())) {
            synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "drawArrays", "attempt to access out of bounds arrays");
            return;
        }
    } else {
        if (!validateRenderingState(0)) {
            synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "drawArrays", "attribs not setup correctly");
            return;
        }
    }

    const char* reason = "framebuffer incomplete";
    if (m_framebufferBinding && !m_framebufferBinding->onAccess(graphicsContext3D(), !isResourceSafe(), &reason)) {
        synthesizeGLError(GraphicsContext3D::INVALID_FRAMEBUFFER_OPERATION, "drawArrays", reason);
        return;
    }

    clearIfComposited();

    bool vertexAttrib0Simulated = false;
    if (!isGLES2Compliant())
        vertexAttrib0Simulated = simulateVertexAttrib0(first + count - 1);
    if (!isGLES2NPOTStrict())
        handleNPOTTextures("drawArrays", true);
    m_context->drawArrays(mode, first, count);
    if (!isGLES2Compliant() && vertexAttrib0Simulated)
        restoreStatesAfterVertexAttrib0Simulation();
    if (!isGLES2NPOTStrict())
        handleNPOTTextures("drawArrays", false);
    cleanupAfterGraphicsCall(true);
}

void WebGLRenderingContext::drawElements(GC3Denum mode, GC3Dsizei count, GC3Denum type, long long offset, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);

    if (isContextLost() || !validateDrawMode("drawElements", mode))
        return;

    if (!validateStencilSettings("drawElements"))
        return;

    switch (type) {
    case GraphicsContext3D::UNSIGNED_BYTE:
    case GraphicsContext3D::UNSIGNED_SHORT:
        break;
    case GraphicsContext3D::UNSIGNED_INT:
        if (m_oesElementIndexUint)
            break;
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "drawElements", "invalid type");
        return;
    default:
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "drawElements", "invalid type");
        return;
    }

    if (count < 0 || offset < 0) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "drawElements", "count or offset < 0");
        return;
    }

    if (!count) {
        cleanupAfterGraphicsCall(true);
        return;
    }

    if (!m_boundVertexArrayObject->getElementArrayBuffer()) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "drawElements", "no ELEMENT_ARRAY_BUFFER bound");
        return;
    }

    unsigned numElements = 0;
    if (!isErrorGeneratedOnOutOfBoundsAccesses()) {
        // Ensure we have a valid rendering state
        if (!validateElementArraySize(count, type, static_cast<GC3Dintptr>(offset))) {
            synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "drawElements", "request out of bounds for current ELEMENT_ARRAY_BUFFER");
            return;
        }
        if (!count)
            return;
        if (!validateIndexArrayConservative(type, numElements) || !validateRenderingState(numElements)) {
            if (!validateIndexArrayPrecise(count, type, static_cast<GC3Dintptr>(offset), numElements) || !validateRenderingState(numElements)) {
                synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "drawElements", "attempt to access out of bounds arrays");
                return;
            }
        }
    } else {
        if (!validateRenderingState(0)) {
            synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "drawElements", "attribs not setup correctly");
            return;
        }
    }

    const char* reason = "framebuffer incomplete";
    if (m_framebufferBinding && !m_framebufferBinding->onAccess(graphicsContext3D(), !isResourceSafe(), &reason)) {
        synthesizeGLError(GraphicsContext3D::INVALID_FRAMEBUFFER_OPERATION, "drawElements", reason);
        return;
    }
    clearIfComposited();

    bool vertexAttrib0Simulated = false;
    if (!isGLES2Compliant()) {
        if (!numElements)
            validateIndexArrayPrecise(count, type, static_cast<GC3Dintptr>(offset), numElements);
        vertexAttrib0Simulated = simulateVertexAttrib0(numElements);
    }
    if (!isGLES2NPOTStrict())
        handleNPOTTextures("drawElements", true);
    m_context->drawElements(mode, count, type, static_cast<GC3Dintptr>(offset));
    if (!isGLES2Compliant() && vertexAttrib0Simulated)
        restoreStatesAfterVertexAttrib0Simulation();
    if (!isGLES2NPOTStrict())
        handleNPOTTextures("drawElements", false);
    cleanupAfterGraphicsCall(true);
}

void WebGLRenderingContext::enable(GC3Denum cap)
{
    if (isContextLost() || !validateCapability("enable", cap))
        return;
    if (cap == GraphicsContext3D::STENCIL_TEST) {
        m_stencilEnabled = true;
        applyStencilTest();
        cleanupAfterGraphicsCall(false);
        return;
    }
    if (cap == GraphicsContext3D::SCISSOR_TEST) {
        m_scissorEnabled = true;
        if (m_drawingBuffer)
            m_drawingBuffer->setScissorEnabled(m_scissorEnabled);
    }
    m_context->enable(cap);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::enableVertexAttribArray(GC3Duint index, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return;
    if (index >= m_maxVertexAttribs) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "enableVertexAttribArray", "index out of range");
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
    if (isContextLost() || !validateFramebufferFuncParameters("framebufferRenderbuffer", target, attachment))
        return;
    if (renderbuffertarget != GraphicsContext3D::RENDERBUFFER) {
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "framebufferRenderbuffer", "invalid target");
        return;
    }
    if (buffer && !buffer->validate(contextGroup(), this)) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "framebufferRenderbuffer", "no buffer or buffer not from this context");
        return;
    }
    // Don't allow the default framebuffer to be mutated; all current
    // implementations use an FBO internally in place of the default
    // FBO.
    if (!m_framebufferBinding || !m_framebufferBinding->object()) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "framebufferRenderbuffer", "no framebuffer bound");
        return;
    }
    Platform3DObject bufferObject = objectOrZero(buffer);
    switch (attachment) {
    case GraphicsContext3D::DEPTH_STENCIL_ATTACHMENT:
        m_context->framebufferRenderbuffer(target, GraphicsContext3D::DEPTH_ATTACHMENT, renderbuffertarget, bufferObject);
        m_context->framebufferRenderbuffer(target, GraphicsContext3D::STENCIL_ATTACHMENT, renderbuffertarget, bufferObject);
        break;
    default:
        m_context->framebufferRenderbuffer(target, attachment, renderbuffertarget, bufferObject);
    }
    m_framebufferBinding->setAttachmentForBoundFramebuffer(attachment, buffer);
    applyStencilTest();
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::framebufferTexture2D(GC3Denum target, GC3Denum attachment, GC3Denum textarget, WebGLTexture* texture, GC3Dint level, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateFramebufferFuncParameters("framebufferTexture2D", target, attachment))
        return;
    if (level) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "framebufferTexture2D", "level not 0");
        return;
    }
    if (texture && !texture->validate(contextGroup(), this)) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "framebufferTexture2D", "no texture or texture not from this context");
        return;
    }
    // Don't allow the default framebuffer to be mutated; all current
    // implementations use an FBO internally in place of the default
    // FBO.
    if (!m_framebufferBinding || !m_framebufferBinding->object()) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "framebufferTexture2D", "no framebuffer bound");
        return;
    }
    Platform3DObject textureObject = objectOrZero(texture);
    switch (attachment) {
    case GraphicsContext3D::DEPTH_STENCIL_ATTACHMENT:
        m_context->framebufferTexture2D(target, GraphicsContext3D::DEPTH_ATTACHMENT, textarget, textureObject, level);
        m_context->framebufferTexture2D(target, GraphicsContext3D::STENCIL_ATTACHMENT, textarget, textureObject, level);
        break;
    case GraphicsContext3D::DEPTH_ATTACHMENT:
        m_context->framebufferTexture2D(target, attachment, textarget, textureObject, level);
        break;
    case GraphicsContext3D::STENCIL_ATTACHMENT:
        m_context->framebufferTexture2D(target, attachment, textarget, textureObject, level);
        break;
    default:
        m_context->framebufferTexture2D(target, attachment, textarget, textureObject, level);
    }
    m_framebufferBinding->setAttachmentForBoundFramebuffer(attachment, textarget, texture, level);
    applyStencilTest();
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
    WebGLTexture* tex = validateTextureBinding("generateMipmap", target, false);
    if (!tex)
        return;
    if (!tex->canGenerateMipmaps()) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "generateMipmap", "level 0 not power of 2 or not all the same size");
        return;
    }
    if (!validateSettableTexFormat("generateMipmap", tex->getInternalFormat(target, 0)))
        return;

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
    if (isContextLost() || !validateWebGLObject("getActiveAttrib", program))
        return 0;
    ActiveInfo info;
    if (!m_context->getActiveAttrib(objectOrZero(program), index, info))
        return 0;
    return WebGLActiveInfo::create(info.name, info.type, info.size);
}

PassRefPtr<WebGLActiveInfo> WebGLRenderingContext::getActiveUniform(WebGLProgram* program, GC3Duint index, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateWebGLObject("getActiveUniform", program))
        return 0;
    ActiveInfo info;
    if (!m_context->getActiveUniform(objectOrZero(program), index, info))
        return 0;
    if (!isGLES2Compliant())
        if (info.size > 1 && !info.name.endsWith("[0]"))
            info.name.append("[0]");
    return WebGLActiveInfo::create(info.name, info.type, info.size);
}

bool WebGLRenderingContext::getAttachedShaders(WebGLProgram* program, Vector<RefPtr<WebGLShader> >& shaderObjects, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    shaderObjects.clear();
    if (isContextLost() || !validateWebGLObject("getAttachedShaders", program))
        return false;

    const GC3Denum shaderType[] = {
        GraphicsContext3D::VERTEX_SHADER,
        GraphicsContext3D::FRAGMENT_SHADER
    };
    for (unsigned i = 0; i < sizeof(shaderType) / sizeof(GC3Denum); ++i) {
        WebGLShader* shader = program->getAttachedShader(shaderType[i]);
        if (shader)
            shaderObjects.append(shader);
    }
    return true;
}

GC3Dint WebGLRenderingContext::getAttribLocation(WebGLProgram* program, const String& name)
{
    if (isContextLost() || !validateWebGLObject("getAttribLocation", program))
        return -1;
    if (!validateLocationLength("getAttribLocation", name))
        return -1;
    if (!validateString("getAttribLocation", name))
        return -1;
    if (isPrefixReserved(name))
        return -1;
    if (!program->getLinkStatus()) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "getAttribLocation", "program not linked");
        return 0;
    }
    return m_context->getAttribLocation(objectOrZero(program), name);
}

WebGLGetInfo WebGLRenderingContext::getBufferParameter(GC3Denum target, GC3Denum pname, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return WebGLGetInfo();
    if (target != GraphicsContext3D::ARRAY_BUFFER && target != GraphicsContext3D::ELEMENT_ARRAY_BUFFER) {
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "getBufferParameter", "invalid target");
        return WebGLGetInfo();
    }

    if (pname != GraphicsContext3D::BUFFER_SIZE && pname != GraphicsContext3D::BUFFER_USAGE) {
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "getBufferParameter", "invalid parameter name");
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

    // Also, we need to enforce requested values of "false" for depth
    // and stencil, regardless of the properties of the underlying
    // GraphicsContext3D or DrawingBuffer.
    RefPtr<WebGLContextAttributes> attributes = WebGLContextAttributes::create(m_context->getContextAttributes());
    if (!m_attributes.depth)
        attributes->setDepth(false);
    if (!m_attributes.stencil)
        attributes->setStencil(false);
    if (m_drawingBuffer) {
        // The DrawingBuffer obtains its parameters from GraphicsContext3D::getContextAttributes(),
        // but it makes its own determination of whether multisampling is supported.
        attributes->setAntialias(m_drawingBuffer->multisample());
    }
    return attributes.release();
}

GC3Denum WebGLRenderingContext::getError()
{
    return m_context->getError();
}

WebGLExtension* WebGLRenderingContext::getExtension(const String& name)
{
    if (isContextLost())
        return 0;

    if (equalIgnoringCase(name, "WEBKIT_EXT_texture_filter_anisotropic")
        && m_context->getExtensions()->supports("GL_EXT_texture_filter_anisotropic")) {
        if (!m_extTextureFilterAnisotropic) {
            m_context->getExtensions()->ensureEnabled("GL_EXT_texture_filter_anisotropic");
            m_extTextureFilterAnisotropic = EXTTextureFilterAnisotropic::create(this);
        }
        return m_extTextureFilterAnisotropic.get();
    }
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
    if (equalIgnoringCase(name, "OES_texture_half_float")
        && m_context->getExtensions()->supports("GL_OES_texture_half_float")) {
        if (!m_oesTextureHalfFloat) {
            m_context->getExtensions()->ensureEnabled("GL_OES_texture_half_float");
            m_oesTextureHalfFloat = OESTextureHalfFloat::create(this);
        }
        return m_oesTextureHalfFloat.get();
    }
    if (equalIgnoringCase(name, "OES_vertex_array_object")
        && m_context->getExtensions()->supports("GL_OES_vertex_array_object")) {
        if (!m_oesVertexArrayObject) {
            m_context->getExtensions()->ensureEnabled("GL_OES_vertex_array_object");
            m_oesVertexArrayObject = OESVertexArrayObject::create(this);
        }
        return m_oesVertexArrayObject.get();
    }
    if (equalIgnoringCase(name, "OES_element_index_uint")
        && m_context->getExtensions()->supports("GL_OES_element_index_uint")) {
        if (!m_oesElementIndexUint) {
            m_context->getExtensions()->ensureEnabled("GL_OES_element_index_uint");
            m_oesElementIndexUint = OESElementIndexUint::create(this);
        }
        return m_oesElementIndexUint.get();
    }
    if (equalIgnoringCase(name, "WEBGL_lose_context")) {
        if (!m_webglLoseContext)
            m_webglLoseContext = WebGLLoseContext::create(this);
        return m_webglLoseContext.get();
    }
    if ((equalIgnoringCase(name, "WEBKIT_WEBGL_compressed_texture_atc"))
        && WebGLCompressedTextureATC::supported(this)) {
        if (!m_webglCompressedTextureATC)
            m_webglCompressedTextureATC = WebGLCompressedTextureATC::create(this);
        return m_webglCompressedTextureATC.get();
    }
    if ((equalIgnoringCase(name, "WEBKIT_WEBGL_compressed_texture_pvrtc"))
        && WebGLCompressedTexturePVRTC::supported(this)) {
        if (!m_webglCompressedTexturePVRTC)
            m_webglCompressedTexturePVRTC = WebGLCompressedTexturePVRTC::create(this);
    }
    if (equalIgnoringCase(name, "WEBGL_compressed_texture_s3tc")
        && WebGLCompressedTextureS3TC::supported(this)) {
        if (!m_webglCompressedTextureS3TC)
            m_webglCompressedTextureS3TC = WebGLCompressedTextureS3TC::create(this);
        return m_webglCompressedTextureS3TC.get();
    }
    if (equalIgnoringCase(name, "WEBGL_depth_texture")
        && WebGLDepthTexture::supported(graphicsContext3D())) {
        if (!m_webglDepthTexture) {
            m_context->getExtensions()->ensureEnabled("GL_CHROMIUM_depth_texture");
            m_webglDepthTexture = WebGLDepthTexture::create(this);
        }
        return m_webglDepthTexture.get();
    }
    if (equalIgnoringCase(name, "EXT_draw_buffers") && supportsDrawBuffers()) {
        if (!m_extDrawBuffers) {
            m_context->getExtensions()->ensureEnabled("GL_EXT_draw_buffers");
            m_extDrawBuffers = EXTDrawBuffers::create(this);
        }
        return m_extDrawBuffers.get();
    }
    if (allowPrivilegedExtensions()) {
        if (equalIgnoringCase(name, "WEBGL_debug_renderer_info")) {
            if (!m_webglDebugRendererInfo)
                m_webglDebugRendererInfo = WebGLDebugRendererInfo::create(this);
            return m_webglDebugRendererInfo.get();
        }
        if (equalIgnoringCase(name, "WEBGL_debug_shaders")
            && m_context->getExtensions()->supports("GL_ANGLE_translated_shader_source")) {
            if (!m_webglDebugShaders)
                m_webglDebugShaders = WebGLDebugShaders::create(this);
            return m_webglDebugShaders.get();
        }
    }

    return 0;
}

WebGLGetInfo WebGLRenderingContext::getFramebufferAttachmentParameter(GC3Denum target, GC3Denum attachment, GC3Denum pname, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateFramebufferFuncParameters("getFramebufferAttachmentParameter", target, attachment))
        return WebGLGetInfo();

    if (!m_framebufferBinding || !m_framebufferBinding->object()) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "getFramebufferAttachmentParameter", "no framebuffer bound");
        return WebGLGetInfo();
    }

    WebGLSharedObject* object = m_framebufferBinding->getAttachmentObject(attachment);
    if (!object) {
        if (pname == GraphicsContext3D::FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE)
            return WebGLGetInfo(GraphicsContext3D::NONE);
        // OpenGL ES 2.0 specifies INVALID_ENUM in this case, while desktop GL
        // specifies INVALID_OPERATION.
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "getFramebufferAttachmentParameter", "invalid parameter name");
        return WebGLGetInfo();
    }

    ASSERT(object->isTexture() || object->isRenderbuffer());
    if (object->isTexture()) {
        switch (pname) {
        case GraphicsContext3D::FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
            return WebGLGetInfo(GraphicsContext3D::TEXTURE);
        case GraphicsContext3D::FRAMEBUFFER_ATTACHMENT_OBJECT_NAME:
            return WebGLGetInfo(PassRefPtr<WebGLTexture>(reinterpret_cast<WebGLTexture*>(object)));
        case GraphicsContext3D::FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL:
        case GraphicsContext3D::FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE:
            {
                WebGLStateRestorer(this, false);
                GC3Dint value = 0;
                m_context->getFramebufferAttachmentParameteriv(target, attachment, pname, &value);
                return WebGLGetInfo(value);
            }
        default:
            synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "getFramebufferAttachmentParameter", "invalid parameter name for texture attachment");
            return WebGLGetInfo();
        }
    } else {
        switch (pname) {
        case GraphicsContext3D::FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
            return WebGLGetInfo(GraphicsContext3D::RENDERBUFFER);
        case GraphicsContext3D::FRAMEBUFFER_ATTACHMENT_OBJECT_NAME:
            return WebGLGetInfo(PassRefPtr<WebGLRenderbuffer>(reinterpret_cast<WebGLRenderbuffer*>(object)));
        default:
            synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "getFramebufferAttachmentParameter", "invalid parameter name for renderbuffer attachment");
            return WebGLGetInfo();
        }
    }
}

WebGLGetInfo WebGLRenderingContext::getParameter(GC3Denum pname, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return WebGLGetInfo();
    const int intZero = 0;
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
        return WebGLGetInfo(Uint32Array::create(m_compressedTextureFormats.data(), m_compressedTextureFormats.size()));
    case GraphicsContext3D::CULL_FACE:
        return getBooleanParameter(pname);
    case GraphicsContext3D::CULL_FACE_MODE:
        return getUnsignedIntParameter(pname);
    case GraphicsContext3D::CURRENT_PROGRAM:
        return WebGLGetInfo(PassRefPtr<WebGLProgram>(m_currentProgram));
    case GraphicsContext3D::DEPTH_BITS:
        if (!m_framebufferBinding && !m_attributes.depth)
            return WebGLGetInfo(intZero);
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
        return WebGLGetInfo(String("WebKit WebGL"));
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
        if (!m_framebufferBinding && !m_attributes.stencil)
            return WebGLGetInfo(intZero);
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
        return WebGLGetInfo(PassRefPtr<WebGLTexture>(m_textureUnits[m_activeTextureUnit].m_textureBinding));
    case GraphicsContext3D::TEXTURE_BINDING_CUBE_MAP:
        return WebGLGetInfo(PassRefPtr<WebGLTexture>(m_textureUnits[m_activeTextureUnit].m_textureBinding));
    case GraphicsContext3D::UNPACK_ALIGNMENT:
        return getIntParameter(pname);
    case GraphicsContext3D::UNPACK_FLIP_Y_WEBGL:
        return WebGLGetInfo(m_unpackFlipY);
    case GraphicsContext3D::UNPACK_PREMULTIPLY_ALPHA_WEBGL:
        return WebGLGetInfo(m_unpackPremultiplyAlpha);
    case GraphicsContext3D::UNPACK_COLORSPACE_CONVERSION_WEBGL:
        return WebGLGetInfo(m_unpackColorspaceConversion);
    case GraphicsContext3D::VENDOR:
        return WebGLGetInfo(String("WebKit"));
    case GraphicsContext3D::VERSION:
        return WebGLGetInfo("WebGL 1.0 (" + m_context->getString(GraphicsContext3D::VERSION) + ")");
    case GraphicsContext3D::VIEWPORT:
        return getWebGLIntArrayParameter(pname);
    case Extensions3D::FRAGMENT_SHADER_DERIVATIVE_HINT_OES: // OES_standard_derivatives
        if (m_oesStandardDerivatives)
            return getUnsignedIntParameter(Extensions3D::FRAGMENT_SHADER_DERIVATIVE_HINT_OES);
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "getParameter", "invalid parameter name, OES_standard_derivatives not enabled");
        return WebGLGetInfo();
    case WebGLDebugRendererInfo::UNMASKED_RENDERER_WEBGL:
        if (m_webglDebugRendererInfo)
            return WebGLGetInfo(m_context->getString(GraphicsContext3D::RENDERER));
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "getParameter", "invalid parameter name, WEBGL_debug_renderer_info not enabled");
        return WebGLGetInfo();
    case WebGLDebugRendererInfo::UNMASKED_VENDOR_WEBGL:
        if (m_webglDebugRendererInfo)
            return WebGLGetInfo(m_context->getString(GraphicsContext3D::VENDOR));
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "getParameter", "invalid parameter name, WEBGL_debug_renderer_info not enabled");
        return WebGLGetInfo();
    case Extensions3D::VERTEX_ARRAY_BINDING_OES: // OES_vertex_array_object
        if (m_oesVertexArrayObject) {
            if (!m_boundVertexArrayObject->isDefaultObject())
                return WebGLGetInfo(PassRefPtr<WebGLVertexArrayObjectOES>(m_boundVertexArrayObject));
            return WebGLGetInfo();
        }
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "getParameter", "invalid parameter name, OES_vertex_array_object not enabled");
        return WebGLGetInfo();
    case Extensions3D::MAX_TEXTURE_MAX_ANISOTROPY_EXT: // EXT_texture_filter_anisotropic
        if (m_extTextureFilterAnisotropic)
            return getUnsignedIntParameter(Extensions3D::MAX_TEXTURE_MAX_ANISOTROPY_EXT);
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "getParameter", "invalid parameter name, EXT_texture_filter_anisotropic not enabled");
        return WebGLGetInfo();
    case Extensions3D::MAX_COLOR_ATTACHMENTS_EXT: // EXT_draw_buffers BEGIN
        if (m_extDrawBuffers)
            return WebGLGetInfo(getMaxColorAttachments());
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "getParameter", "invalid parameter name, EXT_draw_buffers not enabled");
        return WebGLGetInfo();
    case Extensions3D::MAX_DRAW_BUFFERS_EXT:
        if (m_extDrawBuffers)
            return WebGLGetInfo(getMaxDrawBuffers());
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "getParameter", "invalid parameter name, EXT_draw_buffers not enabled");
        return WebGLGetInfo();
    default:
        if (m_extDrawBuffers
            && pname >= Extensions3D::DRAW_BUFFER0_EXT
            && pname < static_cast<GC3Denum>(Extensions3D::DRAW_BUFFER0_EXT + getMaxDrawBuffers())) {
            GC3Dint value = GraphicsContext3D::NONE;
            if (m_framebufferBinding)
                value = m_framebufferBinding->getDrawBuffer(pname);
            else // emulated backbuffer
                value = m_backDrawBuffer;
            return WebGLGetInfo(value);
        }
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "getParameter", "invalid parameter name");
        return WebGLGetInfo();
    }
}

WebGLGetInfo WebGLRenderingContext::getProgramParameter(WebGLProgram* program, GC3Denum pname, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateWebGLObject("getProgramParameter", program))
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
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "getProgramParameter", "invalid parameter name");
        return WebGLGetInfo();
    }
}

String WebGLRenderingContext::getProgramInfoLog(WebGLProgram* program, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return String();
    if (!validateWebGLObject("getProgramInfoLog", program))
        return "";
    WebGLStateRestorer(this, false);
    return ensureNotNull(m_context->getProgramInfoLog(objectOrZero(program)));
}

WebGLGetInfo WebGLRenderingContext::getRenderbufferParameter(GC3Denum target, GC3Denum pname, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return WebGLGetInfo();
    if (target != GraphicsContext3D::RENDERBUFFER) {
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "getRenderbufferParameter", "invalid target");
        return WebGLGetInfo();
    }
    if (!m_renderbufferBinding || !m_renderbufferBinding->object()) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "getRenderbufferParameter", "no renderbuffer bound");
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
            synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "getRenderbufferParameter", "invalid parameter name");
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
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "getRenderbufferParameter", "invalid parameter name");
        return WebGLGetInfo();
    }
}

WebGLGetInfo WebGLRenderingContext::getShaderParameter(WebGLShader* shader, GC3Denum pname, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateWebGLObject("getShaderParameter", shader))
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
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "getShaderParameter", "invalid parameter name");
        return WebGLGetInfo();
    }
}

String WebGLRenderingContext::getShaderInfoLog(WebGLShader* shader, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return String();
    if (!validateWebGLObject("getShaderInfoLog", shader))
        return "";
    WebGLStateRestorer(this, false);
    return ensureNotNull(m_context->getShaderInfoLog(objectOrZero(shader)));
}

PassRefPtr<WebGLShaderPrecisionFormat> WebGLRenderingContext::getShaderPrecisionFormat(GC3Denum shaderType, GC3Denum precisionType, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return 0;
    switch (shaderType) {
    case GraphicsContext3D::VERTEX_SHADER:
    case GraphicsContext3D::FRAGMENT_SHADER:
        break;
    default:
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "getShaderPrecisionFormat", "invalid shader type");
        return 0;
    }
    switch (precisionType) {
    case GraphicsContext3D::LOW_FLOAT:
    case GraphicsContext3D::MEDIUM_FLOAT:
    case GraphicsContext3D::HIGH_FLOAT:
    case GraphicsContext3D::LOW_INT:
    case GraphicsContext3D::MEDIUM_INT:
    case GraphicsContext3D::HIGH_INT:
        break;
    default:
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "getShaderPrecisionFormat", "invalid precision type");
        return 0;
    }

    GC3Dint range[2] = {0, 0};
    GC3Dint precision = 0;
    m_context->getShaderPrecisionFormat(shaderType, precisionType, range, &precision);
    return WebGLShaderPrecisionFormat::create(range[0], range[1], precision);
}

String WebGLRenderingContext::getShaderSource(WebGLShader* shader, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return String();
    if (!validateWebGLObject("getShaderSource", shader))
        return "";
    return ensureNotNull(shader->getSource());
}

Vector<String> WebGLRenderingContext::getSupportedExtensions()
{
    Vector<String> result;
    if (m_context->getExtensions()->supports("GL_OES_texture_float"))
        result.append("OES_texture_float");
    if (m_context->getExtensions()->supports("GL_OES_standard_derivatives"))
        result.append("OES_standard_derivatives");
    if (m_context->getExtensions()->supports("GL_EXT_texture_filter_anisotropic"))
        result.append("WEBKIT_EXT_texture_filter_anisotropic");
    if (m_context->getExtensions()->supports("GL_OES_vertex_array_object"))
        result.append("OES_vertex_array_object");
    if (m_context->getExtensions()->supports("GL_OES_element_index_uint"))
        result.append("OES_element_index_uint");
    result.append("WEBGL_lose_context");
    if (WebGLCompressedTextureATC::supported(this))
        result.append("WEBKIT_WEBGL_compressed_texture_atc");
    if (WebGLCompressedTexturePVRTC::supported(this))
        result.append("WEBKIT_WEBGL_compressed_texture_pvrtc");
    if (WebGLCompressedTextureS3TC::supported(this))
        result.append("WEBGL_compressed_texture_s3tc");
    if (WebGLDepthTexture::supported(graphicsContext3D()))
        result.append("WEBGL_depth_texture");
    if (supportsDrawBuffers())
        result.append("EXT_draw_buffers");

    if (allowPrivilegedExtensions()) {
        if (m_context->getExtensions()->supports("GL_ANGLE_translated_shader_source"))
            result.append("WEBGL_debug_shaders");
        result.append("WEBGL_debug_renderer_info");
    }

    return result;
}

WebGLGetInfo WebGLRenderingContext::getTexParameter(GC3Denum target, GC3Denum pname, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost())
        return WebGLGetInfo();
    WebGLTexture* tex = validateTextureBinding("getTexParameter", target, false);
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
    case Extensions3D::TEXTURE_MAX_ANISOTROPY_EXT: // EXT_texture_filter_anisotropic
        if (m_extTextureFilterAnisotropic) {
            m_context->getTexParameteriv(target, pname, &value);
            return WebGLGetInfo(static_cast<unsigned int>(value));
        }
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "getTexParameter", "invalid parameter name, EXT_texture_filter_anisotropic not enabled");
        return WebGLGetInfo();
    default:
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "getTexParameter", "invalid parameter name");
        return WebGLGetInfo();
    }
}

WebGLGetInfo WebGLRenderingContext::getUniform(WebGLProgram* program, const WebGLUniformLocation* uniformLocation, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateWebGLObject("getUniform", program))
        return WebGLGetInfo();
    if (!uniformLocation || uniformLocation->program() != program) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "getUniform", "no uniformlocation or not valid for this program");
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
                    synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "getUniform", "unhandled type");
                    return WebGLGetInfo();
                }
                switch (baseType) {
                case GraphicsContext3D::FLOAT: {
                    GC3Dfloat value[16] = {0};
                    if (m_isRobustnessEXTSupported)
                        m_context->getExtensions()->getnUniformfvEXT(objectOrZero(program), location, 16 * sizeof(GC3Dfloat), value);
                    else
                        m_context->getUniformfv(objectOrZero(program), location, value);
                    if (length == 1)
                        return WebGLGetInfo(value[0]);
                    return WebGLGetInfo(Float32Array::create(value, length));
                }
                case GraphicsContext3D::INT: {
                    GC3Dint value[4] = {0};
                    if (m_isRobustnessEXTSupported)
                        m_context->getExtensions()->getnUniformivEXT(objectOrZero(program), location, 4 * sizeof(GC3Dint), value);
                    else
                        m_context->getUniformiv(objectOrZero(program), location, value);
                    if (length == 1)
                        return WebGLGetInfo(value[0]);
                    return WebGLGetInfo(Int32Array::create(value, length));
                }
                case GraphicsContext3D::BOOL: {
                    GC3Dint value[4] = {0};
                    if (m_isRobustnessEXTSupported)
                        m_context->getExtensions()->getnUniformivEXT(objectOrZero(program), location, 4 * sizeof(GC3Dint), value);
                    else
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
    synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "getUniform", "unknown error");
    return WebGLGetInfo();
}

PassRefPtr<WebGLUniformLocation> WebGLRenderingContext::getUniformLocation(WebGLProgram* program, const String& name, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateWebGLObject("getUniformLocation", program))
        return 0;
    if (!validateLocationLength("getUniformLocation", name))
        return 0;
    if (!validateString("getUniformLocation", name))
        return 0;
    if (isPrefixReserved(name))
        return 0;
    if (!program->getLinkStatus()) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "getUniformLocation", "program not linked");
        return 0;
    }
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
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "getVertexAttrib", "index out of range");
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
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "getVertexAttrib", "invalid parameter name");
        return WebGLGetInfo();
    }
}

long long WebGLRenderingContext::getVertexAttribOffset(GC3Duint index, GC3Denum pname)
{
    if (isContextLost())
        return 0;
    GC3Dsizeiptr result = m_context->getVertexAttribOffset(index, pname);
    cleanupAfterGraphicsCall(false);
    return static_cast<long long>(result);
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
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "hint", "invalid target");
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
    return m_contextLost;
}

GC3Dboolean WebGLRenderingContext::isEnabled(GC3Denum cap)
{
    if (isContextLost() || !validateCapability("isEnabled", cap))
        return 0;
    if (cap == GraphicsContext3D::STENCIL_TEST)
        return m_stencilEnabled;
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
    if (isContextLost() || !validateWebGLObject("linkProgram", program))
        return;
    if (!isGLES2Compliant()) {
        if (!program->getAttachedShader(GraphicsContext3D::VERTEX_SHADER) || !program->getAttachedShader(GraphicsContext3D::FRAGMENT_SHADER)) {
            program->setLinkStatus(false);
            return;
        }
    }

    m_context->linkProgram(objectOrZero(program));
    program->increaseLinkCount();
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
            synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "pixelStorei", "invalid parameter for UNPACK_COLORSPACE_CONVERSION_WEBGL");
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
            synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "pixelStorei", "invalid parameter for alignment");
            return;
        }
        break;
    default:
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "pixelStorei", "invalid parameter name");
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

void WebGLRenderingContext::readPixels(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height, GC3Denum format, GC3Denum type, ArrayBufferView* pixels, ExceptionCode&)
{
    if (isContextLost())
        return;
    // Due to WebGL's same-origin restrictions, it is not possible to
    // taint the origin using the WebGL API.
    ASSERT(canvas()->originClean());
    // Validate input parameters.
    if (!pixels) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "readPixels", "no destination ArrayBufferView");
        return;
    }
    switch (format) {
    case GraphicsContext3D::ALPHA:
    case GraphicsContext3D::RGB:
    case GraphicsContext3D::RGBA:
        break;
    default:
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "readPixels", "invalid format");
        return;
    }
    switch (type) {
    case GraphicsContext3D::UNSIGNED_BYTE:
    case GraphicsContext3D::UNSIGNED_SHORT_5_6_5:
    case GraphicsContext3D::UNSIGNED_SHORT_4_4_4_4:
    case GraphicsContext3D::UNSIGNED_SHORT_5_5_5_1:
        break;
    default:
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "readPixels", "invalid type");
        return;
    }
    if (format != GraphicsContext3D::RGBA || type != GraphicsContext3D::UNSIGNED_BYTE) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "readPixels", "format not RGBA or type not UNSIGNED_BYTE");
        return;
    }
    // Validate array type against pixel type.
    if (pixels->getType() != ArrayBufferView::TypeUint8) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "readPixels", "ArrayBufferView not Uint8Array");
        return;
    }
    const char* reason = "framebuffer incomplete";
    if (m_framebufferBinding && !m_framebufferBinding->onAccess(graphicsContext3D(), !isResourceSafe(), &reason)) {
        synthesizeGLError(GraphicsContext3D::INVALID_FRAMEBUFFER_OPERATION, "readPixels", reason);
        return;
    }
    // Calculate array size, taking into consideration of PACK_ALIGNMENT.
    unsigned int totalBytesRequired = 0;
    unsigned int padding = 0;
    if (!m_isRobustnessEXTSupported) {
        GC3Denum error = m_context->computeImageSizeInBytes(format, type, width, height, m_packAlignment, &totalBytesRequired, &padding);
        if (error != GraphicsContext3D::NO_ERROR) {
            synthesizeGLError(error, "readPixels", "invalid dimensions");
            return;
        }
        if (pixels->byteLength() < totalBytesRequired) {
            synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "readPixels", "ArrayBufferView not large enough for dimensions");
            return;
        }
    }

    clearIfComposited();
    void* data = pixels->baseAddress();

    {
        ScopedDrawingBufferBinder binder(m_drawingBuffer.get(), m_framebufferBinding.get());
        if (m_isRobustnessEXTSupported)
            m_context->getExtensions()->readnPixelsEXT(x, y, width, height, format, type, pixels->byteLength(), data);
        else
            m_context->readPixels(x, y, width, height, format, type, data);
    }

#if OS(DARWIN) || OS(QNX)
    if (m_isRobustnessEXTSupported) // we haven't computed padding
        m_context->computeImageSizeInBytes(format, type, width, height, m_packAlignment, &totalBytesRequired, &padding);
    // FIXME: remove this section when GL driver bug on Mac AND the GLES driver bug
    // on QC & Imagination QNX is fixed, i.e., when alpha is off, readPixels should
    // set alpha to 255 instead of 0.
    if (!m_framebufferBinding && !m_context->getContextAttributes().alpha) {
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
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "renderbufferStorage", "invalid target");
        return;
    }
    if (!m_renderbufferBinding || !m_renderbufferBinding->object()) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "renderbufferStorage", "no bound renderbuffer");
        return;
    }
    if (!validateSize("renderbufferStorage", width, height))
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
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "renderbufferStorage", "invalid internalformat");
        return;
    }
    applyStencilTest();
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
    if (!validateSize("scissor", width, height))
        return;
    m_context->scissor(x, y, width, height);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::shaderSource(WebGLShader* shader, const String& string, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateWebGLObject("shaderSource", shader))
        return;
    String stringWithoutComments = StripComments(string).result();
    if (!validateString("shaderSource", stringWithoutComments))
        return;
    shader->setSource(string);
    m_context->shaderSource(objectOrZero(shader), stringWithoutComments);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::stencilFunc(GC3Denum func, GC3Dint ref, GC3Duint mask)
{
    if (isContextLost())
        return;
    if (!validateStencilFunc("stencilFunc", func))
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
    if (!validateStencilFunc("stencilFuncSeparate", func))
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
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "stencilFuncSeparate", "invalid face");
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
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "stencilMaskSeparate", "invalid face");
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

void WebGLRenderingContext::texImage2DBase(GC3Denum target, GC3Dint level, GC3Denum internalformat, GC3Dsizei width, GC3Dsizei height, GC3Dint border, GC3Denum format, GC3Denum type, const void* pixels, ExceptionCode& ec)
{
    // FIXME: For now we ignore any errors returned
    ec = 0;
    WebGLTexture* tex = validateTextureBinding("texImage2D", target, true);
    ASSERT(validateTexFuncParameters("texImage2D", NotTexSubImage2D, target, level, internalformat, width, height, border, format, type));
    ASSERT(tex);
    ASSERT(!level || !WebGLTexture::isNPOT(width, height));
    if (!pixels) {
        // Note: Chromium's OpenGL implementation clears textures and isResourceSafe() is therefore true.
        // For other implementations, if they are using ANGLE_depth_texture, ANGLE depth textures
        // can not be cleared with texImage2D and must be cleared by binding to an fbo and calling
        // clear.
        if (isResourceSafe())
            m_context->texImage2D(target, level, internalformat, width, height, border, format, type, 0);
        else {
            bool succeed = m_context->texImage2DResourceSafe(target, level, internalformat, width, height,
                                                             border, format, type, m_unpackAlignment);
            if (!succeed)
                return;
        }
    } else {
        ASSERT(validateSettableTexFormat("texImage2D", internalformat));
        m_context->texImage2D(target, level, internalformat, width, height,
                              border, format, type, pixels);
    }
    tex->setLevelInfo(target, level, internalformat, width, height, type);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::texImage2DImpl(GC3Denum target, GC3Dint level, GC3Denum internalformat, GC3Denum format, GC3Denum type, Image* image, GraphicsContext3D::ImageHtmlDomSource domSource, bool flipY, bool premultiplyAlpha, ExceptionCode& ec)
{
    ec = 0;
    Vector<uint8_t> data;
    GraphicsContext3D::ImageExtractor imageExtractor(image, domSource, premultiplyAlpha, m_unpackColorspaceConversion == GraphicsContext3D::NONE);
    if (!imageExtractor.extractSucceeded()) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "texImage2D", "bad image data");
        return;
    }
    GraphicsContext3D::DataFormat sourceDataFormat = imageExtractor.imageSourceFormat();
    GraphicsContext3D::AlphaOp alphaOp = imageExtractor.imageAlphaOp();
    const void* imagePixelData = imageExtractor.imagePixelData();

    bool needConversion = true;
    if (type == GraphicsContext3D::UNSIGNED_BYTE && sourceDataFormat == GraphicsContext3D::DataFormatRGBA8 && format == GraphicsContext3D::RGBA && alphaOp == GraphicsContext3D::AlphaDoNothing && !flipY)
        needConversion = false;
    else {
        if (!m_context->packImageData(image, imagePixelData, format, type, flipY, alphaOp, sourceDataFormat, imageExtractor.imageWidth(), imageExtractor.imageHeight(), imageExtractor.imageSourceUnpackAlignment(), data)) {
            synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "texImage2D", "packImage error");
            return;
        }
    }

    if (m_unpackAlignment != 1)
        m_context->pixelStorei(GraphicsContext3D::UNPACK_ALIGNMENT, 1);
    texImage2DBase(target, level, internalformat, image->width(), image->height(), 0, format, type, needConversion ? data.data() : imagePixelData, ec);
    if (m_unpackAlignment != 1)
        m_context->pixelStorei(GraphicsContext3D::UNPACK_ALIGNMENT, m_unpackAlignment);
}

bool WebGLRenderingContext::validateTexFunc(const char* functionName, TexFuncValidationFunctionType functionType, TexFuncValidationSourceType sourceType, GC3Denum target, GC3Dint level, GC3Denum internalformat, GC3Dsizei width, GC3Dsizei height, GC3Dint border, GC3Denum format, GC3Denum type, GC3Dint xoffset, GC3Dint yoffset)
{
    // FIXME: Uploading {ImageData, HTMLImageElement, HTMLCanvasElement, HTMLVideoElement} to half floating point texture is not supported yet.
    // See https://bugs.webkit.org/show_bug.cgi?id=110936.
    if (sourceType != SourceArrayBufferView && type == GraphicsContext3D::HALF_FLOAT_OES) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, functionName, "Operation not supported yet");
        return false;
    }

    if (!validateTexFuncParameters(functionName, functionType, target, level, internalformat, width, height, border, format, type))
        return false;

    WebGLTexture* texture = validateTextureBinding(functionName, target, true);
    if (!texture)
        return false;

    if (functionType == NotTexSubImage2D) {
        if (level && WebGLTexture::isNPOT(width, height)) {
            synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "level > 0 not power of 2");
            return false;
        }
        // For SourceArrayBufferView, function validateTexFuncData() would handle whether to validate the SettableTexFormat
        // by checking if the ArrayBufferView is null or not.
        if (sourceType != SourceArrayBufferView) {
            if (!validateSettableTexFormat(functionName, format))
                return false;
        }
    } else {
        if (!validateSettableTexFormat(functionName, format))
            return false;
        if (!validateSize(functionName, xoffset, yoffset))
            return false;
        // Before checking if it is in the range, check if overflow happens first.
        if (xoffset + width < 0 || yoffset + height < 0) {
            synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "bad dimensions");
            return false;
        }
        if (xoffset + width > texture->getWidth(target, level) || yoffset + height > texture->getHeight(target, level)) {
            synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "dimensions out of range");
            return false;
        }
        if (texture->getInternalFormat(target, level) != format || texture->getType(target, level) != type) {
            synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, functionName, "type and format do not match texture");
            return false;
        }
    }

    return true;
}

void WebGLRenderingContext::texImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat,
                                       GC3Dsizei width, GC3Dsizei height, GC3Dint border,
                                       GC3Denum format, GC3Denum type, ArrayBufferView* pixels, ExceptionCode& ec)
{
    if (isContextLost() || !validateTexFuncData("texImage2D", level, width, height, format, type, pixels, NullAllowed)
        || !validateTexFunc("texImage2D", NotTexSubImage2D, SourceArrayBufferView, target, level, internalformat, width, height, border, format, type, 0, 0))
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
    if (isContextLost() || !pixels || !validateTexFunc("texImage2D", NotTexSubImage2D, SourceImageData, target, level, internalformat, pixels->width(), pixels->height(), 0, format, type, 0, 0))
        return;
    Vector<uint8_t> data;
    bool needConversion = true;
    // The data from ImageData is always of format RGBA8.
    // No conversion is needed if destination format is RGBA and type is USIGNED_BYTE and no Flip or Premultiply operation is required.
    if (!m_unpackFlipY && !m_unpackPremultiplyAlpha && format == GraphicsContext3D::RGBA && type == GraphicsContext3D::UNSIGNED_BYTE)
        needConversion = false;
    else {
        if (!m_context->extractImageData(pixels, format, type, m_unpackFlipY, m_unpackPremultiplyAlpha, data)) {
            synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "texImage2D", "bad image data");
            return;
        }
    }
    if (m_unpackAlignment != 1)
        m_context->pixelStorei(GraphicsContext3D::UNPACK_ALIGNMENT, 1);
    texImage2DBase(target, level, internalformat, pixels->width(), pixels->height(), 0, format, type, needConversion ? data.data() : pixels->data()->data(), ec);
    if (m_unpackAlignment != 1)
        m_context->pixelStorei(GraphicsContext3D::UNPACK_ALIGNMENT, m_unpackAlignment);
}

PassRefPtr<Image> WebGLRenderingContext::drawImageIntoBuffer(Image* image, int width, int height, int deviceScaleFactor)
{
    IntSize size(width, height);
    size.scale(deviceScaleFactor);
    ImageBuffer* buf = m_generatedImageCache.imageBuffer(size);
    if (!buf) {
        synthesizeGLError(GraphicsContext3D::OUT_OF_MEMORY, "texImage2D", "out of memory");
        return 0;
    }

    IntRect srcRect(IntPoint(), image->size());
    IntRect destRect(IntPoint(), size);
    buf->context()->drawImage(image, ColorSpaceDeviceRGB, destRect, srcRect);
    return buf->copyImage(ImageBuffer::fastCopyImageMode());
}

void WebGLRenderingContext::texImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat,
                                       GC3Denum format, GC3Denum type, HTMLImageElement* image, ExceptionCode& ec)
{
    ec = 0;
    if (isContextLost() || !validateHTMLImageElement("texImage2D", image, ec))
        return;

    RefPtr<Image> imageForRender = image->cachedImage()->imageForRenderer(image->renderer());
    if (imageForRender->isSVGImage())
        imageForRender = drawImageIntoBuffer(imageForRender.get(), image->width(), image->height(), canvas()->deviceScaleFactor());

    if (!imageForRender || !validateTexFunc("texImage2D", NotTexSubImage2D, SourceHTMLImageElement, target, level, internalformat, imageForRender->width(), imageForRender->height(), 0, format, type, 0, 0))
        return;

    texImage2DImpl(target, level, internalformat, format, type, imageForRender.get(), GraphicsContext3D::HtmlDomImage, m_unpackFlipY, m_unpackPremultiplyAlpha, ec);
}

void WebGLRenderingContext::texImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat,
                                       GC3Denum format, GC3Denum type, HTMLCanvasElement* canvas, ExceptionCode& ec)
{
    ec = 0;
    if (isContextLost() || !validateHTMLCanvasElement("texImage2D", canvas, ec) || !validateTexFunc("texImage2D", NotTexSubImage2D, SourceHTMLCanvasElement, target, level, internalformat, canvas->width(), canvas->height(), 0, format, type, 0, 0))
        return;

    WebGLTexture* texture = validateTextureBinding("texImage2D", target, true);
    // If possible, copy from the canvas element directly to the texture
    // via the GPU, without a read-back to system memory.
    //
    // FIXME: restriction of (RGB || RGBA)/UNSIGNED_BYTE should be lifted when
    // ImageBuffer::copyToPlatformTexture implementations are fully functional.
    if (GraphicsContext3D::TEXTURE_2D == target && texture && type == texture->getType(target, level)
        && (format == GraphicsContext3D::RGB || format == GraphicsContext3D::RGBA) && type == GraphicsContext3D::UNSIGNED_BYTE) {
        ImageBuffer* buffer = canvas->buffer();
        if (buffer && buffer->copyToPlatformTexture(*m_context.get(), texture->object(), internalformat, m_unpackPremultiplyAlpha, m_unpackFlipY)) {
            texture->setLevelInfo(target, level, internalformat, canvas->width(), canvas->height(), type);
            cleanupAfterGraphicsCall(false);
            return;
        }
    }

    RefPtr<ImageData> imageData = canvas->getImageData();
    if (imageData)
        texImage2D(target, level, internalformat, format, type, imageData.get(), ec);
    else
        texImage2DImpl(target, level, internalformat, format, type, canvas->copiedImage(), GraphicsContext3D::HtmlDomCanvas, m_unpackFlipY, m_unpackPremultiplyAlpha, ec);
}

#if ENABLE(VIDEO)
PassRefPtr<Image> WebGLRenderingContext::videoFrameToImage(HTMLVideoElement* video, BackingStoreCopy backingStoreCopy, ExceptionCode&)
{
    IntSize size(video->videoWidth(), video->videoHeight());
    ImageBuffer* buf = m_generatedImageCache.imageBuffer(size);
    if (!buf) {
        synthesizeGLError(GraphicsContext3D::OUT_OF_MEMORY, "texImage2D", "out of memory");
        return 0;
    }
    IntRect destRect(0, 0, size.width(), size.height());
    // FIXME: Turn this into a GPU-GPU texture copy instead of CPU readback.
    video->paintCurrentFrameInContext(buf->context(), destRect);
    return buf->copyImage(backingStoreCopy);
}

void WebGLRenderingContext::texImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat,
                                       GC3Denum format, GC3Denum type, HTMLVideoElement* video, ExceptionCode& ec)
{
    ec = 0;
    if (isContextLost() || !validateHTMLVideoElement("texImage2D", video, ec)
        || !validateTexFunc("texImage2D", NotTexSubImage2D, SourceHTMLVideoElement, target, level, internalformat, video->videoWidth(), video->videoHeight(), 0, format, type, 0, 0))
        return;

    // Go through the fast path doing a GPU-GPU textures copy without a readback to system memory if possible.
    // Otherwise, it will fall back to the normal SW path.
    // FIXME: The current restrictions require that format shoud be RGB or RGBA,
    // type should be UNSIGNED_BYTE and level should be 0. It may be lifted in the future.
    WebGLTexture* texture = validateTextureBinding("texImage2D", target, true);
    if (GraphicsContext3D::TEXTURE_2D == target && texture
        && (format == GraphicsContext3D::RGB || format == GraphicsContext3D::RGBA)
        && type == GraphicsContext3D::UNSIGNED_BYTE
        && (texture->getType(target, level) == GraphicsContext3D::UNSIGNED_BYTE || !texture->isValid(target, level))
        && !level) {
        if (video->copyVideoTextureToPlatformTexture(m_context.get(), texture->object(), level, type, internalformat, m_unpackPremultiplyAlpha, m_unpackFlipY)) {
            texture->setLevelInfo(target, level, internalformat, video->videoWidth(), video->videoHeight(), type);
            cleanupAfterGraphicsCall(false);
            return;
        }
    }

    // Normal pure SW path.
    RefPtr<Image> image = videoFrameToImage(video, ImageBuffer::fastCopyImageMode(), ec);
    if (!image)
        return;
    texImage2DImpl(target, level, internalformat, format, type, image.get(), GraphicsContext3D::HtmlDomVideo, m_unpackFlipY, m_unpackPremultiplyAlpha, ec);
}
#endif

void WebGLRenderingContext::texParameter(GC3Denum target, GC3Denum pname, GC3Dfloat paramf, GC3Dint parami, bool isFloat)
{
    if (isContextLost())
        return;
    WebGLTexture* tex = validateTextureBinding("texParameter", target, false);
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
            synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "texParameter", "invalid parameter");
            return;
        }
        break;
    case Extensions3D::TEXTURE_MAX_ANISOTROPY_EXT: // EXT_texture_filter_anisotropic
        if (!m_extTextureFilterAnisotropic) {
            synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "texParameter", "invalid parameter, EXT_texture_filter_anisotropic not enabled");
            return;
        }
        break;
    default:
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "texParameter", "invalid parameter name");
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

void WebGLRenderingContext::texSubImage2DBase(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset, GC3Dsizei width, GC3Dsizei height, GC3Denum format, GC3Denum type, const void* pixels, ExceptionCode& ec)
{
    // FIXME: For now we ignore any errors returned
    ec = 0;
    ASSERT(!isContextLost());
    ASSERT(validateTexFuncParameters("texSubImage2D", TexSubImage2D, target, level, format, width, height, 0, format, type));
    ASSERT(validateSize("texSubImage2D", xoffset, yoffset));
    ASSERT(validateSettableTexFormat("texSubImage2D", format));
    WebGLTexture* tex = validateTextureBinding("texSubImage2D", target, true);
    if (!tex) {
        ASSERT_NOT_REACHED();
        return;
    }
    ASSERT((xoffset + width) >= 0);
    ASSERT((yoffset + height) >= 0);
    ASSERT(tex->getWidth(target, level) >= (xoffset + width));
    ASSERT(tex->getHeight(target, level) >= (yoffset + height));
    ASSERT(tex->getInternalFormat(target, level) == format);
    ASSERT(tex->getType(target, level) == type);
    m_context->texSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::texSubImage2DImpl(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset, GC3Denum format, GC3Denum type, Image* image, GraphicsContext3D::ImageHtmlDomSource domSource, bool flipY, bool premultiplyAlpha, ExceptionCode& ec)
{
    ec = 0;
    Vector<uint8_t> data;
    GraphicsContext3D::ImageExtractor imageExtractor(image, domSource, premultiplyAlpha, m_unpackColorspaceConversion == GraphicsContext3D::NONE);  
    if (!imageExtractor.extractSucceeded()) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "texSubImage2D", "bad image");
        return;
    }
    GraphicsContext3D::DataFormat sourceDataFormat = imageExtractor.imageSourceFormat();
    GraphicsContext3D::AlphaOp alphaOp = imageExtractor.imageAlphaOp();
    const void* imagePixelData = imageExtractor.imagePixelData();

    bool needConversion = true;
    if (type == GraphicsContext3D::UNSIGNED_BYTE && sourceDataFormat == GraphicsContext3D::DataFormatRGBA8 && format == GraphicsContext3D::RGBA && alphaOp == GraphicsContext3D::AlphaDoNothing && !flipY)
        needConversion = false;
    else {
        if (!m_context->packImageData(image, imagePixelData, format, type, flipY, alphaOp, sourceDataFormat, imageExtractor.imageWidth(), imageExtractor.imageHeight(), imageExtractor.imageSourceUnpackAlignment(), data)) {
            synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "texImage2D", "bad image data");
            return;
        }
    }

    if (m_unpackAlignment != 1)
        m_context->pixelStorei(GraphicsContext3D::UNPACK_ALIGNMENT, 1);
    texSubImage2DBase(target, level, xoffset, yoffset, image->width(), image->height(), format, type,  needConversion ? data.data() : imagePixelData, ec);
    if (m_unpackAlignment != 1)
        m_context->pixelStorei(GraphicsContext3D::UNPACK_ALIGNMENT, m_unpackAlignment);
}

void WebGLRenderingContext::texSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset,
                                          GC3Dsizei width, GC3Dsizei height,
                                          GC3Denum format, GC3Denum type, ArrayBufferView* pixels, ExceptionCode& ec)
{
    if (isContextLost() || !validateTexFuncData("texSubImage2D", level, width, height, format, type, pixels, NullNotAllowed)
        || !validateTexFunc("texSubImage2D", TexSubImage2D, SourceArrayBufferView, target, level, format, width, height, 0, format, type, xoffset, yoffset))
        return;
    void* data = pixels->baseAddress();
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
    if (isContextLost() || !pixels || !validateTexFunc("texSubImage2D", TexSubImage2D, SourceImageData, target, level, format,  pixels->width(), pixels->height(), 0, format, type, xoffset, yoffset))
        return;

    Vector<uint8_t> data;
    bool needConversion = true;
    // The data from ImageData is always of format RGBA8.
    // No conversion is needed if destination format is RGBA and type is USIGNED_BYTE and no Flip or Premultiply operation is required.
    if (format == GraphicsContext3D::RGBA && type == GraphicsContext3D::UNSIGNED_BYTE && !m_unpackFlipY && !m_unpackPremultiplyAlpha)
        needConversion = false;
    else {
        if (!m_context->extractImageData(pixels, format, type, m_unpackFlipY, m_unpackPremultiplyAlpha, data)) {
            synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "texSubImage2D", "bad image data");
            return;
        }
    }
    if (m_unpackAlignment != 1)
        m_context->pixelStorei(GraphicsContext3D::UNPACK_ALIGNMENT, 1);
    texSubImage2DBase(target, level, xoffset, yoffset, pixels->width(), pixels->height(), format, type, needConversion ? data.data() : pixels->data()->data(), ec);
    if (m_unpackAlignment != 1)
        m_context->pixelStorei(GraphicsContext3D::UNPACK_ALIGNMENT, m_unpackAlignment);
}

void WebGLRenderingContext::texSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset,
                                          GC3Denum format, GC3Denum type, HTMLImageElement* image, ExceptionCode& ec)
{
    ec = 0;
    if (isContextLost() || !validateHTMLImageElement("texSubImage2D", image, ec))
        return;

    RefPtr<Image> imageForRender = image->cachedImage()->imageForRenderer(image->renderer());
    if (imageForRender->isSVGImage())
        imageForRender = drawImageIntoBuffer(imageForRender.get(), image->width(), image->height(), canvas()->deviceScaleFactor());

    if (!imageForRender || !validateTexFunc("texSubImage2D", TexSubImage2D, SourceHTMLImageElement, target, level, format, imageForRender->width(), imageForRender->height(), 0, format, type, xoffset, yoffset))
        return;

    texSubImage2DImpl(target, level, xoffset, yoffset, format, type, imageForRender.get(), GraphicsContext3D::HtmlDomImage, m_unpackFlipY, m_unpackPremultiplyAlpha, ec);
}

void WebGLRenderingContext::texSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset,
                                          GC3Denum format, GC3Denum type, HTMLCanvasElement* canvas, ExceptionCode& ec)
{
    ec = 0;
    if (isContextLost() || !validateHTMLCanvasElement("texSubImage2D", canvas, ec)
        || !validateTexFunc("texSubImage2D", TexSubImage2D, SourceHTMLCanvasElement, target, level, format, canvas->width(), canvas->height(), 0, format, type, xoffset, yoffset))
        return;

    RefPtr<ImageData> imageData = canvas->getImageData();
    if (imageData)
        texSubImage2D(target, level, xoffset, yoffset, format, type, imageData.get(), ec);
    else
        texSubImage2DImpl(target, level, xoffset, yoffset, format, type, canvas->copiedImage(), GraphicsContext3D::HtmlDomCanvas, m_unpackFlipY, m_unpackPremultiplyAlpha, ec);
}

#if ENABLE(VIDEO)
void WebGLRenderingContext::texSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset,
                                          GC3Denum format, GC3Denum type, HTMLVideoElement* video, ExceptionCode& ec)
{
    ec = 0;
    if (isContextLost() || !validateHTMLVideoElement("texSubImage2D", video, ec)
        || !validateTexFunc("texSubImage2D", TexSubImage2D, SourceHTMLVideoElement, target, level, format, video->videoWidth(), video->videoHeight(), 0, format, type, xoffset, yoffset))
        return;

    RefPtr<Image> image = videoFrameToImage(video, ImageBuffer::fastCopyImageMode(), ec);
    if (!image)
        return;
    texSubImage2DImpl(target, level, xoffset, yoffset, format, type, image.get(), GraphicsContext3D::HtmlDomVideo, m_unpackFlipY, m_unpackPremultiplyAlpha, ec);
}
#endif

void WebGLRenderingContext::uniform1f(const WebGLUniformLocation* location, GC3Dfloat x, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !location)
        return;

    if (location->program() != m_currentProgram) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "uniform1f", "location not for current program");
        return;
    }

    m_context->uniform1f(location->location(), x);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform1fv(const WebGLUniformLocation* location, Float32Array* v, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters("uniform1fv", location, v, 1))
        return;

    m_context->uniform1fv(location->location(), v->length(), v->data());
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform1fv(const WebGLUniformLocation* location, GC3Dfloat* v, GC3Dsizei size, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters("uniform1fv", location, v, size, 1))
        return;

    m_context->uniform1fv(location->location(), size, v);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform1i(const WebGLUniformLocation* location, GC3Dint x, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !location)
        return;

    if (location->program() != m_currentProgram) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "uniform1i", "location not for current program");
        return;
    }

    m_context->uniform1i(location->location(), x);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform1iv(const WebGLUniformLocation* location, Int32Array* v, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters("uniform1iv", location, v, 1))
        return;

    m_context->uniform1iv(location->location(), v->length(), v->data());
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform1iv(const WebGLUniformLocation* location, GC3Dint* v, GC3Dsizei size, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters("uniform1iv", location, v, size, 1))
        return;

    m_context->uniform1iv(location->location(), size, v);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform2f(const WebGLUniformLocation* location, GC3Dfloat x, GC3Dfloat y, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !location)
        return;

    if (location->program() != m_currentProgram) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "uniform2f", "location not for current program");
        return;
    }

    m_context->uniform2f(location->location(), x, y);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform2fv(const WebGLUniformLocation* location, Float32Array* v, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters("uniform2fv", location, v, 2))
        return;

    m_context->uniform2fv(location->location(), v->length() / 2, v->data());
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform2fv(const WebGLUniformLocation* location, GC3Dfloat* v, GC3Dsizei size, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters("uniform2fv", location, v, size, 2))
        return;

    m_context->uniform2fv(location->location(), size / 2, v);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform2i(const WebGLUniformLocation* location, GC3Dint x, GC3Dint y, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !location)
        return;

    if (location->program() != m_currentProgram) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "uniform2i", "location not for current program");
        return;
    }

    m_context->uniform2i(location->location(), x, y);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform2iv(const WebGLUniformLocation* location, Int32Array* v, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters("uniform2iv", location, v, 2))
        return;

    m_context->uniform2iv(location->location(), v->length() / 2, v->data());
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform2iv(const WebGLUniformLocation* location, GC3Dint* v, GC3Dsizei size, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters("uniform2iv", location, v, size, 2))
        return;

    m_context->uniform2iv(location->location(), size / 2, v);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform3f(const WebGLUniformLocation* location, GC3Dfloat x, GC3Dfloat y, GC3Dfloat z, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !location)
        return;

    if (location->program() != m_currentProgram) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "uniform3f", "location not for current program");
        return;
    }

    m_context->uniform3f(location->location(), x, y, z);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform3fv(const WebGLUniformLocation* location, Float32Array* v, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters("uniform3fv", location, v, 3))
        return;

    m_context->uniform3fv(location->location(), v->length() / 3, v->data());
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform3fv(const WebGLUniformLocation* location, GC3Dfloat* v, GC3Dsizei size, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters("uniform3fv", location, v, size, 3))
        return;

    m_context->uniform3fv(location->location(), size / 3, v);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform3i(const WebGLUniformLocation* location, GC3Dint x, GC3Dint y, GC3Dint z, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !location)
        return;

    if (location->program() != m_currentProgram) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "uniform3i", "location not for current program");
        return;
    }

    m_context->uniform3i(location->location(), x, y, z);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform3iv(const WebGLUniformLocation* location, Int32Array* v, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters("uniform3iv", location, v, 3))
        return;

    m_context->uniform3iv(location->location(), v->length() / 3, v->data());
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform3iv(const WebGLUniformLocation* location, GC3Dint* v, GC3Dsizei size, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters("uniform3iv", location, v, size, 3))
        return;

    m_context->uniform3iv(location->location(), size / 3, v);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform4f(const WebGLUniformLocation* location, GC3Dfloat x, GC3Dfloat y, GC3Dfloat z, GC3Dfloat w, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !location)
        return;

    if (location->program() != m_currentProgram) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "uniform4f", "location not for current program");
        return;
    }

    m_context->uniform4f(location->location(), x, y, z, w);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform4fv(const WebGLUniformLocation* location, Float32Array* v, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters("uniform4fv", location, v, 4))
        return;

    m_context->uniform4fv(location->location(), v->length() / 4, v->data());
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform4fv(const WebGLUniformLocation* location, GC3Dfloat* v, GC3Dsizei size, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters("uniform4fv", location, v, size, 4))
        return;

    m_context->uniform4fv(location->location(), size / 4, v);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform4i(const WebGLUniformLocation* location, GC3Dint x, GC3Dint y, GC3Dint z, GC3Dint w, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !location)
        return;

    if (location->program() != m_currentProgram) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "uniform4i", "location not for current program");
        return;
    }

    m_context->uniform4i(location->location(), x, y, z, w);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform4iv(const WebGLUniformLocation* location, Int32Array* v, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters("uniform4iv", location, v, 4))
        return;

    m_context->uniform4iv(location->location(), v->length() / 4, v->data());
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniform4iv(const WebGLUniformLocation* location, GC3Dint* v, GC3Dsizei size, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformParameters("uniform4iv", location, v, size, 4))
        return;

    m_context->uniform4iv(location->location(), size / 4, v);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniformMatrix2fv(const WebGLUniformLocation* location, GC3Dboolean transpose, Float32Array* v, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformMatrixParameters("uniformMatrix2fv", location, transpose, v, 4))
        return;
    m_context->uniformMatrix2fv(location->location(), v->length() / 4, transpose, v->data());
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniformMatrix2fv(const WebGLUniformLocation* location, GC3Dboolean transpose, GC3Dfloat* v, GC3Dsizei size, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformMatrixParameters("uniformMatrix2fv", location, transpose, v, size, 4))
        return;
    m_context->uniformMatrix2fv(location->location(), size / 4, transpose, v);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniformMatrix3fv(const WebGLUniformLocation* location, GC3Dboolean transpose, Float32Array* v, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformMatrixParameters("uniformMatrix3fv", location, transpose, v, 9))
        return;
    m_context->uniformMatrix3fv(location->location(), v->length() / 9, transpose, v->data());
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniformMatrix3fv(const WebGLUniformLocation* location, GC3Dboolean transpose, GC3Dfloat* v, GC3Dsizei size, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformMatrixParameters("uniformMatrix3fv", location, transpose, v, size, 9))
        return;
    m_context->uniformMatrix3fv(location->location(), size / 9, transpose, v);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniformMatrix4fv(const WebGLUniformLocation* location, GC3Dboolean transpose, Float32Array* v, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformMatrixParameters("uniformMatrix4fv", location, transpose, v, 16))
        return;
    m_context->uniformMatrix4fv(location->location(), v->length() / 16, transpose, v->data());
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::uniformMatrix4fv(const WebGLUniformLocation* location, GC3Dboolean transpose, GC3Dfloat* v, GC3Dsizei size, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    if (isContextLost() || !validateUniformMatrixParameters("uniformMatrix4fv", location, transpose, v, size, 16))
        return;
    m_context->uniformMatrix4fv(location->location(), size / 16, transpose, v);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::useProgram(WebGLProgram* program, ExceptionCode& ec)
{
    UNUSED_PARAM(ec);
    bool deleted;
    if (!checkObjectToBeBound("useProgram", program, deleted))
        return;
    if (deleted)
        program = 0;
    if (program && !program->getLinkStatus()) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "useProgram", "program not valid");
        cleanupAfterGraphicsCall(false);
        return;
    }
    if (m_currentProgram != program) {
        if (m_currentProgram)
            m_currentProgram->onDetached(graphicsContext3D());
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
    if (isContextLost() || !validateWebGLObject("validateProgram", program))
        return;
    m_context->validateProgram(objectOrZero(program));
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::vertexAttrib1f(GC3Duint index, GC3Dfloat v0)
{
    vertexAttribfImpl("vertexAttrib1f", index, 1, v0, 0.0f, 0.0f, 1.0f);
}

void WebGLRenderingContext::vertexAttrib1fv(GC3Duint index, Float32Array* v)
{
    vertexAttribfvImpl("vertexAttrib1fv", index, v, 1);
}

void WebGLRenderingContext::vertexAttrib1fv(GC3Duint index, GC3Dfloat* v, GC3Dsizei size)
{
    vertexAttribfvImpl("vertexAttrib1fv", index, v, size, 1);
}

void WebGLRenderingContext::vertexAttrib2f(GC3Duint index, GC3Dfloat v0, GC3Dfloat v1)
{
    vertexAttribfImpl("vertexAttrib2f", index, 2, v0, v1, 0.0f, 1.0f);
}

void WebGLRenderingContext::vertexAttrib2fv(GC3Duint index, Float32Array* v)
{
    vertexAttribfvImpl("vertexAttrib2fv", index, v, 2);
}

void WebGLRenderingContext::vertexAttrib2fv(GC3Duint index, GC3Dfloat* v, GC3Dsizei size)
{
    vertexAttribfvImpl("vertexAttrib2fv", index, v, size, 2);
}

void WebGLRenderingContext::vertexAttrib3f(GC3Duint index, GC3Dfloat v0, GC3Dfloat v1, GC3Dfloat v2)
{
    vertexAttribfImpl("vertexAttrib3f", index, 3, v0, v1, v2, 1.0f);
}

void WebGLRenderingContext::vertexAttrib3fv(GC3Duint index, Float32Array* v)
{
    vertexAttribfvImpl("vertexAttrib3fv", index, v, 3);
}

void WebGLRenderingContext::vertexAttrib3fv(GC3Duint index, GC3Dfloat* v, GC3Dsizei size)
{
    vertexAttribfvImpl("vertexAttrib3fv", index, v, size, 3);
}

void WebGLRenderingContext::vertexAttrib4f(GC3Duint index, GC3Dfloat v0, GC3Dfloat v1, GC3Dfloat v2, GC3Dfloat v3)
{
    vertexAttribfImpl("vertexAttrib4f", index, 4, v0, v1, v2, v3);
}

void WebGLRenderingContext::vertexAttrib4fv(GC3Duint index, Float32Array* v)
{
    vertexAttribfvImpl("vertexAttrib4fv", index, v, 4);
}

void WebGLRenderingContext::vertexAttrib4fv(GC3Duint index, GC3Dfloat* v, GC3Dsizei size)
{
    vertexAttribfvImpl("vertexAttrib4fv", index, v, size, 4);
}

void WebGLRenderingContext::vertexAttribPointer(GC3Duint index, GC3Dint size, GC3Denum type, GC3Dboolean normalized, GC3Dsizei stride, long long offset, ExceptionCode& ec)
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
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "vertexAttribPointer", "invalid type");
        return;
    }
    if (index >= m_maxVertexAttribs) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "vertexAttribPointer", "index out of range");
        return;
    }
    if (size < 1 || size > 4 || stride < 0 || stride > 255 || offset < 0) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, "vertexAttribPointer", "bad size, stride or offset");
        return;
    }
    if (!m_boundArrayBuffer) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "vertexAttribPointer", "no bound ARRAY_BUFFER");
        return;
    }
    // Determine the number of elements the bound buffer can hold, given the offset, size, type and stride
    unsigned int typeSize = sizeInBytes(type);
    if (!typeSize) {
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, "vertexAttribPointer", "invalid type");
        return;
    }
    if ((stride % typeSize) || (static_cast<GC3Dintptr>(offset) % typeSize)) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "vertexAttribPointer", "stride or offset not valid for type");
        return;
    }
    GC3Dsizei bytesPerElement = size * typeSize;

    m_boundVertexArrayObject->setVertexAttribState(index, bytesPerElement, size, type, normalized, stride, static_cast<GC3Dintptr>(offset), m_boundArrayBuffer);
    m_context->vertexAttribPointer(index, size, type, normalized, stride, static_cast<GC3Dintptr>(offset));
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::viewport(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height)
{
    if (isContextLost())
        return;
    if (!validateSize("viewport", width, height))
        return;
    m_context->viewport(x, y, width, height);
    cleanupAfterGraphicsCall(false);
}

void WebGLRenderingContext::forceLostContext(WebGLRenderingContext::LostContextMode mode)
{
    if (isContextLost()) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "loseContext", "context already lost");
        return;
    }

    m_contextGroup->loseContextGroup(mode);
}

void WebGLRenderingContext::loseContextImpl(WebGLRenderingContext::LostContextMode mode)
{
    if (isContextLost())
        return;

    m_contextLost = true;
    m_contextLostMode = mode;

    if (mode == RealLostContext) {
        // Inform the embedder that a lost context was received. In response, the embedder might
        // decide to take action such as asking the user for permission to use WebGL again.
        if (Document* document = canvas()->document()) {
            if (Frame* frame = document->frame())
                frame->loader()->client()->didLoseWebGLContext(m_context->getExtensions()->getGraphicsResetStatusARB());
        }
    }

    detachAndRemoveAllObjects();

    if (m_drawingBuffer) {
        // Make absolutely sure we do not refer to an already-deleted texture or framebuffer.
        m_drawingBuffer->setTexture2DBinding(0);
        m_drawingBuffer->setFramebufferBinding(0);
    }

    // There is no direct way to clear errors from a GL implementation and
    // looping until getError() becomes NO_ERROR might cause an infinite loop if
    // the driver or context implementation had a bug. So, loop a reasonably
    // large number of times to clear any existing errors.
    for (int i = 0; i < 100; ++i) {
        if (m_context->getError() == GraphicsContext3D::NO_ERROR)
            break;
    }
    ConsoleDisplayPreference display = (mode == RealLostContext) ? DisplayInConsole: DontDisplayInConsole;
    synthesizeGLError(GraphicsContext3D::CONTEXT_LOST_WEBGL, "loseContext", "context lost", display);

    // Don't allow restoration unless the context lost event has both been
    // dispatched and its default behavior prevented.
    m_restoreAllowed = false;

    // Always defer the dispatch of the context lost event, to implement
    // the spec behavior of queueing a task.
    m_dispatchContextLostEventTimer.startOneShot(0);
}

void WebGLRenderingContext::forceRestoreContext()
{
    if (!isContextLost()) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "restoreContext", "context not lost");
        return;
    }

    if (!m_restoreAllowed) {
        if (m_contextLostMode == SyntheticLostContext)
            synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "restoreContext", "context restoration not allowed");
        return;
    }

    if (!m_restoreTimer.isActive())
        m_restoreTimer.startOneShot(0);
}

#if USE(ACCELERATED_COMPOSITING)
PlatformLayer* WebGLRenderingContext::platformLayer() const
{
    return m_context->platformLayer();
}
#endif

void WebGLRenderingContext::removeSharedObject(WebGLSharedObject* object)
{
    m_contextGroup->removeObject(object);
}

void WebGLRenderingContext::addSharedObject(WebGLSharedObject* object)
{
    ASSERT(!isContextLost());
    m_contextGroup->addObject(object);
}

void WebGLRenderingContext::removeContextObject(WebGLContextObject* object)
{
    m_contextObjects.remove(object);
}

void WebGLRenderingContext::addContextObject(WebGLContextObject* object)
{
    ASSERT(!isContextLost());
    m_contextObjects.add(object);
}

void WebGLRenderingContext::detachAndRemoveAllObjects()
{
    while (m_contextObjects.size() > 0) {
        HashSet<WebGLContextObject*>::iterator it = m_contextObjects.begin();
        (*it)->detachContext();
    }
}

bool WebGLRenderingContext::hasPendingActivity() const
{
    return false;
}

void WebGLRenderingContext::stop()
{
    if (!isContextLost()) {
        forceLostContext(SyntheticLostContext);
        destroyGraphicsContext3D();
    }
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

void WebGLRenderingContext::handleNPOTTextures(const char* functionName, bool prepareToDraw)
{
    bool resetActiveUnit = false;
    for (unsigned ii = 0; ii < m_textureUnits.size(); ++ii) {
        if (m_textureUnits[ii].m_textureBinding && m_textureUnits[ii].m_textureBinding->needToUseBlackTexture()) {
            if (ii != m_activeTextureUnit) {
                m_context->activeTexture(ii);
                resetActiveUnit = true;
            } else if (resetActiveUnit) {
                m_context->activeTexture(ii);
                resetActiveUnit = false;
            }
            WebGLTexture* texture = 0;
            GC3Denum target = m_textureUnits[ii].m_textureBinding->getTarget();
            if (prepareToDraw) {
                String msg(String("texture bound to texture unit ") + String::number(ii)
                    + " is not renderable. It maybe non-power-of-2 and have incompatible texture filtering or is not 'texture complete'");
                printGLWarningToConsole(functionName, msg.utf8().data());
                if (target == GraphicsContext3D::TEXTURE_2D)
                    texture = m_blackTexture2D.get();
                else if (target == GraphicsContext3D::TEXTURE_CUBE_MAP)
                    texture = m_blackTextureCubeMap.get();
            } else
                texture = m_textureUnits[ii].m_textureBinding.get();
            m_context->bindTexture(target, objectOrZero(texture));
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
    unsigned need = GraphicsContext3D::getChannelBitsByFormat(texInternalFormat);
    unsigned have = GraphicsContext3D::getChannelBitsByFormat(colorBufferFormat);
    return (need & have) == need;
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
        return m_framebufferBinding->getColorBufferWidth();
    return m_drawingBuffer ? m_drawingBuffer->size().width() : m_context->getInternalFramebufferSize().width();
}

int WebGLRenderingContext::getBoundFramebufferHeight()
{
    if (m_framebufferBinding && m_framebufferBinding->object())
        return m_framebufferBinding->getColorBufferHeight();
    return m_drawingBuffer ? m_drawingBuffer->size().height() : m_context->getInternalFramebufferSize().height();
}

WebGLTexture* WebGLRenderingContext::validateTextureBinding(const char* functionName, GC3Denum target, bool useSixEnumsForCubeMap)
{
    switch (target) {
    case GraphicsContext3D::TEXTURE_2D:
        break;
    case GraphicsContext3D::TEXTURE_CUBE_MAP_POSITIVE_X:
    case GraphicsContext3D::TEXTURE_CUBE_MAP_NEGATIVE_X:
    case GraphicsContext3D::TEXTURE_CUBE_MAP_POSITIVE_Y:
    case GraphicsContext3D::TEXTURE_CUBE_MAP_NEGATIVE_Y:
    case GraphicsContext3D::TEXTURE_CUBE_MAP_POSITIVE_Z:
    case GraphicsContext3D::TEXTURE_CUBE_MAP_NEGATIVE_Z:
        if (!useSixEnumsForCubeMap) {
            synthesizeGLError(GraphicsContext3D::INVALID_ENUM, functionName, "invalid texture target");
            return 0;
        }
        break;
    case GraphicsContext3D::TEXTURE_CUBE_MAP:
        if (useSixEnumsForCubeMap) {
            synthesizeGLError(GraphicsContext3D::INVALID_ENUM, functionName, "invalid texture target");
            return 0;
        }
        break;
    default:
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, functionName, "invalid texture target");
        return 0;
    }
    WebGLTexture* tex = m_textureUnits[m_activeTextureUnit].m_textureBinding.get();
    if (!tex)
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, functionName, "no texture");
    return tex;
}

bool WebGLRenderingContext::validateLocationLength(const char* functionName, const String& string)
{
    const unsigned maxWebGLLocationLength = 256;
    if (string.length() > maxWebGLLocationLength) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "location length > 256");
        return false;
    }
    return true;
}

bool WebGLRenderingContext::validateSize(const char* functionName, GC3Dint x, GC3Dint y)
{
    if (x < 0 || y < 0) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "size < 0");
        return false;
    }
    return true;
}

bool WebGLRenderingContext::validateString(const char* functionName, const String& string)
{
    for (size_t i = 0; i < string.length(); ++i) {
        if (!validateCharacter(string[i])) {
            synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "string not ASCII");
            return false;
        }
    }
    return true;
}

bool WebGLRenderingContext::validateTexFuncFormatAndType(const char* functionName, GC3Denum format, GC3Denum type, GC3Dint level)
{
    switch (format) {
    case GraphicsContext3D::ALPHA:
    case GraphicsContext3D::LUMINANCE:
    case GraphicsContext3D::LUMINANCE_ALPHA:
    case GraphicsContext3D::RGB:
    case GraphicsContext3D::RGBA:
        break;
    case GraphicsContext3D::DEPTH_STENCIL:
    case GraphicsContext3D::DEPTH_COMPONENT:
        if (m_webglDepthTexture)
            break;
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, functionName, "depth texture formats not enabled");
        return false;
    default:
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, functionName, "invalid texture format");
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
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, functionName, "invalid texture type");
        return false;
    case GraphicsContext3D::HALF_FLOAT_OES:
        if (m_oesTextureHalfFloat)
            break;
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, functionName, "invalid texture type");
        return false;
    case GraphicsContext3D::UNSIGNED_INT:
    case GraphicsContext3D::UNSIGNED_INT_24_8:
    case GraphicsContext3D::UNSIGNED_SHORT:
        if (m_webglDepthTexture)
            break;
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, functionName, "invalid texture type");
        return false;
    default:
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, functionName, "invalid texture type");
        return false;
    }

    // Verify that the combination of format and type is supported.
    switch (format) {
    case GraphicsContext3D::ALPHA:
    case GraphicsContext3D::LUMINANCE:
    case GraphicsContext3D::LUMINANCE_ALPHA:
        if (type != GraphicsContext3D::UNSIGNED_BYTE
            && type != GraphicsContext3D::FLOAT
            && type != GraphicsContext3D::HALF_FLOAT_OES) {
            synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, functionName, "invalid type for format");
            return false;
        }
        break;
    case GraphicsContext3D::RGB:
        if (type != GraphicsContext3D::UNSIGNED_BYTE
            && type != GraphicsContext3D::UNSIGNED_SHORT_5_6_5
            && type != GraphicsContext3D::FLOAT
            && type != GraphicsContext3D::HALF_FLOAT_OES) {
            synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, functionName, "invalid type for RGB format");
            return false;
        }
        break;
    case GraphicsContext3D::RGBA:
        if (type != GraphicsContext3D::UNSIGNED_BYTE
            && type != GraphicsContext3D::UNSIGNED_SHORT_4_4_4_4
            && type != GraphicsContext3D::UNSIGNED_SHORT_5_5_5_1
            && type != GraphicsContext3D::FLOAT
            && type != GraphicsContext3D::HALF_FLOAT_OES) {
            synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, functionName, "invalid type for RGBA format");
            return false;
        }
        break;
    case GraphicsContext3D::DEPTH_COMPONENT:
        if (!m_webglDepthTexture) {
            synthesizeGLError(GraphicsContext3D::INVALID_ENUM, functionName, "invalid format. DEPTH_COMPONENT not enabled");
            return false;
        }
        if (type != GraphicsContext3D::UNSIGNED_SHORT
            && type != GraphicsContext3D::UNSIGNED_INT) {
            synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, functionName, "invalid type for DEPTH_COMPONENT format");
            return false;
        }
        if (level > 0) {
          synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, functionName, "level must be 0 for DEPTH_COMPONENT format");
          return false;
        }
        break;
    case GraphicsContext3D::DEPTH_STENCIL:
        if (!m_webglDepthTexture) {
            synthesizeGLError(GraphicsContext3D::INVALID_ENUM, functionName, "invalid format. DEPTH_STENCIL not enabled");
            return false;
        }
        if (type != GraphicsContext3D::UNSIGNED_INT_24_8) {
            synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, functionName, "invalid type for DEPTH_STENCIL format");
            return false;
        }
        if (level > 0) {
          synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, functionName, "level must be 0 for DEPTH_STENCIL format");
          return false;
        }
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    return true;
}

bool WebGLRenderingContext::validateTexFuncLevel(const char* functionName, GC3Denum target, GC3Dint level)
{
    if (level < 0) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "level < 0");
        return false;
    }
    switch (target) {
    case GraphicsContext3D::TEXTURE_2D:
        if (level > m_maxTextureLevel) {
            synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "level out of range");
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
            synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "level out of range");
            return false;
        }
        break;
    }
    // This function only checks if level is legal, so we return true and don't
    // generate INVALID_ENUM if target is illegal.
    return true;
}

bool WebGLRenderingContext::validateTexFuncParameters(const char* functionName,
                                                      TexFuncValidationFunctionType functionType,
                                                      GC3Denum target, GC3Dint level,
                                                      GC3Denum internalformat,
                                                      GC3Dsizei width, GC3Dsizei height, GC3Dint border,
                                                      GC3Denum format, GC3Denum type)
{
    // We absolutely have to validate the format and type combination.
    // The texImage2D entry points taking HTMLImage, etc. will produce
    // temporary data based on this combination, so it must be legal.
    if (!validateTexFuncFormatAndType(functionName, format, type, level) || !validateTexFuncLevel(functionName, target, level))
        return false;

    if (width < 0 || height < 0) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "width or height < 0");
        return false;
    }

    switch (target) {
    case GraphicsContext3D::TEXTURE_2D:
        if (width > m_maxTextureSize || height > m_maxTextureSize) {
            synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "width or height out of range");
            return false;
        }
        break;
    case GraphicsContext3D::TEXTURE_CUBE_MAP_POSITIVE_X:
    case GraphicsContext3D::TEXTURE_CUBE_MAP_NEGATIVE_X:
    case GraphicsContext3D::TEXTURE_CUBE_MAP_POSITIVE_Y:
    case GraphicsContext3D::TEXTURE_CUBE_MAP_NEGATIVE_Y:
    case GraphicsContext3D::TEXTURE_CUBE_MAP_POSITIVE_Z:
    case GraphicsContext3D::TEXTURE_CUBE_MAP_NEGATIVE_Z:
        if (functionType != TexSubImage2D && width != height) {
          synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "width != height for cube map");
          return false;
        }
        // No need to check height here. For texImage width == height.
        // For texSubImage that will be checked when checking yoffset + height is in range.
        if (width > m_maxCubeMapTextureSize) {
            synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "width or height out of range for cube map");
            return false;
        }
        break;
    default:
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, functionName, "invalid target");
        return false;
    }

    if (format != internalformat) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, functionName, "format != internalformat");
        return false;
    }

    if (border) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "border != 0");
        return false;
    }

    return true;
}

bool WebGLRenderingContext::validateTexFuncData(const char* functionName, GC3Dint level,
                                                GC3Dsizei width, GC3Dsizei height,
                                                GC3Denum format, GC3Denum type,
                                                ArrayBufferView* pixels,
                                                NullDisposition disposition)
{
    if (!pixels) {
        if (disposition == NullAllowed)
            return true;
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "no pixels");
        return false;
    }

    if (!validateTexFuncFormatAndType(functionName, format, type, level))
        return false;
    if (!validateSettableTexFormat(functionName, format))
        return false;

    switch (type) {
    case GraphicsContext3D::UNSIGNED_BYTE:
        if (pixels->getType() != ArrayBufferView::TypeUint8) {
            synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, functionName, "type UNSIGNED_BYTE but ArrayBufferView not Uint8Array");
            return false;
        }
        break;
    case GraphicsContext3D::UNSIGNED_SHORT_5_6_5:
    case GraphicsContext3D::UNSIGNED_SHORT_4_4_4_4:
    case GraphicsContext3D::UNSIGNED_SHORT_5_5_5_1:
        if (pixels->getType() != ArrayBufferView::TypeUint16) {
            synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, functionName, "type UNSIGNED_SHORT but ArrayBufferView not Uint16Array");
            return false;
        }
        break;
    case GraphicsContext3D::FLOAT: // OES_texture_float
        if (pixels->getType() != ArrayBufferView::TypeFloat32) {
            synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, functionName, "type FLOAT but ArrayBufferView not Float32Array");
            return false;
        }
        break;
    case GraphicsContext3D::HALF_FLOAT_OES: // OES_texture_half_float
        // As per the specification, ArrayBufferView should be null when
        // OES_texture_half_float is enabled.
        if (pixels) {
            synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, functionName, "type HALF_FLOAT_OES but ArrayBufferView is not NULL");
            return false;
        }
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    unsigned int totalBytesRequired;
    GC3Denum error = m_context->computeImageSizeInBytes(format, type, width, height, m_unpackAlignment, &totalBytesRequired, 0);
    if (error != GraphicsContext3D::NO_ERROR) {
        synthesizeGLError(error, functionName, "invalid texture dimensions");
        return false;
    }
    if (pixels->byteLength() < totalBytesRequired) {
        if (m_unpackAlignment != 1) {
          error = m_context->computeImageSizeInBytes(format, type, width, height, 1, &totalBytesRequired, 0);
          if (pixels->byteLength() == totalBytesRequired) {
            synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, functionName, "ArrayBufferView not big enough for request with UNPACK_ALIGNMENT > 1");
            return false;
          }
        }
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, functionName, "ArrayBufferView not big enough for request");
        return false;
    }
    return true;
}

bool WebGLRenderingContext::validateCompressedTexFormat(GC3Denum format)
{
    return m_compressedTextureFormats.contains(format);
}

bool WebGLRenderingContext::validateCompressedTexFuncData(const char* functionName,
                                                          GC3Dsizei width, GC3Dsizei height,
                                                          GC3Denum format, ArrayBufferView* pixels)
{
    if (!pixels) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "no pixels");
        return false;
    }
    if (width < 0 || height < 0) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "width or height < 0");
        return false;
    }

    unsigned int bytesRequired = 0;

    switch (format) {
    case Extensions3D::COMPRESSED_RGB_S3TC_DXT1_EXT:
    case Extensions3D::COMPRESSED_RGBA_S3TC_DXT1_EXT:
        {
            const int kBlockWidth = 4;
            const int kBlockHeight = 4;
            const int kBlockSize = 8;
            int numBlocksAcross = (width + kBlockWidth - 1) / kBlockWidth;
            int numBlocksDown = (height + kBlockHeight - 1) / kBlockHeight;
            int numBlocks = numBlocksAcross * numBlocksDown;
            bytesRequired = numBlocks * kBlockSize;
        }
        break;
    case Extensions3D::COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case Extensions3D::COMPRESSED_RGBA_S3TC_DXT5_EXT:
        {
            const int kBlockWidth = 4;
            const int kBlockHeight = 4;
            const int kBlockSize = 16;
            int numBlocksAcross = (width + kBlockWidth - 1) / kBlockWidth;
            int numBlocksDown = (height + kBlockHeight - 1) / kBlockHeight;
            int numBlocks = numBlocksAcross * numBlocksDown;
            bytesRequired = numBlocks * kBlockSize;
        }
        break;
    case Extensions3D::COMPRESSED_ATC_RGB_AMD:
        {
            bytesRequired = floor(static_cast<double>((width + 3) / 4)) * floor(static_cast<double>((height + 3) / 4)) * 8;
        }
        break;
    case Extensions3D::COMPRESSED_ATC_RGBA_EXPLICIT_ALPHA_AMD:
    case Extensions3D::COMPRESSED_ATC_RGBA_INTERPOLATED_ALPHA_AMD:
        {
            bytesRequired = floor(static_cast<double>((width + 3) / 4)) * floor(static_cast<double>((height + 3) / 4)) * 16;
        }
        break;
    case Extensions3D::COMPRESSED_RGB_PVRTC_4BPPV1_IMG:
    case Extensions3D::COMPRESSED_RGBA_PVRTC_4BPPV1_IMG:
        {
            bytesRequired = max(width, 8) * max(height, 8) / 2;
        }
        break;
    case Extensions3D::COMPRESSED_RGB_PVRTC_2BPPV1_IMG:
    case Extensions3D::COMPRESSED_RGBA_PVRTC_2BPPV1_IMG:
        {
            bytesRequired = max(width, 8) * max(height, 8) / 4;
        }
        break;
    default:
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, functionName, "invalid format");
        return false;
    }

    if (pixels->byteLength() != bytesRequired) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "length of ArrayBufferView is not correct for dimensions");
        return false;
    }

    return true;
}

bool WebGLRenderingContext::validateCompressedTexDimensions(const char* functionName, GC3Dint level, GC3Dsizei width, GC3Dsizei height, GC3Denum format)
{
    switch (format) {
    case Extensions3D::COMPRESSED_RGB_S3TC_DXT1_EXT:
    case Extensions3D::COMPRESSED_RGBA_S3TC_DXT1_EXT:
    case Extensions3D::COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case Extensions3D::COMPRESSED_RGBA_S3TC_DXT5_EXT: {
        const int kBlockWidth = 4;
        const int kBlockHeight = 4;
        bool widthValid = (level && width == 1) || (level && width == 2) || !(width % kBlockWidth);
        bool heightValid = (level && height == 1) || (level && height == 2) || !(height % kBlockHeight);
        if (!widthValid || !heightValid) {
          synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, functionName, "width or height invalid for level");
          return false;
        }
        return true;
    }
    default:
        return false;
    }
}

bool WebGLRenderingContext::validateCompressedTexSubDimensions(const char* functionName, GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset,
                                                               GC3Dsizei width, GC3Dsizei height, GC3Denum format, WebGLTexture* tex)
{
    if (xoffset < 0 || yoffset < 0) {
      synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "xoffset or yoffset < 0");
      return false;
    }

    switch (format) {
    case Extensions3D::COMPRESSED_RGB_S3TC_DXT1_EXT:
    case Extensions3D::COMPRESSED_RGBA_S3TC_DXT1_EXT:
    case Extensions3D::COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case Extensions3D::COMPRESSED_RGBA_S3TC_DXT5_EXT: {
        const int kBlockWidth = 4;
        const int kBlockHeight = 4;
        if ((xoffset % kBlockWidth) || (yoffset % kBlockHeight)) {
            synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, functionName, "xoffset or yoffset not multiple of 4");
            return false;
        }
        if (width - xoffset > tex->getWidth(target, level)
            || height - yoffset > tex->getHeight(target, level)) {
            synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, functionName, "dimensions out of range");
            return false;
        }
        return validateCompressedTexDimensions(functionName, level, width, height, format);
    }
    default:
        return false;
    }
}

bool WebGLRenderingContext::validateDrawMode(const char* functionName, GC3Denum mode)
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
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, functionName, "invalid draw mode");
        return false;
    }
}

bool WebGLRenderingContext::validateStencilSettings(const char* functionName)
{
    if (m_stencilMask != m_stencilMaskBack || m_stencilFuncRef != m_stencilFuncRefBack || m_stencilFuncMask != m_stencilFuncMaskBack) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, functionName, "front and back stencils settings do not match");
        return false;
    }
    return true;
}

bool WebGLRenderingContext::validateStencilFunc(const char* functionName, GC3Denum func)
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
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, functionName, "invalid function");
        return false;
    }
}

void WebGLRenderingContext::printGLErrorToConsole(const String& message)
{
    if (!m_numGLErrorsToConsoleAllowed)
        return;

    --m_numGLErrorsToConsoleAllowed;
    printWarningToConsole(message);

    if (!m_numGLErrorsToConsoleAllowed)
        printWarningToConsole("WebGL: too many errors, no more errors will be reported to the console for this context.");
}

void WebGLRenderingContext::printWarningToConsole(const String& message)
{
    if (!canvas())
        return;
    Document* document = canvas()->document();
    if (!document)
        return;
    document->addConsoleMessage(RenderingMessageSource, WarningMessageLevel, message);
}

bool WebGLRenderingContext::validateFramebufferFuncParameters(const char* functionName, GC3Denum target, GC3Denum attachment)
{
    if (target != GraphicsContext3D::FRAMEBUFFER) {
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, functionName, "invalid target");
        return false;
    }
    switch (attachment) {
    case GraphicsContext3D::COLOR_ATTACHMENT0:
    case GraphicsContext3D::DEPTH_ATTACHMENT:
    case GraphicsContext3D::STENCIL_ATTACHMENT:
    case GraphicsContext3D::DEPTH_STENCIL_ATTACHMENT:
        break;
    default:
        if (m_extDrawBuffers
            && attachment > GraphicsContext3D::COLOR_ATTACHMENT0
            && attachment < static_cast<GC3Denum>(GraphicsContext3D::COLOR_ATTACHMENT0 + getMaxColorAttachments()))
            break;
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, functionName, "invalid attachment");
        return false;
    }
    return true;
}

bool WebGLRenderingContext::validateBlendEquation(const char* functionName, GC3Denum mode)
{
    switch (mode) {
    case GraphicsContext3D::FUNC_ADD:
    case GraphicsContext3D::FUNC_SUBTRACT:
    case GraphicsContext3D::FUNC_REVERSE_SUBTRACT:
        return true;
    default:
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, functionName, "invalid mode");
        return false;
    }
}

bool WebGLRenderingContext::validateBlendFuncFactors(const char* functionName, GC3Denum src, GC3Denum dst)
{
    if (((src == GraphicsContext3D::CONSTANT_COLOR || src == GraphicsContext3D::ONE_MINUS_CONSTANT_COLOR)
         && (dst == GraphicsContext3D::CONSTANT_ALPHA || dst == GraphicsContext3D::ONE_MINUS_CONSTANT_ALPHA))
        || ((dst == GraphicsContext3D::CONSTANT_COLOR || dst == GraphicsContext3D::ONE_MINUS_CONSTANT_COLOR)
            && (src == GraphicsContext3D::CONSTANT_ALPHA || src == GraphicsContext3D::ONE_MINUS_CONSTANT_ALPHA))) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, functionName, "incompatible src and dst");
        return false;
    }
    return true;
}

bool WebGLRenderingContext::validateCapability(const char* functionName, GC3Denum cap)
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
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, functionName, "invalid capability");
        return false;
    }
}

bool WebGLRenderingContext::validateUniformParameters(const char* functionName, const WebGLUniformLocation* location, Float32Array* v, GC3Dsizei requiredMinSize)
{
    if (!v) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "no array");
        return false;
    }
    return validateUniformMatrixParameters(functionName, location, false, v->data(), v->length(), requiredMinSize);
}

bool WebGLRenderingContext::validateUniformParameters(const char* functionName, const WebGLUniformLocation* location, Int32Array* v, GC3Dsizei requiredMinSize)
{
    if (!v) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "no array");
        return false;
    }
    return validateUniformMatrixParameters(functionName, location, false, v->data(), v->length(), requiredMinSize);
}

bool WebGLRenderingContext::validateUniformParameters(const char* functionName, const WebGLUniformLocation* location, void* v, GC3Dsizei size, GC3Dsizei requiredMinSize)
{
    return validateUniformMatrixParameters(functionName, location, false, v, size, requiredMinSize);
}

bool WebGLRenderingContext::validateUniformMatrixParameters(const char* functionName, const WebGLUniformLocation* location, GC3Dboolean transpose, Float32Array* v, GC3Dsizei requiredMinSize)
{
    if (!v) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "no array");
        return false;
    }
    return validateUniformMatrixParameters(functionName, location, transpose, v->data(), v->length(), requiredMinSize);
}

bool WebGLRenderingContext::validateUniformMatrixParameters(const char* functionName, const WebGLUniformLocation* location, GC3Dboolean transpose, void* v, GC3Dsizei size, GC3Dsizei requiredMinSize)
{
    if (!location)
        return false;
    if (location->program() != m_currentProgram) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, functionName, "location is not from current program");
        return false;
    }
    if (!v) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "no array");
        return false;
    }
    if (transpose) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "transpose not FALSE");
        return false;
    }
    if (size < requiredMinSize || (size % requiredMinSize)) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "invalid size");
        return false;
    }
    return true;
}

WebGLBuffer* WebGLRenderingContext::validateBufferDataParameters(const char* functionName, GC3Denum target, GC3Denum usage)
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
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM, functionName, "invalid target");
        return 0;
    }
    if (!buffer) {
        synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, functionName, "no buffer");
        return 0;
    }
    switch (usage) {
    case GraphicsContext3D::STREAM_DRAW:
    case GraphicsContext3D::STATIC_DRAW:
    case GraphicsContext3D::DYNAMIC_DRAW:
        return buffer;
    }
    synthesizeGLError(GraphicsContext3D::INVALID_ENUM, functionName, "invalid usage");
    return 0;
}

bool WebGLRenderingContext::validateHTMLImageElement(const char* functionName, HTMLImageElement* image, ExceptionCode& ec)
{
    if (!image || !image->cachedImage()) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "no image");
        return false;
    }
    const KURL& url = image->cachedImage()->response().url();
    if (url.isNull() || url.isEmpty() || !url.isValid()) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "invalid image");
        return false;
    }
    if (wouldTaintOrigin(image)) {
        ec = SECURITY_ERR;
        return false;
    }
    return true;
}

bool WebGLRenderingContext::validateHTMLCanvasElement(const char* functionName, HTMLCanvasElement* canvas, ExceptionCode& ec)
{
    if (!canvas || !canvas->buffer()) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "no canvas");
        return false;
    }
    if (wouldTaintOrigin(canvas)) {
        ec = SECURITY_ERR;
        return false;
    }
    return true;
}

#if ENABLE(VIDEO)
bool WebGLRenderingContext::validateHTMLVideoElement(const char* functionName, HTMLVideoElement* video, ExceptionCode& ec)
{
    if (!video || !video->videoWidth() || !video->videoHeight()) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "no video");
        return false;
    }
    if (wouldTaintOrigin(video)) {
        ec = SECURITY_ERR;
        return false;
    }
    return true;
}
#endif

void WebGLRenderingContext::vertexAttribfImpl(const char* functionName, GC3Duint index, GC3Dsizei expectedSize, GC3Dfloat v0, GC3Dfloat v1, GC3Dfloat v2, GC3Dfloat v3)
{
    if (isContextLost())
        return;
    if (index >= m_maxVertexAttribs) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "index out of range");
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

void WebGLRenderingContext::vertexAttribfvImpl(const char* functionName, GC3Duint index, Float32Array* v, GC3Dsizei expectedSize)
{
    if (isContextLost())
        return;
    if (!v) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "no array");
        return;
    }
    vertexAttribfvImpl(functionName, index, v->data(), v->length(), expectedSize);
}

void WebGLRenderingContext::vertexAttribfvImpl(const char* functionName, GC3Duint index, GC3Dfloat* v, GC3Dsizei size, GC3Dsizei expectedSize)
{
    if (isContextLost())
        return;
    if (!v) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "no array");
        return;
    }
    if (size < expectedSize) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "invalid size");
        return;
    }
    if (index >= m_maxVertexAttribs) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE, functionName, "index out of range");
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

void WebGLRenderingContext::dispatchContextLostEvent(Timer<WebGLRenderingContext>*)
{
    RefPtr<WebGLContextEvent> event = WebGLContextEvent::create(eventNames().webglcontextlostEvent, false, true, "");
    canvas()->dispatchEvent(event);
    m_restoreAllowed = event->defaultPrevented();
    if (m_contextLostMode == RealLostContext && m_restoreAllowed)
        m_restoreTimer.startOneShot(0);
}

void WebGLRenderingContext::maybeRestoreContext(Timer<WebGLRenderingContext>*)
{
    ASSERT(m_contextLost);
    if (!m_contextLost)
        return;

    // The rendering context is not restored unless the default behavior of the
    // webglcontextlost event was prevented earlier.
    //
    // Because of the way m_restoreTimer is set up for real vs. synthetic lost
    // context events, we don't have to worry about this test short-circuiting
    // the retry loop for real context lost events.
    if (!m_restoreAllowed)
        return;

    int contextLostReason = m_context->getExtensions()->getGraphicsResetStatusARB();

    switch (contextLostReason) {
    case GraphicsContext3D::NO_ERROR:
        // The GraphicsContext3D implementation might not fully
        // support GL_ARB_robustness semantics yet. Alternatively, the
        // WEBGL_lose_context extension might have been used to force
        // a lost context.
        break;
    case Extensions3D::GUILTY_CONTEXT_RESET_ARB:
        // The rendering context is not restored if this context was
        // guilty of causing the graphics reset.
        printWarningToConsole("WARNING: WebGL content on the page caused the graphics card to reset; not restoring the context");
        return;
    case Extensions3D::INNOCENT_CONTEXT_RESET_ARB:
        // Always allow the context to be restored.
        break;
    case Extensions3D::UNKNOWN_CONTEXT_RESET_ARB:
        // Warn. Ideally, prompt the user telling them that WebGL
        // content on the page might have caused the graphics card to
        // reset and ask them whether they want to continue running
        // the content. Only if they say "yes" should we start
        // attempting to restore the context.
        printWarningToConsole("WARNING: WebGL content on the page might have caused the graphics card to reset");
        break;
    }

    Document* document = canvas()->document();
    if (!document)
        return;
    Frame* frame = document->frame();
    if (!frame)
        return;

    if (!frame->loader()->client()->allowWebGL(frame->settings() && frame->settings()->webGLEnabled()))
        return;

    FrameView* view = frame->view();
    if (!view)
        return;
    ScrollView* root = view->root();
    if (!root)
        return;
    HostWindow* hostWindow = root->hostWindow();
    if (!hostWindow)
        return;

    RefPtr<GraphicsContext3D> context(GraphicsContext3D::create(m_attributes, hostWindow));
    if (!context) {
        if (m_contextLostMode == RealLostContext)
            m_restoreTimer.startOneShot(secondsBetweenRestoreAttempts);
        else
            // This likely shouldn't happen but is the best way to report it to the WebGL app.
            synthesizeGLError(GraphicsContext3D::INVALID_OPERATION, "", "error restoring context");
        return;
    }

    // Construct a new drawing buffer with the new GraphicsContext3D.
    if (m_drawingBuffer) {
        m_drawingBuffer->discardResources();
        DrawingBuffer::PreserveDrawingBuffer preserve = m_attributes.preserveDrawingBuffer ? DrawingBuffer::Preserve : DrawingBuffer::Discard;
        DrawingBuffer::AlphaRequirement alpha = m_attributes.alpha ? DrawingBuffer::Alpha : DrawingBuffer::Opaque;
        m_drawingBuffer = DrawingBuffer::create(context.get(), m_drawingBuffer->size(), preserve, alpha);
        m_drawingBuffer->bind();
    }

    m_context = context;
    m_contextLost = false;
    setupFlags();
    initializeNewContext();
    canvas()->dispatchEvent(WebGLContextEvent::create(eventNames().webglcontextrestoredEvent, false, true, ""));
}

String WebGLRenderingContext::ensureNotNull(const String& text) const
{
    if (text.isNull())
        return WTF::emptyString();
    return text;
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
        if (buf->logicalSize() != size)
            continue;
        bubbleToFront(i);
        return buf;
    }

    OwnPtr<ImageBuffer> temp = ImageBuffer::create(size, 1);
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

namespace {

    String GetErrorString(GC3Denum error)
    {
        switch (error) {
        case GraphicsContext3D::INVALID_ENUM:
            return "INVALID_ENUM";
        case GraphicsContext3D::INVALID_VALUE:
            return "INVALID_VALUE";
        case GraphicsContext3D::INVALID_OPERATION:
            return "INVALID_OPERATION";
        case GraphicsContext3D::OUT_OF_MEMORY:
            return "OUT_OF_MEMORY";
        case GraphicsContext3D::INVALID_FRAMEBUFFER_OPERATION:
            return "INVALID_FRAMEBUFFER_OPERATION";
        case GraphicsContext3D::CONTEXT_LOST_WEBGL:
            return "CONTEXT_LOST_WEBGL";
        default:
            return String::format("WebGL ERROR(%04x)", error);
        }
    }

} // namespace anonymous

void WebGLRenderingContext::synthesizeGLError(GC3Denum error, const char* functionName, const char* description, ConsoleDisplayPreference display)
{
    if (m_synthesizedErrorsToConsole && display == DisplayInConsole) {
      String str = String("WebGL: ") + GetErrorString(error) +  ": " + String(functionName) + ": " + String(description);
      printGLErrorToConsole(str);
    }
    m_context->synthesizeGLError(error);
}


void WebGLRenderingContext::printGLWarningToConsole(const char* functionName, const char* description)
{
    if (m_synthesizedErrorsToConsole) {
        String str = String("WebGL: ") + String(functionName) + ": " + String(description);
        printGLErrorToConsole(str);
    }
}

void WebGLRenderingContext::applyStencilTest()
{
    bool haveStencilBuffer = false;

    if (m_framebufferBinding)
        haveStencilBuffer = m_framebufferBinding->hasStencilBuffer();
    else {
        RefPtr<WebGLContextAttributes> attributes = getContextAttributes();
        haveStencilBuffer = attributes->stencil();
    }
    enableOrDisable(GraphicsContext3D::STENCIL_TEST,
                    m_stencilEnabled && haveStencilBuffer);
}

void WebGLRenderingContext::enableOrDisable(GC3Denum capability, bool enable)
{
    if (enable)
        m_context->enable(capability);
    else
        m_context->disable(capability);
}

IntSize WebGLRenderingContext::clampedCanvasSize()
{
    return IntSize(clamp(canvas()->width(), 1, m_maxViewportDims[0]),
                   clamp(canvas()->height(), 1, m_maxViewportDims[1]));
}

GC3Dint WebGLRenderingContext::getMaxDrawBuffers()
{
    if (!supportsDrawBuffers())
        return 0;
    if (!m_maxDrawBuffers)
        m_context->getIntegerv(Extensions3D::MAX_DRAW_BUFFERS_EXT, &m_maxDrawBuffers);
    if (!m_maxColorAttachments)
        m_context->getIntegerv(Extensions3D::MAX_COLOR_ATTACHMENTS_EXT, &m_maxColorAttachments);
    // WEBGL_draw_buffers requires MAX_COLOR_ATTACHMENTS >= MAX_DRAW_BUFFERS.
    return std::min(m_maxDrawBuffers, m_maxColorAttachments);
}

GC3Dint WebGLRenderingContext::getMaxColorAttachments()
{
    if (!supportsDrawBuffers())
        return 0;
    if (!m_maxColorAttachments)
        m_context->getIntegerv(Extensions3D::MAX_COLOR_ATTACHMENTS_EXT, &m_maxColorAttachments);
    return m_maxColorAttachments;
}

void WebGLRenderingContext::setBackDrawBuffer(GC3Denum buf)
{
    m_backDrawBuffer = buf;
}

void WebGLRenderingContext::restoreCurrentFramebuffer()
{
    ExceptionCode ec;
    bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_framebufferBinding.get(), ec);
}

void WebGLRenderingContext::restoreCurrentTexture2D()
{
    ExceptionCode ec;
    bindTexture(GraphicsContext3D::TEXTURE_2D, m_textureUnits[m_activeTextureUnit].m_textureBinding.get(), ec);
}

bool WebGLRenderingContext::supportsDrawBuffers()
{
    if (!m_drawBuffersWebGLRequirementsChecked) {
        m_drawBuffersWebGLRequirementsChecked = true;
        m_drawBuffersSupported = EXTDrawBuffers::supported(this);
    }
    return m_drawBuffersSupported;
}

} // namespace WebCore

#endif // ENABLE(WEBGL)
