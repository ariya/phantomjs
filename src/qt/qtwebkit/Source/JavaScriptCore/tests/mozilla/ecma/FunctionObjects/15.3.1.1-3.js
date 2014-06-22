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
    File Name:          15.3.1.1-3.js
    ECMA Section:       15.3.1.1 The Function Constructor Called as a Function

                        new Function(p1, p2, ..., pn, body )

    Description:        The last argument specifies the body (executable code)
                        of a function; any preceeding arguments sepcify formal
                        parameters.

                        See the text for description of this section.

                        This test examples from the specification.

    Author:             christine@netscape.com
    Date:               28 october 1997

*/
    var SECTION = "15.3.1.1-3";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "The Function Constructor Called as a Function";

    writeHeaderToLog( SECTION + " "+ TITLE);


    var testcases = new Array();

    var args = "";

    for ( var i = 0; i < 2000; i++ ) {
        args += "arg"+i;
        if ( i != 1999 ) {
            args += ",";
        }
    }

    var s = "";

    for ( var i = 0; i < 2000; i++ ) {
        s += ".0005";
        if ( i != 1999 ) {
            s += ",";
        }
    }

    MyFunc = Function( args, "var r=0; for (var i = 0; i < MyFunc.length; i++ ) { if ( eval('arg'+i) == void 0) break; else r += eval('arg'+i); }; return r");
    MyObject = Function( args, "for (var i = 0; i < MyFunc.length; i++ ) { if ( eval('arg'+i) == void 0) break; eval('this.arg'+i +'=arg'+i); };");

    var MY_OB = eval( "MyFunc("+ s +")" );

    testcases[testcases.length] = new TestCase( SECTION, "MyFunc.length",                       2000,         MyFunc.length );
    testcases[testcases.length] = new TestCase( SECTION, "var MY_OB = eval('MyFunc(s)')",       1,            MY_OB );
    testcases[testcases.length] = new TestCase( SECTION, "var MY_OB = eval('MyFunc(s)')",       1,            eval("var MY_OB = MyFunc("+s+"); MY_OB") );

    testcases[testcases.length] = new TestCase( SECTION, "MyObject.length",                       2000,         MyObject.length );

    testcases[testcases.length] = new TestCase( SECTION, "FUN1 = Function( 'a','b','c', 'return FUN1.length' ); FUN1.length",     3, eval("FUN1 = Function( 'a','b','c', 'return FUN1.length' ); FUN1.length") );
    testcases[testcases.length] = new TestCase( SECTION, "FUN1 = Function( 'a','b','c', 'return FUN1.length' ); FUN1()",          3, eval("FUN1 = Function( 'a','b','c', 'return FUN1.length' ); FUN1()") );
    testcases[testcases.length] = new TestCase( SECTION, "FUN1 = Function( 'a','b','c', 'return FUN1.length' ); FUN1(1,2,3,4,5)", 3, eval("FUN1 = Function( 'a','b','c', 'return FUN1.length' ); FUN1(1,2,3,4,5)") );

    test();

function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+ testcases[tc].actual );

        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }
    stopTest();
    return ( testcases );
}
