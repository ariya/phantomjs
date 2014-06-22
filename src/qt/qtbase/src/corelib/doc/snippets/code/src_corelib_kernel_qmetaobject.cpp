/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/


void wrapInFunction()
{

//! [0]
class MyClass : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("author", "Sabrina Schweinsteiger")
    Q_CLASSINFO("url", "http://doc.moosesoft.co.uk/1.0/")

public:
    ...
};
//! [0]


//! [1]
QByteArray normType = QMetaObject::normalizedType(" int    const  *");
// normType is now "const int*"
//! [1]


//! [2]
QMetaObject::invokeMethod(thread, "quit",
                          Qt::QueuedConnection);
//! [2]


//! [3]
QMetaObject::invokeMethod: Unable to handle unregistered datatype 'MyType'
//! [3]


//! [4]
QString retVal;
QMetaObject::invokeMethod(obj, "compute", Qt::DirectConnection,
                          Q_RETURN_ARG(QString, retVal),
                          Q_ARG(QString, "sqrt"),
                          Q_ARG(int, 42),
                          Q_ARG(double, 9.7));
//! [4]


//! [5]
class MyClass
{
    Q_OBJECT
    Q_CLASSINFO("author", "Sabrina Schweinsteiger")
    Q_CLASSINFO("url", "http://doc.moosesoft.co.uk/1.0/")

public:
    ...
};
//! [5]


//! [propertyCount]
const QMetaObject* metaObject = obj->metaObject();
QStringList properties;
for(int i = metaObject->propertyOffset(); i < metaObject->propertyCount(); ++i)
    properties << QString::fromLatin1(metaObject->property(i).name());
//! [propertyCount]


//! [methodCount]
const QMetaObject* metaObject = obj->metaObject();
QStringList methods;
for(int i = metaObject->methodOffset(); i < metaObject->methodCount(); ++i)
    methods << QString::fromLatin1(metaObject->method(i).methodSignature());
//! [methodCount]

//! [6]
int methodIndex = pushButton->metaObject()->indexOfMethod("animateClick()");
QMetaMethod method = metaObject->method(methodIndex);
method.invoke(pushButton, Qt::QueuedConnection);
//! [6]

//! [7]
QMetaMethod::invoke: Unable to handle unregistered datatype 'MyType'
//! [7]

//! [8]
QString retVal;
QByteArray normalizedSignature = QMetaObject::normalizedSignature("compute(QString, int, double)");
int methodIndex = obj->metaObject()->indexOfMethod(normalizedSignature);
QMetaMethod method = metaObject->method(methodIndex);
method.invoke(obj,
              Qt::DirectConnection,
              Q_RETURN_ARG(QString, retVal),
              Q_ARG(QString, "sqrt"),
              Q_ARG(int, 42),
              Q_ARG(double, 9.7));
//! [8]

//! [9]
QMetaMethod destroyedSignal = QMetaMethod::fromSignal(&QObject::destroyed);
//! [9]

}
