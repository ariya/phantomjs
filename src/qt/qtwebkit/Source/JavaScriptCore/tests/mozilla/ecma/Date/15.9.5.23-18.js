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
    File Name:          15.9.5.23-1.js
    ECMA Section:       15.9.5.23 Date.prototype.setTime(time)
    Description:

    1.  If the this value is not a Date object, generate a runtime error.
    2.  Call ToNumber(time).
    3.  Call TimeClip(Result(1)).
    4.  Set the [[Value]] property of the this value to Result(2).
    5.  Return the value of the [[Value]] property of the this value.

    Author:             christine@netscape.com
    Date:               12 november 1997

*/
    var SECTION = "15.9.5.23-1";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE = "Date.prototype.setTime()";

    writeHeaderToLog( SECTION + " Date.prototype.setTime(time)");

    var testcases = new Array();
    getTestCases();
    test();

function getTestCases() {
    var now = "now";
    addTestCase( now, String( TIME_2000 ) );
}

function addTestCase( startTime, setTime ) {
    if ( startTime == "now" ) {
        DateCase = new Date();
    } else {
        DateCase = new Date( startTime );
    }

    DateCase.setTime( setTime );
    var DateString = "var d = new Date("+startTime+"); d.setTime("+setTime+"); d" ;
    var UTCDate   = UTCDateFromTime ( Number(setTime) );
    var LocalDate = LocalDateFromTime( Number(setTime) );
    var item = testcases.length;

//    fixed_year = ( ExpectDate.year >=1900 || ExpectDate.year < 2000 ) ? ExpectDate.year - 1900 : ExpectDate.year;

    testcases[item++] = new TestCase( SECTION, DateString+".getTime()",             UTCDate.value,      DateCase.getTime() );
    testcases[item++] = new TestCase( SECTION, DateString+".valueOf()",             UTCDate.value,      DateCase.valueOf() );

    testcases[item++] = new TestCase( SECTION, DateString+".getUTCFullYear()",      UTCDate.year,       DateCase.getUTCFullYear() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCMonth()",         UTCDate.month,      DateCase.getUTCMonth() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCDate()",          UTCDate.date,       DateCase.getUTCDate() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCDay()",           UTCDate.day,        DateCase.getUTCDay() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCHours()",         UTCDate.hours,      DateCase.getUTCHours() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCMinutes()",       UTCDate.minutes,    DateCase.getUTCMinutes() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCSeconds()",       UTCDate.seconds,    DateCase.getUTCSeconds() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCMilliseconds()",  UTCDate.ms,         DateCase.getUTCMilliseconds() );

    testcases[item++] = new TestCase( SECTION, DateString+".getFullYear()",         LocalDate.year,     DateCase.getFullYear() );
    testcases[item++] = new TestCase( SECTION, DateString+".getMonth()",            LocalDate.month,    DateCase.getMonth() );
    testcases[item++] = new TestCase( SECTION, DateString+".getDate()",             LocalDate.date,     DateCase.getDate() );
    testcases[item++] = new TestCase( SECTION, DateString+".getDay()",              LocalDate.day,      DateCase.getDay() );
    testcases[item++] = new TestCase( SECTION, DateString+".getHours()",            LocalDate.hours,    DateCase.getHours() );
    testcases[item++] = new TestCase( SECTION, DateString+".getMinutes()",          LocalDate.minutes,  DateCase.getMinutes() );
    testcases[item++] = new TestCase( SECTION, DateString+".getSeconds()",          LocalDate.seconds,  DateCase.getSeconds() );
    testcases[item++] = new TestCase( SECTION, DateString+".getMilliseconds()",     LocalDate.ms,       DateCase.getMilliseconds() );

    DateCase.toString = Object.prototype.toString;

    testcases[item++] = new TestCase( SECTION,
                                      DateString+".toString=Object.prototype.toString;"+DateString+".toString()",
                                      "[object Date]",
                                      DateCase.toString() );
}
function MyDate() {
    this.year = 0;
    this.month = 0;
    this.date = 0;
    this.hours = 0;
    this.minutes = 0;
    this.seconds = 0;
    this.ms = 0;
}
function LocalDateFromTime(t) {
    t = LocalTime(t);
    return ( MyDateFromTime(t) );
}
function UTCDateFromTime(t) {
 return ( MyDateFromTime(t) );
}
function MyDateFromTime( t ) {
    var d       = new MyDate();
    d.year      = YearFromTime(t);
    d.month     = MonthFromTime(t);
    d.date      = DateFromTime(t);
    d.hours     = HourFromTime(t);
    d.minutes   = MinFromTime(t);
    d.seconds   = SecFromTime(t);
    d.ms        = msFromTime(t);
    d.time      = MakeTime( d.hours, d.minutes, d.seconds, d.ms );
    d.value     = TimeClip( MakeDate( MakeDay( d.year, d.month, d.date ), d.time ) );
    d.day       = WeekDay( d.value );
    return (d);
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
