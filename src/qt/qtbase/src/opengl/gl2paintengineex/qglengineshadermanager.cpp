/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
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

#include "qglengineshadermanager_p.h"
#include "qglengineshadersource_p.h"
#include "qpaintengineex_opengl2_p.h"
#include "qglshadercache_p.h"

#include <QtGui/private/qopenglcontext_p.h>

#if defined(QT_DEBUG)
#include <QMetaEnum>
#endif

// #define QT_GL_SHARED_SHADER_DEBUG

QT_BEGIN_NAMESPACE

class QGLEngineSharedShadersResource : public QOpenGLSharedResource
{
public:
    QGLEngineSharedShadersResource(QOpenGLContext *ctx)
        : QOpenGLSharedResource(ctx->shareGroup())
        , m_shaders(new QGLEngineSharedShaders(QGLContext::fromOpenGLContext(ctx)))
    {
    }

    ~QGLEngineSharedShadersResource()
    {
        delete m_shaders;
    }

    void invalidateResource()
    {
        delete m_shaders;
        m_shaders = 0;
    }

    void freeResource(QOpenGLContext *)
    {
    }

    QGLEngineSharedShaders *shaders() const { return m_shaders; }

private:
    QGLEngineSharedShaders *m_shaders;
};

class QGLShaderStorage
{
public:
    QGLEngineSharedShaders *shadersForThread(const QGLContext *context) {
        QOpenGLMultiGroupSharedResource *&shaders = m_storage.localData();
        if (!shaders)
            shaders = new QOpenGLMultiGroupSharedResource;
        QGLEngineSharedShadersResource *resource =
            shaders->value<QGLEngineSharedShadersResource>(context->contextHandle());
        return resource ? resource->shaders() : 0;
    }

private:
    QThreadStorage<QOpenGLMultiGroupSharedResource *> m_storage;
};

Q_GLOBAL_STATIC(QGLShaderStorage, qt_shader_storage);

QGLEngineSharedShaders *QGLEngineSharedShaders::shadersForContext(const QGLContext *context)
{
    return qt_shader_storage()->shadersForThread(context);
}

const char* QGLEngineSharedShaders::qShaderSnippets[] = {
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0
};

QGLEngineSharedShaders::QGLEngineSharedShaders(const QGLContext* context)
    : blitShaderProg(0)
    , simpleShaderProg(0)
{

/*
    Rather than having the shader source array statically initialised, it is initialised
    here instead. This is to allow new shader names to be inserted or existing names moved
    around without having to change the order of the glsl strings. It is hoped this will
    make future hard-to-find runtime bugs more obvious and generally give more solid code.
*/
    static bool snippetsPopulated = false;
    if (!snippetsPopulated) {

        const char** code = qShaderSnippets; // shortcut

        code[MainVertexShader] = qglslMainVertexShader;
        code[MainWithTexCoordsVertexShader] = qglslMainWithTexCoordsVertexShader;
        code[MainWithTexCoordsAndOpacityVertexShader] = qglslMainWithTexCoordsAndOpacityVertexShader;

        code[UntransformedPositionVertexShader] = qglslUntransformedPositionVertexShader;
        code[PositionOnlyVertexShader] = qglslPositionOnlyVertexShader;
        code[ComplexGeometryPositionOnlyVertexShader] = qglslComplexGeometryPositionOnlyVertexShader;
        code[PositionWithPatternBrushVertexShader] = qglslPositionWithPatternBrushVertexShader;
        code[PositionWithLinearGradientBrushVertexShader] = qglslPositionWithLinearGradientBrushVertexShader;
        code[PositionWithConicalGradientBrushVertexShader] = qglslPositionWithConicalGradientBrushVertexShader;
        code[PositionWithRadialGradientBrushVertexShader] = qglslPositionWithRadialGradientBrushVertexShader;
        code[PositionWithTextureBrushVertexShader] = qglslPositionWithTextureBrushVertexShader;
        code[AffinePositionWithPatternBrushVertexShader] = qglslAffinePositionWithPatternBrushVertexShader;
        code[AffinePositionWithLinearGradientBrushVertexShader] = qglslAffinePositionWithLinearGradientBrushVertexShader;
        code[AffinePositionWithConicalGradientBrushVertexShader] = qglslAffinePositionWithConicalGradientBrushVertexShader;
        code[AffinePositionWithRadialGradientBrushVertexShader] = qglslAffinePositionWithRadialGradientBrushVertexShader;
        code[AffinePositionWithTextureBrushVertexShader] = qglslAffinePositionWithTextureBrushVertexShader;

        code[MainFragmentShader_CMO] = qglslMainFragmentShader_CMO;
        code[MainFragmentShader_CM] = qglslMainFragmentShader_CM;
        code[MainFragmentShader_MO] = qglslMainFragmentShader_MO;
        code[MainFragmentShader_M] = qglslMainFragmentShader_M;
        code[MainFragmentShader_CO] = qglslMainFragmentShader_CO;
        code[MainFragmentShader_C] = qglslMainFragmentShader_C;
        code[MainFragmentShader_O] = qglslMainFragmentShader_O;
        code[MainFragmentShader] = qglslMainFragmentShader;
        code[MainFragmentShader_ImageArrays] = qglslMainFragmentShader_ImageArrays;

        code[ImageSrcFragmentShader] = qglslImageSrcFragmentShader;
        code[ImageSrcWithPatternFragmentShader] = qglslImageSrcWithPatternFragmentShader;
        code[NonPremultipliedImageSrcFragmentShader] = qglslNonPremultipliedImageSrcFragmentShader;
        code[CustomImageSrcFragmentShader] = qglslCustomSrcFragmentShader; // Calls "customShader", which must be appended
        code[SolidBrushSrcFragmentShader] = qglslSolidBrushSrcFragmentShader;
        if (!context->contextHandle()->isOpenGLES())
            code[TextureBrushSrcFragmentShader] = qglslTextureBrushSrcFragmentShader_desktop;
        else
            code[TextureBrushSrcFragmentShader] = qglslTextureBrushSrcFragmentShader_ES;
        code[TextureBrushSrcWithPatternFragmentShader] = qglslTextureBrushSrcWithPatternFragmentShader;
        code[PatternBrushSrcFragmentShader] = qglslPatternBrushSrcFragmentShader;
        code[LinearGradientBrushSrcFragmentShader] = qglslLinearGradientBrushSrcFragmentShader;
        code[RadialGradientBrushSrcFragmentShader] = qglslRadialGradientBrushSrcFragmentShader;
        code[ConicalGradientBrushSrcFragmentShader] = qglslConicalGradientBrushSrcFragmentShader;
        code[ShockingPinkSrcFragmentShader] = qglslShockingPinkSrcFragmentShader;

        code[NoMaskFragmentShader] = "";
        code[MaskFragmentShader] = qglslMaskFragmentShader;
        code[RgbMaskFragmentShaderPass1] = qglslRgbMaskFragmentShaderPass1;
        code[RgbMaskFragmentShaderPass2] = qglslRgbMaskFragmentShaderPass2;
        code[RgbMaskWithGammaFragmentShader] = ""; //###

        code[NoCompositionModeFragmentShader] = "";
        code[MultiplyCompositionModeFragmentShader] = ""; //###
        code[ScreenCompositionModeFragmentShader] = ""; //###
        code[OverlayCompositionModeFragmentShader] = ""; //###
        code[DarkenCompositionModeFragmentShader] = ""; //###
        code[LightenCompositionModeFragmentShader] = ""; //###
        code[ColorDodgeCompositionModeFragmentShader] = ""; //###
        code[ColorBurnCompositionModeFragmentShader] = ""; //###
        code[HardLightCompositionModeFragmentShader] = ""; //###
        code[SoftLightCompositionModeFragmentShader] = ""; //###
        code[DifferenceCompositionModeFragmentShader] = ""; //###
        code[ExclusionCompositionModeFragmentShader] = ""; //###

#if defined(QT_DEBUG)
        // Check that all the elements have been filled:
        for (int i = 0; i < TotalSnippetCount; ++i) {
            if (qShaderSnippets[i] == 0) {
                qFatal("Shader snippet for %s (#%d) is missing!",
                       snippetNameStr(SnippetName(i)).constData(), i);
            }
        }
#endif
        snippetsPopulated = true;
    }

    QGLShader* fragShader;
    QGLShader* vertexShader;
    QByteArray vertexSource;
    QByteArray fragSource;

    // Compile up the simple shader:
    vertexSource.append(qShaderSnippets[MainVertexShader]);
    vertexSource.append(qShaderSnippets[PositionOnlyVertexShader]);

    fragSource.append(qShaderSnippets[MainFragmentShader]);
    fragSource.append(qShaderSnippets[ShockingPinkSrcFragmentShader]);

    simpleShaderProg = new QGLShaderProgram(context, 0);

    CachedShader simpleShaderCache(fragSource, vertexSource);

    bool inCache = simpleShaderCache.load(simpleShaderProg, context);

    if (!inCache) {
        vertexShader = new QGLShader(QGLShader::Vertex, context, 0);
        shaders.append(vertexShader);
        if (!vertexShader->compileSourceCode(vertexSource))
            qWarning("Vertex shader for simpleShaderProg (MainVertexShader & PositionOnlyVertexShader) failed to compile");

        fragShader = new QGLShader(QGLShader::Fragment, context, 0);
        shaders.append(fragShader);
        if (!fragShader->compileSourceCode(fragSource))
            qWarning("Fragment shader for simpleShaderProg (MainFragmentShader & ShockingPinkSrcFragmentShader) failed to compile");

        simpleShaderProg->addShader(vertexShader);
        simpleShaderProg->addShader(fragShader);

        simpleShaderProg->bindAttributeLocation("vertexCoordsArray", QT_VERTEX_COORDS_ATTR);
        simpleShaderProg->bindAttributeLocation("pmvMatrix1", QT_PMV_MATRIX_1_ATTR);
        simpleShaderProg->bindAttributeLocation("pmvMatrix2", QT_PMV_MATRIX_2_ATTR);
        simpleShaderProg->bindAttributeLocation("pmvMatrix3", QT_PMV_MATRIX_3_ATTR);
    }

    simpleShaderProg->link();

    if (simpleShaderProg->isLinked()) {
        if (!inCache)
            simpleShaderCache.store(simpleShaderProg, context);
    } else {
        qCritical("Errors linking simple shader: %s", qPrintable(simpleShaderProg->log()));
    }

    // Compile the blit shader:
    vertexSource.clear();
    vertexSource.append(qShaderSnippets[MainWithTexCoordsVertexShader]);
    vertexSource.append(qShaderSnippets[UntransformedPositionVertexShader]);

    fragSource.clear();
    fragSource.append(qShaderSnippets[MainFragmentShader]);
    fragSource.append(qShaderSnippets[ImageSrcFragmentShader]);

    blitShaderProg = new QGLShaderProgram(context, 0);

    CachedShader blitShaderCache(fragSource, vertexSource);

    inCache = blitShaderCache.load(blitShaderProg, context);

    if (!inCache) {
        vertexShader = new QGLShader(QGLShader::Vertex, context, 0);
        shaders.append(vertexShader);
        if (!vertexShader->compileSourceCode(vertexSource))
            qWarning("Vertex shader for blitShaderProg (MainWithTexCoordsVertexShader & UntransformedPositionVertexShader) failed to compile");

        fragShader = new QGLShader(QGLShader::Fragment, context, 0);
        shaders.append(fragShader);
        if (!fragShader->compileSourceCode(fragSource))
            qWarning("Fragment shader for blitShaderProg (MainFragmentShader & ImageSrcFragmentShader) failed to compile");

        blitShaderProg->addShader(vertexShader);
        blitShaderProg->addShader(fragShader);

        blitShaderProg->bindAttributeLocation("textureCoordArray", QT_TEXTURE_COORDS_ATTR);
        blitShaderProg->bindAttributeLocation("vertexCoordsArray", QT_VERTEX_COORDS_ATTR);
    }

    blitShaderProg->link();
    if (blitShaderProg->isLinked()) {
        if (!inCache)
            blitShaderCache.store(blitShaderProg, context);
    } else {
        qCritical("Errors linking blit shader: %s", qPrintable(blitShaderProg->log()));
    }

#ifdef QT_GL_SHARED_SHADER_DEBUG
    qDebug(" -> QGLEngineSharedShaders() %p for thread %p.", this, QThread::currentThread());
#endif
}

QGLEngineSharedShaders::~QGLEngineSharedShaders()
{
#ifdef QT_GL_SHARED_SHADER_DEBUG
    qDebug(" -> ~QGLEngineSharedShaders() %p for thread %p.", this, QThread::currentThread());
#endif
    qDeleteAll(shaders);
    shaders.clear();

    qDeleteAll(cachedPrograms);
    cachedPrograms.clear();

    if (blitShaderProg) {
        delete blitShaderProg;
        blitShaderProg = 0;
    }

    if (simpleShaderProg) {
        delete simpleShaderProg;
        simpleShaderProg = 0;
    }
}

#if defined (QT_DEBUG)
QByteArray QGLEngineSharedShaders::snippetNameStr(SnippetName name)
{
    QMetaEnum m = staticMetaObject.enumerator(staticMetaObject.indexOfEnumerator("SnippetName"));
    return QByteArray(m.valueToKey(name));
}
#endif

// The address returned here will only be valid until next time this function is called.
// The program is return bound.
QGLEngineShaderProg *QGLEngineSharedShaders::findProgramInCache(const QGLEngineShaderProg &prog)
{
    for (int i = 0; i < cachedPrograms.size(); ++i) {
        QGLEngineShaderProg *cachedProg = cachedPrograms[i];
        if (*cachedProg == prog) {
            // Move the program to the top of the list as a poor-man's cache algo
            cachedPrograms.move(i, 0);
            cachedProg->program->bind();
            return cachedProg;
        }
    }

    QScopedPointer<QGLEngineShaderProg> newProg;

    do {
        QByteArray fragSource;
        // Insert the custom stage before the srcPixel shader to work around an ATI driver bug
        // where you cannot forward declare a function that takes a sampler as argument.
        if (prog.srcPixelFragShader == CustomImageSrcFragmentShader)
            fragSource.append(prog.customStageSource);
        fragSource.append(qShaderSnippets[prog.mainFragShader]);
        fragSource.append(qShaderSnippets[prog.srcPixelFragShader]);
        if (prog.compositionFragShader)
            fragSource.append(qShaderSnippets[prog.compositionFragShader]);
        if (prog.maskFragShader)
            fragSource.append(qShaderSnippets[prog.maskFragShader]);

        QByteArray vertexSource;
        vertexSource.append(qShaderSnippets[prog.mainVertexShader]);
        vertexSource.append(qShaderSnippets[prog.positionVertexShader]);

        QScopedPointer<QGLShaderProgram> shaderProgram(new QGLShaderProgram);

        CachedShader shaderCache(fragSource, vertexSource);
        bool inCache = shaderCache.load(shaderProgram.data(), QGLContext::currentContext());

        if (!inCache) {

            QScopedPointer<QGLShader> fragShader(new QGLShader(QGLShader::Fragment));
            QByteArray description;
#if defined(QT_DEBUG)
            // Name the shader for easier debugging
            description.append("Fragment shader: main=");
            description.append(snippetNameStr(prog.mainFragShader));
            description.append(", srcPixel=");
            description.append(snippetNameStr(prog.srcPixelFragShader));
            if (prog.compositionFragShader) {
                description.append(", composition=");
                description.append(snippetNameStr(prog.compositionFragShader));
            }
            if (prog.maskFragShader) {
                description.append(", mask=");
                description.append(snippetNameStr(prog.maskFragShader));
            }
            fragShader->setObjectName(QString::fromLatin1(description));
#endif
            if (!fragShader->compileSourceCode(fragSource)) {
                qWarning() << "Warning:" << description << "failed to compile!";
                break;
            }

            QScopedPointer<QGLShader> vertexShader(new QGLShader(QGLShader::Vertex));
#if defined(QT_DEBUG)
            // Name the shader for easier debugging
            description.clear();
            description.append("Vertex shader: main=");
            description.append(snippetNameStr(prog.mainVertexShader));
            description.append(", position=");
            description.append(snippetNameStr(prog.positionVertexShader));
            vertexShader->setObjectName(QString::fromLatin1(description));
#endif
            if (!vertexShader->compileSourceCode(vertexSource)) {
                qWarning() << "Warning:" << description << "failed to compile!";
                break;
            }

            shaders.append(vertexShader.data());
            shaders.append(fragShader.data());
            shaderProgram->addShader(vertexShader.take());
            shaderProgram->addShader(fragShader.take());

            // We have to bind the vertex attribute names before the program is linked:
            shaderProgram->bindAttributeLocation("vertexCoordsArray", QT_VERTEX_COORDS_ATTR);
            if (prog.useTextureCoords)
                shaderProgram->bindAttributeLocation("textureCoordArray", QT_TEXTURE_COORDS_ATTR);
            if (prog.useOpacityAttribute)
                shaderProgram->bindAttributeLocation("opacityArray", QT_OPACITY_ATTR);
            if (prog.usePmvMatrixAttribute) {
                shaderProgram->bindAttributeLocation("pmvMatrix1", QT_PMV_MATRIX_1_ATTR);
                shaderProgram->bindAttributeLocation("pmvMatrix2", QT_PMV_MATRIX_2_ATTR);
                shaderProgram->bindAttributeLocation("pmvMatrix3", QT_PMV_MATRIX_3_ATTR);
            }
        }

        newProg.reset(new QGLEngineShaderProg(prog));
        newProg->program = shaderProgram.take();

        newProg->program->link();
        if (newProg->program->isLinked()) {
            if (!inCache)
                shaderCache.store(newProg->program, QGLContext::currentContext());
        } else {
            QLatin1String none("none");
            QLatin1String br("\n");
            QString error;
            error = QLatin1String("Shader program failed to link,");
#if defined(QT_DEBUG)
            error += QLatin1String("\n  Shaders Used:\n");
            for (int i = 0; i < newProg->program->shaders().count(); ++i) {
                QGLShader *shader = newProg->program->shaders().at(i);
                error += QLatin1String("    ") + shader->objectName() + QLatin1String(": \n")
                         + QLatin1String(shader->sourceCode()) + br;
            }
#endif
            error += QLatin1String("  Error Log:\n")
                     + QLatin1String("    ") + newProg->program->log();
            qWarning() << error;
            break;
        }

        newProg->program->bind();

        if (newProg->maskFragShader != QGLEngineSharedShaders::NoMaskFragmentShader) {
            GLuint location = newProg->program->uniformLocation("maskTexture");
            newProg->program->setUniformValue(location, QT_MASK_TEXTURE_UNIT);
        }

        if (cachedPrograms.count() > 30) {
            // The cache is full, so delete the last 5 programs in the list.
            // These programs will be least used, as a program us bumped to
            // the top of the list when it's used.
            for (int i = 0; i < 5; ++i) {
                delete cachedPrograms.last();
                cachedPrograms.removeLast();
            }
        }

        cachedPrograms.insert(0, newProg.data());
    } while (false);

    return newProg.take();
}

void QGLEngineSharedShaders::cleanupCustomStage(QGLCustomShaderStage* stage)
{
    // Remove any shader programs which has this as the custom shader src:
    for (int i = 0; i < cachedPrograms.size(); ++i) {
        QGLEngineShaderProg *cachedProg = cachedPrograms[i];
        if (cachedProg->customStageSource == stage->source()) {
            delete cachedProg;
            cachedPrograms.removeAt(i);
            i--;
        }
    }
}


QGLEngineShaderManager::QGLEngineShaderManager(QGLContext* context)
    : ctx(context),
      shaderProgNeedsChanging(true),
      complexGeometry(false),
      srcPixelType(Qt::NoBrush),
      opacityMode(NoOpacity),
      maskType(NoMask),
      compositionMode(QPainter::CompositionMode_SourceOver),
      customSrcStage(0),
      currentShaderProg(0)
{
    sharedShaders = QGLEngineSharedShaders::shadersForContext(context);
}

QGLEngineShaderManager::~QGLEngineShaderManager()
{
    //###
    removeCustomStage();
}

GLuint QGLEngineShaderManager::getUniformLocation(Uniform id)
{
    if (!currentShaderProg)
        return 0;

    QVector<uint> &uniformLocations = currentShaderProg->uniformLocations;
    if (uniformLocations.isEmpty())
        uniformLocations.fill(GLuint(-1), NumUniforms);

    static const char *uniformNames[] = {
        "imageTexture",
        "patternColor",
        "globalOpacity",
        "depth",
        "maskTexture",
        "fragmentColor",
        "linearData",
        "angle",
        "halfViewportSize",
        "fmp",
        "fmp2_m_radius2",
        "inverse_2_fmp2_m_radius2",
        "sqrfr",
        "bradius",
        "invertedTextureSize",
        "brushTransform",
        "brushTexture",
        "matrix",
        "translateZ"
    };

    if (uniformLocations.at(id) == GLuint(-1))
        uniformLocations[id] = currentShaderProg->program->uniformLocation(uniformNames[id]);

    return uniformLocations.at(id);
}


void QGLEngineShaderManager::optimiseForBrushTransform(QTransform::TransformationType transformType)
{
    Q_UNUSED(transformType); // Currently ignored
}

void QGLEngineShaderManager::setDirty()
{
    shaderProgNeedsChanging = true;
}

void QGLEngineShaderManager::setSrcPixelType(Qt::BrushStyle style)
{
    Q_ASSERT(style != Qt::NoBrush);
    if (srcPixelType == PixelSrcType(style))
        return;

    srcPixelType = style;
    shaderProgNeedsChanging = true; //###
}

void QGLEngineShaderManager::setSrcPixelType(PixelSrcType type)
{
    if (srcPixelType == type)
        return;

    srcPixelType = type;
    shaderProgNeedsChanging = true; //###
}

void QGLEngineShaderManager::setOpacityMode(OpacityMode mode)
{
    if (opacityMode == mode)
        return;

    opacityMode = mode;
    shaderProgNeedsChanging = true; //###
}

void QGLEngineShaderManager::setMaskType(MaskType type)
{
    if (maskType == type)
        return;

    maskType = type;
    shaderProgNeedsChanging = true; //###
}

void QGLEngineShaderManager::setCompositionMode(QPainter::CompositionMode mode)
{
    if (compositionMode == mode)
        return;

    compositionMode = mode;
    shaderProgNeedsChanging = true; //###
}

void QGLEngineShaderManager::setCustomStage(QGLCustomShaderStage* stage)
{
    if (customSrcStage)
        removeCustomStage();
    customSrcStage = stage;
    shaderProgNeedsChanging = true;
}

void QGLEngineShaderManager::removeCustomStage()
{
    if (customSrcStage)
        customSrcStage->setInactive();
    customSrcStage = 0;
    shaderProgNeedsChanging = true;
}

QGLShaderProgram* QGLEngineShaderManager::currentProgram()
{
    if (currentShaderProg)
        return currentShaderProg->program;
    else
        return sharedShaders->simpleProgram();
}

void QGLEngineShaderManager::useSimpleProgram()
{
    sharedShaders->simpleProgram()->bind();
    QGLContextPrivate* ctx_d = ctx->d_func();
    ctx_d->setVertexAttribArrayEnabled(QT_VERTEX_COORDS_ATTR, true);
    ctx_d->setVertexAttribArrayEnabled(QT_TEXTURE_COORDS_ATTR, false);
    ctx_d->setVertexAttribArrayEnabled(QT_OPACITY_ATTR, false);
    shaderProgNeedsChanging = true;
}

void QGLEngineShaderManager::useBlitProgram()
{
    sharedShaders->blitProgram()->bind();
    QGLContextPrivate* ctx_d = ctx->d_func();
    ctx_d->setVertexAttribArrayEnabled(QT_VERTEX_COORDS_ATTR, true);
    ctx_d->setVertexAttribArrayEnabled(QT_TEXTURE_COORDS_ATTR, true);
    ctx_d->setVertexAttribArrayEnabled(QT_OPACITY_ATTR, false);
    shaderProgNeedsChanging = true;
}

QGLShaderProgram* QGLEngineShaderManager::simpleProgram()
{
    return sharedShaders->simpleProgram();
}

QGLShaderProgram* QGLEngineShaderManager::blitProgram()
{
    return sharedShaders->blitProgram();
}



// Select & use the correct shader program using the current state.
// Returns \c true if program needed changing.
bool QGLEngineShaderManager::useCorrectShaderProg()
{
    if (!shaderProgNeedsChanging)
        return false;

    bool useCustomSrc = customSrcStage != 0;
    if (useCustomSrc && srcPixelType != QGLEngineShaderManager::ImageSrc && srcPixelType != Qt::TexturePattern) {
        useCustomSrc = false;
        qWarning("QGLEngineShaderManager - Ignoring custom shader stage for non image src");
    }

    QGLEngineShaderProg requiredProgram;

    bool texCoords = false;

    // Choose vertex shader shader position function (which typically also sets
    // varyings) and the source pixel (srcPixel) fragment shader function:
    requiredProgram.positionVertexShader = QGLEngineSharedShaders::InvalidSnippetName;
    requiredProgram.srcPixelFragShader = QGLEngineSharedShaders::InvalidSnippetName;
    bool isAffine = brushTransform.isAffine();
    if ( (srcPixelType >= Qt::Dense1Pattern) && (srcPixelType <= Qt::DiagCrossPattern) ) {
        if (isAffine)
            requiredProgram.positionVertexShader = QGLEngineSharedShaders::AffinePositionWithPatternBrushVertexShader;
        else
            requiredProgram.positionVertexShader = QGLEngineSharedShaders::PositionWithPatternBrushVertexShader;

        requiredProgram.srcPixelFragShader = QGLEngineSharedShaders::PatternBrushSrcFragmentShader;
    }
    else switch (srcPixelType) {
        default:
        case Qt::NoBrush:
            qFatal("QGLEngineShaderManager::useCorrectShaderProg() - Qt::NoBrush style is set");
            break;
        case QGLEngineShaderManager::ImageSrc:
            requiredProgram.srcPixelFragShader = QGLEngineSharedShaders::ImageSrcFragmentShader;
            requiredProgram.positionVertexShader = QGLEngineSharedShaders::PositionOnlyVertexShader;
            texCoords = true;
            break;
        case QGLEngineShaderManager::NonPremultipliedImageSrc:
            requiredProgram.srcPixelFragShader = QGLEngineSharedShaders::NonPremultipliedImageSrcFragmentShader;
            requiredProgram.positionVertexShader = QGLEngineSharedShaders::PositionOnlyVertexShader;
            texCoords = true;
            break;
        case QGLEngineShaderManager::PatternSrc:
            requiredProgram.srcPixelFragShader = QGLEngineSharedShaders::ImageSrcWithPatternFragmentShader;
            requiredProgram.positionVertexShader = QGLEngineSharedShaders::PositionOnlyVertexShader;
            texCoords = true;
            break;
        case QGLEngineShaderManager::TextureSrcWithPattern:
            requiredProgram.srcPixelFragShader = QGLEngineSharedShaders::TextureBrushSrcWithPatternFragmentShader;
            requiredProgram.positionVertexShader = isAffine ? QGLEngineSharedShaders::AffinePositionWithTextureBrushVertexShader
                                                : QGLEngineSharedShaders::PositionWithTextureBrushVertexShader;
            break;
        case Qt::SolidPattern:
            requiredProgram.srcPixelFragShader = QGLEngineSharedShaders::SolidBrushSrcFragmentShader;
            requiredProgram.positionVertexShader = QGLEngineSharedShaders::PositionOnlyVertexShader;
            break;
        case Qt::LinearGradientPattern:
            requiredProgram.srcPixelFragShader = QGLEngineSharedShaders::LinearGradientBrushSrcFragmentShader;
            requiredProgram.positionVertexShader = isAffine ? QGLEngineSharedShaders::AffinePositionWithLinearGradientBrushVertexShader
                                                : QGLEngineSharedShaders::PositionWithLinearGradientBrushVertexShader;
            break;
        case Qt::ConicalGradientPattern:
            requiredProgram.srcPixelFragShader = QGLEngineSharedShaders::ConicalGradientBrushSrcFragmentShader;
            requiredProgram.positionVertexShader = isAffine ? QGLEngineSharedShaders::AffinePositionWithConicalGradientBrushVertexShader
                                                : QGLEngineSharedShaders::PositionWithConicalGradientBrushVertexShader;
            break;
        case Qt::RadialGradientPattern:
            requiredProgram.srcPixelFragShader = QGLEngineSharedShaders::RadialGradientBrushSrcFragmentShader;
            requiredProgram.positionVertexShader = isAffine ? QGLEngineSharedShaders::AffinePositionWithRadialGradientBrushVertexShader
                                                : QGLEngineSharedShaders::PositionWithRadialGradientBrushVertexShader;
            break;
        case Qt::TexturePattern:
            requiredProgram.srcPixelFragShader = QGLEngineSharedShaders::TextureBrushSrcFragmentShader;
            requiredProgram.positionVertexShader = isAffine ? QGLEngineSharedShaders::AffinePositionWithTextureBrushVertexShader
                                                : QGLEngineSharedShaders::PositionWithTextureBrushVertexShader;
            break;
    };

    if (useCustomSrc) {
        requiredProgram.srcPixelFragShader = QGLEngineSharedShaders::CustomImageSrcFragmentShader;
        requiredProgram.customStageSource = customSrcStage->source();
    }

    const bool hasCompose = compositionMode > QPainter::CompositionMode_Plus;
    const bool hasMask = maskType != QGLEngineShaderManager::NoMask;

    // Choose fragment shader main function:
    if (opacityMode == AttributeOpacity) {
        Q_ASSERT(!hasCompose && !hasMask);
        requiredProgram.mainFragShader = QGLEngineSharedShaders::MainFragmentShader_ImageArrays;
    } else {
        bool useGlobalOpacity = (opacityMode == UniformOpacity);
        if (hasCompose && hasMask && useGlobalOpacity)
            requiredProgram.mainFragShader = QGLEngineSharedShaders::MainFragmentShader_CMO;
        if (hasCompose && hasMask && !useGlobalOpacity)
            requiredProgram.mainFragShader = QGLEngineSharedShaders::MainFragmentShader_CM;
        if (!hasCompose && hasMask && useGlobalOpacity)
            requiredProgram.mainFragShader = QGLEngineSharedShaders::MainFragmentShader_MO;
        if (!hasCompose && hasMask && !useGlobalOpacity)
            requiredProgram.mainFragShader = QGLEngineSharedShaders::MainFragmentShader_M;
        if (hasCompose && !hasMask && useGlobalOpacity)
            requiredProgram.mainFragShader = QGLEngineSharedShaders::MainFragmentShader_CO;
        if (hasCompose && !hasMask && !useGlobalOpacity)
            requiredProgram.mainFragShader = QGLEngineSharedShaders::MainFragmentShader_C;
        if (!hasCompose && !hasMask && useGlobalOpacity)
            requiredProgram.mainFragShader = QGLEngineSharedShaders::MainFragmentShader_O;
        if (!hasCompose && !hasMask && !useGlobalOpacity)
            requiredProgram.mainFragShader = QGLEngineSharedShaders::MainFragmentShader;
    }

    if (hasMask) {
        if (maskType == PixelMask) {
            requiredProgram.maskFragShader = QGLEngineSharedShaders::MaskFragmentShader;
            texCoords = true;
        } else if (maskType == SubPixelMaskPass1) {
            requiredProgram.maskFragShader = QGLEngineSharedShaders::RgbMaskFragmentShaderPass1;
            texCoords = true;
        } else if (maskType == SubPixelMaskPass2) {
            requiredProgram.maskFragShader = QGLEngineSharedShaders::RgbMaskFragmentShaderPass2;
            texCoords = true;
        } else if (maskType == SubPixelWithGammaMask) {
            requiredProgram.maskFragShader = QGLEngineSharedShaders::RgbMaskWithGammaFragmentShader;
            texCoords = true;
        } else {
            qCritical("QGLEngineShaderManager::useCorrectShaderProg() - Unknown mask type");
        }
    } else {
        requiredProgram.maskFragShader = QGLEngineSharedShaders::NoMaskFragmentShader;
    }

    if (hasCompose) {
        switch (compositionMode) {
            case QPainter::CompositionMode_Multiply:
                requiredProgram.compositionFragShader = QGLEngineSharedShaders::MultiplyCompositionModeFragmentShader;
                break;
            case QPainter::CompositionMode_Screen:
                requiredProgram.compositionFragShader = QGLEngineSharedShaders::ScreenCompositionModeFragmentShader;
                break;
            case QPainter::CompositionMode_Overlay:
                requiredProgram.compositionFragShader = QGLEngineSharedShaders::OverlayCompositionModeFragmentShader;
                break;
            case QPainter::CompositionMode_Darken:
                requiredProgram.compositionFragShader = QGLEngineSharedShaders::DarkenCompositionModeFragmentShader;
                break;
            case QPainter::CompositionMode_Lighten:
                requiredProgram.compositionFragShader = QGLEngineSharedShaders::LightenCompositionModeFragmentShader;
                break;
            case QPainter::CompositionMode_ColorDodge:
                requiredProgram.compositionFragShader = QGLEngineSharedShaders::ColorDodgeCompositionModeFragmentShader;
                break;
            case QPainter::CompositionMode_ColorBurn:
                requiredProgram.compositionFragShader = QGLEngineSharedShaders::ColorBurnCompositionModeFragmentShader;
                break;
            case QPainter::CompositionMode_HardLight:
                requiredProgram.compositionFragShader = QGLEngineSharedShaders::HardLightCompositionModeFragmentShader;
                break;
            case QPainter::CompositionMode_SoftLight:
                requiredProgram.compositionFragShader = QGLEngineSharedShaders::SoftLightCompositionModeFragmentShader;
                break;
            case QPainter::CompositionMode_Difference:
                requiredProgram.compositionFragShader = QGLEngineSharedShaders::DifferenceCompositionModeFragmentShader;
                break;
            case QPainter::CompositionMode_Exclusion:
                requiredProgram.compositionFragShader = QGLEngineSharedShaders::ExclusionCompositionModeFragmentShader;
                break;
            default:
                qWarning("QGLEngineShaderManager::useCorrectShaderProg() - Unsupported composition mode");
        }
    } else {
        requiredProgram.compositionFragShader = QGLEngineSharedShaders::NoCompositionModeFragmentShader;
    }

    // Choose vertex shader main function
    if (opacityMode == AttributeOpacity) {
        Q_ASSERT(texCoords);
        requiredProgram.mainVertexShader = QGLEngineSharedShaders::MainWithTexCoordsAndOpacityVertexShader;
    } else if (texCoords) {
        requiredProgram.mainVertexShader = QGLEngineSharedShaders::MainWithTexCoordsVertexShader;
    } else {
        requiredProgram.mainVertexShader = QGLEngineSharedShaders::MainVertexShader;
    }
    requiredProgram.useTextureCoords = texCoords;
    requiredProgram.useOpacityAttribute = (opacityMode == AttributeOpacity);
    if (complexGeometry && srcPixelType == Qt::SolidPattern) {
        requiredProgram.positionVertexShader = QGLEngineSharedShaders::ComplexGeometryPositionOnlyVertexShader;
        requiredProgram.usePmvMatrixAttribute = false;
    } else {
        requiredProgram.usePmvMatrixAttribute = true;

        // Force complexGeometry off, since we currently don't support that mode for
        // non-solid brushes
        complexGeometry = false;
    }

    // At this point, requiredProgram is fully populated so try to find the program in the cache
    currentShaderProg = sharedShaders->findProgramInCache(requiredProgram);

    if (currentShaderProg && useCustomSrc) {
        customSrcStage->setUniforms(currentShaderProg->program);
    }

    // Make sure all the vertex attribute arrays the program uses are enabled (and the ones it
    // doesn't use are disabled)
    QGLContextPrivate* ctx_d = ctx->d_func();
    ctx_d->setVertexAttribArrayEnabled(QT_VERTEX_COORDS_ATTR, true);
    ctx_d->setVertexAttribArrayEnabled(QT_TEXTURE_COORDS_ATTR, currentShaderProg && currentShaderProg->useTextureCoords);
    ctx_d->setVertexAttribArrayEnabled(QT_OPACITY_ATTR, currentShaderProg && currentShaderProg->useOpacityAttribute);

    shaderProgNeedsChanging = false;
    return true;
}

QT_END_NAMESPACE
