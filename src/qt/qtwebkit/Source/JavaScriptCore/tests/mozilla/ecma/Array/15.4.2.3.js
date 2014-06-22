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
    File Name:          15.4.2.3.js
    ECMA Section:       15.4.2.3 new Array()
    Description:        The [[Prototype]] property of the newly constructed
                        object is set to the origianl Array prototype object,
                        the one that is the initial value of Array.prototype.
                        The [[Class]] property of the new object is set to
                        "Array".  The length of the object is set to 0.

    Author:             christine@netscape.com
    Date:               7 october 1997
*/

    var SECTION = "15.4.2.3";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "The Array Constructor:  new Array()";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;
    array[item++] = new TestCase( SECTION,	"new   Array() +''",        "",                 (new Array()) +"" );
    array[item++] = new TestCase( SECTION,	"typeof new Array()",       "object",           (typeof new Array()) );
    array[item++] = new TestCase(   SECTION,
                                    "var arr = new Array(); arr.getClass = Object.prototype.toString; arr.getClass()",
                                    "[object Array]",
                                    eval("var arr = new Array(); arr.getClass = Object.prototype.toString; arr.getClass()") );

    array[item++] = new TestCase( SECTION,	"(new Array()).length",     0,                  (new Array()).length );
    array[item++] = new TestCase( SECTION,	"(new Array()).toString == Array.prototype.toString",   true,       (new Array()).toString == Array.prototype.toString );
    array[item++] = new TestCase( SECTION,	"(new Array()).join  == Array.prototype.join",          true,       (new Array()).join  == Array.prototype.join );
    array[item++] = new TestCase( SECTION,	"(new Array()).reverse == Array.prototype.reverse",     true,       (new Array()).reverse  == Array.prototype.reverse );
    array[item++] = new TestCase( SECTION,	"(new Array()).sort  == Array.prototype.sort",          true,       (new Array()).sort  == Array.prototype.sort );

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
