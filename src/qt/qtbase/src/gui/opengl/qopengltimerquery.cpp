/****************************************************************************
**
** Copyright (C) 2013 Klaralvdalens Datakonsult AB (KDAB).
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

#include "qopengltimerquery.h"

#include "qopenglqueryhelper_p.h"
#include <QtCore/private/qobject_p.h>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>

QT_BEGIN_NAMESPACE

// Helper class used as fallback if OpenGL <3.3 is being used with EXT_timer_query
class QExtTimerQueryHelper
{
public:
    QExtTimerQueryHelper(QOpenGLContext *context)
    {
        Q_ASSERT(context);
        GetQueryObjectui64vEXT = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum , GLuint64EXT *)>(context->getProcAddress("glGetQueryObjectui64vEXT"));
        GetQueryObjecti64vEXT = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum , GLint64EXT *)>(context->getProcAddress("glGetQueryObjecti64vEXT"));
    }

    inline void glGetQueryObjectui64vEXT(GLuint id, GLenum pname, GLuint64EXT *params)
    {
        GetQueryObjectui64vEXT(id, pname, params);
    }

    inline void glGetQueryObjecti64vEXT(GLuint id, GLenum pname, GLint64EXT *params)
    {
        GetQueryObjecti64vEXT(id, pname, params);
    }

private:
    void (QOPENGLF_APIENTRYP GetQueryObjectui64vEXT)(GLuint id, GLenum pname, GLuint64EXT *params);
    void (QOPENGLF_APIENTRYP GetQueryObjecti64vEXT)(GLuint id, GLenum pname, GLint64EXT *params);
};

class QOpenGLTimerQueryPrivate : public QObjectPrivate
{
public:
    QOpenGLTimerQueryPrivate()
        : QObjectPrivate(),
          context(0),
          ext(0),
          timeInterval(0),
          timer(0)
    {
    }

    ~QOpenGLTimerQueryPrivate()
    {
        delete core;
        delete ext;
    }

    bool create();
    void destroy();
    void begin();
    void end();
    GLuint64 waitForTimeStamp() const;
    void recordTimestamp();
    bool isResultAvailable() const;
    GLuint64 result() const;

    // There are several cases we must handle:
    //   OpenGL >=3.3 includes timer queries as a core feature
    //   ARB_timer_query has same functionality as above. Requires OpenGL 3.2
    //   EXT_timer_query offers limited support. Can be used with OpenGL >=1.5
    //
    // Note that some implementations (OS X) provide OpenGL 3.2 but do not expose the
    // ARB_timer_query extension. In such situations we must also be able to handle
    // using the EXT_timer_query extension with any version of OpenGL.
    //
    // OpenGL 1.5 or above contains the generic query API and OpenGL 3.3 and
    // ARB_timer_query provide the 64-bit query API. These are wrapped by
    // QOpenGLQueryHelper. All we need to handle in addition is the EXT_timer_query
    // case and to take care not to call the Core/ARB functions when we only
    // have EXT_timer_query available.
    QOpenGLContext *context;
    QOpenGLQueryHelper *core;
    QExtTimerQueryHelper *ext;
    mutable GLuint64 timeInterval;
    GLuint timer;
};

bool QOpenGLTimerQueryPrivate::create()
{
    QOpenGLContext *ctx = QOpenGLContext::currentContext();

    if (timer && context == ctx)
        return true;

    context = ctx;
    if (!context) {
        qWarning("A current OpenGL context is required to create timer query objects");
        return false;
    }

    if (context->isOpenGLES()) {
        qWarning("QOpenGLTimerQuery: Not supported on OpenGL ES");
        return false;
    }

    // Resolve the functions provided by OpenGL 1.5 and OpenGL 3.3 or ARB_timer_query
    core = new QOpenGLQueryHelper(context);

    // Check to see if we also need to resolve the functions for EXT_timer_query
    QSurfaceFormat f = context->format();
    if (f.version() <= qMakePair<int, int>(3, 2)
        && !context->hasExtension(QByteArrayLiteral("GL_ARB_timer_query"))
        && context->hasExtension(QByteArrayLiteral("GL_EXT_timer_query"))) {
        ext = new QExtTimerQueryHelper(context);
    } else if (f.version() <= qMakePair<int, int>(3, 2)
               && !context->hasExtension(QByteArrayLiteral("GL_ARB_timer_query"))
               && !context->hasExtension(QByteArrayLiteral("GL_EXT_timer_query"))) {
        qWarning("QOpenGLTimerQuery requires one of:\n"
                 "  OpenGL 3.3 or newer,\n"
                 "  OpenGL 3.2 and the ARB_timer_query extension\n"
                 "  or the EXT_timer query extension");
        return false;
    }

    core->glGenQueries(1, &timer);
    return (timer != 0);
}

void QOpenGLTimerQueryPrivate::destroy()
{
    if (!timer)
        return;

    core->glDeleteQueries(1, &timer);
    timer = 0;
    context = 0;
}

// GL_TIME_ELAPSED_EXT is not defined on OS X 10.6
#if !defined(GL_TIME_ELAPSED_EXT)
#define GL_TIME_ELAPSED_EXT 0x88BF
#endif

// GL_TIME_ELAPSED is not defined on OS X 10.7 or 10.8 yet
#if !defined(GL_TIME_ELAPSED)
#define GL_TIME_ELAPSED GL_TIME_ELAPSED_EXT
#endif

void QOpenGLTimerQueryPrivate::begin()
{
    core->glBeginQuery(GL_TIME_ELAPSED, timer);
}

void QOpenGLTimerQueryPrivate::end()
{
    core->glEndQuery(GL_TIME_ELAPSED);
}

void QOpenGLTimerQueryPrivate::recordTimestamp()
{
    // Don't call glQueryCounter if we only have EXT_timer_query
#if defined(GL_TIMESTAMP)
    if (!ext)
        core->glQueryCounter(timer, GL_TIMESTAMP);
    else
        qWarning("QOpenGLTimerQuery::recordTimestamp() requires OpenGL 3.3 or GL_ARB_timer_query");
#else
    qWarning("QOpenGLTimerQuery::recordTimestamp() requires OpenGL 3.3 or GL_ARB_timer_query");
#endif
}

GLuint64 QOpenGLTimerQueryPrivate::waitForTimeStamp() const
{
    GLint64 tmp = 0;
#if defined(GL_TIMESTAMP)
    if (!ext)
        core->glGetInteger64v(GL_TIMESTAMP, &tmp);
    else
        qWarning("QOpenGLTimerQuery::waitForTimestamp() requires OpenGL 3.3 or GL_ARB_timer_query");
#else
    qWarning("QOpenGLTimerQuery::waitForTimestamp() requires OpenGL 3.3 or GL_ARB_timer_query");
#endif
    GLuint64 timestamp(tmp);
    return timestamp;
}

bool QOpenGLTimerQueryPrivate::isResultAvailable() const
{
    GLuint available = GL_FALSE;
    core->glGetQueryObjectuiv(timer, GL_QUERY_RESULT_AVAILABLE, &available);
    return available;
}

GLuint64 QOpenGLTimerQueryPrivate::result() const
{
    if (!ext)
        core->glGetQueryObjectui64v(timer, GL_QUERY_RESULT, &timeInterval);
    else
        ext->glGetQueryObjectui64vEXT(timer, GL_QUERY_RESULT, &timeInterval);
    return timeInterval;
}

/*!
    \class QOpenGLTimerQuery
    \brief The QOpenGLTimerQuery class wraps an OpenGL timer query object.
    \inmodule QtGui
    \since 5.1
    \ingroup painting-3D

    OpenGL timer query objects are OpenGL managed resources to measure the
    execution times of sequences of OpenGL commands on the GPU.

    OpenGL offers various levels of support for timer queries, depending on
    the version of OpenGL you have and the presence of the ARB_timer_query or
    EXT_timer_query extensions. The support can be summarized as:

    \list
        \li OpenGL >=3.3 offers full support for all timer query functionality.
        \li OpenGL 3.2 with the ARB_timer_query extension offers full support
            for all timer query functionality.
        \li OpenGL <=3.2 with the EXT_timer_query extension offers limited support
            in that the timestamp of the GPU cannot be queried. Places where this
            impacts functions provided by Qt classes will be highlighted in the
            function documentation.
        \li OpenGL ES 2 (and OpenGL ES 3) do not provide any support for OpenGL
            timer queries.
    \endlist

    OpenGL represents time with a granularity of 1 nanosecond (1e-9 seconds). As a
    consequence of this, 32-bit integers would only give a total possible duration
    of approximately 4 seconds, which would not be difficult to exceed in poorly
    performing or lengthy operations. OpenGL therefore uses 64 bit integer types
    to represent times. A GLuint64 variable has enough width to contain a duration
    of hundreds of years, which is plenty for real-time rendering needs.

    As with the other Qt OpenGL classes, QOpenGLTimerQuery has a create()
    function to create the underlying OpenGL object. This is to allow the developer to
    ensure that there is a valid current OpenGL context at the time.

    Once created, timer queries can be issued in one of several ways. The simplest
    method is to delimit a block of commands with calls to begin() and end(). This
    instructs OpenGL to measure the time taken from completing all commands issued
    prior to begin() until the completion of all commands issued prior to end().

    At the end of a frame we can retrieve the results by calling waitForResult().
    As this function's name implies, it blocks CPU execution until OpenGL notifies
    that the timer query result is available. To avoid blocking, you can check
    if the query result is available by calling isResultAvailable(). Note that
    modern GPUs are deeply pipelined and query results may not become available for
    between 1-5 frames after they were issued.

    Note that OpenGL does not permit nesting or interleaving of multiple timer queries
    using begin() and end(). Using multiple timer queries and recordTimestamp() avoids
    this limitation. When using recordTimestamp() the result can be obtained at
    some later time using isResultAvailable() and waitForResult(). Qt provides the
    convenience class QOpenGLTimeMonitor that helps with using multiple query objects.

    \sa QOpenGLTimeMonitor
*/

/*!
    Creates a QOpenGLTimerQuery instance with the given \a parent. You must call create()
    with a valid OpenGL context before using.
*/
QOpenGLTimerQuery::QOpenGLTimerQuery(QObject *parent)
    : QObject(*new QOpenGLTimerQueryPrivate, parent)
{
}

/*!
    Destroys the QOpenGLTimerQuery and the underlying OpenGL resource.
*/
QOpenGLTimerQuery::~QOpenGLTimerQuery()
{
    QOpenGLContext* ctx = QOpenGLContext::currentContext();

    Q_D(QOpenGLTimerQuery);
    QOpenGLContext *oldContext = 0;
    if (d->context != ctx) {
        oldContext = ctx;
        if (d->context->makeCurrent(oldContext->surface())) {
            ctx = d->context;
        } else {
            qWarning("QOpenGLTimerQuery::~QOpenGLTimerQuery() failed to make query objects's context current");
            ctx = 0;
        }
    }

    if (ctx)
        destroy();

    if (oldContext) {
        if (!oldContext->makeCurrent(oldContext->surface()))
            qWarning("QOpenGLTimerQuery::~QOpenGLTimerQuery() failed to restore current context");
    }
}

/*!
    Creates the underlying OpenGL timer query object. There must be a valid OpenGL context
    that supports query objects current for this function to succeed.

    Returns \c true if the OpenGL timer query object was successfully created.
*/
bool QOpenGLTimerQuery::create()
{
    Q_D(QOpenGLTimerQuery);
    return d->create();
}

/*!
    Destroys the underlying OpenGL timer query object. The context that was current when
    create() was called must be current when calling this function.
*/
void QOpenGLTimerQuery::destroy()
{
    Q_D(QOpenGLTimerQuery);
    d->destroy();
}

/*!
    Returns \c true if the underlying OpenGL query object has been created. If this
    returns \c true and the associated OpenGL context is current, then you are able to issue
    queries with this object.
*/
bool QOpenGLTimerQuery::isCreated() const
{
    Q_D(const QOpenGLTimerQuery);
    return (d->timer != 0);
}

/*!
    Returns the id of the underlying OpenGL query object.
*/
GLuint QOpenGLTimerQuery::objectId() const
{
    Q_D(const QOpenGLTimerQuery);
    return d->timer;
}

/*!
    Marks the start point in the OpenGL command queue for a sequence of commands to
    be timed by this query object.

    This is useful for simple use-cases. Usually it is better to use recordTimestamp().

    \sa end(), isResultAvailable(), waitForResult(), recordTimestamp()
*/
void QOpenGLTimerQuery::begin()
{
    Q_D(QOpenGLTimerQuery);
    d->begin();
}

/*!
    Marks the end point in the OpenGL command queue for a sequence of commands to
    be timed by this query object.

    This is useful for simple use-cases. Usually it is better to use recordTimestamp().

    \sa begin(), isResultAvailable(), waitForResult(), recordTimestamp()
*/
void QOpenGLTimerQuery::end()
{
    Q_D(QOpenGLTimerQuery);
    d->end();
}

/*!
    Places a marker in the OpenGL command queue for the GPU to record the timestamp
    when this marker is reached by the GPU. This function is non-blocking and the
    result will become available at some later time.

    The availability of the result can be checked with isResultAvailable(). The result
    can be fetched with waitForResult() which will block if the result is not yet
    available.

    \sa waitForResult(), isResultAvailable(), begin(), end()
*/
void QOpenGLTimerQuery::recordTimestamp()
{
    Q_D(QOpenGLTimerQuery);
    return d->recordTimestamp();
}

/*!
    Returns the current timestamp of the GPU when all previously issued OpenGL
    commands have been received but not necessarily executed by the GPU.

    This function blocks until the result is returned.

    \sa recordTimestamp()
*/
GLuint64 QOpenGLTimerQuery::waitForTimestamp() const
{
    Q_D(const QOpenGLTimerQuery);
    return d->waitForTimeStamp();
}

/*!
    Returns \c true if the OpenGL timer query result is available.

    This function is non-blocking and ideally should be used to check for the
    availability of the query result before calling waitForResult().

    \sa waitForResult()
*/
bool QOpenGLTimerQuery::isResultAvailable() const
{
    Q_D(const QOpenGLTimerQuery);
    return d->isResultAvailable();
}

/*!
    Returns the result of the OpenGL timer query.

    This function will block until the result is made available by OpenGL. It is
    recommended to call isResultAvailable() to ensure that the result is available
    to avoid unnecessary blocking and stalling.

    \sa isResultAvailable()
*/
GLuint64 QOpenGLTimerQuery::waitForResult() const
{
    Q_D(const QOpenGLTimerQuery);
    return d->result();
}


class QOpenGLTimeMonitorPrivate : public QObjectPrivate
{
public:
    QOpenGLTimeMonitorPrivate()
        : QObjectPrivate(),
          timers(),
          timeSamples(),
          context(0),
          core(0),
          ext(0),
          requestedSampleCount(2),
          currentSample(-1),
          timerQueryActive(false)
    {
    }

    ~QOpenGLTimeMonitorPrivate()
    {
        delete core;
        delete ext;
    }

    bool create();
    void destroy();
    void recordSample();
    bool isResultAvailable() const;
    QVector<GLuint64> samples() const;
    QVector<GLuint64> intervals() const;
    void reset();

    QVector<GLuint> timers;
    mutable QVector<GLuint64> timeSamples;

    QOpenGLContext *context;
    QOpenGLQueryHelper *core;
    QExtTimerQueryHelper *ext;

    int requestedSampleCount;
    int currentSample;
    mutable bool timerQueryActive;
};

bool QOpenGLTimeMonitorPrivate::create()
{
    if (!timers.isEmpty() && timers.at(0) != 0 && timers.size() == requestedSampleCount)
        return true;

    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    if (context && context != ctx) {
        qWarning("QTimeMonitor: Attempting to use different OpenGL context to recreate timers.\n"
                 "Please call destroy() first or use the same context to previously create");
        return false;
    }

    context = ctx;
    if (!context) {
        qWarning("A current OpenGL context is required to create timer query objects");
        return false;
    }

    // Resize the vectors that hold the timers and the recorded samples
    timers.resize(requestedSampleCount);
    timeSamples.resize(requestedSampleCount);

    // Resolve the functions provided by OpenGL 1.5 and OpenGL 3.3 or ARB_timer_query
    core = new QOpenGLQueryHelper(context);

    // Check to see if we also need to resolve the functions for EXT_timer_query
    QSurfaceFormat f = context->format();
    if (f.version() <= qMakePair<int, int>(3, 2)
        && !context->hasExtension(QByteArrayLiteral("GL_ARB_timer_query"))
        && context->hasExtension(QByteArrayLiteral("GL_EXT_timer_query"))) {
        ext = new QExtTimerQueryHelper(context);
    } else if (f.version() <= qMakePair<int, int>(3, 2)
               && !context->hasExtension(QByteArrayLiteral("GL_ARB_timer_query"))
               && !context->hasExtension(QByteArrayLiteral("GL_EXT_timer_query"))) {
        qWarning("QOpenGLTimeMonitor requires one of:\n"
                 "  OpenGL 3.3 or newer,\n"
                 "  OpenGL 3.2 and the ARB_timer_query extension\n"
                 "  or the EXT_timer query extension");
        return false;
    }

    core->glGenQueries(requestedSampleCount, timers.data());
    return (timers.at(0) != 0);
}

void QOpenGLTimeMonitorPrivate::destroy()
{
    if (timers.isEmpty() || timers.at(0) == 0)
        return;

    core->glDeleteQueries(timers.size(), timers.data());
    timers.clear();
    delete core;
    core = 0;
    delete ext;
    ext = 0;
    context = 0;
}

void QOpenGLTimeMonitorPrivate::recordSample()
{
    // Use glQueryCounter() and GL_TIMESTAMP where available.
    // Otherwise, simulate it with glBeginQuery()/glEndQuery()
    if (!ext) {
#if defined(GL_TIMESTAMP)
        core->glQueryCounter(timers.at(++currentSample), GL_TIMESTAMP);
#endif
    } else {
        if (currentSample == -1) {
            core->glBeginQuery(GL_TIME_ELAPSED_EXT, timers.at(++currentSample));
            timerQueryActive = true;
        } else if (currentSample < timers.size() - 1) {
            core->glEndQuery(GL_TIME_ELAPSED_EXT);
            core->glBeginQuery(GL_TIME_ELAPSED_EXT, timers.at(++currentSample));
        } else {
            if (timerQueryActive) {
                core->glEndQuery(GL_TIME_ELAPSED_EXT);
                timerQueryActive = false;
            }
        }
    }
}

bool QOpenGLTimeMonitorPrivate::isResultAvailable() const
{
    // The OpenGL spec says that if a query result is ready then the results of all queries
    // of the same type issued before it must also be ready. Therefore we only need to check
    // the availability of the result for the last issued query
    GLuint available = GL_FALSE;
    core->glGetQueryObjectuiv(timers.at(currentSample), GL_QUERY_RESULT_AVAILABLE, &available);
    return available;
}

QVector<GLuint64> QOpenGLTimeMonitorPrivate::samples() const
{
    // For the Core and ARB options just ask for the timestamp for each timer query.
    // For the EXT implementation we cannot obtain timestamps so we defer any result
    // collection to the intervals() function
    if (!ext) {
        for (int i = 0; i <= currentSample; ++i)
            core->glGetQueryObjectui64v(timers.at(i), GL_QUERY_RESULT, &timeSamples[i]);
    } else {
        qWarning("QOpenGLTimeMonitor::samples() requires OpenGL >=3.3\n"
                 "or OpenGL 3.2 and GL_ARB_timer_query");
    }
    return timeSamples;
}

QVector<GLuint64> QOpenGLTimeMonitorPrivate::intervals() const
{
    QVector<GLuint64> intervals(timers.size() - 1);
    if (!ext) {
        // Obtain the timestamp samples and calculate the interval durations
        const QVector<GLuint64> timeStamps = samples();
        for (int i = 0; i < intervals.size(); ++i)
            intervals[i] = timeStamps[i+1] - timeStamps[i];
    } else {
        // Stop the last timer if needed
        if (timerQueryActive) {
            core->glEndQuery(GL_TIME_ELAPSED_EXT);
            timerQueryActive = false;
        }

        // Obtain the results from all timers apart from the redundant last one. In this
        // case the results actually are the intervals not timestamps
        for (int i = 0; i < currentSample; ++i)
            ext->glGetQueryObjectui64vEXT(timers.at(i), GL_QUERY_RESULT, &intervals[i]);
    }

    return intervals;
}

void QOpenGLTimeMonitorPrivate::reset()
{
    currentSample = -1;
    timeSamples.fill(0);
}


/*!
    \class QOpenGLTimeMonitor
    \brief The QOpenGLTimeMonitor class wraps a sequence of OpenGL timer query objects.
    \inmodule QtGui
    \since 5.1
    \ingroup painting-3D

    The QOpenGLTimeMonitor class is a convenience wrapper around a collection of OpenGL
    timer query objects used to measure intervals of time on the GPU to the level of
    granularity required by your rendering application.

    The OpenGL timer queries objects are queried in sequence to record the GPU
    timestamps at positions of interest in your rendering code. Once the results for
    all issues timer queries become available, the results can be fetched and
    QOpenGLTimerMonitor will calculate the recorded time intervals for you.

    The typical use case of this class is to either profile your application's rendering
    algorithms or to adjust those algorithms in real-time for dynamic performance/quality
    balancing.

    Prior to using QOpenGLTimeMonitor in your rendering function you should set the
    required number of sample points that you wish to record by calling setSamples(). Note
    that measuring N sample points will produce N-1 time intervals. Once you have set the
    number of sample points, call the create() function with a valid current OpenGL context
    to create the necessary query timer objects. These steps are usually performed just
    once in an initialization function.

    Use the recordSample() function to delimit blocks of code containing OpenGL commands
    that you wish to time. You can check availability of the resulting time
    samples and time intervals with isResultAvailable(). The calculated time intervals and
    the raw timestamp samples can be retrieved with the blocking waitForIntervals() and
    waitForSamples() functions respectively.

    After retrieving the results and before starting a new round of taking samples
    (for example, in the next frame) be sure to call the reset() function which will clear
    the cached results and reset the timer index back to the first timer object.

    \sa QOpenGLTimerQuery
*/

/*!
    Creates a QOpenGLTimeMonitor instance with the given \a parent. You must call create()
    with a valid OpenGL context before using.

    \sa setSampleCount(), create()
*/
QOpenGLTimeMonitor::QOpenGLTimeMonitor(QObject *parent)
    : QObject(*new QOpenGLTimeMonitorPrivate, parent)
{
}

/*!
    Destroys the QOpenGLTimeMonitor and any underlying OpenGL resources.
*/
QOpenGLTimeMonitor::~QOpenGLTimeMonitor()
{
    QOpenGLContext* ctx = QOpenGLContext::currentContext();

    Q_D(QOpenGLTimeMonitor);
    QOpenGLContext *oldContext = 0;
    if (d->context != ctx) {
        oldContext = ctx;
        if (d->context->makeCurrent(oldContext->surface())) {
            ctx = d->context;
        } else {
            qWarning("QOpenGLTimeMonitor::~QOpenGLTimeMonitor() failed to make time monitor's context current");
            ctx = 0;
        }
    }

    if (ctx)
        destroy();

    if (oldContext) {
        if (!oldContext->makeCurrent(oldContext->surface()))
            qWarning("QOpenGLTimeMonitor::~QOpenGLTimeMonitor() failed to restore current context");
    }
}

/*!
    Sets the number of sample points to \a sampleCount. After setting the number
    of samples with this function, you must call create() to instantiate the underlying
    OpenGL timer query objects.

    The new \a sampleCount must be at least 2.

    \sa sampleCount(), create(), recordSample()
*/
void QOpenGLTimeMonitor::setSampleCount(int sampleCount)
{
    // We need at least 2 samples to get an interval
    if (sampleCount < 2)
        return;
    Q_D(QOpenGLTimeMonitor);
    d->requestedSampleCount = sampleCount;
}

/*!
    Returns the number of sample points that have been requested with
    setSampleCount(). If create was successfully called following setSampleCount(),
    then the value returned will be the actual number of sample points
    that can be used.

    The default value for sample count is 2, leading to the measurement of a
    single interval.

    \sa setSampleCount()
*/
int QOpenGLTimeMonitor::sampleCount() const
{
    Q_D(const QOpenGLTimeMonitor);
    return d->requestedSampleCount;
}

/*!
    Instantiate sampleCount() OpenGL timer query objects that will be used
    to track the amount of time taken to execute OpenGL commands between
    successive calls to recordSample().

    Returns \c true if the OpenGL timer query objects could be created.

    \sa destroy(), setSampleCount(), recordSample()
*/
bool QOpenGLTimeMonitor::create()
{
    Q_D(QOpenGLTimeMonitor);
    return d->create();
}

/*!
    Destroys any OpenGL timer query objects used within this instance.

    \sa create()
*/
void QOpenGLTimeMonitor::destroy()
{
    Q_D(QOpenGLTimeMonitor);
    d->destroy();
}

/*!
    Returns \c true if the underlying OpenGL query objects have been created. If this
    returns \c true and the associated OpenGL context is current, then you are able to record
    time samples with this object.
*/
bool QOpenGLTimeMonitor::isCreated() const
{
    Q_D(const QOpenGLTimeMonitor);
    return (!d->timers.isEmpty() && d->timers.at(0) != 0);
}

/*!
    Returns a QVector containing the object Ids of the OpenGL timer query objects.
*/
QVector<GLuint> QOpenGLTimeMonitor::objectIds() const
{
    Q_D(const QOpenGLTimeMonitor);
    return d->timers;
}

/*!
    Issues an OpenGL timer query at this point in the OpenGL command queue. Calling this
    function in a sequence in your application's rendering function, will build up
    details of the GPU time taken to execute the OpenGL commands between successive
    calls to this function.

    \sa setSampleCount(), isResultAvailable(), waitForSamples(), waitForIntervals()
*/
int QOpenGLTimeMonitor::recordSample()
{
    Q_D(QOpenGLTimeMonitor);
    d->recordSample();
    return d->currentSample;
}

/*!
    Returns \c true if the OpenGL timer query results are available.

    \sa waitForSamples(), waitForIntervals()
*/
bool QOpenGLTimeMonitor::isResultAvailable() const
{
    Q_D(const QOpenGLTimeMonitor);
    return d->isResultAvailable();
}

/*!
    Returns a QVector containing the GPU timestamps taken with recordSample().

    This function will block until OpenGL indicates the results are available. It
    is recommended to check the availability of the result prior to calling this
    function with isResultAvailable().

    \note This function only works on systems that have OpenGL >=3.3 or the
          ARB_timer_query extension. See QOpenGLTimerQuery for more details.

    \sa waitForIntervals(), isResultAvailable()
*/
QVector<GLuint64> QOpenGLTimeMonitor::waitForSamples() const
{
    Q_D(const QOpenGLTimeMonitor);
    return d->samples();
}

/*!
    Returns a QVector containing the time intervals delimited by the calls to
    recordSample(). The resulting vector will contain one fewer element as
    this represents the intervening intervals rather than the actual timestamp
    samples.

    This function will block until OpenGL indicates the results are available. It
    is recommended to check the availability of the result prior to calling this
    function with isResultAvailable().

    \sa waitForSamples(), isResultAvailable()
*/
QVector<GLuint64> QOpenGLTimeMonitor::waitForIntervals() const
{
    Q_D(const QOpenGLTimeMonitor);
    return d->intervals();
}

/*!
    Resets the time monitor ready for use in another frame of rendering. Call
    this once you have obtained the previous results and before calling
    recordSample() for the first time on the next frame.

    \sa recordSample()
*/
void QOpenGLTimeMonitor::reset()
{
    Q_D(QOpenGLTimeMonitor);
    d->reset();
}

QT_END_NAMESPACE
