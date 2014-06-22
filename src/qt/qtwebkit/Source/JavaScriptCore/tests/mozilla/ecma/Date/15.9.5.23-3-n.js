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
    File Name:          15.9.5.23-3-n.js
    ECMA Section:       15.9.5.23
    Description:        Date.prototype.setTime

    1.  If the this value is not a Date object, generate a runtime error.
    2.  Call ToNumber(time).
    3.  Call TimeClip(Result(1)).
    4.  Set the [[Value]] property of the this value to Result(2).
    5.  Return the value of the [[Value]] property of the this value.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "15.9.5.23-3-n";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Date.prototype.setTime()";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    var MYDATE = new MyDate(TIME_1970);

    testcases[tc++] = new TestCase( SECTION,
                                    "MYDATE.setTime(TIME_2000)",
                                    "error",
                                    MYDATE.setTime(TIME_2000) );

function MyDate(value) {
    this.value = value;
    this.setTime = Date.prototype.setTime;
    return this;
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
