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

#include "qopenglshaderprogram.h"
#include "qopenglfunctions.h"
#include "private/qopenglcontext_p.h"
#include <QtCore/private/qobject_p.h>
#include <QtCore/qdebug.h>
#include <QtCore/qfile.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qvector.h>
#include <QtGui/qtransform.h>
#include <QtGui/QColor>
#include <QtGui/QSurfaceFormat>

#if !defined(QT_OPENGL_ES_2)
#include <QtGui/qopenglfunctions_4_0_core.h>
#endif

QT_BEGIN_NAMESPACE

/*!
    \class QOpenGLShaderProgram
    \brief The QOpenGLShaderProgram class allows OpenGL shader programs to be linked and used.
    \since 5.0
    \ingroup painting-3D
    \inmodule QtGui

    \section1 Introduction

    This class supports shader programs written in the OpenGL Shading
    Language (GLSL) and in the OpenGL/ES Shading Language (GLSL/ES).

    QOpenGLShader and QOpenGLShaderProgram shelter the programmer from the details of
    compiling and linking vertex and fragment shaders.

    The following example creates a vertex shader program using the
    supplied source \c{code}.  Once compiled and linked, the shader
    program is activated in the current QOpenGLContext by calling
    QOpenGLShaderProgram::bind():

    \snippet code/src_gui_qopenglshaderprogram.cpp 0

    \section1 Writing portable shaders

    Shader programs can be difficult to reuse across OpenGL implementations
    because of varying levels of support for standard vertex attributes and
    uniform variables.  In particular, GLSL/ES lacks all of the
    standard variables that are present on desktop OpenGL systems:
    \c{gl_Vertex}, \c{gl_Normal}, \c{gl_Color}, and so on.  Desktop OpenGL
    lacks the variable qualifiers \c{highp}, \c{mediump}, and \c{lowp}.

    The QOpenGLShaderProgram class makes the process of writing portable shaders
    easier by prefixing all shader programs with the following lines on
    desktop OpenGL:

    \code
    #define highp
    #define mediump
    #define lowp
    \endcode

    This makes it possible to run most GLSL/ES shader programs
    on desktop systems.  The programmer should restrict themselves
    to just features that are present in GLSL/ES, and avoid
    standard variable names that only work on the desktop.

    \section1 Simple shader example

    \snippet code/src_gui_qopenglshaderprogram.cpp 1

    With the above shader program active, we can draw a green triangle
    as follows:

    \snippet code/src_gui_qopenglshaderprogram.cpp 2

    \section1 Binary shaders and programs

    Binary shaders may be specified using \c{glShaderBinary()} on
    the return value from QOpenGLShader::shaderId().  The QOpenGLShader instance
    containing the binary can then be added to the shader program with
    addShader() and linked in the usual fashion with link().

    Binary programs may be specified using \c{glProgramBinaryOES()}
    on the return value from programId().  Then the application should
    call link(), which will notice that the program has already been
    specified and linked, allowing other operations to be performed
    on the shader program. The shader program's id can be explicitly
    created using the create() function.

    \sa QOpenGLShader
*/

/*!
    \class QOpenGLShader
    \brief The QOpenGLShader class allows OpenGL shaders to be compiled.
    \since 5.0
    \ingroup painting-3D
    \inmodule QtGui

    This class supports shaders written in the OpenGL Shading Language (GLSL)
    and in the OpenGL/ES Shading Language (GLSL/ES).

    QOpenGLShader and QOpenGLShaderProgram shelter the programmer from the details of
    compiling and linking vertex and fragment shaders.

    \sa QOpenGLShaderProgram
*/

/*!
    \enum QOpenGLShader::ShaderTypeBit
    This enum specifies the type of QOpenGLShader that is being created.

    \value Vertex Vertex shader written in the OpenGL Shading Language (GLSL).
    \value Fragment Fragment shader written in the OpenGL Shading Language (GLSL).
    \value Geometry Geometry shaders written in the OpenGL Shading Language (GLSL)
           based on the OpenGL core feature (requires OpenGL >= 3.2).
    \value TessellationControl Tessellation control shaders written in the OpenGL
           shading language (GLSL), based on the core feature (requires OpenGL >= 4.0).
    \value TessellationEvaluation Tessellation evaluation shaders written in the OpenGL
           shading language (GLSL), based on the core feature (requires OpenGL >= 4.0).
    \value Compute Compute shaders written in the OpenGL shading language (GLSL),
           based on the core feature (requires OpenGL >= 4.3).
*/

class QOpenGLShaderPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QOpenGLShader)
public:
    QOpenGLShaderPrivate(QOpenGLContext *ctx, QOpenGLShader::ShaderType type)
        : shaderGuard(0)
        , shaderType(type)
        , compiled(false)
        , glfuncs(new QOpenGLFunctions(ctx))
#ifndef QT_OPENGL_ES_2
        , supportsGeometryShaders(false)
        , supportsTessellationShaders(false)
#endif
    {
#ifndef QT_OPENGL_ES_2
        if (!ctx->isOpenGLES()) {
            QSurfaceFormat f = ctx->format();

            // Geometry shaders require OpenGL >= 3.2
            if (shaderType & QOpenGLShader::Geometry)
                supportsGeometryShaders = (f.version() >= qMakePair<int, int>(3, 2));
            else if (shaderType & (QOpenGLShader::TessellationControl | QOpenGLShader::TessellationEvaluation))
                supportsTessellationShaders = (f.version() >= qMakePair<int, int>(4, 0));
        }
#endif
    }
    ~QOpenGLShaderPrivate();

    QOpenGLSharedResourceGuard *shaderGuard;
    QOpenGLShader::ShaderType shaderType;
    bool compiled;
    QString log;

    QOpenGLFunctions *glfuncs;

#ifndef QT_OPENGL_ES_2
    // Support for geometry shaders
    bool supportsGeometryShaders;

    // Support for tessellation shaders
    bool supportsTessellationShaders;
#endif

    bool create();
    bool compile(QOpenGLShader *q);
    void deleteShader();
};

namespace {
    void freeShaderFunc(QOpenGLFunctions *funcs, GLuint id)
    {
        funcs->glDeleteShader(id);
    }
}

QOpenGLShaderPrivate::~QOpenGLShaderPrivate()
{
    delete glfuncs;
    if (shaderGuard)
        shaderGuard->free();
}

bool QOpenGLShaderPrivate::create()
{
    QOpenGLContext *context = const_cast<QOpenGLContext *>(QOpenGLContext::currentContext());
    if (!context)
        return false;
    GLuint shader;
    if (shaderType == QOpenGLShader::Vertex) {
        shader = glfuncs->glCreateShader(GL_VERTEX_SHADER);
#if defined(QT_OPENGL_3_2)
    } else if (shaderType == QOpenGLShader::Geometry && supportsGeometryShaders) {
        shader = glfuncs->glCreateShader(GL_GEOMETRY_SHADER);
#endif
#if defined(QT_OPENGL_4)
    } else if (shaderType == QOpenGLShader::TessellationControl && supportsTessellationShaders) {
        shader = glfuncs->glCreateShader(GL_TESS_CONTROL_SHADER);
    } else if (shaderType == QOpenGLShader::TessellationEvaluation && supportsTessellationShaders) {
        shader = glfuncs->glCreateShader(GL_TESS_EVALUATION_SHADER);
#endif
#if defined(QT_OPENGL_4_3)
    } else if (shaderType == QOpenGLShader::Compute) {
        shader = glfuncs->glCreateShader(GL_COMPUTE_SHADER);
#endif
    } else {
        shader = glfuncs->glCreateShader(GL_FRAGMENT_SHADER);
    }
    if (!shader) {
        qWarning() << "QOpenGLShader: could not create shader";
        return false;
    }
    shaderGuard = new QOpenGLSharedResourceGuard(context, shader, freeShaderFunc);
    return true;
}

bool QOpenGLShaderPrivate::compile(QOpenGLShader *q)
{
    GLuint shader = shaderGuard ? shaderGuard->id() : 0;
    if (!shader)
        return false;

    // Try to compile shader
    glfuncs->glCompileShader(shader);
    GLint value = 0;

    // Get compilation status
    glfuncs->glGetShaderiv(shader, GL_COMPILE_STATUS, &value);
    compiled = (value != 0);

    if (!compiled) {
        // Compilation failed, try to provide some information about the failure
        QString name = q->objectName();

        const char *types[] = {
            "Fragment",
            "Vertex",
            "Geometry",
            ""
        };

        const char *type = types[3];
        if (shaderType == QOpenGLShader::Fragment)
            type = types[0];
        else if (shaderType == QOpenGLShader::Vertex)
            type = types[1];
        else if (shaderType == QOpenGLShader::Geometry)
            type = types[2];

        // Get info and source code lengths
        GLint infoLogLength = 0;
        GLint sourceCodeLength = 0;
        char *logBuffer = 0;
        char *sourceCodeBuffer = 0;

        // Get the compilation info log
        glfuncs->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

        if (infoLogLength > 1) {
            GLint temp;
            logBuffer = new char [infoLogLength];
            glfuncs->glGetShaderInfoLog(shader, infoLogLength, &temp, logBuffer);
        }

        // Get the source code
        glfuncs->glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &sourceCodeLength);

        if (sourceCodeLength > 1) {
            GLint temp;
            sourceCodeBuffer = new char [sourceCodeLength];
            glfuncs->glGetShaderSource(shader, sourceCodeLength, &temp, sourceCodeBuffer);
        }

        if (logBuffer)
            log = QString::fromLatin1(logBuffer);
        else
            log = QLatin1String("failed");

        if (name.isEmpty())
            qWarning("QOpenGLShader::compile(%s): %s", type, qPrintable(log));
        else
            qWarning("QOpenGLShader::compile(%s)[%s]: %s", type, qPrintable(name), qPrintable(log));

        // Dump the source code if we got it
        if (sourceCodeBuffer) {
            qWarning("*** Problematic %s shader source code ***", type);
            qWarning() << qPrintable(QString::fromLatin1(sourceCodeBuffer));
            qWarning("***");
        }

        // Cleanup
        delete [] logBuffer;
        delete [] sourceCodeBuffer;
    }

    return compiled;
}

void QOpenGLShaderPrivate::deleteShader()
{
    if (shaderGuard) {
        shaderGuard->free();
        shaderGuard = 0;
    }
}

/*!
    Constructs a new QOpenGLShader object of the specified \a type
    and attaches it to \a parent.  If shader programs are not supported,
    QOpenGLShaderProgram::hasOpenGLShaderPrograms() will return false.

    This constructor is normally followed by a call to compileSourceCode()
    or compileSourceFile().

    The shader will be associated with the current QOpenGLContext.

    \sa compileSourceCode(), compileSourceFile()
*/
QOpenGLShader::QOpenGLShader(QOpenGLShader::ShaderType type, QObject *parent)
    : QObject(*new QOpenGLShaderPrivate(QOpenGLContext::currentContext(), type), parent)
{
    Q_D(QOpenGLShader);
    d->create();
}

/*!
    Deletes this shader.  If the shader has been attached to a
    QOpenGLShaderProgram object, then the actual shader will stay around
    until the QOpenGLShaderProgram is destroyed.
*/
QOpenGLShader::~QOpenGLShader()
{
}

/*!
    Returns the type of this shader.
*/
QOpenGLShader::ShaderType QOpenGLShader::shaderType() const
{
    Q_D(const QOpenGLShader);
    return d->shaderType;
}

static const char qualifierDefines[] =
    "#define lowp\n"
    "#define mediump\n"
    "#define highp\n";

#if defined(QT_OPENGL_ES) && !defined(QT_OPENGL_FORCE_SHADER_DEFINES)
// The "highp" qualifier doesn't exist in fragment shaders
// on all ES platforms.  When it doesn't exist, use "mediump".
#define QOpenGL_REDEFINE_HIGHP 1
static const char redefineHighp[] =
    "#ifndef GL_FRAGMENT_PRECISION_HIGH\n"
    "#define highp mediump\n"
    "#endif\n";
#endif

/*!
    Sets the \a source code for this shader and compiles it.
    Returns \c true if the source was successfully compiled, false otherwise.

    \sa compileSourceFile()
*/
bool QOpenGLShader::compileSourceCode(const char *source)
{
    Q_D(QOpenGLShader);
    if (d->shaderGuard && d->shaderGuard->id()) {
        QVarLengthArray<const char *, 4> src;
        QVarLengthArray<GLint, 4> srclen;
        int headerLen = 0;
        while (source && source[headerLen] == '#') {
            // Skip #version and #extension directives at the start of
            // the shader code.  We need to insert the qualifierDefines
            // and redefineHighp just after them.
            if (qstrncmp(source + headerLen, "#version", 8) != 0 &&
                    qstrncmp(source + headerLen, "#extension", 10) != 0) {
                break;
            }
            while (source[headerLen] != '\0' && source[headerLen] != '\n')
                ++headerLen;
            if (source[headerLen] == '\n')
                ++headerLen;
        }
        if (headerLen > 0) {
            src.append(source);
            srclen.append(GLint(headerLen));
        }

        // The precision qualifiers are useful on OpenGL/ES systems,
        // but usually not present on desktop systems.
        const QSurfaceFormat currentSurfaceFormat = QOpenGLContext::currentContext()->format();
        QOpenGLContextPrivate *ctx_d = QOpenGLContextPrivate::get(QOpenGLContext::currentContext());
        if (currentSurfaceFormat.renderableType() == QSurfaceFormat::OpenGL
                || ctx_d->workaround_missingPrecisionQualifiers
#ifdef QT_OPENGL_FORCE_SHADER_DEFINES
                || true
#endif
                ) {
            src.append(qualifierDefines);
            srclen.append(GLint(sizeof(qualifierDefines) - 1));
        }

#ifdef QOpenGL_REDEFINE_HIGHP
        if (d->shaderType == Fragment && !ctx_d->workaround_missingPrecisionQualifiers
            && QOpenGLContext::currentContext()->isOpenGLES()) {
            src.append(redefineHighp);
            srclen.append(GLint(sizeof(redefineHighp) - 1));
        }
#endif
        src.append(source + headerLen);
        srclen.append(GLint(qstrlen(source + headerLen)));
        d->glfuncs->glShaderSource(d->shaderGuard->id(), src.size(), src.data(), srclen.data());
        return d->compile(this);
    } else {
        return false;
    }
}

/*!
    \overload

    Sets the \a source code for this shader and compiles it.
    Returns \c true if the source was successfully compiled, false otherwise.

    \sa compileSourceFile()
*/
bool QOpenGLShader::compileSourceCode(const QByteArray& source)
{
    return compileSourceCode(source.constData());
}

/*!
    \overload

    Sets the \a source code for this shader and compiles it.
    Returns \c true if the source was successfully compiled, false otherwise.

    \sa compileSourceFile()
*/
bool QOpenGLShader::compileSourceCode(const QString& source)
{
    return compileSourceCode(source.toLatin1().constData());
}

/*!
    Sets the source code for this shader to the contents of \a fileName
    and compiles it.  Returns \c true if the file could be opened and the
    source compiled, false otherwise.

    \sa compileSourceCode()
*/
bool QOpenGLShader::compileSourceFile(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly)) {
        qWarning() << "QOpenGLShader: Unable to open file" << fileName;
        return false;
    }

    QByteArray contents = file.readAll();
    return compileSourceCode(contents.constData());
}

/*!
    Returns the source code for this shader.

    \sa compileSourceCode()
*/
QByteArray QOpenGLShader::sourceCode() const
{
    Q_D(const QOpenGLShader);
    GLuint shader = d->shaderGuard ? d->shaderGuard->id() : 0;
    if (!shader)
        return QByteArray();
    GLint size = 0;
    d->glfuncs->glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &size);
    if (size <= 0)
        return QByteArray();
    GLint len = 0;
    char *source = new char [size];
    d->glfuncs->glGetShaderSource(shader, size, &len, source);
    QByteArray src(source);
    delete [] source;
    return src;
}

/*!
    Returns \c true if this shader has been compiled; false otherwise.

    \sa compileSourceCode(), compileSourceFile()
*/
bool QOpenGLShader::isCompiled() const
{
    Q_D(const QOpenGLShader);
    return d->compiled;
}

/*!
    Returns the errors and warnings that occurred during the last compile.

    \sa compileSourceCode(), compileSourceFile()
*/
QString QOpenGLShader::log() const
{
    Q_D(const QOpenGLShader);
    return d->log;
}

/*!
    Returns the OpenGL identifier associated with this shader.

    \sa QOpenGLShaderProgram::programId()
*/
GLuint QOpenGLShader::shaderId() const
{
    Q_D(const QOpenGLShader);
    return d->shaderGuard ? d->shaderGuard->id() : 0;
}

class QOpenGLShaderProgramPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QOpenGLShaderProgram)
public:
    QOpenGLShaderProgramPrivate()
        : programGuard(0)
        , linked(false)
        , inited(false)
        , removingShaders(false)
        , glfuncs(new QOpenGLFunctions)
#ifndef QT_OPENGL_ES_2
        , tessellationFuncs(0)
#endif
    {
    }
    ~QOpenGLShaderProgramPrivate();

    QOpenGLSharedResourceGuard *programGuard;
    bool linked;
    bool inited;
    bool removingShaders;

    QString log;
    QList<QOpenGLShader *> shaders;
    QList<QOpenGLShader *> anonShaders;

    QOpenGLFunctions *glfuncs;

#ifndef QT_OPENGL_ES_2
    // Tessellation shader support
    QOpenGLFunctions_4_0_Core *tessellationFuncs;
#endif

    bool hasShader(QOpenGLShader::ShaderType type) const;
};

namespace {
    void freeProgramFunc(QOpenGLFunctions *funcs, GLuint id)
    {
        funcs->glDeleteProgram(id);
    }
}


QOpenGLShaderProgramPrivate::~QOpenGLShaderProgramPrivate()
{
    delete glfuncs;
    if (programGuard)
        programGuard->free();
}

bool QOpenGLShaderProgramPrivate::hasShader(QOpenGLShader::ShaderType type) const
{
    foreach (QOpenGLShader *shader, shaders) {
        if (shader->shaderType() == type)
            return true;
    }
    return false;
}

/*!
    Constructs a new shader program and attaches it to \a parent.
    The program will be invalid until addShader() is called.

    The shader program will be associated with the current QOpenGLContext.

    \sa addShader()
*/
QOpenGLShaderProgram::QOpenGLShaderProgram(QObject *parent)
    : QObject(*new QOpenGLShaderProgramPrivate, parent)
{
}

/*!
    Deletes this shader program.
*/
QOpenGLShaderProgram::~QOpenGLShaderProgram()
{
}

/*!
    Requests the shader program's id to be created immediately. Returns \c true
    if successful; \c false otherwise.

    This function is primarily useful when combining QOpenGLShaderProgram
    with other OpenGL functions that operate directly on the shader
    program id, like \c {GL_OES_get_program_binary}.

    When the shader program is used normally, the shader program's id will
    be created on demand.

    \sa programId()

    \since 5.3
 */
bool QOpenGLShaderProgram::create()
{
    return init();
}

bool QOpenGLShaderProgram::init()
{
    Q_D(QOpenGLShaderProgram);
    if ((d->programGuard && d->programGuard->id()) || d->inited)
        return true;
    d->inited = true;
    QOpenGLContext *context = const_cast<QOpenGLContext *>(QOpenGLContext::currentContext());
    if (!context)
        return false;
    d->glfuncs->initializeOpenGLFunctions();

#ifndef QT_OPENGL_ES_2
    // Resolve OpenGL 4 functions for tessellation shader support
    QSurfaceFormat format = context->format();
    if (!context->isOpenGLES()
        && format.version() >= qMakePair<int, int>(4, 0)) {
        d->tessellationFuncs = context->versionFunctions<QOpenGLFunctions_4_0_Core>();
        d->tessellationFuncs->initializeOpenGLFunctions();
    }
#endif

    GLuint program = d->glfuncs->glCreateProgram();
    if (!program) {
        qWarning() << "QOpenGLShaderProgram: could not create shader program";
        return false;
    }
    if (d->programGuard)
        delete d->programGuard;
    d->programGuard = new QOpenGLSharedResourceGuard(context, program, freeProgramFunc);
    return true;
}

/*!
    Adds a compiled \a shader to this shader program.  Returns \c true
    if the shader could be added, or false otherwise.

    Ownership of the \a shader object remains with the caller.
    It will not be deleted when this QOpenGLShaderProgram instance
    is deleted.  This allows the caller to add the same shader
    to multiple shader programs.

    \sa addShaderFromSourceCode(), addShaderFromSourceFile()
    \sa removeShader(), link(), removeAllShaders()
*/
bool QOpenGLShaderProgram::addShader(QOpenGLShader *shader)
{
    Q_D(QOpenGLShaderProgram);
    if (!init())
        return false;
    if (d->shaders.contains(shader))
        return true;    // Already added to this shader program.
    if (d->programGuard && d->programGuard->id() && shader) {
        if (!shader->d_func()->shaderGuard || !shader->d_func()->shaderGuard->id())
            return false;
        if (d->programGuard->group() != shader->d_func()->shaderGuard->group()) {
            qWarning("QOpenGLShaderProgram::addShader: Program and shader are not associated with same context.");
            return false;
        }
        d->glfuncs->glAttachShader(d->programGuard->id(), shader->d_func()->shaderGuard->id());
        d->linked = false;  // Program needs to be relinked.
        d->shaders.append(shader);
        connect(shader, SIGNAL(destroyed()), this, SLOT(shaderDestroyed()));
        return true;
    } else {
        return false;
    }
}

/*!
    Compiles \a source as a shader of the specified \a type and
    adds it to this shader program.  Returns \c true if compilation
    was successful, false otherwise.  The compilation errors
    and warnings will be made available via log().

    This function is intended to be a short-cut for quickly
    adding vertex and fragment shaders to a shader program without
    creating an instance of QOpenGLShader first.

    \sa addShader(), addShaderFromSourceFile()
    \sa removeShader(), link(), log(), removeAllShaders()
*/
bool QOpenGLShaderProgram::addShaderFromSourceCode(QOpenGLShader::ShaderType type, const char *source)
{
    Q_D(QOpenGLShaderProgram);
    if (!init())
        return false;
    QOpenGLShader *shader = new QOpenGLShader(type, this);
    if (!shader->compileSourceCode(source)) {
        d->log = shader->log();
        delete shader;
        return false;
    }
    d->anonShaders.append(shader);
    return addShader(shader);
}

/*!
    \overload

    Compiles \a source as a shader of the specified \a type and
    adds it to this shader program.  Returns \c true if compilation
    was successful, false otherwise.  The compilation errors
    and warnings will be made available via log().

    This function is intended to be a short-cut for quickly
    adding vertex and fragment shaders to a shader program without
    creating an instance of QOpenGLShader first.

    \sa addShader(), addShaderFromSourceFile()
    \sa removeShader(), link(), log(), removeAllShaders()
*/
bool QOpenGLShaderProgram::addShaderFromSourceCode(QOpenGLShader::ShaderType type, const QByteArray& source)
{
    return addShaderFromSourceCode(type, source.constData());
}

/*!
    \overload

    Compiles \a source as a shader of the specified \a type and
    adds it to this shader program.  Returns \c true if compilation
    was successful, false otherwise.  The compilation errors
    and warnings will be made available via log().

    This function is intended to be a short-cut for quickly
    adding vertex and fragment shaders to a shader program without
    creating an instance of QOpenGLShader first.

    \sa addShader(), addShaderFromSourceFile()
    \sa removeShader(), link(), log(), removeAllShaders()
*/
bool QOpenGLShaderProgram::addShaderFromSourceCode(QOpenGLShader::ShaderType type, const QString& source)
{
    return addShaderFromSourceCode(type, source.toLatin1().constData());
}

/*!
    Compiles the contents of \a fileName as a shader of the specified
    \a type and adds it to this shader program.  Returns \c true if
    compilation was successful, false otherwise.  The compilation errors
    and warnings will be made available via log().

    This function is intended to be a short-cut for quickly
    adding vertex and fragment shaders to a shader program without
    creating an instance of QOpenGLShader first.

    \sa addShader(), addShaderFromSourceCode()
*/
bool QOpenGLShaderProgram::addShaderFromSourceFile
    (QOpenGLShader::ShaderType type, const QString& fileName)
{
    Q_D(QOpenGLShaderProgram);
    if (!init())
        return false;
    QOpenGLShader *shader = new QOpenGLShader(type, this);
    if (!shader->compileSourceFile(fileName)) {
        d->log = shader->log();
        delete shader;
        return false;
    }
    d->anonShaders.append(shader);
    return addShader(shader);
}

/*!
    Removes \a shader from this shader program.  The object is not deleted.

    The shader program must be valid in the current QOpenGLContext.

    \sa addShader(), link(), removeAllShaders()
*/
void QOpenGLShaderProgram::removeShader(QOpenGLShader *shader)
{
    Q_D(QOpenGLShaderProgram);
    if (d->programGuard && d->programGuard->id()
        && shader && shader->d_func()->shaderGuard)
    {
        d->glfuncs->glDetachShader(d->programGuard->id(), shader->d_func()->shaderGuard->id());
    }
    d->linked = false;  // Program needs to be relinked.
    if (shader) {
        d->shaders.removeAll(shader);
        d->anonShaders.removeAll(shader);
        disconnect(shader, SIGNAL(destroyed()), this, SLOT(shaderDestroyed()));
    }
}

/*!
    Returns a list of all shaders that have been added to this shader
    program using addShader().

    \sa addShader(), removeShader()
*/
QList<QOpenGLShader *> QOpenGLShaderProgram::shaders() const
{
    Q_D(const QOpenGLShaderProgram);
    return d->shaders;
}

/*!
    Removes all of the shaders that were added to this program previously.
    The QOpenGLShader objects for the shaders will not be deleted if they
    were constructed externally.  QOpenGLShader objects that are constructed
    internally by QOpenGLShaderProgram will be deleted.

    \sa addShader(), removeShader()
*/
void QOpenGLShaderProgram::removeAllShaders()
{
    Q_D(QOpenGLShaderProgram);
    d->removingShaders = true;
    foreach (QOpenGLShader *shader, d->shaders) {
        if (d->programGuard && d->programGuard->id()
            && shader && shader->d_func()->shaderGuard)
        {
            d->glfuncs->glDetachShader(d->programGuard->id(), shader->d_func()->shaderGuard->id());
        }
    }
    foreach (QOpenGLShader *shader, d->anonShaders) {
        // Delete shader objects that were created anonymously.
        delete shader;
    }
    d->shaders.clear();
    d->anonShaders.clear();
    d->linked = false;  // Program needs to be relinked.
    d->removingShaders = false;
}

/*!
    Links together the shaders that were added to this program with
    addShader().  Returns \c true if the link was successful or
    false otherwise.  If the link failed, the error messages can
    be retrieved with log().

    Subclasses can override this function to initialize attributes
    and uniform variables for use in specific shader programs.

    If the shader program was already linked, calling this
    function again will force it to be re-linked.

    \sa addShader(), log()
*/
bool QOpenGLShaderProgram::link()
{
    Q_D(QOpenGLShaderProgram);
    GLuint program = d->programGuard ? d->programGuard->id() : 0;
    if (!program)
        return false;

    GLint value;
    if (d->shaders.isEmpty()) {
        // If there are no explicit shaders, then it is possible that the
        // application added a program binary with glProgramBinaryOES(),
        // or otherwise populated the shaders itself.  Check to see if the
        // program is already linked and bail out if so.
        value = 0;
        d->glfuncs->glGetProgramiv(program, GL_LINK_STATUS, &value);
        d->linked = (value != 0);
        if (d->linked)
            return true;
    }

    d->glfuncs->glLinkProgram(program);
    value = 0;
    d->glfuncs->glGetProgramiv(program, GL_LINK_STATUS, &value);
    d->linked = (value != 0);
    value = 0;
    d->glfuncs->glGetProgramiv(program, GL_INFO_LOG_LENGTH, &value);
    d->log = QString();
    if (value > 1) {
        char *logbuf = new char [value];
        GLint len;
        d->glfuncs->glGetProgramInfoLog(program, value, &len, logbuf);
        d->log = QString::fromLatin1(logbuf);
        if (!d->linked) {
            QString name = objectName();
            if (name.isEmpty())
                qWarning() << "QOpenGLShader::link:" << d->log;
            else
                qWarning() << "QOpenGLShader::link[" << name << "]:" << d->log;
        }
        delete [] logbuf;
    }
    return d->linked;
}

/*!
    Returns \c true if this shader program has been linked; false otherwise.

    \sa link()
*/
bool QOpenGLShaderProgram::isLinked() const
{
    Q_D(const QOpenGLShaderProgram);
    return d->linked;
}

/*!
    Returns the errors and warnings that occurred during the last link()
    or addShader() with explicitly specified source code.

    \sa link()
*/
QString QOpenGLShaderProgram::log() const
{
    Q_D(const QOpenGLShaderProgram);
    return d->log;
}

/*!
    Binds this shader program to the active QOpenGLContext and makes
    it the current shader program.  Any previously bound shader program
    is released.  This is equivalent to calling \c{glUseProgram()} on
    programId().  Returns \c true if the program was successfully bound;
    false otherwise.  If the shader program has not yet been linked,
    or it needs to be re-linked, this function will call link().

    \sa link(), release()
*/
bool QOpenGLShaderProgram::bind()
{
    Q_D(QOpenGLShaderProgram);
    GLuint program = d->programGuard ? d->programGuard->id() : 0;
    if (!program)
        return false;
    if (!d->linked && !link())
        return false;
#ifndef QT_NO_DEBUG
    if (d->programGuard->group() != QOpenGLContextGroup::currentContextGroup()) {
        qWarning("QOpenGLShaderProgram::bind: program is not valid in the current context.");
        return false;
    }
#endif
    d->glfuncs->glUseProgram(program);
    return true;
}

/*!
    Releases the active shader program from the current QOpenGLContext.
    This is equivalent to calling \c{glUseProgram(0)}.

    \sa bind()
*/
void QOpenGLShaderProgram::release()
{
    Q_D(QOpenGLShaderProgram);
#ifndef QT_NO_DEBUG
    if (d->programGuard && d->programGuard->group() != QOpenGLContextGroup::currentContextGroup())
        qWarning("QOpenGLShaderProgram::release: program is not valid in the current context.");
#endif
    d->glfuncs->glUseProgram(0);
}

/*!
    Returns the OpenGL identifier associated with this shader program.

    \sa QOpenGLShader::shaderId()
*/
GLuint QOpenGLShaderProgram::programId() const
{
    Q_D(const QOpenGLShaderProgram);
    GLuint id = d->programGuard ? d->programGuard->id() : 0;
    if (id)
        return id;

    // Create the identifier if we don't have one yet.  This is for
    // applications that want to create the attached shader configuration
    // themselves, particularly those using program binaries.
    if (!const_cast<QOpenGLShaderProgram *>(this)->init())
        return 0;
    return d->programGuard ? d->programGuard->id() : 0;
}

/*!
    Binds the attribute \a name to the specified \a location.  This
    function can be called before or after the program has been linked.
    Any attributes that have not been explicitly bound when the program
    is linked will be assigned locations automatically.

    When this function is called after the program has been linked,
    the program will need to be relinked for the change to take effect.

    \sa attributeLocation()
*/
void QOpenGLShaderProgram::bindAttributeLocation(const char *name, int location)
{
    Q_D(QOpenGLShaderProgram);
    if (!init() || !d->programGuard || !d->programGuard->id())
        return;
    d->glfuncs->glBindAttribLocation(d->programGuard->id(), location, name);
    d->linked = false;  // Program needs to be relinked.
}

/*!
    \overload

    Binds the attribute \a name to the specified \a location.  This
    function can be called before or after the program has been linked.
    Any attributes that have not been explicitly bound when the program
    is linked will be assigned locations automatically.

    When this function is called after the program has been linked,
    the program will need to be relinked for the change to take effect.

    \sa attributeLocation()
*/
void QOpenGLShaderProgram::bindAttributeLocation(const QByteArray& name, int location)
{
    bindAttributeLocation(name.constData(), location);
}

/*!
    \overload

    Binds the attribute \a name to the specified \a location.  This
    function can be called before or after the program has been linked.
    Any attributes that have not been explicitly bound when the program
    is linked will be assigned locations automatically.

    When this function is called after the program has been linked,
    the program will need to be relinked for the change to take effect.

    \sa attributeLocation()
*/
void QOpenGLShaderProgram::bindAttributeLocation(const QString& name, int location)
{
    bindAttributeLocation(name.toLatin1().constData(), location);
}

/*!
    Returns the location of the attribute \a name within this shader
    program's parameter list.  Returns -1 if \a name is not a valid
    attribute for this shader program.

    \sa uniformLocation(), bindAttributeLocation()
*/
int QOpenGLShaderProgram::attributeLocation(const char *name) const
{
    Q_D(const QOpenGLShaderProgram);
    if (d->linked && d->programGuard && d->programGuard->id()) {
        return d->glfuncs->glGetAttribLocation(d->programGuard->id(), name);
    } else {
        qWarning() << "QOpenGLShaderProgram::attributeLocation(" << name
                   << "): shader program is not linked";
        return -1;
    }
}

/*!
    \overload

    Returns the location of the attribute \a name within this shader
    program's parameter list.  Returns -1 if \a name is not a valid
    attribute for this shader program.

    \sa uniformLocation(), bindAttributeLocation()
*/
int QOpenGLShaderProgram::attributeLocation(const QByteArray& name) const
{
    return attributeLocation(name.constData());
}

/*!
    \overload

    Returns the location of the attribute \a name within this shader
    program's parameter list.  Returns -1 if \a name is not a valid
    attribute for this shader program.

    \sa uniformLocation(), bindAttributeLocation()
*/
int QOpenGLShaderProgram::attributeLocation(const QString& name) const
{
    return attributeLocation(name.toLatin1().constData());
}

/*!
    Sets the attribute at \a location in the current context to \a value.

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue(int location, GLfloat value)
{
    Q_D(QOpenGLShaderProgram);
    if (location != -1)
        d->glfuncs->glVertexAttrib1fv(location, &value);
}

/*!
    \overload

    Sets the attribute called \a name in the current context to \a value.

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue(const char *name, GLfloat value)
{
    setAttributeValue(attributeLocation(name), value);
}

/*!
    Sets the attribute at \a location in the current context to
    the 2D vector (\a x, \a y).

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue(int location, GLfloat x, GLfloat y)
{
    Q_D(QOpenGLShaderProgram);
    if (location != -1) {
        GLfloat values[2] = {x, y};
        d->glfuncs->glVertexAttrib2fv(location, values);
    }
}

/*!
    \overload

    Sets the attribute called \a name in the current context to
    the 2D vector (\a x, \a y).

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue(const char *name, GLfloat x, GLfloat y)
{
    setAttributeValue(attributeLocation(name), x, y);
}

/*!
    Sets the attribute at \a location in the current context to
    the 3D vector (\a x, \a y, \a z).

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue
        (int location, GLfloat x, GLfloat y, GLfloat z)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        GLfloat values[3] = {x, y, z};
        d->glfuncs->glVertexAttrib3fv(location, values);
    }
}

/*!
    \overload

    Sets the attribute called \a name in the current context to
    the 3D vector (\a x, \a y, \a z).

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue
        (const char *name, GLfloat x, GLfloat y, GLfloat z)
{
    setAttributeValue(attributeLocation(name), x, y, z);
}

/*!
    Sets the attribute at \a location in the current context to
    the 4D vector (\a x, \a y, \a z, \a w).

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue
        (int location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    Q_D(QOpenGLShaderProgram);
    if (location != -1) {
        GLfloat values[4] = {x, y, z, w};
        d->glfuncs->glVertexAttrib4fv(location, values);
    }
}

/*!
    \overload

    Sets the attribute called \a name in the current context to
    the 4D vector (\a x, \a y, \a z, \a w).

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue
        (const char *name, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    setAttributeValue(attributeLocation(name), x, y, z, w);
}

/*!
    Sets the attribute at \a location in the current context to \a value.

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue(int location, const QVector2D& value)
{
    Q_D(QOpenGLShaderProgram);
    if (location != -1)
        d->glfuncs->glVertexAttrib2fv(location, reinterpret_cast<const GLfloat *>(&value));
}

/*!
    \overload

    Sets the attribute called \a name in the current context to \a value.

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue(const char *name, const QVector2D& value)
{
    setAttributeValue(attributeLocation(name), value);
}

/*!
    Sets the attribute at \a location in the current context to \a value.

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue(int location, const QVector3D& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glVertexAttrib3fv(location, reinterpret_cast<const GLfloat *>(&value));
}

/*!
    \overload

    Sets the attribute called \a name in the current context to \a value.

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue(const char *name, const QVector3D& value)
{
    setAttributeValue(attributeLocation(name), value);
}

/*!
    Sets the attribute at \a location in the current context to \a value.

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue(int location, const QVector4D& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glVertexAttrib4fv(location, reinterpret_cast<const GLfloat *>(&value));
}

/*!
    \overload

    Sets the attribute called \a name in the current context to \a value.

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue(const char *name, const QVector4D& value)
{
    setAttributeValue(attributeLocation(name), value);
}

/*!
    Sets the attribute at \a location in the current context to \a value.

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue(int location, const QColor& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        GLfloat values[4] = {GLfloat(value.redF()), GLfloat(value.greenF()),
                             GLfloat(value.blueF()), GLfloat(value.alphaF())};
        d->glfuncs->glVertexAttrib4fv(location, values);
    }
}

/*!
    \overload

    Sets the attribute called \a name in the current context to \a value.

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue(const char *name, const QColor& value)
{
    setAttributeValue(attributeLocation(name), value);
}

/*!
    Sets the attribute at \a location in the current context to the
    contents of \a values, which contains \a columns elements, each
    consisting of \a rows elements.  The \a rows value should be
    1, 2, 3, or 4.  This function is typically used to set matrix
    values and column vectors.

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue
    (int location, const GLfloat *values, int columns, int rows)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (rows < 1 || rows > 4) {
        qWarning() << "QOpenGLShaderProgram::setAttributeValue: rows" << rows << "not supported";
        return;
    }
    if (location != -1) {
        while (columns-- > 0) {
            if (rows == 1)
                d->glfuncs->glVertexAttrib1fv(location, values);
            else if (rows == 2)
                d->glfuncs->glVertexAttrib2fv(location, values);
            else if (rows == 3)
                d->glfuncs->glVertexAttrib3fv(location, values);
            else
                d->glfuncs->glVertexAttrib4fv(location, values);
            values += rows;
            ++location;
        }
    }
}

/*!
    \overload

    Sets the attribute called \a name in the current context to the
    contents of \a values, which contains \a columns elements, each
    consisting of \a rows elements.  The \a rows value should be
    1, 2, 3, or 4.  This function is typically used to set matrix
    values and column vectors.

    \sa setUniformValue()
*/
void QOpenGLShaderProgram::setAttributeValue
    (const char *name, const GLfloat *values, int columns, int rows)
{
    setAttributeValue(attributeLocation(name), values, columns, rows);
}

/*!
    Sets an array of vertex \a values on the attribute at \a location
    in this shader program.  The \a tupleSize indicates the number of
    components per vertex (1, 2, 3, or 4), and the \a stride indicates
    the number of bytes between vertices.  A default \a stride value
    of zero indicates that the vertices are densely packed in \a values.

    The array will become active when enableAttributeArray() is called
    on the \a location.  Otherwise the value specified with
    setAttributeValue() for \a location will be used.

    \sa setAttributeValue(), setUniformValue(), enableAttributeArray()
    \sa disableAttributeArray()
*/
void QOpenGLShaderProgram::setAttributeArray
    (int location, const GLfloat *values, int tupleSize, int stride)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        d->glfuncs->glVertexAttribPointer(location, tupleSize, GL_FLOAT, GL_FALSE,
                              stride, values);
    }
}

/*!
    Sets an array of 2D vertex \a values on the attribute at \a location
    in this shader program.  The \a stride indicates the number of bytes
    between vertices.  A default \a stride value of zero indicates that
    the vertices are densely packed in \a values.

    The array will become active when enableAttributeArray() is called
    on the \a location.  Otherwise the value specified with
    setAttributeValue() for \a location will be used.

    \sa setAttributeValue(), setUniformValue(), enableAttributeArray()
    \sa disableAttributeArray()
*/
void QOpenGLShaderProgram::setAttributeArray
        (int location, const QVector2D *values, int stride)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        d->glfuncs->glVertexAttribPointer(location, 2, GL_FLOAT, GL_FALSE,
                              stride, values);
    }
}

/*!
    Sets an array of 3D vertex \a values on the attribute at \a location
    in this shader program.  The \a stride indicates the number of bytes
    between vertices.  A default \a stride value of zero indicates that
    the vertices are densely packed in \a values.

    The array will become active when enableAttributeArray() is called
    on the \a location.  Otherwise the value specified with
    setAttributeValue() for \a location will be used.

    \sa setAttributeValue(), setUniformValue(), enableAttributeArray()
    \sa disableAttributeArray()
*/
void QOpenGLShaderProgram::setAttributeArray
        (int location, const QVector3D *values, int stride)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        d->glfuncs->glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE,
                              stride, values);
    }
}

/*!
    Sets an array of 4D vertex \a values on the attribute at \a location
    in this shader program.  The \a stride indicates the number of bytes
    between vertices.  A default \a stride value of zero indicates that
    the vertices are densely packed in \a values.

    The array will become active when enableAttributeArray() is called
    on the \a location.  Otherwise the value specified with
    setAttributeValue() for \a location will be used.

    \sa setAttributeValue(), setUniformValue(), enableAttributeArray()
    \sa disableAttributeArray()
*/
void QOpenGLShaderProgram::setAttributeArray
        (int location, const QVector4D *values, int stride)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        d->glfuncs->glVertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE,
                              stride, values);
    }
}

/*!
    Sets an array of vertex \a values on the attribute at \a location
    in this shader program.  The \a stride indicates the number of bytes
    between vertices.  A default \a stride value of zero indicates that
    the vertices are densely packed in \a values.

    The \a type indicates the type of elements in the \a values array,
    usually \c{GL_FLOAT}, \c{GL_UNSIGNED_BYTE}, etc.  The \a tupleSize
    indicates the number of components per vertex: 1, 2, 3, or 4.

    The array will become active when enableAttributeArray() is called
    on the \a location.  Otherwise the value specified with
    setAttributeValue() for \a location will be used.

    The setAttributeBuffer() function can be used to set the attribute
    array to an offset within a vertex buffer.

    \note Normalization will be enabled. If this is not desired, call
    glVertexAttribPointer directly through QOpenGLFunctions.

    \sa setAttributeValue(), setUniformValue(), enableAttributeArray()
    \sa disableAttributeArray(), setAttributeBuffer()
*/
void QOpenGLShaderProgram::setAttributeArray
    (int location, GLenum type, const void *values, int tupleSize, int stride)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        d->glfuncs->glVertexAttribPointer(location, tupleSize, type, GL_TRUE,
                              stride, values);
    }
}

/*!
    \overload

    Sets an array of vertex \a values on the attribute called \a name
    in this shader program.  The \a tupleSize indicates the number of
    components per vertex (1, 2, 3, or 4), and the \a stride indicates
    the number of bytes between vertices.  A default \a stride value
    of zero indicates that the vertices are densely packed in \a values.

    The array will become active when enableAttributeArray() is called
    on \a name.  Otherwise the value specified with setAttributeValue()
    for \a name will be used.

    \sa setAttributeValue(), setUniformValue(), enableAttributeArray()
    \sa disableAttributeArray()
*/
void QOpenGLShaderProgram::setAttributeArray
    (const char *name, const GLfloat *values, int tupleSize, int stride)
{
    setAttributeArray(attributeLocation(name), values, tupleSize, stride);
}

/*!
    \overload

    Sets an array of 2D vertex \a values on the attribute called \a name
    in this shader program.  The \a stride indicates the number of bytes
    between vertices.  A default \a stride value of zero indicates that
    the vertices are densely packed in \a values.

    The array will become active when enableAttributeArray() is called
    on \a name.  Otherwise the value specified with setAttributeValue()
    for \a name will be used.

    \sa setAttributeValue(), setUniformValue(), enableAttributeArray()
    \sa disableAttributeArray()
*/
void QOpenGLShaderProgram::setAttributeArray
        (const char *name, const QVector2D *values, int stride)
{
    setAttributeArray(attributeLocation(name), values, stride);
}

/*!
    \overload

    Sets an array of 3D vertex \a values on the attribute called \a name
    in this shader program.  The \a stride indicates the number of bytes
    between vertices.  A default \a stride value of zero indicates that
    the vertices are densely packed in \a values.

    The array will become active when enableAttributeArray() is called
    on \a name.  Otherwise the value specified with setAttributeValue()
    for \a name will be used.

    \sa setAttributeValue(), setUniformValue(), enableAttributeArray()
    \sa disableAttributeArray()
*/
void QOpenGLShaderProgram::setAttributeArray
        (const char *name, const QVector3D *values, int stride)
{
    setAttributeArray(attributeLocation(name), values, stride);
}

/*!
    \overload

    Sets an array of 4D vertex \a values on the attribute called \a name
    in this shader program.  The \a stride indicates the number of bytes
    between vertices.  A default \a stride value of zero indicates that
    the vertices are densely packed in \a values.

    The array will become active when enableAttributeArray() is called
    on \a name.  Otherwise the value specified with setAttributeValue()
    for \a name will be used.

    \sa setAttributeValue(), setUniformValue(), enableAttributeArray()
    \sa disableAttributeArray()
*/
void QOpenGLShaderProgram::setAttributeArray
        (const char *name, const QVector4D *values, int stride)
{
    setAttributeArray(attributeLocation(name), values, stride);
}

/*!
    \overload

    Sets an array of vertex \a values on the attribute called \a name
    in this shader program.  The \a stride indicates the number of bytes
    between vertices.  A default \a stride value of zero indicates that
    the vertices are densely packed in \a values.

    The \a type indicates the type of elements in the \a values array,
    usually \c{GL_FLOAT}, \c{GL_UNSIGNED_BYTE}, etc.  The \a tupleSize
    indicates the number of components per vertex: 1, 2, 3, or 4.

    The array will become active when enableAttributeArray() is called
    on the \a name.  Otherwise the value specified with
    setAttributeValue() for \a name will be used.

    The setAttributeBuffer() function can be used to set the attribute
    array to an offset within a vertex buffer.

    \sa setAttributeValue(), setUniformValue(), enableAttributeArray()
    \sa disableAttributeArray(), setAttributeBuffer()
*/
void QOpenGLShaderProgram::setAttributeArray
    (const char *name, GLenum type, const void *values, int tupleSize, int stride)
{
    setAttributeArray(attributeLocation(name), type, values, tupleSize, stride);
}

/*!
    Sets an array of vertex values on the attribute at \a location in
    this shader program, starting at a specific \a offset in the
    currently bound vertex buffer.  The \a stride indicates the number
    of bytes between vertices.  A default \a stride value of zero
    indicates that the vertices are densely packed in the value array.

    The \a type indicates the type of elements in the vertex value
    array, usually \c{GL_FLOAT}, \c{GL_UNSIGNED_BYTE}, etc.  The \a
    tupleSize indicates the number of components per vertex: 1, 2, 3,
    or 4.

    The array will become active when enableAttributeArray() is called
    on the \a location.  Otherwise the value specified with
    setAttributeValue() for \a location will be used.

    \note Normalization will be enabled. If this is not desired, call
    glVertexAttribPointer directly through QOpenGLFunctions.

    \sa setAttributeArray()
*/
void QOpenGLShaderProgram::setAttributeBuffer
    (int location, GLenum type, int offset, int tupleSize, int stride)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        d->glfuncs->glVertexAttribPointer(location, tupleSize, type, GL_TRUE, stride,
                              reinterpret_cast<const void *>(offset));
    }
}

/*!
    \overload

    Sets an array of vertex values on the attribute called \a name
    in this shader program, starting at a specific \a offset in the
    currently bound vertex buffer.  The \a stride indicates the number
    of bytes between vertices.  A default \a stride value of zero
    indicates that the vertices are densely packed in the value array.

    The \a type indicates the type of elements in the vertex value
    array, usually \c{GL_FLOAT}, \c{GL_UNSIGNED_BYTE}, etc.  The \a
    tupleSize indicates the number of components per vertex: 1, 2, 3,
    or 4.

    The array will become active when enableAttributeArray() is called
    on the \a name.  Otherwise the value specified with
    setAttributeValue() for \a name will be used.

    \sa setAttributeArray()
*/
void QOpenGLShaderProgram::setAttributeBuffer
    (const char *name, GLenum type, int offset, int tupleSize, int stride)
{
    setAttributeBuffer(attributeLocation(name), type, offset, tupleSize, stride);
}

/*!
    Enables the vertex array at \a location in this shader program
    so that the value set by setAttributeArray() on \a location
    will be used by the shader program.

    \sa disableAttributeArray(), setAttributeArray(), setAttributeValue()
    \sa setUniformValue()
*/
void QOpenGLShaderProgram::enableAttributeArray(int location)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glEnableVertexAttribArray(location);
}

/*!
    \overload

    Enables the vertex array called \a name in this shader program
    so that the value set by setAttributeArray() on \a name
    will be used by the shader program.

    \sa disableAttributeArray(), setAttributeArray(), setAttributeValue()
    \sa setUniformValue()
*/
void QOpenGLShaderProgram::enableAttributeArray(const char *name)
{
    enableAttributeArray(attributeLocation(name));
}

/*!
    Disables the vertex array at \a location in this shader program
    that was enabled by a previous call to enableAttributeArray().

    \sa enableAttributeArray(), setAttributeArray(), setAttributeValue()
    \sa setUniformValue()
*/
void QOpenGLShaderProgram::disableAttributeArray(int location)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glDisableVertexAttribArray(location);
}

/*!
    \overload

    Disables the vertex array called \a name in this shader program
    that was enabled by a previous call to enableAttributeArray().

    \sa enableAttributeArray(), setAttributeArray(), setAttributeValue()
    \sa setUniformValue()
*/
void QOpenGLShaderProgram::disableAttributeArray(const char *name)
{
    disableAttributeArray(attributeLocation(name));
}

/*!
    Returns the location of the uniform variable \a name within this shader
    program's parameter list.  Returns -1 if \a name is not a valid
    uniform variable for this shader program.

    \sa attributeLocation()
*/
int QOpenGLShaderProgram::uniformLocation(const char *name) const
{
    Q_D(const QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (d->linked && d->programGuard && d->programGuard->id()) {
        return d->glfuncs->glGetUniformLocation(d->programGuard->id(), name);
    } else {
        qWarning() << "QOpenGLShaderProgram::uniformLocation(" << name
                   << "): shader program is not linked";
        return -1;
    }
}

/*!
    \overload

    Returns the location of the uniform variable \a name within this shader
    program's parameter list.  Returns -1 if \a name is not a valid
    uniform variable for this shader program.

    \sa attributeLocation()
*/
int QOpenGLShaderProgram::uniformLocation(const QByteArray& name) const
{
    return uniformLocation(name.constData());
}

/*!
    \overload

    Returns the location of the uniform variable \a name within this shader
    program's parameter list.  Returns -1 if \a name is not a valid
    uniform variable for this shader program.

    \sa attributeLocation()
*/
int QOpenGLShaderProgram::uniformLocation(const QString& name) const
{
    return uniformLocation(name.toLatin1().constData());
}

/*!
    Sets the uniform variable at \a location in the current context to \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, GLfloat value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glUniform1fv(location, 1, &value);
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, GLfloat value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable at \a location in the current context to \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, GLint value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glUniform1i(location, value);
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, GLint value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable at \a location in the current context to \a value.
    This function should be used when setting sampler values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, GLuint value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glUniform1i(location, value);
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to \a value.  This function should be used when setting sampler values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, GLuint value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable at \a location in the current context to
    the 2D vector (\a x, \a y).

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, GLfloat x, GLfloat y)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        GLfloat values[2] = {x, y};
        d->glfuncs->glUniform2fv(location, 1, values);
    }
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context to
    the 2D vector (\a x, \a y).

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, GLfloat x, GLfloat y)
{
    setUniformValue(uniformLocation(name), x, y);
}

/*!
    Sets the uniform variable at \a location in the current context to
    the 3D vector (\a x, \a y, \a z).

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue
        (int location, GLfloat x, GLfloat y, GLfloat z)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        GLfloat values[3] = {x, y, z};
        d->glfuncs->glUniform3fv(location, 1, values);
    }
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context to
    the 3D vector (\a x, \a y, \a z).

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue
        (const char *name, GLfloat x, GLfloat y, GLfloat z)
{
    setUniformValue(uniformLocation(name), x, y, z);
}

/*!
    Sets the uniform variable at \a location in the current context to
    the 4D vector (\a x, \a y, \a z, \a w).

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue
        (int location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        GLfloat values[4] = {x, y, z, w};
        d->glfuncs->glUniform4fv(location, 1, values);
    }
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context to
    the 4D vector (\a x, \a y, \a z, \a w).

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue
        (const char *name, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    setUniformValue(uniformLocation(name), x, y, z, w);
}

/*!
    Sets the uniform variable at \a location in the current context to \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QVector2D& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glUniform2fv(location, 1, reinterpret_cast<const GLfloat *>(&value));
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QVector2D& value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable at \a location in the current context to \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QVector3D& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glUniform3fv(location, 1, reinterpret_cast<const GLfloat *>(&value));
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QVector3D& value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable at \a location in the current context to \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QVector4D& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glUniform4fv(location, 1, reinterpret_cast<const GLfloat *>(&value));
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QVector4D& value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable at \a location in the current context to
    the red, green, blue, and alpha components of \a color.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QColor& color)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        GLfloat values[4] = {GLfloat(color.redF()), GLfloat(color.greenF()),
                             GLfloat(color.blueF()), GLfloat(color.alphaF())};
        d->glfuncs->glUniform4fv(location, 1, values);
    }
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context to
    the red, green, blue, and alpha components of \a color.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QColor& color)
{
    setUniformValue(uniformLocation(name), color);
}

/*!
    Sets the uniform variable at \a location in the current context to
    the x and y coordinates of \a point.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QPoint& point)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        GLfloat values[4] = {GLfloat(point.x()), GLfloat(point.y())};
        d->glfuncs->glUniform2fv(location, 1, values);
    }
}

/*!
    \overload

    Sets the uniform variable associated with \a name in the current
    context to the x and y coordinates of \a point.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QPoint& point)
{
    setUniformValue(uniformLocation(name), point);
}

/*!
    Sets the uniform variable at \a location in the current context to
    the x and y coordinates of \a point.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QPointF& point)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        GLfloat values[4] = {GLfloat(point.x()), GLfloat(point.y())};
        d->glfuncs->glUniform2fv(location, 1, values);
    }
}

/*!
    \overload

    Sets the uniform variable associated with \a name in the current
    context to the x and y coordinates of \a point.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QPointF& point)
{
    setUniformValue(uniformLocation(name), point);
}

/*!
    Sets the uniform variable at \a location in the current context to
    the width and height of the given \a size.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QSize& size)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        GLfloat values[4] = {GLfloat(size.width()), GLfloat(size.height())};
        d->glfuncs->glUniform2fv(location, 1, values);
    }
}

/*!
    \overload

    Sets the uniform variable associated with \a name in the current
    context to the width and height of the given \a size.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QSize& size)
{
    setUniformValue(uniformLocation(name), size);
}

/*!
    Sets the uniform variable at \a location in the current context to
    the width and height of the given \a size.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QSizeF& size)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        GLfloat values[4] = {GLfloat(size.width()), GLfloat(size.height())};
        d->glfuncs->glUniform2fv(location, 1, values);
    }
}

/*!
    \overload

    Sets the uniform variable associated with \a name in the current
    context to the width and height of the given \a size.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QSizeF& size)
{
    setUniformValue(uniformLocation(name), size);
}

/*!
    Sets the uniform variable at \a location in the current context
    to a 2x2 matrix \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QMatrix2x2& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    d->glfuncs->glUniformMatrix2fv(location, 1, GL_FALSE, value.constData());
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to a 2x2 matrix \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QMatrix2x2& value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable at \a location in the current context
    to a 2x3 matrix \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QMatrix2x3& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    d->glfuncs->glUniform3fv(location, 2, value.constData());
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to a 2x3 matrix \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QMatrix2x3& value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable at \a location in the current context
    to a 2x4 matrix \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QMatrix2x4& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    d->glfuncs->glUniform4fv(location, 2, value.constData());
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to a 2x4 matrix \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QMatrix2x4& value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable at \a location in the current context
    to a 3x2 matrix \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QMatrix3x2& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    d->glfuncs->glUniform2fv(location, 3, value.constData());
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to a 3x2 matrix \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QMatrix3x2& value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable at \a location in the current context
    to a 3x3 matrix \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QMatrix3x3& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    d->glfuncs->glUniformMatrix3fv(location, 1, GL_FALSE, value.constData());
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to a 3x3 matrix \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QMatrix3x3& value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable at \a location in the current context
    to a 3x4 matrix \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QMatrix3x4& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    d->glfuncs->glUniform4fv(location, 3, value.constData());
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to a 3x4 matrix \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QMatrix3x4& value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable at \a location in the current context
    to a 4x2 matrix \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QMatrix4x2& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    d->glfuncs->glUniform2fv(location, 4, value.constData());
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to a 4x2 matrix \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QMatrix4x2& value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable at \a location in the current context
    to a 4x3 matrix \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QMatrix4x3& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    d->glfuncs->glUniform3fv(location, 4, value.constData());
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to a 4x3 matrix \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QMatrix4x3& value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable at \a location in the current context
    to a 4x4 matrix \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QMatrix4x4& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    d->glfuncs->glUniformMatrix4fv(location, 1, GL_FALSE, value.constData());
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to a 4x4 matrix \a value.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const QMatrix4x4& value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    \overload

    Sets the uniform variable at \a location in the current context
    to a 2x2 matrix \a value.  The matrix elements must be specified
    in column-major order.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const GLfloat value[2][2])
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glUniformMatrix2fv(location, 1, GL_FALSE, value[0]);
}

/*!
    \overload

    Sets the uniform variable at \a location in the current context
    to a 3x3 matrix \a value.  The matrix elements must be specified
    in column-major order.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const GLfloat value[3][3])
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glUniformMatrix3fv(location, 1, GL_FALSE, value[0]);
}

/*!
    \overload

    Sets the uniform variable at \a location in the current context
    to a 4x4 matrix \a value.  The matrix elements must be specified
    in column-major order.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(int location, const GLfloat value[4][4])
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glUniformMatrix4fv(location, 1, GL_FALSE, value[0]);
}


/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to a 2x2 matrix \a value.  The matrix elements must be specified
    in column-major order.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const GLfloat value[2][2])
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to a 3x3 matrix \a value.  The matrix elements must be specified
    in column-major order.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const GLfloat value[3][3])
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context
    to a 4x4 matrix \a value.  The matrix elements must be specified
    in column-major order.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValue(const char *name, const GLfloat value[4][4])
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable at \a location in the current context to a
    3x3 transformation matrix \a value that is specified as a QTransform value.

    To set a QTransform value as a 4x4 matrix in a shader, use
    \c{setUniformValue(location, QMatrix4x4(value))}.
*/
void QOpenGLShaderProgram::setUniformValue(int location, const QTransform& value)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        GLfloat mat[3][3] = {
            {GLfloat(value.m11()), GLfloat(value.m12()), GLfloat(value.m13())},
            {GLfloat(value.m21()), GLfloat(value.m22()), GLfloat(value.m23())},
            {GLfloat(value.m31()), GLfloat(value.m32()), GLfloat(value.m33())}
        };
        d->glfuncs->glUniformMatrix3fv(location, 1, GL_FALSE, mat[0]);
    }
}

/*!
    \overload

    Sets the uniform variable called \a name in the current context to a
    3x3 transformation matrix \a value that is specified as a QTransform value.

    To set a QTransform value as a 4x4 matrix in a shader, use
    \c{setUniformValue(name, QMatrix4x4(value))}.
*/
void QOpenGLShaderProgram::setUniformValue
        (const char *name, const QTransform& value)
{
    setUniformValue(uniformLocation(name), value);
}

/*!
    Sets the uniform variable array at \a location in the current
    context to the \a count elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(int location, const GLint *values, int count)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glUniform1iv(location, count, values);
}

/*!
    \overload

    Sets the uniform variable array called \a name in the current
    context to the \a count elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray
        (const char *name, const GLint *values, int count)
{
    setUniformValueArray(uniformLocation(name), values, count);
}

/*!
    Sets the uniform variable array at \a location in the current
    context to the \a count elements of \a values.  This overload
    should be used when setting an array of sampler values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(int location, const GLuint *values, int count)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glUniform1iv(location, count, reinterpret_cast<const GLint *>(values));
}

/*!
    \overload

    Sets the uniform variable array called \a name in the current
    context to the \a count elements of \a values.  This overload
    should be used when setting an array of sampler values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray
        (const char *name, const GLuint *values, int count)
{
    setUniformValueArray(uniformLocation(name), values, count);
}

/*!
    Sets the uniform variable array at \a location in the current
    context to the \a count elements of \a values.  Each element
    has \a tupleSize components.  The \a tupleSize must be 1, 2, 3, or 4.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(int location, const GLfloat *values, int count, int tupleSize)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1) {
        if (tupleSize == 1)
            d->glfuncs->glUniform1fv(location, count, values);
        else if (tupleSize == 2)
            d->glfuncs->glUniform2fv(location, count, values);
        else if (tupleSize == 3)
            d->glfuncs->glUniform3fv(location, count, values);
        else if (tupleSize == 4)
            d->glfuncs->glUniform4fv(location, count, values);
        else
            qWarning() << "QOpenGLShaderProgram::setUniformValue: size" << tupleSize << "not supported";
    }
}

/*!
    \overload

    Sets the uniform variable array called \a name in the current
    context to the \a count elements of \a values.  Each element
    has \a tupleSize components.  The \a tupleSize must be 1, 2, 3, or 4.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray
        (const char *name, const GLfloat *values, int count, int tupleSize)
{
    setUniformValueArray(uniformLocation(name), values, count, tupleSize);
}

/*!
    Sets the uniform variable array at \a location in the current
    context to the \a count 2D vector elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(int location, const QVector2D *values, int count)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glUniform2fv(location, count, reinterpret_cast<const GLfloat *>(values));
}

/*!
    \overload

    Sets the uniform variable array called \a name in the current
    context to the \a count 2D vector elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(const char *name, const QVector2D *values, int count)
{
    setUniformValueArray(uniformLocation(name), values, count);
}

/*!
    Sets the uniform variable array at \a location in the current
    context to the \a count 3D vector elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(int location, const QVector3D *values, int count)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glUniform3fv(location, count, reinterpret_cast<const GLfloat *>(values));
}

/*!
    \overload

    Sets the uniform variable array called \a name in the current
    context to the \a count 3D vector elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(const char *name, const QVector3D *values, int count)
{
    setUniformValueArray(uniformLocation(name), values, count);
}

/*!
    Sets the uniform variable array at \a location in the current
    context to the \a count 4D vector elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(int location, const QVector4D *values, int count)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    if (location != -1)
        d->glfuncs->glUniform4fv(location, count, reinterpret_cast<const GLfloat *>(values));
}

/*!
    \overload

    Sets the uniform variable array called \a name in the current
    context to the \a count 4D vector elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(const char *name, const QVector4D *values, int count)
{
    setUniformValueArray(uniformLocation(name), values, count);
}

// We have to repack matrix arrays from qreal to GLfloat.
#define setUniformMatrixArray(func,location,values,count,type,cols,rows) \
    if (location == -1 || count <= 0) \
        return; \
    if (sizeof(type) == sizeof(GLfloat) * cols * rows) { \
        func(location, count, GL_FALSE, \
             reinterpret_cast<const GLfloat *>(values[0].constData())); \
    } else { \
        QVarLengthArray<GLfloat> temp(cols * rows * count); \
        for (int index = 0; index < count; ++index) { \
            for (int index2 = 0; index2 < (cols * rows); ++index2) { \
                temp.data()[cols * rows * index + index2] = \
                    values[index].constData()[index2]; \
            } \
        } \
        func(location, count, GL_FALSE, temp.constData()); \
    }
#define setUniformGenericMatrixArray(colfunc,location,values,count,type,cols,rows) \
    if (location == -1 || count <= 0) \
        return; \
    if (sizeof(type) == sizeof(GLfloat) * cols * rows) { \
        const GLfloat *data = reinterpret_cast<const GLfloat *> \
            (values[0].constData());  \
        colfunc(location, count * cols, data); \
    } else { \
        QVarLengthArray<GLfloat> temp(cols * rows * count); \
        for (int index = 0; index < count; ++index) { \
            for (int index2 = 0; index2 < (cols * rows); ++index2) { \
                temp.data()[cols * rows * index + index2] = \
                    values[index].constData()[index2]; \
            } \
        } \
        colfunc(location, count * cols, temp.constData()); \
    }

/*!
    Sets the uniform variable array at \a location in the current
    context to the \a count 2x2 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(int location, const QMatrix2x2 *values, int count)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    setUniformMatrixArray
        (d->glfuncs->glUniformMatrix2fv, location, values, count, QMatrix2x2, 2, 2);
}

/*!
    \overload

    Sets the uniform variable array called \a name in the current
    context to the \a count 2x2 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(const char *name, const QMatrix2x2 *values, int count)
{
    setUniformValueArray(uniformLocation(name), values, count);
}

/*!
    Sets the uniform variable array at \a location in the current
    context to the \a count 2x3 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(int location, const QMatrix2x3 *values, int count)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    setUniformGenericMatrixArray
        (d->glfuncs->glUniform3fv, location, values, count,
         QMatrix2x3, 2, 3);
}

/*!
    \overload

    Sets the uniform variable array called \a name in the current
    context to the \a count 2x3 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(const char *name, const QMatrix2x3 *values, int count)
{
    setUniformValueArray(uniformLocation(name), values, count);
}

/*!
    Sets the uniform variable array at \a location in the current
    context to the \a count 2x4 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(int location, const QMatrix2x4 *values, int count)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    setUniformGenericMatrixArray
        (d->glfuncs->glUniform4fv, location, values, count,
         QMatrix2x4, 2, 4);
}

/*!
    \overload

    Sets the uniform variable array called \a name in the current
    context to the \a count 2x4 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(const char *name, const QMatrix2x4 *values, int count)
{
    setUniformValueArray(uniformLocation(name), values, count);
}

/*!
    Sets the uniform variable array at \a location in the current
    context to the \a count 3x2 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(int location, const QMatrix3x2 *values, int count)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    setUniformGenericMatrixArray
        (d->glfuncs->glUniform2fv, location, values, count,
         QMatrix3x2, 3, 2);
}

/*!
    \overload

    Sets the uniform variable array called \a name in the current
    context to the \a count 3x2 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(const char *name, const QMatrix3x2 *values, int count)
{
    setUniformValueArray(uniformLocation(name), values, count);
}

/*!
    Sets the uniform variable array at \a location in the current
    context to the \a count 3x3 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(int location, const QMatrix3x3 *values, int count)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    setUniformMatrixArray
        (d->glfuncs->glUniformMatrix3fv, location, values, count, QMatrix3x3, 3, 3);
}

/*!
    \overload

    Sets the uniform variable array called \a name in the current
    context to the \a count 3x3 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(const char *name, const QMatrix3x3 *values, int count)
{
    setUniformValueArray(uniformLocation(name), values, count);
}

/*!
    Sets the uniform variable array at \a location in the current
    context to the \a count 3x4 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(int location, const QMatrix3x4 *values, int count)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    setUniformGenericMatrixArray
        (d->glfuncs->glUniform4fv, location, values, count,
         QMatrix3x4, 3, 4);
}

/*!
    \overload

    Sets the uniform variable array called \a name in the current
    context to the \a count 3x4 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(const char *name, const QMatrix3x4 *values, int count)
{
    setUniformValueArray(uniformLocation(name), values, count);
}

/*!
    Sets the uniform variable array at \a location in the current
    context to the \a count 4x2 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(int location, const QMatrix4x2 *values, int count)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    setUniformGenericMatrixArray
        (d->glfuncs->glUniform2fv, location, values, count,
         QMatrix4x2, 4, 2);
}

/*!
    \overload

    Sets the uniform variable array called \a name in the current
    context to the \a count 4x2 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(const char *name, const QMatrix4x2 *values, int count)
{
    setUniformValueArray(uniformLocation(name), values, count);
}

/*!
    Sets the uniform variable array at \a location in the current
    context to the \a count 4x3 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(int location, const QMatrix4x3 *values, int count)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    setUniformGenericMatrixArray
        (d->glfuncs->glUniform3fv, location, values, count,
         QMatrix4x3, 4, 3);
}

/*!
    \overload

    Sets the uniform variable array called \a name in the current
    context to the \a count 4x3 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(const char *name, const QMatrix4x3 *values, int count)
{
    setUniformValueArray(uniformLocation(name), values, count);
}

/*!
    Sets the uniform variable array at \a location in the current
    context to the \a count 4x4 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(int location, const QMatrix4x4 *values, int count)
{
    Q_D(QOpenGLShaderProgram);
    Q_UNUSED(d);
    setUniformMatrixArray
        (d->glfuncs->glUniformMatrix4fv, location, values, count, QMatrix4x4, 4, 4);
}

/*!
    \overload

    Sets the uniform variable array called \a name in the current
    context to the \a count 4x4 matrix elements of \a values.

    \sa setAttributeValue()
*/
void QOpenGLShaderProgram::setUniformValueArray(const char *name, const QMatrix4x4 *values, int count)
{
    setUniformValueArray(uniformLocation(name), values, count);
}

/*!
    Returns the hardware limit for how many vertices a geometry shader
    can output.
*/
int QOpenGLShaderProgram::maxGeometryOutputVertices() const
{
    GLint n = 0;
#if defined(QT_OPENGL_3_2)
    Q_D(const QOpenGLShaderProgram);
    d->glfuncs->glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES, &n);
#endif
    return n;
}

/*!
    Use this function to specify to OpenGL the number of vertices in
    a patch to \a count. A patch is a custom OpenGL primitive whose interpretation
    is entirely defined by the tessellation shader stages. Therefore, calling
    this function only makes sense when using a QOpenGLShaderProgram
    containing tessellation stage shaders. When using OpenGL tessellation,
    the only primitive that can be rendered with \c{glDraw*()} functions is
    \c{GL_PATCHES}.

    This is equivalent to calling glPatchParameteri(GL_PATCH_VERTICES, count).

    \note This modifies global OpenGL state and is not specific to this
    QOpenGLShaderProgram instance. You should call this in your render
    function when needed, as QOpenGLShaderProgram will not apply this for
    you. This is purely a convenience function.

    \sa patchVertexCount()
*/
void QOpenGLShaderProgram::setPatchVertexCount(int count)
{
#if defined(QT_OPENGL_4)
    Q_D(QOpenGLShaderProgram);
    if (d->tessellationFuncs)
        d->tessellationFuncs->glPatchParameteri(GL_PATCH_VERTICES, count);
#else
    Q_UNUSED(count);
#endif
}

/*!
    Returns the number of vertices per-patch to be used when rendering.

    \note This returns the global OpenGL state value. It is not specific to
    this QOpenGLShaderProgram instance.

    \sa setPatchVertexCount()
*/
int QOpenGLShaderProgram::patchVertexCount() const
{
    int patchVertices = 0;
#if defined(QT_OPENGL_4)
    Q_D(const QOpenGLShaderProgram);
    if (d->tessellationFuncs)
        d->tessellationFuncs->glGetIntegerv(GL_PATCH_VERTICES, &patchVertices);
#endif
    return patchVertices;
}

/*!
    Sets the default outer tessellation levels to be used by the tessellation
    primitive generator in the event that the tessellation control shader
    does not output them to \a levels. For more details on OpenGL and Tessellation
    shaders see \l{OpenGL Tessellation Shaders}.

    The \a levels argument should be a QVector consisting of 4 floats. Not all
    of the values make sense for all tessellation modes. If you specify a vector with
    fewer than 4 elements, the remaining elements will be given a default value of 1.

    \note This modifies global OpenGL state and is not specific to this
    QOpenGLShaderProgram instance. You should call this in your render
    function when needed, as QOpenGLShaderProgram will not apply this for
    you. This is purely a convenience function.

    \sa defaultOuterTessellationLevels(), setDefaultInnerTessellationLevels()
*/
void QOpenGLShaderProgram::setDefaultOuterTessellationLevels(const QVector<float> &levels)
{
#if defined(QT_OPENGL_4)
    QVector<float> tessLevels = levels;

    // Ensure we have the required 4 outer tessellation levels
    // Use default of 1 for missing entries (same as spec)
    const int argCount = 4;
    if (tessLevels.size() < argCount) {
        tessLevels.reserve(argCount);
        for (int i = tessLevels.size(); i < argCount; ++i)
            tessLevels.append(1.0f);
    }

    Q_D(QOpenGLShaderProgram);
    if (d->tessellationFuncs)
        d->tessellationFuncs->glPatchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL, tessLevels.data());
#else
    Q_UNUSED(levels);
#endif
}

/*!
    Returns the default outer tessellation levels to be used by the tessellation
    primitive generator in the event that the tessellation control shader
    does not output them. For more details on OpenGL and Tessellation shaders see
    \l{OpenGL Tessellation Shaders}.

    Returns a QVector of floats describing the outer tessellation levels. The vector
    will always have four elements but not all of them make sense for every mode
    of tessellation.

    \note This returns the global OpenGL state value. It is not specific to
    this QOpenGLShaderProgram instance.

    \sa setDefaultOuterTessellationLevels(), defaultInnerTessellationLevels()
*/
QVector<float> QOpenGLShaderProgram::defaultOuterTessellationLevels() const
{
    QVector<float> tessLevels(4, 1.0f);
#if defined(QT_OPENGL_4)
    Q_D(const QOpenGLShaderProgram);
    if (d->tessellationFuncs)
        d->tessellationFuncs->glGetFloatv(GL_PATCH_DEFAULT_OUTER_LEVEL, tessLevels.data());
#endif
    return tessLevels;
}

/*!
    Sets the default outer tessellation levels to be used by the tessellation
    primitive generator in the event that the tessellation control shader
    does not output them to \a levels. For more details on OpenGL and Tessellation shaders see
    \l{OpenGL Tessellation Shaders}.

    The \a levels argument should be a QVector consisting of 2 floats. Not all
    of the values make sense for all tessellation modes. If you specify a vector with
    fewer than 2 elements, the remaining elements will be given a default value of 1.

    \note This modifies global OpenGL state and is not specific to this
    QOpenGLShaderProgram instance. You should call this in your render
    function when needed, as QOpenGLShaderProgram will not apply this for
    you. This is purely a convenience function.

    \sa defaultInnerTessellationLevels(), setDefaultOuterTessellationLevels()
*/
void QOpenGLShaderProgram::setDefaultInnerTessellationLevels(const QVector<float> &levels)
{
#if defined(QT_OPENGL_4)
    QVector<float> tessLevels = levels;

    // Ensure we have the required 2 inner tessellation levels
    // Use default of 1 for missing entries (same as spec)
    const int argCount = 2;
    if (tessLevels.size() < argCount) {
        tessLevels.reserve(argCount);
        for (int i = tessLevels.size(); i < argCount; ++i)
            tessLevels.append(1.0f);
    }

    Q_D(QOpenGLShaderProgram);
    if (d->tessellationFuncs)
        d->tessellationFuncs->glPatchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL, tessLevels.data());
#else
    Q_UNUSED(levels);
#endif
}

/*!
    Returns the default inner tessellation levels to be used by the tessellation
    primitive generator in the event that the tessellation control shader
    does not output them. For more details on OpenGL and Tessellation shaders see
    \l{OpenGL Tessellation Shaders}.

    Returns a QVector of floats describing the inner tessellation levels. The vector
    will always have two elements but not all of them make sense for every mode
    of tessellation.

    \note This returns the global OpenGL state value. It is not specific to
    this QOpenGLShaderProgram instance.

    \sa setDefaultInnerTessellationLevels(), defaultOuterTessellationLevels()
*/
QVector<float> QOpenGLShaderProgram::defaultInnerTessellationLevels() const
{
    QVector<float> tessLevels(2, 1.0f);
#if defined(QT_OPENGL_4)
    Q_D(const QOpenGLShaderProgram);
    if (d->tessellationFuncs)
        d->tessellationFuncs->glGetFloatv(GL_PATCH_DEFAULT_OUTER_LEVEL, tessLevels.data());
#endif
    return tessLevels;
}


/*!
    Returns \c true if shader programs written in the OpenGL Shading
    Language (GLSL) are supported on this system; false otherwise.

    The \a context is used to resolve the GLSL extensions.
    If \a context is null, then QOpenGLContext::currentContext() is used.
*/
bool QOpenGLShaderProgram::hasOpenGLShaderPrograms(QOpenGLContext *context)
{
#if !defined(QT_OPENGL_ES_2)
    if (!context)
        context = QOpenGLContext::currentContext();
    if (!context)
        return false;
    return QOpenGLFunctions(context).hasOpenGLFeature(QOpenGLFunctions::Shaders);
#else
    Q_UNUSED(context);
    return true;
#endif
}

/*!
    \internal
*/
void QOpenGLShaderProgram::shaderDestroyed()
{
    Q_D(QOpenGLShaderProgram);
    QOpenGLShader *shader = qobject_cast<QOpenGLShader *>(sender());
    if (shader && !d->removingShaders)
        removeShader(shader);
}

/*!
    Returns \c true if shader programs of type \a type are supported on
    this system; false otherwise.

    The \a context is used to resolve the GLSL extensions.
    If \a context is null, then QOpenGLContext::currentContext() is used.
*/
bool QOpenGLShader::hasOpenGLShaders(ShaderType type, QOpenGLContext *context)
{
    if (!context)
        context = QOpenGLContext::currentContext();
    if (!context)
        return false;

    if ((type & ~(Geometry | Vertex | Fragment | TessellationControl | TessellationEvaluation | Compute)) || type == 0)
        return false;

    QSurfaceFormat format = context->format();
    if (type == Geometry) {
#ifndef QT_OPENGL_ES_2
        // Geometry shaders require OpenGL 3.2 or newer
        QSurfaceFormat format = context->format();
        return (!context->isOpenGLES())
            && (format.version() >= qMakePair<int, int>(3, 2));
#else
        // No geometry shader support in OpenGL ES2
        return false;
#endif
    } else if (type == TessellationControl || type == TessellationEvaluation) {
#if !defined(QT_OPENGL_ES_2)
        return (!context->isOpenGLES())
            && (format.version() >= qMakePair<int, int>(4, 0));
#else
        // No tessellation shader support in OpenGL ES2
        return false;
#endif
    } else if (type == Compute) {
#if defined(QT_OPENGL_4_3)
        return (format.version() >= qMakePair<int, int>(4, 3));
#else
        // No compute shader support without OpenGL 4.3 or newer
        return false;
#endif
    }

    // Unconditional support of vertex and fragment shaders implicitly assumes
    // a minimum OpenGL version of 2.0
    return true;
}

QT_END_NAMESPACE
