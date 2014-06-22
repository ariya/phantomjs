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

#include <QtGui>


//![0]
struct StringEvent : public QEvent
{
    StringEvent(const QString &val)
    : QEvent(QEvent::Type(QEvent::User+1)),
      value(val) {}

    QString value;
};
//![0]

//![1]
class StringTransition : public QAbstractTransition
{
    Q_OBJECT

public:
    StringTransition(const QString &value)
        : m_value(value) {}

protected:
    virtual bool eventTest(QEvent *e)
    {
        if (e->type() != QEvent::Type(QEvent::User+1)) // StringEvent
            return false;
        StringEvent *se = static_cast<StringEvent*>(e);
        return (m_value == se->value);
    }

    virtual void onTransition(QEvent *) {}

private:
    QString m_value;
};
//![1]

int main(int argv, char **args)
{
    QApplication app(argv, args);

//![2]
    QStateMachine machine;
    QState *s1 = new QState();
    QState *s2 = new QState();
    QFinalState *done = new QFinalState();

    StringTransition *t1 = new StringTransition("Hello");
    t1->setTargetState(s2);
    s1->addTransition(t1);
    StringTransition *t2 = new StringTransition("world");
    t2->setTargetState(done);
    s2->addTransition(t2);

    machine.addState(s1);
    machine.addState(s2);
    machine.addState(done);
    machine.setInitialState(s1);
//![2]

//![3]
    machine.postEvent(new StringEvent("Hello"));
    machine.postEvent(new StringEvent("world"));
//![3]

    return app.exec();
}

#include "main4.moc"

