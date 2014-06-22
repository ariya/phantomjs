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
    File Name:          15.4.2.1-1.js
    ECMA Section:       15.4.2.1 new Array( item0, item1, ... )
    Description:        This description only applies of the constructor is
                        given two or more arguments.

                        The [[Prototype]] property of the newly constructed
                        object is set to the original Array prototype object,
                        the one that is the initial value of Array.prototype
                        (15.4.3.1).

                        The [[Class]] property of the newly constructed object
                        is set to "Array".

                        The length property of the newly constructed object is
                        set to the number of arguments.

                        The 0 property of the newly constructed object is set
                        to item0... in general, for as many arguments as there
                        are, the k property of the newly constructed object is
                        set to argument k, where the first argument is
                        considered to be argument number 0.

                        This file tests the typeof the newly constructed object.

    Author:             christine@netscape.com
    Date:               7 october 1997
*/

    var SECTION = "15.4.2.1-1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "The Array Constructor:  new Array( item0, item1, ...)";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();


function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,	"typeof new Array(1,2)",        "object",           typeof new Array(1,2) );
    array[item++] = new TestCase( SECTION,	"(new Array(1,2)).toString",    Array.prototype.toString,    (new Array(1,2)).toString );
    array[item++] = new TestCase( SECTION,
                                    "var arr = new Array(1,2,3); arr.getClass = Object.prototype.toString; arr.getClass()",
                                    "[object Array]",
                                    eval("var arr = new Array(1,2,3); arr.getClass = Object.prototype.toString; arr.getClass()") );

    array[item++] = new TestCase( SECTION,	"(new Array(1,2)).length",      2,                  (new Array(1,2)).length );
    array[item++] = new TestCase( SECTION,	"var arr = (new Array(1,2)); arr[0]",  1,           eval("var arr = (new Array(1,2)); arr[0]") );
    array[item++] = new TestCase( SECTION,	"var arr = (new Array(1,2)); arr[1]",  2,           eval("var arr = (new Array(1,2)); arr[1]") );
    array[item++] = new TestCase( SECTION,	"var arr = (new Array(1,2)); String(arr)",  "1,2",  eval("var arr = (new Array(1,2)); String(arr)") );

    return ( array );
}
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
