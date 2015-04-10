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

//! [0]
int number = 6;

void method1()
{
    number *= 5;
    number /= 4;
}

void method2()
{
    number *= 3;
    number /= 2;
}
//! [0]


//! [1]
// method1()
number *= 5;        // number is now 30
number /= 4;        // number is now 7

// method2()
number *= 3;        // number is now 21
number /= 2;        // number is now 10
//! [1]


//! [2]
// Thread 1 calls method1()
number *= 5;        // number is now 30

// Thread 2 calls method2().
//
// Most likely Thread 1 has been put to sleep by the operating
// system to allow Thread 2 to run.
number *= 3;        // number is now 90
number /= 2;        // number is now 45

// Thread 1 finishes executing.
number /= 4;        // number is now 11, instead of 10
//! [2]


//! [3]
QMutex mutex;
int number = 6;

void method1()
{
    mutex.lock();
    number *= 5;
    number /= 4;
    mutex.unlock();
}

void method2()
{
    mutex.lock();
    number *= 3;
    number /= 2;
    mutex.unlock();
}
//! [3]


//! [4]
int complexFunction(int flag)
{
    mutex.lock();

    int retVal = 0;

    switch (flag) {
    case 0:
    case 1:
        retVal = moreComplexFunction(flag);
        break;
    case 2:
        {
            int status = anotherFunction();
            if (status < 0) {
                mutex.unlock();
                return -2;
            }
            retVal = status + flag;
        }
        break;
    default:
        if (flag > 10) {
            mutex.unlock();
            return -1;
        }
        break;
    }

    mutex.unlock();
    return retVal;
}
//! [4]


//! [5]
int complexFunction(int flag)
{
    QMutexLocker locker(&mutex);

    int retVal = 0;

    switch (flag) {
    case 0:
    case 1:
        return moreComplexFunction(flag);
    case 2:
        {
            int status = anotherFunction();
            if (status < 0)
                return -2;
            retVal = status + flag;
        }
        break;
    default:
        if (flag > 10)
            return -1;
        break;
    }

    return retVal;
}
//! [5]


//! [6]
class SignalWaiter
{
private:
    QMutexLocker locker;

public:
    SignalWaiter(QMutex *mutex)
        : locker(mutex)
    {
    }

    void waitForSignal()
    {
        ...
        while (!signalled)
            waitCondition.wait(locker.mutex());
        ...
    }
};
//! [6]
