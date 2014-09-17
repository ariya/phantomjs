/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "qgesturerecognizer.h"

#include "private/qgesture_p.h"
#include "private/qgesturemanager_p.h"

#ifndef QT_NO_GESTURES

QT_BEGIN_NAMESPACE

/*!
   \class QGestureRecognizer
   \since 4.6
   \brief The QGestureRecognizer class provides the infrastructure for gesture recognition.
   \ingroup gestures

   Gesture recognizers are responsible for creating and managing QGesture objects and
   monitoring input events sent to QWidget and QGraphicsObject subclasses.
   QGestureRecognizer is the base class for implementing custom gestures.

   Developers that only need to provide gesture recognition for standard gestures do not
   need to use this class directly. Instances will be created behind the scenes by the
   framework.

   For an overview of gesture handling in Qt and information on using gestures
   in your applications, see the \l{Gestures Programming} document.

   \section1 Recognizing Gestures

   The process of recognizing gestures involves filtering input events sent to specific
   objects, and modifying the associated QGesture objects to include relevant information
   about the user's input.

   Gestures are created when the framework calls create() to handle user input
   for a particular instance of a QWidget or QGraphicsObject subclass. A QGesture object
   is created for each widget or item that is configured to use gestures.

   Once a QGesture has been created for a target object, the gesture recognizer will
   receive events for it in its recognize() handler function.

   When a gesture is canceled, the reset() function is called, giving the recognizer the
   chance to update the appropriate properties in the corresponding QGesture object.

   \section1 Supporting New Gestures

   To add support for new gestures, you need to derive from QGestureRecognizer to create
   a custom recognizer class, construct an instance of this class, and register it with
   the application by calling QGestureRecognizer::registerRecognizer(). You can also
   subclass QGesture to create a custom gesture class, or rely on dynamic properties
   to express specific details of the gesture you want to handle.

   Your custom QGestureRecognizer subclass needs to reimplement the recognize()
   function to handle and filter the incoming input events for QWidget and
   QGraphicsObject subclasses. Although the logic for gesture recognition is
   implemented in this function, you can store persistent information about the
   state of the recognition process in the QGesture object supplied. The
   recognize() function must return a value of QGestureRecognizer::Result that
   indicates the state of recognition for a given gesture and target object.
   This determines whether or not a gesture event will be delivered to a target
   object.

   If you choose to represent a gesture by a custom QGesture subclass, you will need to
   reimplement the create() function to construct instances of your gesture class.
   Similarly, you may need to reimplement the reset() function if your custom gesture
   objects need to be specially handled when a gesture is canceled.

   \sa QGesture
*/

/*!
    \enum QGestureRecognizer::ResultFlag

    This enum describes the result of the current event filtering step in
    a gesture recognizer state machine.

    The result consists of a state value (one of Ignore, MayBeGesture,
    TriggerGesture, FinishGesture, CancelGesture) and an optional hint
    (ConsumeEventHint).

    \value Ignore The event does not change the state of the recognizer.

    \value MayBeGesture The event changed the internal state of the recognizer,
    but it isn't clear yet if it is a gesture or not. The recognizer needs to
    filter more events to decide. Gesture recognizers in the MayBeGesture state
    may be reset automatically if they take too long to recognize gestures.

    \value TriggerGesture The gesture has been triggered and the appropriate
    QGesture object will be delivered to the target as a part of a
    QGestureEvent.

    \value FinishGesture The gesture has been finished successfully and the
    appropriate QGesture object will be delivered to the target as a part of a
    QGestureEvent.

    \value CancelGesture The event made it clear that it is not a gesture. If
    the gesture recognizer was in GestureTriggered state before, then the
    gesture is canceled and the appropriate QGesture object will be delivered
    to the target as a part of a QGestureEvent.

    \value ConsumeEventHint This hint specifies that the gesture framework
    should consume the filtered event and not deliver it to the receiver.

    \omitvalue ResultState_Mask
    \omitvalue ResultHint_Mask

    \sa QGestureRecognizer::recognize()
*/

/*!
    Constructs a new gesture recognizer object.
*/
QGestureRecognizer::QGestureRecognizer()
{
}

/*!
    Destroys the gesture recognizer.
*/
QGestureRecognizer::~QGestureRecognizer()
{
}

/*!
    This function is called by Qt to create a new QGesture object for the
    given \a target (QWidget or QGraphicsObject).

    Reimplement this function to create a custom QGesture-derived gesture
    object if necessary.

    The application takes ownership of the created gesture object.
*/
QGesture *QGestureRecognizer::create(QObject *target)
{
    Q_UNUSED(target);
    return new QGesture;
}

/*!
    This function is called by the framework to reset a given \a gesture.

    Reimplement this function to implement additional requirements for custom QGesture
    objects. This may be necessary if you implement a custom QGesture whose properties
    need special handling when the gesture is reset.
*/
void QGestureRecognizer::reset(QGesture *gesture)
{
    if (gesture) {
        QGesturePrivate *d = gesture->d_func();
        d->state = Qt::NoGesture;
        d->hotSpot = QPointF();
        d->sceneHotSpot = QPointF();
        d->isHotSpotSet = false;
    }
}

/*!
    \fn QGestureRecognizer::recognize(QGesture *gesture, QObject *watched, QEvent *event)

    Handles the given \a event for the \a watched object, updating the state of the \a gesture
    object as required, and returns a suitable result for the current recognition step.

    This function is called by the framework to allow the recognizer to filter input events
    dispatched to QWidget or QGraphicsObject instances that it is monitoring.

    The result reflects how much of the gesture has been recognized. The state of the
    \a gesture object is set depending on the result.

    \sa QGestureRecognizer::Result
*/

/*!
    Registers the given \a recognizer in the gesture framework and returns a gesture ID
    for it.

    The application takes ownership of the \a recognizer and returns the gesture type
    ID associated with it. For gesture recognizers which handle custom QGesture
    objects (i.e., those which return Qt::CustomGesture in a QGesture::gestureType()
    function) the return value is a generated gesture ID with the Qt::CustomGesture
    flag set.

    \sa unregisterRecognizer(), QGestureRecognizer::create(), QGesture
*/
Qt::GestureType QGestureRecognizer::registerRecognizer(QGestureRecognizer *recognizer)
{
    return QGestureManager::instance()->registerGestureRecognizer(recognizer);
}

/*!
    Unregisters all gesture recognizers of the specified \a type.

    \sa registerRecognizer()
*/
void QGestureRecognizer::unregisterRecognizer(Qt::GestureType type)
{
    QGestureManager::instance()->unregisterGestureRecognizer(type);
}

QT_END_NAMESPACE

#endif // QT_NO_GESTURES
