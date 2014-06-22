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
    File Name:          15.9.4.2.js
    ECMA Section:       15.9.4.2 Date.parse()
    Description:        The parse() function applies the to ToString() operator
                        to its argument and interprets the resulting string as
                        a date.  It returns a number, the UTC time value
                        corresponding to the date.

                        The string may be interpreted as a local time, a UTC
                        time, or a time in some other time zone, depending on
                        the contents of the string.

                        (need to test strings containing stuff with the time
                        zone specified, and verify that parse() returns the
                        correct GMT time)

                        so for any Date object x, all of these things should
                        be equal:

                        value                       tested in function:
                        x.valueOf()                 test_value()
                        Date.parse(x.toString())    test_tostring()
                        Date.parse(x.toGMTString()) test_togmt()

                        Date.parse(x.toLocaleString()) is not required to
                        produce the same number value as the preceeding three
                        expressions.  in general the value produced by
                        Date.parse is implementation dependent when given any
                        string value that could not be produced in that
                        implementation by the toString or toGMTString method.

                        value                           tested in function:
                        Date.parse( x.toLocaleString()) test_tolocale()

    Author:             christine@netscape.com
    Date:               10 july 1997

*/

    var VERSION = "ECMA_1";
    startTest();
    var SECTION = "15.9.4.2";
    var TITLE   = "Date.parse()";

    var TIME        = 0;
    var UTC_YEAR    = 1;
    var UTC_MONTH   = 2;
    var UTC_DATE    = 3;
    var UTC_DAY     = 4;
    var UTC_HOURS   = 5;
    var UTC_MINUTES = 6;
    var UTC_SECONDS = 7;
    var UTC_MS      = 8;

    var YEAR        = 9;
    var MONTH       = 10;
    var DATE        = 11;
    var DAY         = 12;
    var HOURS       = 13;
    var MINUTES     = 14;
    var SECONDS     = 15;
    var MS          = 16;
    var TYPEOF  = "object";

//  for TCMS, the testcases array must be global.
    writeHeaderToLog("15.9.4.2 Date.parse()" );
    var tc= 0;
    testcases = new Array();
    getTestCases();

//  all tests must call a function that returns an array of TestCase objects.
    test();

function getTestCases() {

    // Dates around 1970

    addNewTestCase( new Date(0),
                    "new Date(0)",
                    [0,1970,0,1,4,0,0,0,0,1969,11,31,3,16,0,0,0] );

    addNewTestCase( new Date(-1),
                    "new Date(-1)",
                    [-1,1969,11,31,3,23,59,59,999,1969,11,31,3,15,59,59,999] );
    addNewTestCase( new Date(28799999),
                    "new Date(28799999)",
                    [28799999,1970,0,1,4,7,59,59,999,1969,11,31,3,23,59,59,999] );
    addNewTestCase( new Date(28800000),
                    "new Date(28800000)",
                    [28800000,1970,0,1,4,8,0,0,0,1970,0,1,4,0,0,0,0] );

    // Dates around 2000

    addNewTestCase( new Date(946684799999),
                    "new Date(946684799999)",
                    [946684799999,1999,11,31,5,23,59,59,999,1999,11,31,5,15,59,59,999] );
    addNewTestCase( new Date(946713599999),
                    "new Date(946713599999)",
                    [946713599999,2000,0,1,6,7,59,59,999,1999,11,31,5,23,59,59,999] );
    addNewTestCase( new Date(946684800000),
                    "new Date(946684800000)",
                    [946684800000,2000,0,1,6,0,0,0,0,1999,11,31,5, 16,0,0,0] );
    addNewTestCase( new Date(946713600000),
                    "new Date(946713600000)",
                    [946713600000,2000,0,1,6,8,0,0,0,2000,0,1,6,0,0,0,0] );

    // Dates around 1900

    addNewTestCase( new Date(-2208988800000),
                    "new Date(-2208988800000)",
                    [-2208988800000,1900,0,1,1,0,0,0,0,1899,11,31,0,16,0,0,0] );

    addNewTestCase( new Date(-2208988800001),
                    "new Date(-2208988800001)",
                    [-2208988800001,1899,11,31,0,23,59,59,999,1899,11,31,0,15,59,59,999] );

    addNewTestCase( new Date(-2208960000001),
                    "new Date(-2208960000001)",
                    [-2208960000001,1900,0,1,1,7,59,59,0,1899,11,31,0,23,59,59,999] );
    addNewTestCase( new Date(-2208960000000),
                    "new Date(-2208960000000)",
                    [-2208960000000,1900,0,1,1,8,0,0,0,1900,0,1,1,0,0,0,0] );
    addNewTestCase( new Date(-2208959999999),
                    "new Date(-2208959999999)",
                    [-2208959999999,1900,0,1,1,8,0,0,1,1900,0,1,1,0,0,0,1] );

    // Dates around Feb 29, 2000

    var UTC_FEB_29_2000 = TIME_2000 + 31*msPerDay + 28*msPerDay;
    var PST_FEB_29_2000 = UTC_FEB_29_2000 + 8*msPerHour;
    addNewTestCase( new Date(UTC_FEB_29_2000),
                    "new Date(" + UTC_FEB_29_2000 +")",
                    [UTC_FEB_29_2000,2000,0,1,6,0,0,0,0,1999,11,31,5,16,0,0,0] );
    addNewTestCase( new Date(PST_FEB_29_2000),
                    "new Date(" + PST_FEB_29_2000 +")",
                    [PST_FEB_29_2000,2000,0,1,6,8.0,0,0,2000,0,1,6,0,0,0,0]);

    // Dates around Jan 1 2005

    var UTC_JAN_1_2005 = TIME_2000 + TimeInYear(2000) + TimeInYear(2001) +
    TimeInYear(2002) + TimeInYear(2003) + TimeInYear(2004);
    var PST_JAN_1_2005 = UTC_JAN_1_2005 + 8*msPerHour;

    addNewTestCase( new Date(UTC_JAN_1_2005),
                    "new Date("+ UTC_JAN_1_2005 +")",
                    [UTC_JAN_1_2005,2005,0,1,6,0,0,0,0,2004,11,31,5,16,0,0,0] );
    addNewTestCase( new Date(PST_JAN_1_2005),
                    "new Date("+ PST_JAN_1_2005 +")",
                    [PST_JAN_1_2005,2005,0,1,6,8,0,0,0,2005,0,1,6,0,0,0,0] );

}
function addNewTestCase( DateCase, DateString, ResultArray ) {
    DateCase = DateCase;

    item = testcases.length;

    testcases[item++] = new TestCase( SECTION, DateString+".getTime()", ResultArray[TIME],       DateCase.getTime() );
    testcases[item++] = new TestCase( SECTION, DateString+".valueOf()", ResultArray[TIME],       DateCase.valueOf() );
    testcases[item++] = new TestCase( SECTION, "Date.parse(" + DateCase.toString() +")",    Math.floor(ResultArray[TIME]/1000)*1000,  Date.parse(DateCase.toString()) );
    testcases[item++] = new TestCase( SECTION, "Date.parse(" + DateCase.toGMTString() +")", Math.floor(ResultArray[TIME]/1000)*1000,  Date.parse(DateCase.toGMTString()) );
/*
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCFullYear()",        ResultArray[UTC_YEAR],   DateCase.getUTCFullYear() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCMonth()",         ResultArray[UTC_MONTH],  DateCase.getUTCMonth() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCDate()",          ResultArray[UTC_DATE],   DateCase.getUTCDate() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCDay()",           ResultArray[UTC_DAY],    DateCase.getUTCDay() z inutes() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCSeconds()",       ResultArray[UTC_SECONDS],DateCase.getUTCSeconds() );
//    testcases[item++] = new TestCase( SECTION, DateString+".getUTCMilliseconds()",  ResultArray[UTC_MS],     DateCase.getUTCMilliseconds() );

    testcases[item++] = new TestCase( SECTION, DateString+".getFullYear()",         ResultArray[YEAR],       DateCase.getFullYear() );
    testcases[item++] = new TestCase( SECTION, DateString+".getMonth()",            ResultArray[MONTH],      DateCase.getMonth() );
    testcases[item++] = new TestCase( SECTION, DateString+".getDate()",             ResultArray[DATE],       DateCase.getDate() );
    testcases[item++] = new TestCase( SECTION, DateString+".getDay()",              ResultArray[DAY],        DateCase.getDay() );
    testcases[item++] = new TestCase( SECTION, DateString+".getHours()",            ResultArray[HOURS],      DateCase.getHours() );
    testcases[item++] = new TestCase( SECTION, DateString+".getMinutes()",          ResultArray[MINUTES],    DateCase.getMinutes() );
    testcases[item++] = new TestCase( SECTION, DateString+".getSeconds()",          ResultArray[SECONDS],    DateCase.getSeconds() );
//    testcases[item++] = new TestCase( SECTION, DateString+".getMilliseconds()",     ResultArray[MS],         DateCase.getMilliseconds() );
*/
}
function test() {
    for( tc = 0; tc < testcases.length; tc++ ) {

        testcases[tc].passed = writeTestCaseResult(
                                testcases[tc].expect,
                                testcases[tc].actual,
                                testcases[tc].description +" = " +
                                testcases[tc].actual );
    }
        stopTest();

    //  all tests must return an array of TestCase objects
        return ( testcases );
}
