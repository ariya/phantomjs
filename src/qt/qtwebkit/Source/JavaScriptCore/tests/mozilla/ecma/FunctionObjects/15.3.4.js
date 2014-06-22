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
    File Name:          15.3.4.js
    ECMA Section:       15.3.4  Properties of the Function Prototype Object

    Description:        The Function prototype object is itself a Function
                        object ( its [[Class]] is "Function") that, when
                        invoked, accepts any arguments and returns undefined.

                        The value of the internal [[Prototype]] property
                        object is the Object prototype object.

                        It is a function with an "empty body"; if it is
                        invoked, it merely returns undefined.

                        The Function prototype object does not have a valueOf
                        property of its own; however it inherits the valueOf
                        property from the Object prototype Object.

    Author:             christine@netscape.com
    Date:               28 october 1997

*/
    var SECTION = "15.3.4";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Properties of the Function Prototype Object";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;
    array[item++] = new TestCase(   SECTION,
                                    "var myfunc = Function.prototype; myfunc.toString = Object.prototype.toString; myfunc.toString()",
                                    "[object Function]",
                                    eval("var myfunc = Function.prototype; myfunc.toString = Object.prototype.toString; myfunc.toString()"));


//  array[item++] = new TestCase( SECTION,  "Function.prototype.__proto__",     Object.prototype,           Function.prototype.__proto__ );
    array[item++] = new TestCase( SECTION,  "Function.prototype.valueOf",       Object.prototype.valueOf,   Function.prototype.valueOf );
    array[item++] = new TestCase( SECTION,  "Function.prototype()",             (void 0),                   Function.prototype() );
    array[item++] = new TestCase( SECTION,  "Function.prototype(1,true,false,'string', new Date(),null)",  (void 0), Function.prototype(1,true,false,'string', new Date(),null) );
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
