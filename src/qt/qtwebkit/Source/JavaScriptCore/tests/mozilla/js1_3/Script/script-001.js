/* The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, released March
 * 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 * 
 */
/**
    File Name:          script-001.js
    Section:
    Description:        new NativeScript object


js> parseInt(123,"hi")
123
js> parseInt(123, "blah")
123
js> s
js: s is not defined
js> s = new Script

undefined;


js> s = new Script()

undefined;


js> s.getJSClass
js> s.getJSClass = Object.prototype.toString
function toString() {
        [native code]
}

js> s.getJSClass()
[object Script]
js> s.compile( "return 3+4" )
js: JavaScript exception: javax.javascript.EvaluatorException: "<Scr
js> s.compile( "3+4" )

3 + 4;


js> typeof s
function
js> s()
Jit failure!
invalid opcode: 1
Jit Pass1 Failure!
javax/javascript/gen/c13 initScript (Ljavax/javascript/Scriptable;)V
An internal JIT error has occurred.  Please report this with .class
jit-bugs@itools.symantec.com

7
js> s.compile("3+4")

3 + 4;


js> s()
Jit failure!
invalid opcode: 1
Jit Pass1 Failure!
javax/javascript/gen/c17 initScript (Ljavax/javascript/Scriptable;)V
An internal JIT error has occurred.  Please report this with .class
jit-bugs@itools.symantec.com

7
js> quit()

C:\src\ns_priv\js\tests\ecma>shell

C:\src\ns_priv\js\tests\ecma>java -classpath c:\cafe\java\JavaScope;
:\src\ns_priv\js\tests javax.javascript.examples.Shell
Symantec Java! JustInTime Compiler Version 210.054 for JDK 1.1.2
Copyright (C) 1996-97 Symantec Corporation

js> s = new Script("3+4")

3 + 4;


js> s()
7
js> s2 = new Script();

undefined;


js> s.compile( "3+4")

3 + 4;


js> s()
Jit failure!
invalid opcode: 1
Jit Pass1 Failure!
javax/javascript/gen/c7 initScript (Ljavax/javascript/Scriptable;)V
An internal JIT error has occurred.  Please report this with .class
jit-bugs@itools.symantec.com

7
js> quit()
    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "script-001";
    var VERSION = "JS1_3";
    var TITLE   = "NativeScript";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    var s = new Script();
    s.getJSClass = Object.prototype.toString;

    testcases[tc++] = new TestCase( SECTION,
        "var s = new Script(); typeof s",
        "function",
        typeof s );

    testcases[tc++] = new TestCase( SECTION,
        "s.getJSClass()",
        "[object Script]",
        s.getJSClass() );

    test();
function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+
                            testcases[tc].actual );

        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }
    stopTest();
    return ( testcases );
}
