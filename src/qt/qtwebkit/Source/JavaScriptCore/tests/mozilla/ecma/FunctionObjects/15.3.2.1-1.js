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
    File Name:          15.3.2.1.js
    ECMA Section:       15.3.2.1 The Function Constructor
                        new Function(p1, p2, ..., pn, body )

    Description:        The last argument specifies the body (executable code)
                        of a function; any preceeding arguments sepcify formal
                        parameters.

                        See the text for description of this section.

                        This test examples from the specification.

    Author:             christine@netscape.com
    Date:               28 october 1997

*/
    var SECTION = "15.3.2.1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "The Function Constructor";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var MyObject = new Function( "value", "this.value = value; this.valueOf = new Function( 'return this.value' ); this.toString = new Function( 'return String(this.value);' )" );

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    var myfunc = new Function();

//    not going to test toString here since it is implementation dependent.
//    array[item++] = new TestCase( SECTION,  "myfunc.toString()",     "function anonymous() { }",    myfunc.toString() );

     myfunc.toString = Object.prototype.toString;

    array[item++] = new TestCase( SECTION,  "myfunc = new Function(); myfunc.toString = Object.prototype.toString; myfunc.toString()",
                                            "[object Function]",
                                            myfunc.toString() );
    array[item++] = new TestCase( SECTION,  "myfunc.length",                            0,                      myfunc.length );
    array[item++] = new TestCase( SECTION,  "myfunc.prototype.toString()",              "[object Object]",      myfunc.prototype.toString() );

    array[item++] = new TestCase( SECTION,  "myfunc.prototype.constructor",             myfunc,                 myfunc.prototype.constructor );
    array[item++] = new TestCase( SECTION,  "myfunc.arguments",                         null,                   myfunc.arguments );

    array[item++] = new TestCase( SECTION,  "var OBJ = new MyObject(true); OBJ.valueOf()",    true,             eval("var OBJ = new MyObject(true); OBJ.valueOf()") );
    array[item++] = new TestCase( SECTION,  "OBJ.toString()",                           "true",                 OBJ.toString() );
    array[item++] = new TestCase( SECTION,  "OBJ.toString = Object.prototype.toString; OBJ.toString()", "[object Object]",  eval("OBJ.toString = Object.prototype.toString; OBJ.toString()") );
    array[item++] = new TestCase( SECTION,  "MyObject.toString = Object.prototype.toString; MyObject.toString()",    "[object Function]",   eval("MyObject.toString = Object.prototype.toString; MyObject.toString()") );

    array[item++] = new TestCase( SECTION,  "MyObject.__proto__ == Function.prototype",     true,   MyObject.__proto__ == Function.prototype );
    array[item++] = new TestCase( SECTION,  "MyObject.length",                              1,      MyObject.length );
    array[item++] = new TestCase( SECTION,  "MyObject.prototype.constructor",               MyObject,   MyObject.prototype.constructor );
    array[item++] = new TestCase( SECTION,  "MyObject.arguments",                           null,   MyObject.arguments );

    return ( array );
}
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
