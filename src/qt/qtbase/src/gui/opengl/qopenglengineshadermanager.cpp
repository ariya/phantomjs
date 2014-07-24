/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qopenglengineshadermanager_p.h"
#include "qopenglengineshadersource_p.h"
#include "qopenglpaintengine_p.h"
#include "qopenglshadercache_p.h"

#include <QtGui/private/qopenglcontext_p.h>
#include <QtCore/qthreadstorage.h>

#if defined(QT_DEBUG)
#include <QMetaEnum>
#endif

// #define QT_GL_SHARED_SHADER_DEBUG

QT_BEGIN_NAMESPACE

class QOpenGLEngineSharedShadersResource : public QOpenGLSharedResource
{
public:
    QOpenGLEngineSharedShadersResource(QOpenGLContext *ctx)
        : QOpenGLSharedResource(ctx->shareGroup())
        , m_shaders(new QOpenGLEngineSharedShaders(ctx))
    {
    }

    ~QOpenGLEngineSharedShadersResource()
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

    QOpenGLEngineSharedShaders *shaders() const { return m_shaders; }

private:
    QOpenGLEngineSharedShaders *m_shaders;
};

class QOpenGLShaderStorage
{
public:
    QOpenGLEngineSharedShaders *shadersForThread(QOpenGLContext *context) {
        QOpenGLMultiGroupSharedResource *&shaders = m_storage.localData();
        if (!shaders)
            shaders = new QOpenGLMultiGroupSharedResource;
        QOpenGLEngineSharedShadersResource *resource =
            shaders->value<QOpenGLEngineSharedShadersResource>(context);
        return resource ? resource->shaders() : 0;
    }

private:
    QThreadStorage<QOpenGLMultiGroupSharedResource *> m_storage;
};

Q_GLOBAL_STATIC(QOpenGLShaderStorage, qt_shader_storage);

QOpenGLEngineSharedShaders *QOpenGLEngineSharedShaders::shadersForContext(QOpenGLContext *context)
{
    return qt_shader_storage()->shadersForThread(context);
}

const char* QOpenGLEngineSharedShaders::qShaderSnippets[] = {
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0
};

QOpenGLEngineSharedShaders::QOpenGLEngineSharedShaders(QOpenGLContext* context)
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

        code[MainVertexShader] = qopenglslMainVertexShader;
        code[MainWithTexCoordsVertexShader] = qopenglslMainWithTexCoordsVertexShader;
        code[MainWithTexCoordsAndOpacityVertexShader] = qopenglslMainWithTexCoordsAndOpacityVertexShader;

        code[UntransformedPositionVertexShader] = qopenglslUntransformedPositionVertexShader;
        code[PositionOnlyVertexShader] = qopenglslPositionOnlyVertexShader;
        code[ComplexGeometryPositionOnlyVertexShader] = qopenglslComplexGeometryPositionOnlyVertexShader;
        code[PositionWithPatternBrushVertexShader] = qopenglslPositionWithPatternBrushVertexShader;
        code[PositionWithLinearGradientBrushVertexShader] = qopenglslPositionWithLinearGradientBrushVertexShader;
        code[PositionWithConicalGradientBrushVertexShader] = qopenglslPositionWithConicalGradientBrushVertexShader;
        code[PositionWithRadialGradientBrushVertexShader] = qopenglslPositionWithRadialGradientBrushVertexShader;
        code[PositionWithTextureBrushVertexShader] = qopenglslPositionWithTextureBrushVertexShader;
        code[AffinePositionWithPatternBrushVertexShader] = qopenglslAffinePositionWithPatternBrushVertexShader;
        code[AffinePositionWithLinearGradientBrushVertexShader] = qopenglslAffinePositionWithLinearGradientBrushVertexShader;
        code[AffinePositionWithConicalGradientBrushVertexShader] = qopenglslAffinePositionWithConicalGradientBrushVertexShader;
        code[AffinePositionWithRadialGradientBrushVertexShader] = qopenglslAffinePositionWithRadialGradientBrushVertexShader;
        code[AffinePositionWithTextureBrushVertexShader] = qopenglslAffinePositionWithTextureBrushVertexShader;

        code[MainFragmentShader_CMO] = qopenglslMainFragmentShader_CMO;
        code[MainFragmentShader_CM] = qopenglslMainFragmentShader_CM;
        code[MainFragmentShader_MO] = qopenglslMainFragmentShader_MO;
        code[MainFragmentShader_M] = qopenglslMainFragmentShader_M;
        code[MainFragmentShader_CO] = qopenglslMainFragmentShader_CO;
        code[MainFragmentShader_C] = qopenglslMainFragmentShader_C;
        code[MainFragmentShader_O] = qopenglslMainFragmentShader_O;
        code[MainFragmentShader] = qopenglslMainFragmentShader;
        code[MainFragmentShader_ImageArrays] = qopenglslMainFragmentShader_ImageArrays;

        code[ImageSrcFragmentShader] = qopenglslImageSrcFragmentShader;
        code[ImageSrcWithPatternFragmentShader] = qopenglslImageSrcWithPatternFragmentShader;
        code[NonPremultipliedImageSrcFragmentShader] = qopenglslNonPremultipliedImageSrcFragmentShader;
        code[CustomImageSrcFragmentShader] = qopenglslCustomSrcFragmentShader; // Calls "customShader", which must be appended
        code[SolidBrushSrcFragmentShader] = qopenglslSolidBrushSrcFragmentShader;
        if (context->isOpenGLES())
            code[TextureBrushSrcFragmentShader] = qopenglslTextureBrushSrcFragmentShader_ES;
        else
            code[TextureBrushSrcFragmentShader] = qopenglslTextureBrushSrcFragmentShader_desktop;
        code[TextureBrushSrcWithPatternFragmentShader] = qopenglslTextureBrushSrcWithPatternFragmentShader;
        code[PatternBrushSrcFragmentShader] = qopenglslPatternBrushSrcFragmentShader;
        code[LinearGradientBrushSrcFragmentShader] = qopenglslLinearGradientBrushSrcFragmentShader;
        code[RadialGradientBrushSrcFragmentShader] = qopenglslRadialGradientBrushSrcFragmentShader;
        code[ConicalGradientBrushSrcFragmentShader] = qopenglslConicalGradientBrushSrcFragmentShader;
        code[ShockingPinkSrcFragmentShader] = qopenglslShockingPinkSrcFragmentShader;

        code[NoMaskFragmentShader] = "";
        code[MaskFragmentShader] = qopenglslMaskFragmentShader;
        code[RgbMaskFragmentShaderPass1] = qopenglslRgbMaskFragmentShaderPass1;
        code[RgbMaskFragmentShaderPass2] = qopenglslRgbMaskFragmentShaderPass2;
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

    QOpenGLShader* fragShader;
    QOpenGLShader* vertexShader;
    QByteArray vertexSource;
    QByteArray fragSource;

    // Compile up the simple shader:
    vertexSource.append(qShaderSnippets[MainVertexShader]);
    vertexSource.append(qShaderSnippets[PositionOnlyVertexShader]);

    fragSource.append(qShaderSnippets[MainFragmentShader]);
    fragSource.append(qShaderSnippets[ShockingPinkSrcFragmentShader]);

    simpleShaderProg = new QOpenGLShaderProgram;

    CachedShader simpleShaderCache(fragSource, vertexSource);

    bool inCache = simpleShaderCache.load(simpleShaderProg, context);

    if (!inCache) {
        vertexShader = new QOpenGLShader(QOpenGLShader::Vertex);
        shaders.append(vertexShader);
        if (!vertexShader->compileSourceCode(vertexSource))
            qWarning("Vertex shader for simpleShaderProg (MainVertexShader & PositionOnlyVertexShader) failed to compile");

        fragShader = new QOpenGLShader(QOpenGLShader::Fragment);
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

    blitShaderProg = new QOpenGLShaderProgram;

    CachedShader blitShaderCache(fragSource, vertexSource);

    inCache = blitShaderCache.load(blitShaderProg, context);

    if (!inCache) {
        vertexShader = new QOpenGLShader(QOpenGLShader::Vertex);
        shaders.append(vertexShader);
        if (!vertexShader->compileSourceCode(vertexSource))
            qWarning("Vertex shader for blitShaderProg (MainWithTexCoordsVertexShader & UntransformedPositionVertexShader) failed to compile");

        fragShader = new QOpenGLShader(QOpenGLShader::Fragment);
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
    qDebug(" -> QOpenGLEngineSharedShaders() %p for thread %p.", this, QThread::currentThread());
#endif
}

QOpenGLEngineSharedShaders::~QOpenGLEngineSharedShaders()
{
#ifdef QT_GL_SHARED_SHADER_DEBUG
    qDebug(" -> ~QOpenGLEngineSharedShaders() %p for thread %p.", this, QThread::currentThread());
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
QByteArray QOpenGLEngineSharedShaders::snippetNameStr(SnippetName name)
{
    QMetaEnum m = staticMetaObject.enumerator(staticMetaObject.indexOfEnumerator("SnippetName"));
    return QByteArray(m.valueToKey(name));
}
#endif

// The address returned here will only be valid until next time this function is called.
// The program is return bound.
QOpenGLEngineShaderProg *QOpenGLEngineSharedShaders::findProgramInCache(const QOpenGLEngineShaderProg &prog)
{
    for (int i = 0; i < cachedPrograms.size(); ++i) {
        QOpenGLEngineShaderProg *cachedProg = cachedPrograms[i];
        if (*cachedProg == prog) {
            // Move the program to the top of the list as a poor-man's cache algo
            cachedPrograms.move(i, 0);
            cachedProg->program->bind();
            return cachedProg;
        }
    }

    QScopedPointer<QOpenGLEngineShaderProg> newProg;

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

        QScopedPointer<QOpenGLShaderProgram> shaderProgram(new QOpenGLShaderProgram);

        CachedShader shaderCache(fragSource, vertexSource);
        bool inCache = shaderCache.load(shaderProgram.data(), QOpenGLContext::currentContext());

        if (!inCache) {

            QScopedPointer<QOpenGLShader> fragShader(new QOpenGLShader(QOpenGLShader::Fragment));
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

            QScopedPointer<QOpenGLShader> vertexShader(new QOpenGLShader(QOpenGLShader::Vertex));
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

        newProg.reset(new QOpenGLEngineShaderProg(prog));
        newProg->program = shaderProgram.take();

        newProg->program->link();
        if (newProg->program->isLinked()) {
            if (!inCache)
                shaderCache.store(newProg->program, QOpenGLContext::currentContext());
        } else {
            QLatin1String none("none");
            QLatin1String br("\n");
            QString error;
            error = QLatin1String("Shader program failed to link,");
#if defined(QT_DEBUG)
            error += QLatin1String("\n  Shaders Used:\n");
            for (int i = 0; i < newProg->program->shaders().count(); ++i) {
                QOpenGLShader *shader = newProg->program->shaders().at(i);
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

        if (newProg->maskFragShader != QOpenGLEngineSharedShaders::NoMaskFragmentShader) {
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

void QOpenGLEngineSharedShaders::cleanupCustomStage(QOpenGLCustomShaderStage* stage)
{
    // Remove any shader programs which has this as the custom shader src:
    for (int i = 0; i < cachedPrograms.size(); ++i) {
        QOpenGLEngineShaderProg *cachedProg = cachedPrograms[i];
        if (cachedProg->customStageSource == stage->source()) {
            delete cachedProg;
            cachedPrograms.removeAt(i);
            i--;
        }
    }
}


QOpenGLEngineShaderManager::QOpenGLEngineShaderManager(QOpenGLContext* context)
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
    sharedShaders = QOpenGLEngineSharedShaders::shadersForContext(context);
}

QOpenGLEngineShaderManager::~QOpenGLEngineShaderManager()
{
    //###
    removeCustomStage();
}

GLuint QOpenGLEngineShaderManager::getUniformLocation(Uniform id)
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
        "matrix"
    };

    if (uniformLocations.at(id) == GLuint(-1))
        uniformLocations[id] = currentShaderProg->program->uniformLocation(uniformNames[id]);

    return uniformLocations.at(id);
}


void QOpenGLEngineShaderManager::optimiseForBrushTransform(QTransform::TransformationType transformType)
{
    Q_UNUSED(transformType); // Currently ignored
}

void QOpenGLEngineShaderManager::setDirty()
{
    shaderProgNeedsChanging = true;
}

void QOpenGLEngineShaderManager::setSrcPixelType(Qt::BrushStyle style)
{
    Q_ASSERT(style != Qt::NoBrush);
    if (srcPixelType == PixelSrcType(style))
        return;

    srcPixelType = style;
    shaderProgNeedsChanging = true; //###
}

void QOpenGLEngineShaderManager::setSrcPixelType(PixelSrcType type)
{
    if (srcPixelType == type)
        return;

    srcPixelType = type;
    shaderProgNeedsChanging = true; //###
}

void QOpenGLEngineShaderManager::setOpacityMode(OpacityMode mode)
{
    if (opacityMode == mode)
        return;

    opacityMode = mode;
    shaderProgNeedsChanging = true; //###
}

void QOpenGLEngineShaderManager::setMaskType(MaskType type)
{
    if (maskType == type)
        return;

    maskType = type;
    shaderProgNeedsChanging = true; //###
}

void QOpenGLEngineShaderManager::setCompositionMode(QPainter::CompositionMode mode)
{
    if (compositionMode == mode)
        return;

    compositionMode = mode;
    shaderProgNeedsChanging = true; //###
}

void QOpenGLEngineShaderManager::setCustomStage(QOpenGLCustomShaderStage* stage)
{
    if (customSrcStage)
        removeCustomStage();
    customSrcStage = stage;
    shaderProgNeedsChanging = true;
}

void QOpenGLEngineShaderManager::removeCustomStage()
{
    if (customSrcStage)
        customSrcStage->setInactive();
    customSrcStage = 0;
    shaderProgNeedsChanging = true;
}

QOpenGLShaderProgram* QOpenGLEngineShaderManager::currentProgram()
{
    if (currentShaderProg)
        return currentShaderProg->program;
    else
        return sharedShaders->simpleProgram();
}

void QOpenGLEngineShaderManager::useSimpleProgram()
{
    sharedShaders->simpleProgram()->bind();
    QOpenGLContextPrivate* ctx_d = ctx->d_func();
    Q_UNUSED(ctx_d);

    QOpenGL2PaintEngineEx *active_engine = static_cast<QOpenGL2PaintEngineEx *>(ctx_d->active_engine);

    active_engine->d_func()->setVertexAttribArrayEnabled(QT_VERTEX_COORDS_ATTR, true);
    active_engine->d_func()->setVertexAttribArrayEnabled(QT_TEXTURE_COORDS_ATTR, false);
    active_engine->d_func()->setVertexAttribArrayEnabled(QT_OPACITY_ATTR, false);

    shaderProgNeedsChanging = true;
}

void QOpenGLEngineShaderManager::useBlitProgram()
{
    sharedShaders->blitProgram()->bind();
    QOpenGLContextPrivate* ctx_d = ctx->d_func();
    QOpenGL2PaintEngineEx *active_engine = static_cast<QOpenGL2PaintEngineEx *>(ctx_d->active_engine);
    active_engine->d_func()->setVertexAttribArrayEnabled(QT_VERTEX_COORDS_ATTR, true);
    active_engine->d_func()->setVertexAttribArrayEnabled(QT_TEXTURE_COORDS_ATTR, true);
    active_engine->d_func()->setVertexAttribArrayEnabled(QT_OPACITY_ATTR, false);
    shaderProgNeedsChanging = true;
}

QOpenGLShaderProgram* QOpenGLEngineShaderManager::simpleProgram()
{
    return sharedShaders->simpleProgram();
}

QOpenGLShaderProgram* QOpenGLEngineShaderManager::blitProgram()
{
    return sharedShaders->blitProgram();
}



// Select & use the correct shader program using the current state.
// Returns \c true if program needed changing.
bool QOpenGLEngineShaderManager::useCorrectShaderProg()
{
    if (!shaderProgNeedsChanging)
        return false;

    bool useCustomSrc = customSrcStage != 0;
    if (useCustomSrc && srcPixelType != QOpenGLEngineShaderManager::ImageSrc && srcPixelType != Qt::TexturePattern) {
        useCustomSrc = false;
        qWarning("QOpenGLEngineShaderManager - Ignoring custom shader stage for non image src");
    }

    QOpenGLEngineShaderProg requiredProgram;

    bool texCoords = false;

    // Choose vertex shader shader position function (which typically also sets
    // varyings) and the source pixel (srcPixel) fragment shader function:
    requiredProgram.positionVertexShader = QOpenGLEngineSharedShaders::InvalidSnippetName;
    requiredProgram.srcPixelFragShader = QOpenGLEngineSharedShaders::InvalidSnippetName;
    bool isAffine = brushTransform.isAffine();
    if ( (srcPixelType >= Qt::Dense1Pattern) && (srcPixelType <= Qt::DiagCrossPattern) ) {
        if (isAffine)
            requiredProgram.positionVertexShader = QOpenGLEngineSharedShaders::AffinePositionWithPatternBrushVertexShader;
        else
            requiredProgram.positionVertexShader = QOpenGLEngineSharedShaders::PositionWithPatternBrushVertexShader;

        requiredProgram.srcPixelFragShader = QOpenGLEngineSharedShaders::PatternBrushSrcFragmentShader;
    }
    else switch (srcPixelType) {
        default:
        case Qt::NoBrush:
            qFatal("QOpenGLEngineShaderManager::useCorrectShaderProg() - Qt::NoBrush style is set");
            break;
        case QOpenGLEngineShaderManager::ImageSrc:
            requiredProgram.srcPixelFragShader = QOpenGLEngineSharedShaders::ImageSrcFragmentShader;
            requiredProgram.positionVertexShader = QOpenGLEngineSharedShaders::PositionOnlyVertexShader;
            texCoords = true;
            break;
        case QOpenGLEngineShaderManager::NonPremultipliedImageSrc:
            requiredProgram.srcPixelFragShader = QOpenGLEngineSharedShaders::NonPremultipliedImageSrcFragmentShader;
            requiredProgram.positionVertexShader = QOpenGLEngineSharedShaders::PositionOnlyVertexShader;
            texCoords = true;
            break;
        case QOpenGLEngineShaderManager::PatternSrc:
            requiredProgram.srcPixelFragShader = QOpenGLEngineSharedShaders::ImageSrcWithPatternFragmentShader;
            requiredProgram.positionVertexShader = QOpenGLEngineSharedShaders::PositionOnlyVertexShader;
            texCoords = true;
            break;
        case QOpenGLEngineShaderManager::TextureSrcWithPattern:
            requiredProgram.srcPixelFragShader = QOpenGLEngineSharedShaders::TextureBrushSrcWithPatternFragmentShader;
            requiredProgram.positionVertexShader = isAffine ? QOpenGLEngineSharedShaders::AffinePositionWithTextureBrushVertexShader
                                                : QOpenGLEngineSharedShaders::PositionWithTextureBrushVertexShader;
            break;
        case Qt::SolidPattern:
            requiredProgram.srcPixelFragShader = QOpenGLEngineSharedShaders::SolidBrushSrcFragmentShader;
            requiredProgram.positionVertexShader = QOpenGLEngineSharedShaders::PositionOnlyVertexShader;
            break;
        case Qt::LinearGradientPattern:
            requiredProgram.srcPixelFragShader = QOpenGLEngineSharedShaders::LinearGradientBrushSrcFragmentShader;
            requiredProgram.positionVertexShader = isAffine ? QOpenGLEngineSharedShaders::AffinePositionWithLinearGradientBrushVertexShader
                                                : QOpenGLEngineSharedShaders::PositionWithLinearGradientBrushVertexShader;
            break;
        case Qt::ConicalGradientPattern:
            requiredProgram.srcPixelFragShader = QOpenGLEngineSharedShaders::ConicalGradientBrushSrcFragmentShader;
            requiredProgram.positionVertexShader = isAffine ? QOpenGLEngineSharedShaders::AffinePositionWithConicalGradientBrushVertexShader
                                                : QOpenGLEngineSharedShaders::PositionWithConicalGradientBrushVertexShader;
            break;
        case Qt::RadialGradientPattern:
            requiredProgram.srcPixelFragShader = QOpenGLEngineSharedShaders::RadialGradientBrushSrcFragmentShader;
            requiredProgram.positionVertexShader = isAffine ? QOpenGLEngineSharedShaders::AffinePositionWithRadialGradientBrushVertexShader
                                                : QOpenGLEngineSharedShaders::PositionWithRadialGradientBrushVertexShader;
            break;
        case Qt::TexturePattern:
            requiredProgram.srcPixelFragShader = QOpenGLEngineSharedShaders::TextureBrushSrcFragmentShader;
            requiredProgram.positionVertexShader = isAffine ? QOpenGLEngineSharedShaders::AffinePositionWithTextureBrushVertexShader
                                                : QOpenGLEngineSharedShaders::PositionWithTextureBrushVertexShader;
            break;
    };

    if (useCustomSrc) {
        requiredProgram.srcPixelFragShader = QOpenGLEngineSharedShaders::CustomImageSrcFragmentShader;
        requiredProgram.customStageSource = customSrcStage->source();
    }

    const bool hasCompose = compositionMode > QPainter::CompositionMode_Plus;
    const bool hasMask = maskType != QOpenGLEngineShaderManager::NoMask;

    // Choose fragment shader main function:
    if (opacityMode == AttributeOpacity) {
        Q_ASSERT(!hasCompose && !hasMask);
        requiredProgram.mainFragShader = QOpenGLEngineSharedShaders::MainFragmentShader_ImageArrays;
    } else {
        bool useGlobalOpacity = (opacityMode == UniformOpacity);
        if (hasCompose && hasMask && useGlobalOpacity)
            requiredProgram.mainFragShader = QOpenGLEngineSharedShaders::MainFragmentShader_CMO;
        if (hasCompose && hasMask && !useGlobalOpacity)
            requiredProgram.mainFragShader = QOpenGLEngineSharedShaders::MainFragmentShader_CM;
        if (!hasCompose && hasMask && useGlobalOpacity)
            requiredProgram.mainFragShader = QOpenGLEngineSharedShaders::MainFragmentShader_MO;
        if (!hasCompose && hasMask && !useGlobalOpacity)
            requiredProgram.mainFragShader = QOpenGLEngineSharedShaders::MainFragmentShader_M;
        if (hasCompose && !hasMask && useGlobalOpacity)
            requiredProgram.mainFragShader = QOpenGLEngineSharedShaders::MainFragmentShader_CO;
        if (hasCompose && !hasMask && !useGlobalOpacity)
            requiredProgram.mainFragShader = QOpenGLEngineSharedShaders::MainFragmentShader_C;
        if (!hasCompose && !hasMask && useGlobalOpacity)
            requiredProgram.mainFragShader = QOpenGLEngineSharedShaders::MainFragmentShader_O;
        if (!hasCompose && !hasMask && !useGlobalOpacity)
            requiredProgram.mainFragShader = QOpenGLEngineSharedShaders::MainFragmentShader;
    }

    if (hasMask) {
        if (maskType == PixelMask) {
            requiredProgram.maskFragShader = QOpenGLEngineSharedShaders::MaskFragmentShader;
            texCoords = true;
        } else if (maskType == SubPixelMaskPass1) {
            requiredProgram.maskFragShader = QOpenGLEngineSharedShaders::RgbMaskFragmentShaderPass1;
            texCoords = true;
        } else if (maskType == SubPixelMaskPass2) {
            requiredProgram.maskFragShader = QOpenGLEngineSharedShaders::RgbMaskFragmentShaderPass2;
            texCoords = true;
        } else if (maskType == SubPixelWithGammaMask) {
            requiredProgram.maskFragShader = QOpenGLEngineSharedShaders::RgbMaskWithGammaFragmentShader;
            texCoords = true;
        } else {
            qCritical("QOpenGLEngineShaderManager::useCorrectShaderProg() - Unknown mask type");
        }
    } else {
        requiredProgram.maskFragShader = QOpenGLEngineSharedShaders::NoMaskFragmentShader;
    }

    if (hasCompose) {
        switch (compositionMode) {
            case QPainter::CompositionMode_Multiply:
                requiredProgram.compositionFragShader = QOpenGLEngineSharedShaders::MultiplyCompositionModeFragmentShader;
                break;
            case QPainter::CompositionMode_Screen:
                requiredProgram.compositionFragShader = QOpenGLEngineSharedShaders::ScreenCompositionModeFragmentShader;
                break;
            case QPainter::CompositionMode_Overlay:
                requiredProgram.compositionFragShader = QOpenGLEngineSharedShaders::OverlayCompositionModeFragmentShader;
                break;
            case QPainter::CompositionMode_Darken:
                requiredProgram.compositionFragShader = QOpenGLEngineSharedShaders::DarkenCompositionModeFragmentShader;
                break;
            case QPainter::CompositionMode_Lighten:
                requiredProgram.compositionFragShader = QOpenGLEngineSharedShaders::LightenCompositionModeFragmentShader;
                break;
            case QPainter::CompositionMode_ColorDodge:
                requiredProgram.compositionFragShader = QOpenGLEngineSharedShaders::ColorDodgeCompositionModeFragmentShader;
                break;
            case QPainter::CompositionMode_ColorBurn:
                requiredProgram.compositionFragShader = QOpenGLEngineSharedShaders::ColorBurnCompositionModeFragmentShader;
                break;
            case QPainter::CompositionMode_HardLight:
                requiredProgram.compositionFragShader = QOpenGLEngineSharedShaders::HardLightCompositionModeFragmentShader;
                break;
            case QPainter::CompositionMode_SoftLight:
                requiredProgram.compositionFragShader = QOpenGLEngineSharedShaders::SoftLightCompositionModeFragmentShader;
                break;
            case QPainter::CompositionMode_Difference:
                requiredProgram.compositionFragShader = QOpenGLEngineSharedShaders::DifferenceCompositionModeFragmentShader;
                break;
            case QPainter::CompositionMode_Exclusion:
                requiredProgram.compositionFragShader = QOpenGLEngineSharedShaders::ExclusionCompositionModeFragmentShader;
                break;
            default:
                qWarning("QOpenGLEngineShaderManager::useCorrectShaderProg() - Unsupported composition mode");
        }
    } else {
        requiredProgram.compositionFragShader = QOpenGLEngineSharedShaders::NoCompositionModeFragmentShader;
    }

    // Choose vertex shader main function
    if (opacityMode == AttributeOpacity) {
        Q_ASSERT(texCoords);
        requiredProgram.mainVertexShader = QOpenGLEngineSharedShaders::MainWithTexCoordsAndOpacityVertexShader;
    } else if (texCoords) {
        requiredProgram.mainVertexShader = QOpenGLEngineSharedShaders::MainWithTexCoordsVertexShader;
    } else {
        requiredProgram.mainVertexShader = QOpenGLEngineSharedShaders::MainVertexShader;
    }
    requiredProgram.useTextureCoords = texCoords;
    requiredProgram.useOpacityAttribute = (opacityMode == AttributeOpacity);
    if (complexGeometry && srcPixelType == Qt::SolidPattern) {
        requiredProgram.positionVertexShader = QOpenGLEngineSharedShaders::ComplexGeometryPositionOnlyVertexShader;
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
    QOpenGLContextPrivate* ctx_d = ctx->d_func();
    QOpenGL2PaintEngineEx *active_engine = static_cast<QOpenGL2PaintEngineEx *>(ctx_d->active_engine);
    active_engine->d_func()->setVertexAttribArrayEnabled(QT_VERTEX_COORDS_ATTR, true);
    active_engine->d_func()->setVertexAttribArrayEnabled(QT_TEXTURE_COORDS_ATTR, currentShaderProg && currentShaderProg->useTextureCoords);
    active_engine->d_func()->setVertexAttribArrayEnabled(QT_OPACITY_ATTR, currentShaderProg && currentShaderProg->useOpacityAttribute);

    shaderProgNeedsChanging = false;
    return true;
}

QT_END_NAMESPACE
