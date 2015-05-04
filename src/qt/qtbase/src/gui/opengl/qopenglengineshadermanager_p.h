/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

/*
    VERTEX SHADERS
    ==============

    Vertex shaders are specified as multiple (partial) shaders. On desktop,
    this works fine. On ES, QOpenGLShader & QOpenGLShaderProgram will make partial
    shaders work by concatenating the source in each QOpenGLShader and compiling
    it as a single shader. This is abstracted nicely by QOpenGLShaderProgram and
    the GL2 engine doesn't need to worry about it.

    Generally, there's two vertex shader objects. The position shaders are
    the ones which set gl_Position. There's also two "main" vertex shaders,
    one which just calls the position shader and another which also passes
    through some texture coordinates from a vertex attribute array to a
    varying. These texture coordinates are used for mask position in text
    rendering and for the source coordinates in drawImage/drawPixmap. There's
    also a "Simple" vertex shader for rendering a solid colour (used to render
    into the stencil buffer where the actual colour value is discarded).

    The position shaders for brushes look scary. This is because many of the
    calculations which logically belong in the fragment shader have been moved
    into the vertex shader to improve performance. This is why the position
    calculation is in a separate shader. Not only does it calculate the
    position, but it also calculates some data to be passed to the fragment
    shader as a varying. It is optimal to move as much of the calculation as
    possible into the vertex shader as this is executed less often.

    The varyings passed to the fragment shaders are interpolated (which is
    cheap). Unfortunately, GL will apply perspective correction to the
    interpolation calusing errors. To get around this, the vertex shader must
    apply perspective correction itself and set the w-value of gl_Position to
    zero. That way, GL will be tricked into thinking it doesn't need to apply a
    perspective correction and use linear interpolation instead (which is what
    we want). Of course, if the brush transform is affeine, no perspective
    correction is needed and a simpler vertex shader can be used instead.

    So there are the following "main" vertex shaders:
        qopenglslMainVertexShader
        qopenglslMainWithTexCoordsVertexShader

    And the following position vertex shaders:
        qopenglslPositionOnlyVertexShader
        qopenglslPositionWithTextureBrushVertexShader
        qopenglslPositionWithPatternBrushVertexShader
        qopenglslPositionWithLinearGradientBrushVertexShader
        qopenglslPositionWithRadialGradientBrushVertexShader
        qopenglslPositionWithConicalGradientBrushVertexShader
        qopenglslAffinePositionWithTextureBrushVertexShader
        qopenglslAffinePositionWithPatternBrushVertexShader
        qopenglslAffinePositionWithLinearGradientBrushVertexShader
        qopenglslAffinePositionWithRadialGradientBrushVertexShader
        qopenglslAffinePositionWithConicalGradientBrushVertexShader

    Leading to 23 possible vertex shaders


    FRAGMENT SHADERS
    ================

    Fragment shaders are also specified as multiple (partial) shaders. The
    different fragment shaders represent the different stages in Qt's fragment
    pipeline. There are 1-3 stages in this pipeline: First stage is to get the
    fragment's colour value. The next stage is to get the fragment's mask value
    (coverage value for anti-aliasing) and the final stage is to blend the
    incoming fragment with the background (for composition modes not supported
    by GL).

    Of these, the first stage will always be present. If Qt doesn't need to
    apply anti-aliasing (because it's off or handled by multisampling) then
    the coverage value doesn't need to be applied. (Note: There are two types
    of mask, one for regular anti-aliasing and one for sub-pixel anti-
    aliasing.) If the composition mode is one which GL supports natively then
    the blending stage doesn't need to be applied.

    As eash stage can have multiple implementations, they are abstracted as
    GLSL function calls with the following signatures:

    Brushes & image drawing are implementations of "qcolorp vec4 srcPixel()":
        qopenglslImageSrcFragShader
        qopenglslImageSrcWithPatternFragShader
        qopenglslNonPremultipliedImageSrcFragShader
        qopenglslSolidBrushSrcFragShader
        qopenglslTextureBrushSrcFragShader
        qopenglslTextureBrushWithPatternFragShader
        qopenglslPatternBrushSrcFragShader
        qopenglslLinearGradientBrushSrcFragShader
        qopenglslRadialGradientBrushSrcFragShader
        qopenglslConicalGradientBrushSrcFragShader
    NOTE: It is assumed the colour returned by srcPixel() is pre-multiplied

    Masks are implementations of "qcolorp vec4 applyMask(qcolorp vec4 src)":
        qopenglslMaskFragmentShader
        qopenglslRgbMaskFragmentShaderPass1
        qopenglslRgbMaskFragmentShaderPass2
        qopenglslRgbMaskWithGammaFragmentShader

    Composition modes are "qcolorp vec4 compose(qcolorp vec4 src)":
        qopenglslColorBurnCompositionModeFragmentShader
        qopenglslColorDodgeCompositionModeFragmentShader
        qopenglslDarkenCompositionModeFragmentShader
        qopenglslDifferenceCompositionModeFragmentShader
        qopenglslExclusionCompositionModeFragmentShader
        qopenglslHardLightCompositionModeFragmentShader
        qopenglslLightenCompositionModeFragmentShader
        qopenglslMultiplyCompositionModeFragmentShader
        qopenglslOverlayCompositionModeFragmentShader
        qopenglslScreenCompositionModeFragmentShader
        qopenglslSoftLightCompositionModeFragmentShader


    Note: In the future, some GLSL compilers will support an extension allowing
          a new 'color' precision specifier. To support this, qcolorp is used for
          all color components so it can be defined to colorp or lowp depending upon
          the implementation.

    So there are differnt frament shader main functions, depending on the
    number & type of pipelines the fragment needs to go through.

    The choice of which main() fragment shader string to use depends on:
        - Use of global opacity
        - Brush style (some brushes apply opacity themselves)
        - Use & type of mask (TODO: Need to support high quality anti-aliasing & text)
        - Use of non-GL Composition mode

    Leading to the following fragment shader main functions:
        gl_FragColor = compose(applyMask(srcPixel()*globalOpacity));
        gl_FragColor = compose(applyMask(srcPixel()));
        gl_FragColor = applyMask(srcPixel()*globalOpacity);
        gl_FragColor = applyMask(srcPixel());
        gl_FragColor = compose(srcPixel()*globalOpacity);
        gl_FragColor = compose(srcPixel());
        gl_FragColor = srcPixel()*globalOpacity;
        gl_FragColor = srcPixel();

    Called:
        qopenglslMainFragmentShader_CMO
        qopenglslMainFragmentShader_CM
        qopenglslMainFragmentShader_MO
        qopenglslMainFragmentShader_M
        qopenglslMainFragmentShader_CO
        qopenglslMainFragmentShader_C
        qopenglslMainFragmentShader_O
        qopenglslMainFragmentShader

    Where:
        M = Mask
        C = Composition
        O = Global Opacity


    CUSTOM SHADER CODE
    ==================

    The use of custom shader code is supported by the engine for drawImage and
    drawPixmap calls. This is implemented via hooks in the fragment pipeline.

    The custom shader is passed to the engine as a partial fragment shader
    (QOpenGLCustomShaderStage). The shader will implement a pre-defined method name
    which Qt's fragment pipeline will call:

        lowp vec4 customShader(lowp sampler2d imageTexture, highp vec2 textureCoords)

    The provided src and srcCoords parameters can be used to sample from the
    source image.

    Transformations, clipping, opacity, and composition modes set using QPainter
    will be respected when using the custom shader hook.
*/

#ifndef QOPENGLENGINE_SHADER_MANAGER_H
#define QOPENGLENGINE_SHADER_MANAGER_H

#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QPainter>
#include <private/qopenglcontext_p.h>
#include <private/qopenglcustomshaderstage_p.h>

QT_BEGIN_NAMESPACE



/*
struct QOpenGLEngineCachedShaderProg
{
    QOpenGLEngineCachedShaderProg(QOpenGLEngineShaderManager::ShaderName vertexMain,
                              QOpenGLEngineShaderManager::ShaderName vertexPosition,
                              QOpenGLEngineShaderManager::ShaderName fragMain,
                              QOpenGLEngineShaderManager::ShaderName pixelSrc,
                              QOpenGLEngineShaderManager::ShaderName mask,
                              QOpenGLEngineShaderManager::ShaderName composition);

    int cacheKey;
    QOpenGLShaderProgram* program;
}
*/

static const GLuint QT_VERTEX_COORDS_ATTR  = 0;
static const GLuint QT_TEXTURE_COORDS_ATTR = 1;
static const GLuint QT_OPACITY_ATTR = 2;
static const GLuint QT_PMV_MATRIX_1_ATTR = 3;
static const GLuint QT_PMV_MATRIX_2_ATTR = 4;
static const GLuint QT_PMV_MATRIX_3_ATTR = 5;

class QOpenGLEngineShaderProg;

class Q_GUI_EXPORT QOpenGLEngineSharedShaders
{
    Q_GADGET
public:

    enum SnippetName {
        MainVertexShader,
        MainWithTexCoordsVertexShader,
        MainWithTexCoordsAndOpacityVertexShader,

        // UntransformedPositionVertexShader must be first in the list:
        UntransformedPositionVertexShader,
        PositionOnlyVertexShader,
        ComplexGeometryPositionOnlyVertexShader,
        PositionWithPatternBrushVertexShader,
        PositionWithLinearGradientBrushVertexShader,
        PositionWithConicalGradientBrushVertexShader,
        PositionWithRadialGradientBrushVertexShader,
        PositionWithTextureBrushVertexShader,
        AffinePositionWithPatternBrushVertexShader,
        AffinePositionWithLinearGradientBrushVertexShader,
        AffinePositionWithConicalGradientBrushVertexShader,
        AffinePositionWithRadialGradientBrushVertexShader,
        AffinePositionWithTextureBrushVertexShader,

        // MainFragmentShader_CMO must be first in the list:
        MainFragmentShader_CMO,
        MainFragmentShader_CM,
        MainFragmentShader_MO,
        MainFragmentShader_M,
        MainFragmentShader_CO,
        MainFragmentShader_C,
        MainFragmentShader_O,
        MainFragmentShader,
        MainFragmentShader_ImageArrays,

        // ImageSrcFragmentShader must be first in the list::
        ImageSrcFragmentShader,
        ImageSrcWithPatternFragmentShader,
        NonPremultipliedImageSrcFragmentShader,
        CustomImageSrcFragmentShader,
        SolidBrushSrcFragmentShader,
        TextureBrushSrcFragmentShader,
        TextureBrushSrcWithPatternFragmentShader,
        PatternBrushSrcFragmentShader,
        LinearGradientBrushSrcFragmentShader,
        RadialGradientBrushSrcFragmentShader,
        ConicalGradientBrushSrcFragmentShader,
        ShockingPinkSrcFragmentShader,

        // NoMaskFragmentShader must be first in the list:
        NoMaskFragmentShader,
        MaskFragmentShader,
        RgbMaskFragmentShaderPass1,
        RgbMaskFragmentShaderPass2,
        RgbMaskWithGammaFragmentShader,

        // NoCompositionModeFragmentShader must be first in the list:
        NoCompositionModeFragmentShader,
        MultiplyCompositionModeFragmentShader,
        ScreenCompositionModeFragmentShader,
        OverlayCompositionModeFragmentShader,
        DarkenCompositionModeFragmentShader,
        LightenCompositionModeFragmentShader,
        ColorDodgeCompositionModeFragmentShader,
        ColorBurnCompositionModeFragmentShader,
        HardLightCompositionModeFragmentShader,
        SoftLightCompositionModeFragmentShader,
        DifferenceCompositionModeFragmentShader,
        ExclusionCompositionModeFragmentShader,

        TotalSnippetCount, InvalidSnippetName
    };
#if defined (QT_DEBUG)
    Q_ENUMS(SnippetName)
    static QByteArray snippetNameStr(SnippetName snippetName);
#endif

/*
    // These allow the ShaderName enum to be used as a cache key
    const int mainVertexOffset = 0;
    const int positionVertexOffset = (1<<2) - PositionOnlyVertexShader;
    const int mainFragOffset = (1<<6) - MainFragmentShader_CMO;
    const int srcPixelOffset = (1<<10) - ImageSrcFragmentShader;
    const int maskOffset = (1<<14) - NoMaskShader;
    const int compositionOffset = (1 << 16) - MultiplyCompositionModeFragmentShader;
*/

    QOpenGLEngineSharedShaders(QOpenGLContext *context);
    ~QOpenGLEngineSharedShaders();

    QOpenGLShaderProgram *simpleProgram() { return simpleShaderProg; }
    QOpenGLShaderProgram *blitProgram() { return blitShaderProg; }
    // Compile the program if it's not already in the cache, return the item in the cache.
    QOpenGLEngineShaderProg *findProgramInCache(const QOpenGLEngineShaderProg &prog);
    // Compile the custom shader if it's not already in the cache, return the item in the cache.

    static QOpenGLEngineSharedShaders *shadersForContext(QOpenGLContext *context);

    // Ideally, this would be static and cleanup all programs in all contexts which
    // contain the custom code. Currently it is just a hint and we rely on deleted
    // custom shaders being cleaned up by being kicked out of the cache when it's
    // full.
    void cleanupCustomStage(QOpenGLCustomShaderStage* stage);

private:
    QOpenGLShaderProgram *blitShaderProg;
    QOpenGLShaderProgram *simpleShaderProg;
    QList<QOpenGLEngineShaderProg*> cachedPrograms;
    QList<QOpenGLShader *> shaders;

    static const char* qShaderSnippets[TotalSnippetCount];
};


class QOpenGLEngineShaderProg
{
public:
    QOpenGLEngineShaderProg() : program(0) {}

    ~QOpenGLEngineShaderProg() {
        if (program)
            delete program;
    }

    QOpenGLEngineSharedShaders::SnippetName mainVertexShader;
    QOpenGLEngineSharedShaders::SnippetName positionVertexShader;
    QOpenGLEngineSharedShaders::SnippetName mainFragShader;
    QOpenGLEngineSharedShaders::SnippetName srcPixelFragShader;
    QOpenGLEngineSharedShaders::SnippetName maskFragShader;
    QOpenGLEngineSharedShaders::SnippetName compositionFragShader;

    QByteArray          customStageSource; //TODO: Decent cache key for custom stages
    QOpenGLShaderProgram*   program;

    QVector<uint> uniformLocations;

    bool                useTextureCoords;
    bool                useOpacityAttribute;
    bool                usePmvMatrixAttribute;

    bool operator==(const QOpenGLEngineShaderProg& other) const {
        // We don't care about the program
        return ( mainVertexShader      == other.mainVertexShader &&
                 positionVertexShader  == other.positionVertexShader &&
                 mainFragShader        == other.mainFragShader &&
                 srcPixelFragShader    == other.srcPixelFragShader &&
                 maskFragShader        == other.maskFragShader &&
                 compositionFragShader == other.compositionFragShader &&
                 customStageSource     == other.customStageSource
               );
    }
};

class Q_GUI_EXPORT QOpenGLEngineShaderManager : public QObject
{
    Q_OBJECT
public:
    QOpenGLEngineShaderManager(QOpenGLContext* context);
    ~QOpenGLEngineShaderManager();

    enum MaskType {NoMask, PixelMask, SubPixelMaskPass1, SubPixelMaskPass2, SubPixelWithGammaMask};
    enum PixelSrcType {
        ImageSrc = Qt::TexturePattern+1,
        NonPremultipliedImageSrc = Qt::TexturePattern+2,
        PatternSrc = Qt::TexturePattern+3,
        TextureSrcWithPattern = Qt::TexturePattern+4
    };

    enum Uniform {
        ImageTexture,
        PatternColor,
        GlobalOpacity,
        Depth,
        MaskTexture,
        FragmentColor,
        LinearData,
        Angle,
        HalfViewportSize,
        Fmp,
        Fmp2MRadius2,
        Inverse2Fmp2MRadius2,
        SqrFr,
        BRadius,
        InvertedTextureSize,
        BrushTransform,
        BrushTexture,
        Matrix,
        NumUniforms
    };

    enum OpacityMode {
        NoOpacity,
        UniformOpacity,
        AttributeOpacity
    };

    // There are optimizations we can do, depending on the brush transform:
    //    1) May not have to apply perspective-correction
    //    2) Can use lower precision for matrix
    void optimiseForBrushTransform(QTransform::TransformationType transformType);
    void setSrcPixelType(Qt::BrushStyle);
    void setSrcPixelType(PixelSrcType); // For non-brush sources, like pixmaps & images
    void setOpacityMode(OpacityMode);
    void setMaskType(MaskType);
    void setCompositionMode(QPainter::CompositionMode);
    void setCustomStage(QOpenGLCustomShaderStage* stage);
    void removeCustomStage();

    GLuint getUniformLocation(Uniform id);

    void setDirty(); // someone has manually changed the current shader program
    bool useCorrectShaderProg(); // returns true if the shader program needed to be changed

    void useSimpleProgram();
    void useBlitProgram();
    void setHasComplexGeometry(bool hasComplexGeometry)
    {
        complexGeometry = hasComplexGeometry;
        shaderProgNeedsChanging = true;
    }
    bool hasComplexGeometry() const
    {
        return complexGeometry;
    }

    QOpenGLShaderProgram* currentProgram(); // Returns pointer to the shader the manager has chosen
    QOpenGLShaderProgram* simpleProgram(); // Used to draw into e.g. stencil buffers
    QOpenGLShaderProgram* blitProgram(); // Used to blit a texture into the framebuffer

    QOpenGLEngineSharedShaders* sharedShaders;

private:
    QOpenGLContext*     ctx;
    bool            shaderProgNeedsChanging;
    bool            complexGeometry;

    // Current state variables which influence the choice of shader:
    QTransform                  brushTransform;
    int                         srcPixelType;
    OpacityMode                 opacityMode;
    MaskType                    maskType;
    QPainter::CompositionMode   compositionMode;
    QOpenGLCustomShaderStage*       customSrcStage;

    QOpenGLEngineShaderProg*    currentShaderProg;
};

QT_END_NAMESPACE

#endif //QOPENGLENGINE_SHADER_MANAGER_H
