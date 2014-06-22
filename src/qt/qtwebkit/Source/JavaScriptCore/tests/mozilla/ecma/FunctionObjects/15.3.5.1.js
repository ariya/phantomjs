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
    File Name:          15.3.5.1.js
    ECMA Section:       Function.length
    Description:

    The value of the length property is usually an integer that indicates the
    "typical" number of arguments expected by the function.  However, the
    language permits the function to be invoked with some other number of
    arguments. The behavior of a function when invoked on a number of arguments
    other than the number specified by its length property depends on the function.

    this test needs a 1.2 version check.

    http://scopus.mcom.com/bugsplat/show_bug.cgi?id=104204


    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "15.3.5.1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Function.length";
    var BUGNUMBER="104204";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    var f = new Function( "a","b", "c", "return f.length");

    testcases[tc++] = new TestCase( SECTION,
        'var f = new Function( "a","b", "c", "return f.length"); f()',
        3,
        f() );


    testcases[tc++] = new TestCase( SECTION,
        'var f = new Function( "a","b", "c", "return f.length"); f(1,2,3,4,5)',
        3,
        f(1,2,3,4,5) );


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
