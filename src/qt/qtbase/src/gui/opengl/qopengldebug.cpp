/****************************************************************************
**
** Copyright (C) 2013 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
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

#include <QtCore/private/qobject_p.h>
#include <QtCore/qglobal.h>
#include <QtCore/qvarlengtharray.h>
#include <QtGui/qopengl.h>
#include <QtGui/qopenglfunctions.h>

#include "qopengldebug.h"

QT_BEGIN_NAMESPACE

/*!
    \class QOpenGLDebugMessage
    \brief The QOpenGLDebugMessage class wraps an OpenGL debug message.
    \inmodule QtGui
    \reentrant
    \since 5.1
    \ingroup shared
    \ingroup painting-3D

    Debug messages are usually created by the OpenGL server and then read by
    OpenGL clients (either from the OpenGL internal debug log, or logged in real-time).
    A debug message has a textual representation, a vendor-specific numeric id,
    a source, a type and a severity.

    It's also possible for applications or third-party libraries and toolkits
    to create and insert messages in the debug log. In order to do so, you can use
    the createApplicationMessage() or the createThirdPartyMessage() static functions.

    \sa QOpenGLDebugLogger
*/

/*!
    \class QOpenGLDebugLogger
    \brief The QOpenGLDebugLogger enables logging of OpenGL debugging messages.
    \inmodule QtGui
    \since 5.1
    \ingroup painting-3D

    \tableofcontents

    \section1 Introduction

    OpenGL programming can be very error prone. Most of the time, a single
    failing call to OpenGL can cause an entire portion of an application to
    stop working, with nothing being drawn on the screen.

    The only way to be sure that no errors are being returned from the OpenGL
    implementation is checking with \c{glGetError} after each and every API
    call. Moreover, OpenGL errors stack up, therefore glGetError should always
    be used in a loop like this:

    \code

    GLenum error = GL_NO_ERROR;
    do {
        error = glGetError();
        if (error != GL_NO_ERROR)
            // handle the error
    } while (error != GL_NO_ERROR);

    \endcode

    There are also many other information we are interested in (as application
    developers), for instance performance issues, or warnings about using
    deprecated APIs. Those kind of messages are not reported through the
    ordinary OpenGL error reporting mechanisms.

    QOpenGLDebugLogger aims at addressing these issues by providing access to
    the \e{OpenGL debug log}. If your OpenGL implementation supports it (by
    exposing the \c{GL_KHR_debug} extension), messages from the OpenGL server
    will be either logged in an internal OpenGL log, or passed in "real-time"
    to listeners as they're generated from OpenGL.

    QOpenGLDebugLogger supports both these modes of operation. Refer to the
    following sections to find out the differences between them.

    \section1 Creating an OpenGL debug context

    For efficiency reasons, OpenGL implementations are allowed not to create
    any debug output at all, unless the OpenGL context is a debug context. In order
    to create a debug context from Qt, you must set the QSurfaceFormat::DebugContext
    format option on the QSurfaceFormat used to create the QOpenGLContext object:

    \code

    QSurfaceFormat format;
    // asks for a OpenGL 3.2 debug context using the Core profile
    format.setMajorVersion(3);
    format.setMinorVersion(2);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setOption(QSurfaceFormat::DebugContext);

    QOpenGLContext *context = new QOpenGLContext;
    context->setFormat(format);
    context->create();

    \endcode

    Note that requesting a 3.2 OpenGL Core Profile is just for the example's
    purposes; this class is not tied to any specific OpenGL or OpenGL ES
    version, as it relies on the availability of the \c{GL_KHR_debug} extension
    (see below).

    \section1 Creating and initializing a QOpenGLDebugLogger

    QOpenGLDebugLogger is a simple QObject-derived class. Just like all QObject
    subclasses, you create an instance (and optionally specify a parent
    object), and like the other OpenGL functions in Qt you \e{must} initialize
    it before usage by calling initialize() whilst there is a current OpenGL context:

    \code

    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    QOpenGLDebugLogger *logger = new QOpenGLDebugLogger(this);

    logger->initialize(); // initializes in the current context, i.e. ctx

    \endcode

    Note that the \c{GL_KHR_debug} extension \e{must} be available in the context
    in order to access the messages logged by OpenGL. You can check the
    presence of this extension by calling:

    \code

    ctx->hasExtension(QByteArrayLiteral("GL_KHR_debug"))

    \endcode

    where \c{ctx} is a valid QOpenGLContext. If the extension is not available,
    initialize() will return false.

    \section1 Reading the internal OpenGL debug log

    OpenGL implementations keep an internal log of debug messages. Messages
    stored in this log can be retrieved by using the loggedMessages() function:

    \code

    QList<QOpenGLDebugMessage> messages = logger->loggedMessages();
    foreach (const QOpenGLDebugMessage &message, messages)
        qDebug() << message;

    \endcode

    The internal log has a limited size; when it fills up, older messages will
    get discarded to make room for the new incoming messages. When you call
    loggedMessages(), the internal log will be emptied as well.

    If you want to be sure not to lose any debug message, you must use real-time
    logging instead of calling this function. However, debug messages might
    still be generated in the timespan between context creation and activation
    of real-time logging (or, in general, when the real-time logging is disabled).

    \section1 Real-time logging of messages

    It is also possible to receive a stream of debug messages from the OpenGL
    server \e{as they are generated} by the implementation. In order to do so,
    you need to connect a suitable slot to the messageLogged() signal, and
    start logging by calling startLogging():

    \code

    connect(logger, &QOpenGLDebugLogger::messageLogged, receiver, &LogHandler::handleLoggedMessage);
    logger->startLogging();

    \endcode

    Similarly, logging can be disabled at any time by calling the stopLogging()
    function.

    Real-time logging can be either asynchronous or synchronous, depending on
    the parameter passed to startLogging(). When logging in asynchronous mode
    (the default, as it has a very small overhead), the OpenGL implementation
    can generate messages at any time, and/or in an order which is different from the
    order of the OpenGL commands which caused those messages to be logged.
    The messages could also be generated from a thread that it's different from
    the thread the context is currently bound to. This is because OpenGL
    implementations are usually highly threaded and asynchronous, and therefore
    no warranties are made about the relative order and the timings of the
    debug messages.

    On the other hand, logging in synchronous mode has a high overhead, but
    the OpenGL implementation guarantees that all the messages caused by a
    certain command are received in order, before the command returns,
    and from the same thread the OpenGL context is bound to.

    This means that when logging in synchronous mode you will be able to run
    your OpenGL application in a debugger, put a breakpoint on a slot connected
    to the messageLogged() signal, and see in the backtrace the exact call
    that caused the logged message. This can be extremely useful to debug
    an OpenGL problem. Note that if OpenGL rendering is happening in another
    thread, you must force the signal/slot connection type to Qt::DirectConnection
    in order to be able to see the actual backtrace.

    Refer to the LoggingMode enum documentation for more information about
    logging modes.

    \note When real-time logging is enabled, debug messages will \e{not} be
    inserted in the internal OpenGL debug log any more; messages already
    present in the internal log will not be deleted, nor they will be emitted
    through the messageLogged() signal. Since some messages might be generated
    before real-time logging is started (and therefore be kept in the internal
    OpenGL log), it is important to always check if it contains any message
    after calling startLogging().

    \section1 Inserting messages in the debug log

    It is possible for applications and libraries to insert custom messages in
    the debug log, for instance for marking a group of related OpenGL commands
    and therefore being then able to identify eventual messages coming from them.

    In order to do so, you can create a QOpenGLDebugMessage object by calling
    \l{QOpenGLDebugMessage::}{createApplicationMessage()} or
    \l{QOpenGLDebugMessage::}{createThirdPartyMessage()}, and then inserting it
    into the log by calling logMessage():

    \code

    QOpenGLDebugMessage message =
        QOpenGLDebugMessage::createApplicationMessage(QStringLiteral("Custom message"));

    logger->logMessage(message);

    \endcode

    Note that OpenGL implementations have a vendor-specific limit to the length
    of the messages that can be inserted in the debug log. You can retrieve
    this length by calling the maximumMessageLength() method; messages longer
    than the limit will automatically get truncated.

    \section1 Controlling the debug output

    QOpenGLDebugMessage is also able to apply filters to the debug messages, and
    therefore limit the amount of messages logged. You can enable or disable
    logging of messages by calling enableMessages() and disableMessages()
    respectively. By default, all messages are logged.

    It is possible to enable or disable messages by selecting them by:

    \list
    \li source, type and severity (and including all ids in the selection);
    \li id, source and type (and including all severities in the selection).
    \endlist

    Note that the "enabled" status for a given message is a property of the
    (id, source, type, severity) tuple; the message attributes \e{do not} form
    a hierarchy of any kind. You should be careful about the order of the calls
    to enableMessages() and disableMessages(), as it will change which
    messages will are enabled / disabled.

    It's not possible to filter by the message text itself; applications
    have to do that on their own (in slots connected to the messageLogged()
    signal, or after fetching the messages in the internal debug log
    through loggedMessages()).

    In order to simplify the management of the enabled / disabled statuses,
    QOpenGLDebugMessage also supports the concept of \c{debug groups}. A debug
    group contains the group of enabled / disabled configurations of debug
    messages. Moreover, debug groups are organized in a stack: it is possible
    to push and pop groups by calling pushGroup() and popGroup() respectively.
    (When an OpenGL context is created, there is already a group in the stack).

    The enableMessages() and disableMessages() functions will modify the
    configuration in the current debug group, that is, the one at the top of
    the debug groups stack.

    When a new group is pushed onto the debug groups stack, it will inherit
    the configuration of the group that was previously on the top of the stack.
    Vice versa, popping a debug group will restore the configuration of
    the debug group that becomes the new top.

    Pushing (respectively popping) debug groups will also automatically generate
    a debug message of type QOpenGLDebugMessage::GroupPushType (respectively
    \l{QOpenGLDebugMessage::}{GroupPopType}).

    \sa QOpenGLDebugMessage
*/

/*!
    \enum QOpenGLDebugMessage::Source

    The Source enum defines the source of the debug message.

    \value InvalidSource
        The source of the message is invalid; this is the source of a
        default-constructed QOpenGLDebugMessage object.

    \value APISource
        The message was generated in response to OpenGL API calls.

    \value WindowSystemSource
        The message was generated by the window system.

    \value ShaderCompilerSource
        The message was generated by the shader compiler.

    \value ThirdPartySource
        The message was generated by a third party, for instance an OpenGL
        framework a or debugging toolkit.

    \value ApplicationSource
        The message was generated by the application itself.

    \value OtherSource
        The message was generated by a source not included in this
        enumeration.

    \omitvalue LastSource

    \value AnySource
        This value corresponds to a mask of all possible message sources.
*/

/*!
    \enum QOpenGLDebugMessage::Type

    The Type enum defines the type of the debug message.

    \value InvalidType
        The type of the message is invalid; this is the type of a
        default-constructed QOpenGLDebugMessage object.

    \value ErrorType
        The message represents an error.

    \value DeprecatedBehaviorType
        The message represents an usage of deprecated behavior.

    \value UndefinedBehaviorType
        The message represents an usage of undefined behavior.

    \value PortabilityType
        The message represents an usage of vendor-specific behavior,
        that might pose portability concerns.

    \value PerformanceType
        The message represents a performance issue.

    \value OtherType
        The message represents a type not included in this
        enumeration.

    \value MarkerType
        The message represents a marker in the debug log.

    \value GroupPushType
        The message represents a debug group push operation.

    \value GroupPopType
        The message represents a debug group pop operation.

    \omitvalue LastType

    \value AnyType
        This value corresponds to a mask of all possible message types.
*/

/*!
    \enum QOpenGLDebugMessage::Severity

    The Severity enum defines the severity of the debug message.

    \value InvalidSeverity
        The severity of the message is invalid; this is the severity of a
        default-constructed QOpenGLDebugMessage object.

    \value HighSeverity
        The message has a high severity.

    \value MediumSeverity
        The message has a medium severity.

    \value LowSeverity
        The message has a low severity.

    \value NotificationSeverity
        The message is a notification.

    \omitvalue LastSeverity

    \value AnySeverity
        This value corresponds to a mask of all possible message severities.
*/

/*!
    \property QOpenGLDebugLogger::loggingMode

    \brief the logging mode passed to startLogging().

    Note that logging must have been started or the value of this property
    will be meaningless.

    \sa startLogging(), isLogging()
*/
/*!
    \enum QOpenGLDebugLogger::LoggingMode

    The LoggingMode enum defines the logging mode of the logger object.

    \value AsynchronousLogging
        Messages from the OpenGL server are logged asynchronously. This means
        that messages can be logged some time after the corresponding OpenGL
        actions that caused them, and even be received in an out-of-order
        fashion, depending on the OpenGL implementation. This mode has a very low
        performance penalty, as OpenGL implementations are heavily threaded
        and asynchronous by nature.

    \value SynchronousLogging
        Messages from the OpenGL server are logged synchronously and
        sequentially. This has a severe performance hit, as OpenGL
        implementations are very asynchronous by nature; but it's very useful
        to debug OpenGL problems, as OpenGL guarantees that the messages
        generated by a OpenGL command will be logged before the corresponding
        command execution has returned. Therefore, you can install a breakpoint
        on the messageLogged() signal and see in the backtrace which OpenGL
        command caused it; the only caveat is that if you are using OpenGL from
        multiple threads you may need to force direct connection when
        connecting to the messageLogged() signal.
*/

// Under OSX (at least up to 10.8) we cannot include our copy of glext.h,
// but we use the system-wide one, which unfortunately lacks all the needed
// defines/typedefs. In order to make the code compile, we just add here
// the GL_KHR_debug defines.

#ifndef GL_KHR_debug
#define GL_KHR_debug 1

#ifndef GL_DEBUG_OUTPUT_SYNCHRONOUS
#define GL_DEBUG_OUTPUT_SYNCHRONOUS       0x8242
#endif
#ifndef GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH
#define GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH 0x8243
#endif
#ifndef GL_DEBUG_CALLBACK_FUNCTION
#define GL_DEBUG_CALLBACK_FUNCTION        0x8244
#endif
#ifndef GL_DEBUG_CALLBACK_USER_PARAM
#define GL_DEBUG_CALLBACK_USER_PARAM      0x8245
#endif
#ifndef GL_DEBUG_SOURCE_API
#define GL_DEBUG_SOURCE_API               0x8246
#endif
#ifndef GL_DEBUG_SOURCE_WINDOW_SYSTEM
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM     0x8247
#endif
#ifndef GL_DEBUG_SOURCE_SHADER_COMPILER
#define GL_DEBUG_SOURCE_SHADER_COMPILER   0x8248
#endif
#ifndef GL_DEBUG_SOURCE_THIRD_PARTY
#define GL_DEBUG_SOURCE_THIRD_PARTY       0x8249
#endif
#ifndef GL_DEBUG_SOURCE_APPLICATION
#define GL_DEBUG_SOURCE_APPLICATION       0x824A
#endif
#ifndef GL_DEBUG_SOURCE_OTHER
#define GL_DEBUG_SOURCE_OTHER             0x824B
#endif
#ifndef GL_DEBUG_TYPE_ERROR
#define GL_DEBUG_TYPE_ERROR               0x824C
#endif
#ifndef GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR 0x824D
#endif
#ifndef GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR  0x824E
#endif
#ifndef GL_DEBUG_TYPE_PORTABILITY
#define GL_DEBUG_TYPE_PORTABILITY         0x824F
#endif
#ifndef GL_DEBUG_TYPE_PERFORMANCE
#define GL_DEBUG_TYPE_PERFORMANCE         0x8250
#endif
#ifndef GL_DEBUG_TYPE_OTHER
#define GL_DEBUG_TYPE_OTHER               0x8251
#endif
#ifndef GL_DEBUG_TYPE_MARKER
#define GL_DEBUG_TYPE_MARKER              0x8268
#endif
#ifndef GL_DEBUG_TYPE_PUSH_GROUP
#define GL_DEBUG_TYPE_PUSH_GROUP          0x8269
#endif
#ifndef GL_DEBUG_TYPE_POP_GROUP
#define GL_DEBUG_TYPE_POP_GROUP           0x826A
#endif
#ifndef GL_DEBUG_SEVERITY_NOTIFICATION
#define GL_DEBUG_SEVERITY_NOTIFICATION    0x826B
#endif
#ifndef GL_MAX_DEBUG_GROUP_STACK_DEPTH
#define GL_MAX_DEBUG_GROUP_STACK_DEPTH    0x826C
#endif
#ifndef GL_DEBUG_GROUP_STACK_DEPTH
#define GL_DEBUG_GROUP_STACK_DEPTH        0x826D
#endif
#ifndef GL_BUFFER
#define GL_BUFFER                         0x82E0
#endif
#ifndef GL_SHADER
#define GL_SHADER                         0x82E1
#endif
#ifndef GL_PROGRAM
#define GL_PROGRAM                        0x82E2
#endif
#ifndef GL_QUERY
#define GL_QUERY                          0x82E3
#endif
#ifndef GL_PROGRAM_PIPELINE
#define GL_PROGRAM_PIPELINE               0x82E4
#endif
#ifndef GL_SAMPLER
#define GL_SAMPLER                        0x82E6
#endif
#ifndef GL_DISPLAY_LIST
#define GL_DISPLAY_LIST                   0x82E7
#endif
#ifndef GL_MAX_LABEL_LENGTH
#define GL_MAX_LABEL_LENGTH               0x82E8
#endif
#ifndef GL_MAX_DEBUG_MESSAGE_LENGTH
#define GL_MAX_DEBUG_MESSAGE_LENGTH       0x9143
#endif
#ifndef GL_MAX_DEBUG_LOGGED_MESSAGES
#define GL_MAX_DEBUG_LOGGED_MESSAGES      0x9144
#endif
#ifndef GL_DEBUG_LOGGED_MESSAGES
#define GL_DEBUG_LOGGED_MESSAGES          0x9145
#endif
#ifndef GL_DEBUG_SEVERITY_HIGH
#define GL_DEBUG_SEVERITY_HIGH            0x9146
#endif
#ifndef GL_DEBUG_SEVERITY_MEDIUM
#define GL_DEBUG_SEVERITY_MEDIUM          0x9147
#endif
#ifndef GL_DEBUG_SEVERITY_LOW
#define GL_DEBUG_SEVERITY_LOW             0x9148
#endif
#ifndef GL_DEBUG_OUTPUT
#define GL_DEBUG_OUTPUT                   0x92E0
#endif
#ifndef GL_CONTEXT_FLAG_DEBUG_BIT
#define GL_CONTEXT_FLAG_DEBUG_BIT         0x00000002
#endif
#ifndef GL_STACK_OVERFLOW
#define GL_STACK_OVERFLOW                 0x0503
#endif
#ifndef GL_STACK_UNDERFLOW
#define GL_STACK_UNDERFLOW                0x0504
#endif

typedef void (QOPENGLF_APIENTRY *GLDEBUGPROC)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,GLvoid *userParam);

#endif /* GL_KHR_debug */


/*!
    \internal
*/
static QOpenGLDebugMessage::Source qt_messageSourceFromGL(GLenum source)
{
    switch (source) {
    case GL_DEBUG_SOURCE_API:
        return QOpenGLDebugMessage::APISource;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        return QOpenGLDebugMessage::WindowSystemSource;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        return QOpenGLDebugMessage::ShaderCompilerSource;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        return QOpenGLDebugMessage::ThirdPartySource;
    case GL_DEBUG_SOURCE_APPLICATION:
        return QOpenGLDebugMessage::ApplicationSource;
    case GL_DEBUG_SOURCE_OTHER:
        return QOpenGLDebugMessage::OtherSource;
    }

    Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown message source from GL");
    return QOpenGLDebugMessage::OtherSource;
}

/*!
    \internal
*/
static GLenum qt_messageSourceToGL(QOpenGLDebugMessage::Source source)
{
    switch (source) {
    case QOpenGLDebugMessage::InvalidSource:
        break;
    case QOpenGLDebugMessage::APISource:
        return GL_DEBUG_SOURCE_API;
    case QOpenGLDebugMessage::WindowSystemSource:
        return GL_DEBUG_SOURCE_WINDOW_SYSTEM;
    case QOpenGLDebugMessage::ShaderCompilerSource:
        return GL_DEBUG_SOURCE_SHADER_COMPILER;
    case QOpenGLDebugMessage::ThirdPartySource:
        return GL_DEBUG_SOURCE_THIRD_PARTY;
    case QOpenGLDebugMessage::ApplicationSource:
        return GL_DEBUG_SOURCE_APPLICATION;
    case QOpenGLDebugMessage::OtherSource:
        return GL_DEBUG_SOURCE_OTHER;
    case QOpenGLDebugMessage::AnySource:
        break;
    }

    Q_ASSERT_X(false, Q_FUNC_INFO, "Invalid message source");
    return GL_DEBUG_SOURCE_OTHER;
}

/*!
    \internal
*/
static QString qt_messageSourceToString(QOpenGLDebugMessage::Source source)
{
    switch (source) {
    case QOpenGLDebugMessage::InvalidSource:
        return QStringLiteral("InvalidSource");
    case QOpenGLDebugMessage::APISource:
        return QStringLiteral("APISource");
    case QOpenGLDebugMessage::WindowSystemSource:
        return QStringLiteral("WindowSystemSource");
    case QOpenGLDebugMessage::ShaderCompilerSource:
        return QStringLiteral("ShaderCompilerSource");
    case QOpenGLDebugMessage::ThirdPartySource:
        return QStringLiteral("ThirdPartySource");
    case QOpenGLDebugMessage::ApplicationSource:
        return QStringLiteral("ApplicationSource");
    case QOpenGLDebugMessage::OtherSource:
        return QStringLiteral("OtherSource");
    case QOpenGLDebugMessage::AnySource:
        return QStringLiteral("AnySource");
    }

    Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown message source");
    return QString();
}

/*!
    \internal
*/
static QOpenGLDebugMessage::Type qt_messageTypeFromGL(GLenum type)
{
    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        return QOpenGLDebugMessage::ErrorType;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
    return QOpenGLDebugMessage::DeprecatedBehaviorType;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        return QOpenGLDebugMessage::UndefinedBehaviorType;
    case GL_DEBUG_TYPE_PORTABILITY:
        return QOpenGLDebugMessage::PortabilityType;
    case GL_DEBUG_TYPE_PERFORMANCE:
        return QOpenGLDebugMessage::PerformanceType;
    case GL_DEBUG_TYPE_OTHER:
        return QOpenGLDebugMessage::OtherType;
    case GL_DEBUG_TYPE_MARKER:
        return QOpenGLDebugMessage::MarkerType;
    case GL_DEBUG_TYPE_PUSH_GROUP:
        return QOpenGLDebugMessage::GroupPushType;
    case GL_DEBUG_TYPE_POP_GROUP:
        return QOpenGLDebugMessage::GroupPopType;
    }

    Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown message type from GL");
    return QOpenGLDebugMessage::OtherType;
}

/*!
    \internal
*/
static GLenum qt_messageTypeToGL(QOpenGLDebugMessage::Type type)
{
    switch (type) {
    case QOpenGLDebugMessage::InvalidType:
        break;
    case QOpenGLDebugMessage::ErrorType:
        return GL_DEBUG_TYPE_ERROR;
    case QOpenGLDebugMessage::DeprecatedBehaviorType:
        return GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR;
    case QOpenGLDebugMessage::UndefinedBehaviorType:
        return GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR;
    case QOpenGLDebugMessage::PortabilityType:
        return GL_DEBUG_TYPE_PORTABILITY;
    case QOpenGLDebugMessage::PerformanceType:
        return GL_DEBUG_TYPE_PERFORMANCE;
    case QOpenGLDebugMessage::OtherType:
        return GL_DEBUG_TYPE_OTHER;
    case QOpenGLDebugMessage::MarkerType:
        return GL_DEBUG_TYPE_MARKER;
    case QOpenGLDebugMessage::GroupPushType:
        return GL_DEBUG_TYPE_PUSH_GROUP;
    case QOpenGLDebugMessage::GroupPopType:
        return GL_DEBUG_TYPE_POP_GROUP;
    case QOpenGLDebugMessage::AnyType:
        break;
    }

    Q_ASSERT_X(false, Q_FUNC_INFO, "Invalid message type");
    return GL_DEBUG_TYPE_OTHER;
}

/*!
    \internal
*/
static QString qt_messageTypeToString(QOpenGLDebugMessage::Type type)
{
    switch (type) {
    case QOpenGLDebugMessage::InvalidType:
        return QStringLiteral("InvalidType");
    case QOpenGLDebugMessage::ErrorType:
        return QStringLiteral("ErrorType");
    case QOpenGLDebugMessage::DeprecatedBehaviorType:
        return QStringLiteral("DeprecatedBehaviorType");
    case QOpenGLDebugMessage::UndefinedBehaviorType:
        return QStringLiteral("UndefinedBehaviorType");
    case QOpenGLDebugMessage::PortabilityType:
        return QStringLiteral("PortabilityType");
    case QOpenGLDebugMessage::PerformanceType:
        return QStringLiteral("PerformanceType");
    case QOpenGLDebugMessage::OtherType:
        return QStringLiteral("OtherType");
    case QOpenGLDebugMessage::MarkerType:
        return QStringLiteral("MarkerType");
    case QOpenGLDebugMessage::GroupPushType:
        return QStringLiteral("GroupPushType");
    case QOpenGLDebugMessage::GroupPopType:
        return QStringLiteral("GroupPopType");
    case QOpenGLDebugMessage::AnyType:
        return QStringLiteral("AnyType");
    }

    Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown message type");
    return QString();
}

/*!
    \internal
*/
static QOpenGLDebugMessage::Severity qt_messageSeverityFromGL(GLenum severity)
{
    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
        return QOpenGLDebugMessage::HighSeverity;
    case GL_DEBUG_SEVERITY_MEDIUM:
        return QOpenGLDebugMessage::MediumSeverity;
    case GL_DEBUG_SEVERITY_LOW:
        return QOpenGLDebugMessage::LowSeverity;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        return QOpenGLDebugMessage::NotificationSeverity;
    }

    Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown message severity from GL");
    return QOpenGLDebugMessage::NotificationSeverity;
}

/*!
    \internal
*/
static GLenum qt_messageSeverityToGL(QOpenGLDebugMessage::Severity severity)
{
    switch (severity) {
    case QOpenGLDebugMessage::InvalidSeverity:
        break;
    case QOpenGLDebugMessage::HighSeverity:
        return GL_DEBUG_SEVERITY_HIGH;
    case QOpenGLDebugMessage::MediumSeverity:
        return GL_DEBUG_SEVERITY_MEDIUM;
    case QOpenGLDebugMessage::LowSeverity:
        return GL_DEBUG_SEVERITY_LOW;
    case QOpenGLDebugMessage::NotificationSeverity:
        return GL_DEBUG_SEVERITY_NOTIFICATION;
    case QOpenGLDebugMessage::AnySeverity:
        break;
    }

    Q_ASSERT_X(false, Q_FUNC_INFO, "Invalid message severity");
    return GL_DEBUG_SEVERITY_NOTIFICATION;
}

/*!
    \internal
*/
static QString qt_messageSeverityToString(QOpenGLDebugMessage::Severity severity)
{
    switch (severity) {
    case QOpenGLDebugMessage::InvalidSeverity:
        return QStringLiteral("InvalidSeverity");
    case QOpenGLDebugMessage::HighSeverity:
        return QStringLiteral("HighSeverity");
    case QOpenGLDebugMessage::MediumSeverity:
        return QStringLiteral("MediumSeverity");
    case QOpenGLDebugMessage::LowSeverity:
        return QStringLiteral("LowSeverity");
    case QOpenGLDebugMessage::NotificationSeverity:
        return QStringLiteral("NotificationSeverity");
    case QOpenGLDebugMessage::AnySeverity:
        return QStringLiteral("AnySeverity");
    }

    Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown message severity");
    return QString();
}

class QOpenGLDebugMessagePrivate : public QSharedData
{
public:
    QOpenGLDebugMessagePrivate();

    QString message;
    GLuint id;
    QOpenGLDebugMessage::Source source;
    QOpenGLDebugMessage::Type type;
    QOpenGLDebugMessage::Severity severity;
};

/*!
    \internal
*/
QOpenGLDebugMessagePrivate::QOpenGLDebugMessagePrivate()
    : message(),
      id(0),
      source(QOpenGLDebugMessage::InvalidSource),
      type(QOpenGLDebugMessage::InvalidType),
      severity(QOpenGLDebugMessage::InvalidSeverity)
{
}


/*!
    Constructs a debug message with an empty message string, id set to 0,
    source set to InvalidSource, type set to InvalidType, and severity set to
    InvalidSeverity.

    \note This constructor should not be used to create a debug message;
    instead, use the createApplicationMessage() or the createThirdPartyMessage()
    static functions.

    \sa createApplicationMessage(), createThirdPartyMessage()
*/
QOpenGLDebugMessage::QOpenGLDebugMessage()
    : d(new QOpenGLDebugMessagePrivate)
{
}

/*!
    Constructs a debug message as a copy of \a debugMessage.

    \sa operator=()
*/
QOpenGLDebugMessage::QOpenGLDebugMessage(const QOpenGLDebugMessage &debugMessage)
    : d(debugMessage.d)
{
}

/*!
    Destroys this debug message.
*/
QOpenGLDebugMessage::~QOpenGLDebugMessage()
{
}

/*!
    Assigns the message \a debugMessage to this object, and returns a reference
    to the copy.
*/
QOpenGLDebugMessage &QOpenGLDebugMessage::operator=(const QOpenGLDebugMessage &debugMessage)
{
    d = debugMessage.d;
    return *this;
}

/*!
    \fn void QOpenGLDebugMessage::swap(QOpenGLDebugMessage &debugMessage)

    Swaps the message \a debugMessage with this message. This operation is very
    fast and never fails.
*/

/*!
    Returns the source of the debug message.
*/
QOpenGLDebugMessage::Source QOpenGLDebugMessage::source() const
{
    return d->source;
}

/*!
    Returns the type of the debug message.
*/
QOpenGLDebugMessage::Type QOpenGLDebugMessage::type() const
{
    return d->type;
}

/*!
    Returns the severity of the debug message.
*/
QOpenGLDebugMessage::Severity QOpenGLDebugMessage::severity() const
{
    return d->severity;
}

/*!
    Returns the id of the debug message. Ids are generally vendor-specific.
*/
GLuint QOpenGLDebugMessage::id() const
{
    return d->id;
}

/*!
    Returns the textual message contained by this debug message.
*/
QString QOpenGLDebugMessage::message() const
{
    return d->message;
}

/*!
    Constructs and returns a debug message with \a text as its text, \a id
    as id, \a severity as severity, and \a type as type. The message source
    will be set to ApplicationSource.

    \sa QOpenGLDebugLogger::logMessage(), createThirdPartyMessage()
*/
QOpenGLDebugMessage QOpenGLDebugMessage::createApplicationMessage(const QString &text,
                                                                  GLuint id,
                                                                  QOpenGLDebugMessage::Severity severity,
                                                                  QOpenGLDebugMessage::Type type)
{
    QOpenGLDebugMessage m;
    m.d->message = text;
    m.d->id = id;
    m.d->severity = severity;
    m.d->type = type;
    m.d->source = ApplicationSource;
    return m;
}

/*!
    Constructs and returns a debug message with \a text as its text, \a id
    as id, \a severity as severity, and \a type as type. The message source
    will be set to ThirdPartySource.

    \sa QOpenGLDebugLogger::logMessage(), createApplicationMessage()
*/
QOpenGLDebugMessage QOpenGLDebugMessage::createThirdPartyMessage(const QString &text,
                                                                 GLuint id,
                                                                 QOpenGLDebugMessage::Severity severity,
                                                                 QOpenGLDebugMessage::Type type)
{
    QOpenGLDebugMessage m;
    m.d->message = text;
    m.d->id = id;
    m.d->severity = severity;
    m.d->type = type;
    m.d->source = ThirdPartySource;
    return m;
}

/*!
    Returns \c true if this debug message is equal to \a debugMessage, or false
    otherwise. Two debugging messages are equal if they have the same textual
    message, the same id, the same source, the same type and the same severity.

    \sa operator!=()
*/
bool QOpenGLDebugMessage::operator==(const QOpenGLDebugMessage &debugMessage) const
{
    return (d == debugMessage.d)
            || (d->id == debugMessage.d->id
                && d->source == debugMessage.d->source
                && d->type == debugMessage.d->type
                && d->severity == debugMessage.d->severity
                && d->message == debugMessage.d->message);
}

/*!
    \fn bool QOpenGLDebugMessage::operator!=(const QOpenGLDebugMessage &debugMessage) const

    Returns \c true if this message is different from \a debugMessage, or false
    otherwise.

    \sa operator==()
*/

#ifndef QT_NO_DEBUG_STREAM
/*!
    \relates QOpenGLDebugMessage

    Writes the source \a source into the debug object \a debug for debugging
    purposes.
*/
QDebug operator<<(QDebug debug, QOpenGLDebugMessage::Source source)
{
    debug.nospace() << "QOpenGLDebugMessage::Source("
                    << qt_messageSourceToString(source)
                    << ")";
    return debug.space();
}

/*!
    \relates QOpenGLDebugMessage

    Writes the type \a type into the debug object \a debug for debugging
    purposes.
*/
QDebug operator<<(QDebug debug, QOpenGLDebugMessage::Type type)
{
    debug.nospace() << "QOpenGLDebugMessage::Type("
                    << qt_messageTypeToString(type)
                    << ")";
    return debug.space();
}

/*!
    \relates QOpenGLDebugMessage

    Writes the severity \a severity into the debug object \a debug for debugging
    purposes.
*/
QDebug operator<<(QDebug debug, QOpenGLDebugMessage::Severity severity)
{
    debug.nospace() << "QOpenGLDebugMessage::Severity("
                    << qt_messageSeverityToString(severity)
                    << ")";
    return debug.space();
}

/*!
    \relates QOpenGLDebugMessage

    Writes the message \a message into the debug object \a debug for debugging
    purposes.
*/
QDebug operator<<(QDebug debug, const QOpenGLDebugMessage &message)
{
    debug.nospace() << "QOpenGLDebugMessage("
                    << qt_messageSourceToString(message.source()) << ", "
                    << message.id() << ", "
                    << message.message() << ", "
                    << qt_messageSeverityToString(message.severity()) << ", "
                    << qt_messageTypeToString(message.type()) << ")";
    return debug.space();

}
#endif // QT_NO_DEBUG_STREAM

typedef void (QOPENGLF_APIENTRYP qt_glDebugMessageControl_t)(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled);
typedef void (QOPENGLF_APIENTRYP qt_glDebugMessageInsert_t)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *buf);
typedef void (QOPENGLF_APIENTRYP qt_glDebugMessageCallback_t)(GLDEBUGPROC callback, const void *userParam);
typedef GLuint (QOPENGLF_APIENTRYP qt_glGetDebugMessageLog_t)(GLuint count, GLsizei bufsize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog);
typedef void (QOPENGLF_APIENTRYP qt_glPushDebugGroup_t)(GLenum source, GLuint id, GLsizei length, const GLchar *message);
typedef void (QOPENGLF_APIENTRYP qt_glPopDebugGroup_t)();
typedef void (QOPENGLF_APIENTRYP qt_glGetPointerv_t)(GLenum pname, GLvoid **params);

class QOpenGLDebugLoggerPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QOpenGLDebugLogger)
public:
    QOpenGLDebugLoggerPrivate();

    void handleMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *rawMessage);
    void controlDebugMessages(QOpenGLDebugMessage::Sources sources,
                              QOpenGLDebugMessage::Types types,
                              QOpenGLDebugMessage::Severities severities,
                              const QVector<GLuint> &ids,
                              const QByteArray &callerName,
                              bool enable);
    void _q_contextAboutToBeDestroyed();

    qt_glDebugMessageControl_t glDebugMessageControl;
    qt_glDebugMessageInsert_t glDebugMessageInsert;
    qt_glDebugMessageCallback_t glDebugMessageCallback;
    qt_glGetDebugMessageLog_t glGetDebugMessageLog;
    qt_glPushDebugGroup_t glPushDebugGroup;
    qt_glPopDebugGroup_t glPopDebugGroup;
    qt_glGetPointerv_t glGetPointerv;

    GLDEBUGPROC oldDebugCallbackFunction;
    void *oldDebugCallbackParameter;
    QOpenGLContext *context;
    GLint maxMessageLength;
    QOpenGLDebugLogger::LoggingMode loggingMode;
    bool initialized : 1;
    bool isLogging : 1;
    bool debugWasEnabled : 1;
    bool syncDebugWasEnabled : 1;
};

/*!
    \internal
*/
QOpenGLDebugLoggerPrivate::QOpenGLDebugLoggerPrivate()
    : glDebugMessageControl(0),
      glDebugMessageInsert(0),
      glDebugMessageCallback(0),
      glGetDebugMessageLog(0),
      glPushDebugGroup(0),
      glPopDebugGroup(0),
      oldDebugCallbackFunction(0),
      context(0),
      maxMessageLength(0),
      loggingMode(QOpenGLDebugLogger::AsynchronousLogging),
      initialized(false),
      isLogging(false),
      debugWasEnabled(false),
      syncDebugWasEnabled(false)
{
}

/*!
    \internal
*/
void QOpenGLDebugLoggerPrivate::handleMessage(GLenum source,
                                              GLenum type,
                                              GLuint id,
                                              GLenum severity,
                                              GLsizei length,
                                              const GLchar *rawMessage)
{
    if (oldDebugCallbackFunction)
        oldDebugCallbackFunction(source, type, id, severity, length, rawMessage, oldDebugCallbackParameter);

    QOpenGLDebugMessage message;

    QOpenGLDebugMessagePrivate *messagePrivate = message.d.data();
    messagePrivate->source = qt_messageSourceFromGL(source);
    messagePrivate->type = qt_messageTypeFromGL(type);
    messagePrivate->id = id;
    messagePrivate->severity = qt_messageSeverityFromGL(severity);
    // not passing the length to fromUtf8, as some bugged OpenGL drivers
    // do not handle the length correctly. Just rely on the message to be NUL terminated.
    messagePrivate->message = QString::fromUtf8(rawMessage);

    Q_Q(QOpenGLDebugLogger);
    emit q->messageLogged(message);
}

/*!
    \internal
*/
void QOpenGLDebugLoggerPrivate::controlDebugMessages(QOpenGLDebugMessage::Sources sources,
                                                     QOpenGLDebugMessage::Types types,
                                                     QOpenGLDebugMessage::Severities severities,
                                                     const QVector<GLuint> &ids,
                                                     const QByteArray &callerName,
                                                     bool enable)
{
    if (!initialized) {
        qWarning("QOpenGLDebugLogger::%s(): object must be initialized before enabling/disabling messages", callerName.constData());
        return;
    }
    if (sources == QOpenGLDebugMessage::InvalidSource) {
        qWarning("QOpenGLDebugLogger::%s(): invalid source specified", callerName.constData());
        return;
    }
    if (types == QOpenGLDebugMessage::InvalidType) {
        qWarning("QOpenGLDebugLogger::%s(): invalid type specified", callerName.constData());
        return;
    }
    if (severities == QOpenGLDebugMessage::InvalidSeverity) {
        qWarning("QOpenGLDebugLogger::%s(): invalid severity specified", callerName.constData());
        return;
    }

    QVarLengthArray<GLenum, 8> glSources;
    QVarLengthArray<GLenum, 8> glTypes;
    QVarLengthArray<GLenum, 8> glSeverities;

    if (ids.count() > 0) {
        Q_ASSERT(severities == QOpenGLDebugMessage::AnySeverity);

        // The GL_KHR_debug extension says:
        //
        //        - If <count> is greater than zero, then <ids> is an array of <count>
        //          message IDs for the specified combination of <source> and <type>. In
        //          this case, if <source> or <type> is DONT_CARE, or <severity> is not
        //          DONT_CARE, the error INVALID_OPERATION is generated. If <count> is
        //          zero, the value if <ids> is ignored.
        //
        // This means we can't convert AnySource or AnyType into DONT_CARE, but we have to roll
        // them into individual sources/types.

        if (sources == QOpenGLDebugMessage::AnySource) {
            sources = QOpenGLDebugMessage::InvalidSource;
            for (uint i = 1; i <= QOpenGLDebugMessage::LastSource; i = i << 1)
                sources |= QOpenGLDebugMessage::Source(i);
        }

        if (types == QOpenGLDebugMessage::AnyType) {
            types = QOpenGLDebugMessage::InvalidType;
            for (uint i = 1; i <= QOpenGLDebugMessage::LastType; i = i << 1)
                types |= QOpenGLDebugMessage::Type(i);
        }
    }

#define CONVERT_TO_GL_DEBUG_MESSAGE_CONTROL_PARAMETERS(type, source, target) \
    if (source == QOpenGLDebugMessage::Any ## type) { \
        target << GL_DONT_CARE; \
    } else { \
        for (uint i = 1; i <= QOpenGLDebugMessage::Last ## type; i = i << 1) \
            if (source.testFlag(QOpenGLDebugMessage:: type (i))) \
                target << qt_message ## type ## ToGL (QOpenGLDebugMessage:: type (i)); \
    }

    CONVERT_TO_GL_DEBUG_MESSAGE_CONTROL_PARAMETERS(Source, sources, glSources)
    CONVERT_TO_GL_DEBUG_MESSAGE_CONTROL_PARAMETERS(Type, types, glTypes)
    CONVERT_TO_GL_DEBUG_MESSAGE_CONTROL_PARAMETERS(Severity, severities, glSeverities)
#undef CONVERT_TO_GL_DEBUG_MESSAGE_CONTROL_PARAMETERS

    const GLsizei idCount = ids.count();
    // The GL_KHR_debug extension says that if idCount is 0, idPtr must be ignored.
    // Unfortunately, some bugged drivers do NOT ignore it, so pass NULL in case.
    const GLuint * const idPtr = idCount ? ids.constData() : 0;

    foreach (GLenum source, glSources)
        foreach (GLenum type, glTypes)
            foreach (GLenum severity, glSeverities)
                glDebugMessageControl(source, type, severity, idCount, idPtr, GLboolean(enable));
}

/*!
    \internal
*/
void QOpenGLDebugLoggerPrivate::_q_contextAboutToBeDestroyed()
{
    Q_Q(QOpenGLDebugLogger);
    q->stopLogging();
    initialized = false;
}

extern "C" {
static void QOPENGLF_APIENTRY qt_opengl_debug_callback(GLenum source,
                                                       GLenum type,
                                                       GLuint id,
                                                       GLenum severity,
                                                       GLsizei length,
                                                       const GLchar *rawMessage,
                                                       GLvoid *userParam)
{
    QOpenGLDebugLoggerPrivate *loggerPrivate = static_cast<QOpenGLDebugLoggerPrivate *>(userParam);
    loggerPrivate->handleMessage(source, type, id, severity, length, rawMessage);
}
}

/*!
    Constructs a new logger object with the given \a parent.

    \note The object must be initialized before logging can happen.

    \sa initialize()
*/
QOpenGLDebugLogger::QOpenGLDebugLogger(QObject *parent)
    : QObject(*new QOpenGLDebugLoggerPrivate, parent)
{
    // QOpenGLDebugMessage is going to be mostly used as an argument
    // of a cross thread connection, therefore let's ease the life for the users
    // and register the type for them.
    qRegisterMetaType<QOpenGLDebugMessage>();
}

/*!
    Destroys the logger object.
*/
QOpenGLDebugLogger::~QOpenGLDebugLogger()
{
    stopLogging();
}

/*!
    Initializes the object in the current OpenGL context. The context must
    support the \c{GL_KHR_debug} extension for the initialization to succeed.
    The object must be initialized before any logging can happen.

    It is safe to call this function multiple times from the same context.

    This function can also be used to change the context of a previously
    initialized object; note that in this case the object must not be logging
    when you call this function.

    Returns \c true if the logger is successfully initialized; false otherwise.

    \sa QOpenGLContext
*/
bool QOpenGLDebugLogger::initialize()
{
    QOpenGLContext *context = QOpenGLContext::currentContext();
    if (!context) {
        qWarning("QOpenGLDebugLogger::initialize(): no current OpenGL context found.");
        return false;
    }

    Q_D(QOpenGLDebugLogger);
    if (d->context == context) {
        // context is non-NULL, d->context is non NULL only on successful initialization.
        Q_ASSERT(d->initialized);
        return true;
    }

    if (d->isLogging) {
        qWarning("QOpenGLDebugLogger::initialize(): cannot initialize the object while logging. Please stop the logging first.");
        return false;
    }

    if (d->context)
        disconnect(d->context, SIGNAL(aboutToBeDestroyed()), this, SLOT(_q_contextAboutToBeDestroyed()));

    d->initialized = false;
    d->context = 0;

    if (!context->hasExtension(QByteArrayLiteral("GL_KHR_debug")))
        return false;

    d->context = context;
    connect(d->context, SIGNAL(aboutToBeDestroyed()), this, SLOT(_q_contextAboutToBeDestroyed()));

#define GET_DEBUG_PROC_ADDRESS(procName) \
    d->procName = reinterpret_cast< qt_ ## procName ## _t >( \
        d->context->getProcAddress(QByteArrayLiteral( #procName )) \
    );

    GET_DEBUG_PROC_ADDRESS(glDebugMessageControl);
    GET_DEBUG_PROC_ADDRESS(glDebugMessageInsert);
    GET_DEBUG_PROC_ADDRESS(glDebugMessageCallback);
    GET_DEBUG_PROC_ADDRESS(glGetDebugMessageLog);
    GET_DEBUG_PROC_ADDRESS(glPushDebugGroup);
    GET_DEBUG_PROC_ADDRESS(glPopDebugGroup);

    // Windows' Desktop GL doesn't allow resolution of "basic GL entry points"
    // through wglGetProcAddress
#if defined(Q_OS_WIN) && !defined(QT_OPENGL_ES_2)
    {
        HMODULE handle = static_cast<HMODULE>(QOpenGLContext::openGLModuleHandle());
        if (!handle)
            handle = GetModuleHandleA("opengl32.dll");
        d->glGetPointerv = reinterpret_cast<qt_glGetPointerv_t>(GetProcAddress(handle, QByteArrayLiteral("glGetPointerv")));
    }
#else
    GET_DEBUG_PROC_ADDRESS(glGetPointerv)
#endif

#undef GET_DEBUG_PROC_ADDRESS

    QOpenGLContext::currentContext()->functions()->glGetIntegerv(GL_MAX_DEBUG_MESSAGE_LENGTH, &d->maxMessageLength);

#ifndef QT_NO_DEBUG
    if (!d->context->format().testOption(QSurfaceFormat::DebugContext)) {
        qWarning("QOpenGLDebugLogger::initialize(): the current context is not a debug context:\n"
                 "    this means that the GL may not generate any debug output at all.\n"
                 "    To avoid this warning, try creating the context with the\n"
                 "    QSurfaceFormat::DebugContext surface format option.");
    }
#endif // QT_NO_DEBUG

    d->initialized = true;
    return true;
}

/*!
    Returns \c true if this object is currently logging, false otherwise.

    \sa startLogging()
*/
bool QOpenGLDebugLogger::isLogging() const
{
    Q_D(const QOpenGLDebugLogger);
    return d->isLogging;
}

/*!
    Starts logging messages coming from the OpenGL server. When a new message
    is received, the signal messageLogged() is emitted, carrying the logged
    message as argument.

    \a loggingMode specifies whether the logging must be asynchronous (the default)
    or synchronous.

    QOpenGLDebugLogger will record the values of \c{GL_DEBUG_OUTPUT} and
    \c{GL_DEBUG_OUTPUT_SYNCHRONOUS} when logging is started, and set them back
    when logging is stopped. Moreover, any user-defined OpenGL debug callback
    installed when this function is invoked will be restored when logging is
    stopped; QOpenGLDebugLogger will ensure that the pre-existing callback will
    still be invoked when logging.

    \note It's not possible to change the logging mode without stopping and
    starting logging again. This might change in a future version of Qt.

    \note The object must be initialized before logging can happen.

    \sa stopLogging(), initialize()
*/
void QOpenGLDebugLogger::startLogging(QOpenGLDebugLogger::LoggingMode loggingMode)
{
    Q_D(QOpenGLDebugLogger);
    if (!d->initialized) {
        qWarning("QOpenGLDebugLogger::startLogging(): object must be initialized before logging can start");
        return;
    }
    if (d->isLogging) {
        qWarning("QOpenGLDebugLogger::startLogging(): this object is already logging");
        return;
    }

    d->isLogging = true;
    d->loggingMode = loggingMode;

    d->glGetPointerv(GL_DEBUG_CALLBACK_FUNCTION, reinterpret_cast<void **>(&d->oldDebugCallbackFunction));
    d->glGetPointerv(GL_DEBUG_CALLBACK_USER_PARAM, &d->oldDebugCallbackParameter);

    d->glDebugMessageCallback(&qt_opengl_debug_callback, d);

    QOpenGLFunctions *funcs = QOpenGLContext::currentContext()->functions();
    d->debugWasEnabled = funcs->glIsEnabled(GL_DEBUG_OUTPUT);
    d->syncDebugWasEnabled = funcs->glIsEnabled(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    if (d->loggingMode == SynchronousLogging)
        funcs->glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    else
        funcs->glDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    funcs->glEnable(GL_DEBUG_OUTPUT);
}

/*!
    Returns the logging mode of the object.

    \sa startLogging()
*/
QOpenGLDebugLogger::LoggingMode QOpenGLDebugLogger::loggingMode() const
{
    Q_D(const QOpenGLDebugLogger);
    return d->loggingMode;
}

/*!
    Stops logging messages from the OpenGL server.

    \sa startLogging()
*/
void QOpenGLDebugLogger::stopLogging()
{
    Q_D(QOpenGLDebugLogger);
    if (!d->isLogging)
        return;

    d->isLogging = false;

    d->glDebugMessageCallback(d->oldDebugCallbackFunction, d->oldDebugCallbackParameter);

    QOpenGLFunctions *funcs = QOpenGLContext::currentContext()->functions();
    if (!d->debugWasEnabled)
        funcs->glDisable(GL_DEBUG_OUTPUT);

    if (d->syncDebugWasEnabled)
        funcs->glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    else
        funcs->glDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
}

/*!
    Inserts the message \a debugMessage into the OpenGL debug log. This provides
    a way for applications or libraries to insert custom messages that can
    ease the debugging of OpenGL applications.

    \note \a debugMessage must have QOpenGLDebugMessage::ApplicationSource or
    QOpenGLDebugMessage::ThirdPartySource as its source, and a valid
    type and severity, otherwise it will not be inserted into the log.

    \note The object must be initialized before logging can happen.

    \sa initialize()
*/
void QOpenGLDebugLogger::logMessage(const QOpenGLDebugMessage &debugMessage)
{
    Q_D(QOpenGLDebugLogger);
    if (!d->initialized) {
        qWarning("QOpenGLDebugLogger::logMessage(): object must be initialized before logging messages");
        return;
    }
    if (debugMessage.source() != QOpenGLDebugMessage::ApplicationSource
            && debugMessage.source() != QOpenGLDebugMessage::ThirdPartySource) {
        qWarning("QOpenGLDebugLogger::logMessage(): using a message source different from ApplicationSource\n"
                 "    or ThirdPartySource is not supported by GL_KHR_debug. The message will not be logged.");
        return;
    }
    if (debugMessage.type() == QOpenGLDebugMessage::InvalidType
            || debugMessage.type() == QOpenGLDebugMessage::AnyType
            || debugMessage.severity() == QOpenGLDebugMessage::InvalidSeverity
            || debugMessage.severity() == QOpenGLDebugMessage::AnySeverity) {
        qWarning("QOpenGLDebugLogger::logMessage(): the message has a non-valid type and/or severity. The message will not be logged.");
        return;
    }

    const GLenum source = qt_messageSourceToGL(debugMessage.source());
    const GLenum type = qt_messageTypeToGL(debugMessage.type());
    const GLenum severity = qt_messageSeverityToGL(debugMessage.severity());
    QByteArray rawMessage = debugMessage.message().toUtf8();
    rawMessage.append('\0');

    if (rawMessage.length() > d->maxMessageLength) {
        qWarning("QOpenGLDebugLogger::logMessage(): message too long, truncating it\n"
                 "    (%d bytes long, but the GL accepts up to %d bytes)", rawMessage.length(), d->maxMessageLength);
        rawMessage.resize(d->maxMessageLength - 1);
        rawMessage.append('\0');
    }

    // Don't pass rawMessage.length(), as unfortunately bugged
    // OpenGL drivers will eat the trailing NUL in the message. Just rely
    // on the message being NUL terminated.
    d->glDebugMessageInsert(source,
                            type,
                            debugMessage.id(),
                            severity,
                            -1,
                            rawMessage.constData());
}

/*!
    Pushes a debug group with name \a name, id \a id, and source \a source onto
    the debug groups stack. If the group is successfully pushed, OpenGL will
    automatically log a message with message \a name, id \a id, source \a
    source, type QOpenGLDebugMessage::GroupPushType and severity
    QOpenGLDebugMessage::NotificationSeverity.

    The newly pushed group will inherit the same filtering settings of the
    group that was on the top of the stack; that is, the filtering will not be
    changed by pushing a new group.

    \note The \a source must either be QOpenGLDebugMessage::ApplicationSource or
    QOpenGLDebugMessage::ThirdPartySource, otherwise the group will not be pushed.

    \note The object must be initialized before managing debug groups.

    \sa popGroup(), enableMessages(), disableMessages()
*/
void QOpenGLDebugLogger::pushGroup(const QString &name, GLuint id, QOpenGLDebugMessage::Source source)
{
    Q_D(QOpenGLDebugLogger);
    if (!d->initialized) {
        qWarning("QOpenGLDebugLogger::pushGroup(): object must be initialized before pushing a debug group");
        return;
    }
    if (source != QOpenGLDebugMessage::ApplicationSource
            && source != QOpenGLDebugMessage::ThirdPartySource) {
        qWarning("QOpenGLDebugLogger::pushGroup(): using a source different from ApplicationSource\n"
                 "    or ThirdPartySource is not supported by GL_KHR_debug. The group will not be pushed.");
        return;
    }

    QByteArray rawName = name.toUtf8();
    rawName.append('\0');
    if (rawName.length() > d->maxMessageLength) {
        qWarning("QOpenGLDebugLogger::pushGroup(): group name too long, truncating it\n"
                 "    (%d bytes long, but the GL accepts up to %d bytes)", rawName.length(), d->maxMessageLength);
        rawName.resize(d->maxMessageLength - 1);
        rawName.append('\0');
    }

    // Don't pass rawMessage.length(), as unfortunately bugged
    // OpenGL drivers will eat the trailing NUL in the name. Just rely
    // on the name being NUL terminated.
    d->glPushDebugGroup(qt_messageSourceToGL(source), id, -1, rawName.constData());
}

/*!
    Pops the topmost debug group from the debug groups stack. If the group is
    successfully popped, OpenGL will automatically log a message with message,
    id and source matching those of the popped group, type
    QOpenGLDebugMessage::GroupPopType and severity
    QOpenGLDebugMessage::NotificationSeverity.

    Popping a debug group will restore the message filtering settings of the
    group that becomes the top of the debug groups stack.

    \note The object must be initialized before managing debug groups.

    \sa pushGroup()
*/
void QOpenGLDebugLogger::popGroup()
{
    Q_D(QOpenGLDebugLogger);
    if (!d->initialized) {
        qWarning("QOpenGLDebugLogger::pushGroup(): object must be initialized before popping a debug group");
        return;
    }

    d->glPopDebugGroup();
}

/*!
    Enables the logging of messages from the given \a sources, of the given \a
    types and with the given \a severities and any message id.

    The logging will be enabled in the current control group.

    \sa disableMessages(), pushGroup(), popGroup()
*/
void QOpenGLDebugLogger::enableMessages(QOpenGLDebugMessage::Sources sources,
                                        QOpenGLDebugMessage::Types types,
                                        QOpenGLDebugMessage::Severities severities)
{
    Q_D(QOpenGLDebugLogger);
    d->controlDebugMessages(sources,
                            types,
                            severities,
                            QVector<GLuint>(),
                            QByteArrayLiteral("enableMessages"),
                            true);
}

/*!
    Enables the logging of messages with the given \a ids, from the given \a
    sources and of the given \a types and any severity.

    The logging will be enabled in the current control group.

    \sa disableMessages(), pushGroup(), popGroup()
*/
void QOpenGLDebugLogger::enableMessages(const QVector<GLuint> &ids,
                                        QOpenGLDebugMessage::Sources sources,
                                        QOpenGLDebugMessage::Types types)
{
    Q_D(QOpenGLDebugLogger);
    d->controlDebugMessages(sources,
                            types,
                            QOpenGLDebugMessage::AnySeverity,
                            ids,
                            QByteArrayLiteral("enableMessages"),
                            true);
}

/*!
    Disables the logging of messages with the given \a sources, of the given \a
    types and with the given \a severities and any message id.

    The logging will be disabled in the current control group.

    \sa enableMessages(), pushGroup(), popGroup()
*/
void QOpenGLDebugLogger::disableMessages(QOpenGLDebugMessage::Sources sources,
                                         QOpenGLDebugMessage::Types types,
                                         QOpenGLDebugMessage::Severities severities)
{
    Q_D(QOpenGLDebugLogger);
    d->controlDebugMessages(sources,
                            types,
                            severities,
                            QVector<GLuint>(),
                            QByteArrayLiteral("disableMessages"),
                            false);
}

/*!
    Disables the logging of messages with the given \a ids, from the given \a
    sources and of the given \a types and any severity.

    The logging will be disabled in the current control group.

    \sa enableMessages(), pushGroup(), popGroup()
*/
void QOpenGLDebugLogger::disableMessages(const QVector<GLuint> &ids,
                                         QOpenGLDebugMessage::Sources sources,
                                         QOpenGLDebugMessage::Types types)
{
    Q_D(QOpenGLDebugLogger);
    d->controlDebugMessages(sources,
                            types,
                            QOpenGLDebugMessage::AnySeverity,
                            ids,
                            QByteArrayLiteral("disableMessages"),
                            false);
}

/*!
    Reads all the available messages in the OpenGL internal debug log and
    returns them. Moreover, this function will clear the internal debug log,
    so that subsequent invocations will not return messages that were
    already returned.

    \sa startLogging()
*/
QList<QOpenGLDebugMessage> QOpenGLDebugLogger::loggedMessages() const
{
    Q_D(const QOpenGLDebugLogger);
    if (!d->initialized) {
        qWarning("QOpenGLDebugLogger::loggedMessages(): object must be initialized before reading logged messages");
        return QList<QOpenGLDebugMessage>();
    }

    static const GLuint maxMessageCount = 128;
    GLuint messagesRead;
    GLenum messageSources[maxMessageCount];
    GLenum messageTypes[maxMessageCount];
    GLuint messageIds[maxMessageCount];
    GLenum messageSeverities[maxMessageCount];
    GLsizei messageLengths[maxMessageCount];

    QByteArray messagesBuffer;
    messagesBuffer.resize(maxMessageCount * d->maxMessageLength);

    QList<QOpenGLDebugMessage> messages;
    do {
        messagesRead = d->glGetDebugMessageLog(maxMessageCount,
                                               GLsizei(messagesBuffer.size()),
                                               messageSources,
                                               messageTypes,
                                               messageIds,
                                               messageSeverities,
                                               messageLengths,
                                               messagesBuffer.data());

        const char *messagesBufferPtr = messagesBuffer.constData();
        for (GLuint i = 0; i < messagesRead; ++i) {
            QOpenGLDebugMessage message;

            QOpenGLDebugMessagePrivate *messagePrivate = message.d.data();
            messagePrivate->source = qt_messageSourceFromGL(messageSources[i]);
            messagePrivate->type = qt_messageTypeFromGL(messageTypes[i]);
            messagePrivate->id = messageIds[i];
            messagePrivate->severity = qt_messageSeverityFromGL(messageSeverities[i]);
            messagePrivate->message = QString::fromUtf8(messagesBufferPtr, messageLengths[i] - 1);

            messagesBufferPtr += messageLengths[i];
            messages << message;
        }
    } while (messagesRead == maxMessageCount);

    return messages;
}

/*!
    \fn void QOpenGLDebugLogger::messageLogged(const QOpenGLDebugMessage &debugMessage)

    This signal is emitted when a debug message (wrapped by the \a debugMessage
    argument) is logged from the OpenGL server.

    Depending on the OpenGL implementation, this signal can be emitted
    from other threads than the one(s) the receiver(s) lives in, and even
    different from the thread the QOpenGLContext in which this object has
    been initialized lives in. Moreover, the signal could be emitted from
    multiple threads at the same time. This is normally not a problem,
    as Qt will utilize a queued connection for cross-thread signal emissions,
    but if you force the connection type to Direct then you must be aware of
    the potential races in the slots connected to this signal.

    If logging have been started in SynchronousLogging mode, OpenGL guarantees
    that this signal will be emitted from the same thread the QOpenGLContext
    has been bound to, and no concurrent invocations will ever happen.

    \note Logging must have been started, or this signal will not be emitted.

    \sa startLogging()
*/

/*!
    Returns the maximum supported length, in bytes, for the text of the messages
    passed to logMessage(). This is also the maximum length of a debug group
    name, as pushing or popping groups will automatically log a message with
    the debug group name as the message text.

    If a message text is too long, it will be automatically truncated by
    QOpenGLDebugLogger.

    \note Message texts are encoded in UTF-8 when they get passed to OpenGL, so
    their size in bytes does not usually match the amount of UTF-16 code units,
    as returned f.i. by QString::length(). (It does if the message contains
    7-bit ASCII only data, which is typical for debug messages.)
*/
qint64 QOpenGLDebugLogger::maximumMessageLength() const
{
    Q_D(const QOpenGLDebugLogger);
    if (!d->initialized) {
        qWarning("QOpenGLDebugLogger::maximumMessageLength(): object must be initialized before reading the maximum message length");
        return -1;
    }
    return d->maxMessageLength;
}


QT_END_NAMESPACE

#include "moc_qopengldebug.cpp"
