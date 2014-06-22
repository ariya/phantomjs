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
void myFunction(bool useSubClass)
{
    MyClass *p = useSubClass ? new MyClass() : new MySubClass;
    QIODevice *device = handsOverOwnership();

    if (m_value > 3) {
        delete p;
        delete device;
        return;
    }

    try {
        process(device);
    }
    catch (...) {
        delete p;
        delete device;
        throw;
    }

    delete p;
    delete device;
}
//! [0]

//! [1]
void myFunction(bool useSubClass)
{
    // assuming that MyClass has a virtual destructor
    QScopedPointer<MyClass> p(useSubClass ? new MyClass() : new MySubClass);
    QScopedPointer<QIODevice> device(handsOverOwnership());

    if (m_value > 3)
        return;

    process(device);
}
//! [1]

//! [2]
    const QWidget *const p = new QWidget();
    // is equivalent to:
    const QScopedPointer<const QWidget> p(new QWidget());

    QWidget *const p = new QWidget();
    // is equivalent to:
    const QScopedPointer<QWidget> p(new QWidget());

    const QWidget *p = new QWidget();
    // is equivalent to:
    QScopedPointer<const QWidget> p(new QWidget());
//! [2]

//! [3]
if (scopedPointer) {
    ...
}
//! [3]

//! [4]
class MyPrivateClass; // forward declare MyPrivateClass

class MyClass
{
private:
    QScopedPointer<MyPrivateClass> privatePtr; // QScopedPointer to forward declared class

public:
    MyClass(); // OK
    inline ~MyClass() {} // VIOLATION - Destructor must not be inline

private:
    Q_DISABLE_COPY(MyClass) // OK - copy constructor and assignment operators
                             // are now disabled, so the compiler won't implicitely
                             // generate them.
};
//! [4]

//! [5]
// this QScopedPointer deletes its data using the delete[] operator:
QScopedPointer<int, QScopedPointerArrayDeleter<int> > arrayPointer(new int[42]);

// this QScopedPointer frees its data using free():
QScopedPointer<int, QScopedPointerPodDeleter> podPointer(reinterpret_cast<int *>(malloc(42)));

// this struct calls "myCustomDeallocator" to delete the pointer
struct ScopedPointerCustomDeleter
{
    static inline void cleanup(MyCustomClass *pointer)
    {
        myCustomDeallocator(pointer);
    }
};

// QScopedPointer using a custom deleter:
QScopedPointer<MyCustomClass, ScopedPointerCustomDeleter> customPointer(new MyCustomClass);
//! [5]
