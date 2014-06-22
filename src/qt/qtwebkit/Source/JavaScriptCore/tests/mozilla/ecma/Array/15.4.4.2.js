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
    File Name:          15.4.4.2.js
    ECMA Section:       15.4.4.2 Array.prototype.toString()
    Description:        The elements of this object are converted to strings
                        and these strings are then concatenated, separated by
                        comma characters.  The result is the same as if the
                        built-in join method were invoiked for this object
                        with no argument.
    Author:             christine@netscape.com
    Date:               7 october 1997
*/

    var SECTION = "15.4.4.2";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Array.prototype.toString";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,  "Array.prototype.toString.length",  0,  Array.prototype.toString.length );

    array[item++] = new TestCase( SECTION,  "(new Array()).toString()",     "",     (new Array()).toString() );
    array[item++] = new TestCase( SECTION,  "(new Array(2)).toString()",    ",",    (new Array(2)).toString() );
    array[item++] = new TestCase( SECTION,  "(new Array(0,1)).toString()",  "0,1",  (new Array(0,1)).toString() );
    array[item++] = new TestCase( SECTION,  "(new Array( Number.NaN, Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY)).toString()",  "NaN,Infinity,-Infinity",   (new Array( Number.NaN, Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY)).toString() );

    array[item++] = new TestCase( SECTION,  "(new Array( Boolean(1), Boolean(0))).toString()",   "true,false",   (new Array(Boolean(1),Boolean(0))).toString() );
    array[item++] = new TestCase( SECTION,  "(new Array(void 0,null)).toString()",    ",",    (new Array(void 0,null)).toString() );

    var EXPECT_STRING = "";
    var MYARR = new Array();

    for ( var i = -50; i < 50; i+= 0.25 ) {
        MYARR[MYARR.length] = i;
        EXPECT_STRING += i +",";
    }

    EXPECT_STRING = EXPECT_STRING.substring( 0, EXPECT_STRING.length -1 );

    array[item++] = new TestCase( SECTION, "MYARR.toString()",  EXPECT_STRING,  MYARR.toString() );


    return ( array );
}
function test() {
    for ( tc=0 ; tc < testcases.length; tc++ ) {
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+ testcases[tc].actual );
        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }
    stopTest();
    return ( testcases );
}
