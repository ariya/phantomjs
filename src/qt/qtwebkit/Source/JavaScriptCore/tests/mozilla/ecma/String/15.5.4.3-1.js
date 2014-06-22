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
    File Name:          15.5.4.3-1.js
    ECMA Section:       15.5.4.3 String.prototype.valueOf()

    Description:        Returns this string value.

                        The valueOf function is not generic; it generates a
                        runtime error if its this value is not a String object.
                        Therefore it connot be transferred to the other kinds of
                        objects for use as a method.

    Author:             christine@netscape.com
    Date:               1 october 1997
*/

    var SECTION = "15.5.4.3-1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "String.prototype.valueOf";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,   "String.prototype.valueOf.length", 0,      String.prototype.valueOf.length );

    array[item++] = new TestCase( SECTION,   "String.prototype.valueOf()",        "",     String.prototype.valueOf() );
    array[item++] = new TestCase( SECTION,   "(new String()).valueOf()",          "",     (new String()).valueOf() );
    array[item++] = new TestCase( SECTION,   "(new String(\"\")).valueOf()",      "",     (new String("")).valueOf() );
    array[item++] = new TestCase( SECTION,   "(new String( String() )).valueOf()","",    (new String(String())).valueOf() );
    array[item++] = new TestCase( SECTION,   "(new String( \"h e l l o\" )).valueOf()",       "h e l l o",    (new String("h e l l o")).valueOf() );
    array[item++] = new TestCase( SECTION,   "(new String( 0 )).valueOf()",       "0",    (new String(0)).valueOf() );
    return ( array );
}
function test( array ) {
    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+ testcases[tc].actual );
    }
    stopTest();
    return ( testcases );
}
