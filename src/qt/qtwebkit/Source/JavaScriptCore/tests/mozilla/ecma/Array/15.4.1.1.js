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
    File Name:          15.4.1.1.js
    ECMA Section:       15.4.1 Array( item0, item1,... )

    Description:        When Array is called as a function rather than as a
                        constructor, it creates and initializes a new array
                        object.  Thus, the function call Array(...) is
                        equivalent to the object creation new Array(...) with
                        the same arguments.

                        An array is created and returned as if by the expression
                        new Array( item0, item1, ... ).

    Author:             christine@netscape.com
    Date:               7 october 1997
*/
    var SECTION = "15.4.1.1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Array Constructor Called as a Function";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function ToUint32( n ) {
    n = Number( n );
    if( isNaN(n) || n == 0 || n == Number.POSITIVE_INFINITY ||
        n == Number.NEGATIVE_INFINITY ) {
        return 0;
    }
    var sign = n < 0 ? -1 : 1;

    return ( sign * ( n * Math.floor( Math.abs(n) ) ) ) % Math.pow(2, 32);
}

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,	"typeof Array(1,2)",        "object",           typeof Array(1,2) );
    array[item++] = new TestCase( SECTION,	"(Array(1,2)).toString",    Array.prototype.toString,    (Array(1,2)).toString );
    array[item++] = new TestCase( SECTION,
                                    "var arr = Array(1,2,3); arr.toString = Object.prototype.toString; arr.toString()",
                                    "[object Array]",
                                    eval("var arr = Array(1,2,3); arr.toString = Object.prototype.toString; arr.toString()") );


    array[item++] = new TestCase( SECTION,	"(Array(1,2)).length",      2,                  (Array(1,2)).length );
    array[item++] = new TestCase( SECTION,	"var arr = (Array(1,2)); arr[0]",  1,           eval("var arr = (Array(1,2)); arr[0]") );
    array[item++] = new TestCase( SECTION,	"var arr = (Array(1,2)); arr[1]",  2,           eval("var arr = (Array(1,2)); arr[1]") );
    array[item++] = new TestCase( SECTION,	"var arr = (Array(1,2)); String(arr)",  "1,2",  eval("var arr = (Array(1,2)); String(arr)") );

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
