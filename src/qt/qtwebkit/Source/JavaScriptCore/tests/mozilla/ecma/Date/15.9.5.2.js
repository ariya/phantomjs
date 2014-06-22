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
    File Name:          15.9.5.2.js
    ECMA Section:       15.9.5.2 Date.prototype.toString
    Description:
    This function returns a string value. The contents of the string are
    implementation dependent, but are intended to represent the Date in a
    convenient, human-readable form in the current time zone.

    The toString function is not generic; it generates a runtime error if its
    this value is not a Date object. Therefore it cannot be transferred to
    other kinds of objects for use as a method.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "15.9.5.2";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Date.prototype.toString";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    testcases[tc++] = new TestCase( SECTION,
                                    "Date.prototype.toString.length",
                                    0,
                                    Date.prototype.toString.length );

    var now = new Date();

    // can't test the content of the string, but can verify that the string is
    // parsable by Date.parse

    testcases[tc++] = new TestCase( SECTION,
                                    "Math.abs(Date.parse(now.toString()) - now.valueOf()) < 1000",
                                    true,
                                    Math.abs(Date.parse(now.toString()) - now.valueOf()) < 1000 );

    testcases[tc++] = new TestCase( SECTION,
                                    "typeof now.toString()",
                                    "string",
                                    typeof now.toString() );
    // 1970

    TZ_ADJUST = TZ_DIFF * msPerHour;

    testcases[tc++] = new TestCase( SECTION,
                                    "Date.parse( (new Date(0)).toString() )",
                                    0,
                                    Date.parse( (new Date(0)).toString() ) )

    testcases[tc++] = new TestCase( SECTION,
                                    "Date.parse( (new Date("+TZ_ADJUST+")).toString() )",
                                    TZ_ADJUST,
                                    Date.parse( (new Date(TZ_ADJUST)).toString() ) )

    // 1900
    testcases[tc++] = new TestCase( SECTION,
                                    "Date.parse( (new Date("+TIME_1900+")).toString() )",
                                    TIME_1900,
                                    Date.parse( (new Date(TIME_1900)).toString() ) )

    testcases[tc++] = new TestCase( SECTION,
                                    "Date.parse( (new Date("+TIME_1900 -TZ_ADJUST+")).toString() )",
                                    TIME_1900 -TZ_ADJUST,
                                    Date.parse( (new Date(TIME_1900 -TZ_ADJUST)).toString() ) )

    // 2000
    testcases[tc++] = new TestCase( SECTION,
                                    "Date.parse( (new Date("+TIME_2000+")).toString() )",
                                    TIME_2000,
                                    Date.parse( (new Date(TIME_2000)).toString() ) )

    testcases[tc++] = new TestCase( SECTION,
                                    "Date.parse( (new Date("+TIME_2000 -TZ_ADJUST+")).toString() )",
                                    TIME_2000 -TZ_ADJUST,
                                    Date.parse( (new Date(TIME_2000 -TZ_ADJUST)).toString() ) )

    // 29 Feb 2000

    var UTC_29_FEB_2000 = TIME_2000 + 31*msPerDay + 28*msPerDay;
    testcases[tc++] = new TestCase( SECTION,
                                    "Date.parse( (new Date("+UTC_29_FEB_2000+")).toString() )",
                                    UTC_29_FEB_2000,
                                    Date.parse( (new Date(UTC_29_FEB_2000)).toString() ) )

    testcases[tc++] = new TestCase( SECTION,
                                    "Date.parse( (new Date("+(UTC_29_FEB_2000-1000)+")).toString() )",
                                    UTC_29_FEB_2000-1000,
                                    Date.parse( (new Date(UTC_29_FEB_2000-1000)).toString() ) )


    testcases[tc++] = new TestCase( SECTION,
                                    "Date.parse( (new Date("+(UTC_29_FEB_2000-TZ_ADJUST)+")).toString() )",
                                    UTC_29_FEB_2000-TZ_ADJUST,
                                    Date.parse( (new Date(UTC_29_FEB_2000-TZ_ADJUST)).toString() ) )
    // 2O05

    var UTC_1_JAN_2005 = TIME_2000 + TimeInYear(2000) + TimeInYear(2001) +
    TimeInYear(2002) + TimeInYear(2003) + TimeInYear(2004);
    testcases[tc++] = new TestCase( SECTION,
                                    "Date.parse( (new Date("+UTC_1_JAN_2005+")).toString() )",
                                    UTC_1_JAN_2005,
                                    Date.parse( (new Date(UTC_1_JAN_2005)).toString() ) )

    testcases[tc++] = new TestCase( SECTION,
                                    "Date.parse( (new Date("+(UTC_1_JAN_2005-1000)+")).toString() )",
                                    UTC_1_JAN_2005-1000,
                                    Date.parse( (new Date(UTC_1_JAN_2005-1000)).toString() ) )

    testcases[tc++] = new TestCase( SECTION,
                                    "Date.parse( (new Date("+(UTC_1_JAN_2005-TZ_ADJUST)+")).toString() )",
                                    UTC_1_JAN_2005-TZ_ADJUST,
                                    Date.parse( (new Date(UTC_1_JAN_2005-TZ_ADJUST)).toString() ) )

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
