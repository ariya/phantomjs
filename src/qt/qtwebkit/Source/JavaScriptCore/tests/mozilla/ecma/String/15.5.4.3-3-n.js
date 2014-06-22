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
    File Name:          15.5.4.3-3-n.js
    ECMA Section:       15.5.4.3 String.prototype.valueOf()

    Description:        Returns this string value.

                        The valueOf function is not generic; it generates a
                        runtime error if its this value is not a String object.
                        Therefore it connot be transferred to the other kinds of
                        objects for use as a method.

    Author:             christine@netscape.com
    Date:               1 october 1997
*/


    var SECTION = "15.5.4.3-3-n";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "String.prototype.valueOf";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].actual = eval(testcases[tc].actual );
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
function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,
                                  "var valof=String.prototype.valueOf; astring=new Number(); astring.valueOf = valof; astring.valof()",
                                  "error",
                                  "var valof=String.prototype.valueOf; astring=new Number(); astring.valueOf = valof; astring.valueOf()" );

    return ( array );
}
