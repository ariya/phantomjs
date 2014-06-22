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
    File Name:          15.9.5.5.js
    ECMA Section:       15.9.5.5
    Description:        Date.prototype.getYear

    This function is specified here for backwards compatibility only. The
    function getFullYear is much to be preferred for nearly all purposes,
    because it avoids the "year 2000 problem."

        1.  Let t be this time value.
    2.  If t is NaN, return NaN.
    3.  Return YearFromTime(LocalTime(t)) 1900.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "15.9.5.5";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Date.prototype.getYear()";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    var TZ_ADJUST = TZ_DIFF * msPerHour;

    // get the current time
    var now = (new Date()).valueOf();

    // get time for 29 feb 2000

    var UTC_FEB_29_2000 = TIME_2000 + 31*msPerDay + 28*msPerHour;

    addTestCase( now );
    addTestCase( TIME_YEAR_0 );
    addTestCase( TIME_1970 );
    addTestCase( TIME_1900 );
    addTestCase( TIME_2000 );
    addTestCase( UTC_FEB_29_2000 );

    testcases[tc++] = new TestCase( SECTION,
                                    "(new Date(NaN)).getYear()",
                                    NaN,
                                    (new Date(NaN)).getYear() );

    testcases[tc++] = new TestCase( SECTION,
                                    "Date.prototype.getYear.length",
                                    0,
                                    Date.prototype.getYear.length );

    test();
function addTestCase( t ) {
    testcases[tc++] = new TestCase( SECTION,
                                    "(new Date("+t+")).getYear()",
                                    GetYear(YearFromTime(LocalTime(t))),
                                    (new Date(t)).getYear() );

    testcases[tc++] = new TestCase( SECTION,
                                    "(new Date("+(t+1)+")).getYear()",
                                    GetYear(YearFromTime(LocalTime(t+1))),
                                    (new Date(t+1)).getYear() );

    testcases[tc++] = new TestCase( SECTION,
                                    "(new Date("+(t-1)+")).getYear()",
                                    GetYear(YearFromTime(LocalTime(t-1))),
                                    (new Date(t-1)).getYear() );

    testcases[tc++] = new TestCase( SECTION,
                                    "(new Date("+(t-TZ_ADJUST)+")).getYear()",
                                    GetYear(YearFromTime(LocalTime(t-TZ_ADJUST))),
                                    (new Date(t-TZ_ADJUST)).getYear() );

    testcases[tc++] = new TestCase( SECTION,
                                    "(new Date("+(t+TZ_ADJUST)+")).getYear()",
                                    GetYear(YearFromTime(LocalTime(t+TZ_ADJUST))),
                                    (new Date(t+TZ_ADJUST)).getYear() );
}
function GetYear( year ) {
/*
    if ( year >= 1900 && year < 2000 ) {
        return year - 1900;
    } else {
        return year;
    }
*/
    return year - 1900;
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
