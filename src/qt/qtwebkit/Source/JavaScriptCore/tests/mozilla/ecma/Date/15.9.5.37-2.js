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
    File Name:          15.9.5.37-1.js
    ECMA Section:       15.9.5.37 Date.prototype.setUTCFullYear(year [, mon [, date ]] )
    Description:

    If mon is not specified, this behaves as if mon were specified with the
    value getUTCMonth( ).  If date is not specified, this behaves as if date
    were specified with the value getUTCDate( ).

   1.   Let t be this time value; but if this time value is NaN, let t be +0.
   2.   Call ToNumber(year).
   3.   If mon is not specified, compute MonthFromTime(t); otherwise, call
        ToNumber(mon).
   4.   If date is not specified, compute DateFromTime(t); otherwise, call
        ToNumber(date).
   5.   Compute MakeDay(Result(2), Result(3), Result(4)).
   6.   Compute MakeDate(Result(5), TimeWithinDay(t)).
   7.   Set the [[Value]] property of the this value to TimeClip(Result(6)).
   8.   Return the value of the [[Value]] property of the this value.

    Author:             christine@netscape.com
    Date:               12 november 1997

    Added some Year 2000 test cases.
*/
    var SECTION = "15.9.5.37-1";
    var VERSION = "ECMA_1";
    startTest();

    writeHeaderToLog( SECTION + " Date.prototype.setUTCFullYear(year [, mon [, date ]] )");

    getTestCases();
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

function getTestCases() {

    // Dates around 2000

    addNewTestCase( "TDATE = new Date(0); TDATE.setUTCFullYear(2000);TDATE",
                    UTCDateFromTime(SetUTCFullYear(0,2000)),
                    LocalDateFromTime(SetUTCFullYear(0,2000)) );

    addNewTestCase( "TDATE = new Date(0); TDATE.setUTCFullYear(2001);TDATE",
                    UTCDateFromTime(SetUTCFullYear(0,2001)),
                    LocalDateFromTime(SetUTCFullYear(0,2001)) );

    addNewTestCase( "TDATE = new Date(0); TDATE.setUTCFullYear(1999);TDATE",
                    UTCDateFromTime(SetUTCFullYear(0,1999)),
                    LocalDateFromTime(SetUTCFullYear(0,1999)) );
/*
    // Dates around 29 February 2000

    var UTC_FEB_29_1972 = TIME_1970 + TimeInYear(1970) + TimeInYear(1971) +
    31*msPerDay + 28*msPerDay;

    var PST_FEB_29_1972 = UTC_FEB_29_1972 - TZ_DIFF * msPerHour;

    addNewTestCase( "TDATE = new Date("+UTC_FEB_29_1972+"); "+
                    "TDATE.setUTCFullYear(2000);TDATE",
                    UTCDateFromTime(SetUTCFullYear(UTC_FEB_29_1972,2000)),
                    LocalDateFromTime(SetUTCFullYear(UTC_FEB_29_1972,2000)) );

    addNewTestCase( "TDATE = new Date("+PST_FEB_29_1972+"); "+
                    "TDATE.setUTCFullYear(2000);TDATE",
                    UTCDateFromTime(SetUTCFullYear(PST_FEB_29_1972,2000)),
                    LocalDateFromTime(SetUTCFullYear(PST_FEB_29_1972,2000)) );

    // Dates around 2005

    addNewTestCase( "TDATE = new Date(0); TDATE.setUTCFullYear(2005);TDATE",
                    UTCDateFromTime(SetUTCFullYear(0,2005)),
                    LocalDateFromTime(SetUTCFullYear(0,2005)) );

    addNewTestCase( "TDATE = new Date(0); TDATE.setUTCFullYear(2004);TDATE",
                    UTCDateFromTime(SetUTCFullYear(0,2004)),
                    LocalDateFromTime(SetUTCFullYear(0,2004)) );

    addNewTestCase( "TDATE = new Date(0); TDATE.setUTCFullYear(2006);TDATE",
                    UTCDateFromTime(SetUTCFullYear(0,2006)),
                    LocalDateFromTime(SetUTCFullYear(0,2006)) );


    // Dates around 1900
    addNewTestCase( "TDATE = new Date(0); TDATE.setUTCFullYear(1900);TDATE",
                    UTCDateFromTime(SetUTCFullYear(0,1900)),
                    LocalDateFromTime(SetUTCFullYear(0,1900)) );

    addNewTestCase( "TDATE = new Date(0); TDATE.setUTCFullYear(1899);TDATE",
                    UTCDateFromTime(SetUTCFullYear(0,1899)),
                    LocalDateFromTime(SetUTCFullYear(0,1899)) );

    addNewTestCase( "TDATE = new Date(0); TDATE.setUTCFullYear(1901);TDATE",
                    UTCDateFromTime(SetUTCFullYear(0,1901)),
                    LocalDateFromTime(SetUTCFullYear(0,1901)) );

*/
}
function addNewTestCase( DateString, UTCDate, LocalDate) {
    DateCase = eval( DateString );

    var item = testcases.length;

//    fixed_year = ( ExpectDate.year >=1900 || ExpectDate.year < 2000 ) ? ExpectDate.year - 1900 : ExpectDate.year;

    testcases[item++] = new TestCase( SECTION, DateString+".getTime()",             UTCDate.value,       DateCase.getTime() );
    testcases[item++] = new TestCase( SECTION, DateString+".valueOf()",             UTCDate.value,       DateCase.valueOf() );

    testcases[item++] = new TestCase( SECTION, DateString+".getUTCFullYear()",      UTCDate.year,    DateCase.getUTCFullYear() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCMonth()",         UTCDate.month,  DateCase.getUTCMonth() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCDate()",          UTCDate.date,   DateCase.getUTCDate() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCDay()",           UTCDate.day,    DateCase.getUTCDay() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCHours()",         UTCDate.hours,  DateCase.getUTCHours() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCMinutes()",       UTCDate.minutes,DateCase.getUTCMinutes() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCSeconds()",       UTCDate.seconds,DateCase.getUTCSeconds() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCMilliseconds()",  UTCDate.ms,     DateCase.getUTCMilliseconds() );

    testcases[item++] = new TestCase( SECTION, DateString+".getFullYear()",         LocalDate.year,       DateCase.getFullYear() );
    testcases[item++] = new TestCase( SECTION, DateString+".getMonth()",            LocalDate.month,      DateCase.getMonth() );
    testcases[item++] = new TestCase( SECTION, DateString+".getDate()",             LocalDate.date,       DateCase.getDate() );
    testcases[item++] = new TestCase( SECTION, DateString+".getDay()",              LocalDate.day,        DateCase.getDay() );
    testcases[item++] = new TestCase( SECTION, DateString+".getHours()",            LocalDate.hours,      DateCase.getHours() );
    testcases[item++] = new TestCase( SECTION, DateString+".getMinutes()",          LocalDate.minutes,    DateCase.getMinutes() );
    testcases[item++] = new TestCase( SECTION, DateString+".getSeconds()",          LocalDate.seconds,    DateCase.getSeconds() );
    testcases[item++] = new TestCase( SECTION, DateString+".getMilliseconds()",     LocalDate.ms,         DateCase.getMilliseconds() );

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
    var d = new MyDate();
    d.year = YearFromTime(t);
    d.month = MonthFromTime(t);
    d.date = DateFromTime(t);
    d.hours = HourFromTime(t);
    d.minutes = MinFromTime(t);
    d.seconds = SecFromTime(t);
    d.ms = msFromTime(t);

    d.time = MakeTime( d.hours, d.minutes, d.seconds, d.ms );
    d.value = TimeClip( MakeDate( MakeDay( d.year, d.month, d.date ), d.time ) );
    d.day = WeekDay( d.value );

    return (d);
}
function SetUTCFullYear( t, year, mon, date ) {
    var T = ( t != t ) ? 0 : t;
    var YEAR = Number(year);
    var MONTH = ( mon == void 0 ) ?     MonthFromTime(T) : Number( mon );
    var DATE  = ( date == void 0 ) ?    DateFromTime(T)  : Number( date );
    var DAY = MakeDay( YEAR, MONTH, DATE );

    return ( TimeClip(MakeDate(DAY, TimeWithinDay(T))) );
}
