/*
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */
#include "config.h"

#include "BridgeJSC.h"
#include "JSCJSValue.h"
#include "JSObject.h"
#include "interpreter.h"
#include "qdebug.h"
#include "qobject.h"
#include "runtime_object.h"
#include "types.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>


class MyObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString testString READ testString WRITE setTestString)
    Q_PROPERTY(int testInt READ testInt WRITE setTestInt)

public:
    MyObject() : QObject(0), integer(0){}

    void setTestString(const QString &str) {
        qDebug() << "called setTestString" << str;
        string = str;
    }
    void setTestInt(int i) {
        qDebug() << "called setTestInt" << i;
        integer = i;
    }
    QString testString() const {
        qDebug() << "called testString" << string;
        return string;
    }
    int testInt() const {
        qDebug() << "called testInt" << integer;
        return integer;
    }
    QString string;
    int integer;

public Q_SLOTS:
    void foo() { qDebug() << "foo invoked"; }
};

// --------------------------------------------------------

using namespace JSC;
using namespace JSC::Bindings;

class Global : public JSNonFinalObject {
public:
    typedef JSNonFinalObject Base;

    static String className(const JSObject*) { return "global"; }
};

static char code[] =
    "myInterface.foo();\n"
    "myInterface.testString = \"Hello\";\n"
    "str = myInterface.testString;\n"
    "myInterface.testInt = 10;\n"
    "i = myInterface.testInt;\n";

int main(int argc, char** argv)
{
    // expecting a filename
    bool ret = true;
    {
        JSLock lock;
        
        // create interpreter w/ global object
        Global* global = new Global();

        // create interpreter
        RefPtr<Interpreter> interp = new Interpreter(global);
        ExecState* exec = interp->globalExec();
        
        MyObject* myObject = new MyObject;
        
        global->methodTable()->put(global, exec, Identifier("myInterface"), Instance::createRuntimeObject(Instance::QtLanguage, (void*)myObject));
        
        
        if (code) {
            // run
            Completion comp(interp->evaluate("", 0, code));
            
            if (comp.complType() == Throw) {
                qDebug() << "exception thrown";
                JSValue* exVal = comp.value();
                char* msg = exVal->toString(exec)->value(exec).ascii();
                int lineno = -1;
                if (exVal->type() == ObjectType) {
                    JSValue* lineVal = exVal->getObject()->get(exec, Identifier("line"));
                    if (lineVal->type() == NumberType)
                        lineno = int(lineVal->toNumber(exec));
                }
                if (lineno != -1)
                    fprintf(stderr,"Exception, line %d: %s\n",lineno,msg);
                else
                    fprintf(stderr,"Exception: %s\n",msg);
                ret = false;
            }
            else if (comp.complType() == ReturnValue) {
                char* msg = comp.value()->toString(interp->globalExec()).ascii();
                fprintf(stderr,"Return value: %s\n",msg);
            }
        }
        
    } // end block, so that Interpreter and global get deleted
    
    return ret ? 0 : 1;
}

#include "testqtbindings.moc"
