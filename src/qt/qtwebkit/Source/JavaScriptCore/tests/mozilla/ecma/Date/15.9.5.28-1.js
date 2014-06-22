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
    File Name:          15.9.5.28-1.js
    ECMA Section:       15.9.5.28 Date.prototype.setMinutes(min [, sec [, ms ]] )
    Description:
    If sec is not specified, this behaves as if sec were specified with the
    value getSeconds ( ).

    If ms is not specified, this behaves as if ms were specified with the
    value getMilliseconds( ).

    1.  Let t be the result of LocalTime(this time value).
    2.  Call ToNumber(min).
    3.  If sec is not specified, compute SecFromTime(t); otherwise, call ToNumber(sec).
    4.  If ms is not specified, compute msFromTime(t); otherwise, call ToNumber(ms).
    5.  Compute MakeTime(HourFromTime(t), Result(2), Result(3), Result(4)).
    6.  Compute UTC(MakeDate(Day(t), Result(5))).
    7.  Set the [[Value]] property of the this value to TimeClip(Result(6)).
    8.  Return the value of the [[Value]] property of the this value.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "15.9.5.28-1";
    var VERSION = "ECMA_1";
    startTest();

    writeHeaderToLog( SECTION + " Date.prototype.setMinutes(sec [,ms] )");

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
    addNewTestCase( 0, 0, void 0, void 0,
                    "TDATE = new Date(0);(TDATE).setMinutes(0);TDATE",
                    UTCDateFromTime(SetMinutes(0,0,0,0)),
                    LocalDateFromTime(SetMinutes(0,0,0,0)) );

    addNewTestCase( 28800000, 59, 59, void 0,
                    "TDATE = new Date(28800000);(TDATE).setMinutes(59,59);TDATE",
                    UTCDateFromTime(SetMinutes(28800000,59,59)),
                    LocalDateFromTime(SetMinutes(28800000,59,59)) );

    addNewTestCase( 28800000, 59, 59, 999,
                    "TDATE = new Date(28800000);(TDATE).setMinutes(59,59,999);TDATE",
                    UTCDateFromTime(SetMinutes(28800000,59,59,999)),
                    LocalDateFromTime(SetMinutes(28800000,59,59,999)) );

    addNewTestCase( 28800000, 59, void 0, void 0,
                    "TDATE = new Date(28800000);(TDATE).setMinutes(59);TDATE",
                    UTCDateFromTime(SetMinutes(28800000,59,0)),
                    LocalDateFromTime(SetMinutes(28800000,59,0)) );

    addNewTestCase( 28800000, -480, void 0, void 0,
                    "TDATE = new Date(28800000);(TDATE).setMinutes(-480);TDATE",
                    UTCDateFromTime(SetMinutes(28800000,-480)),
                    LocalDateFromTime(SetMinutes(28800000,-480)) );

    addNewTestCase( 946684800000, 1234567, void 0, void 0,
                    "TDATE = new Date(946684800000);(TDATE).setMinutes(1234567);TDATE",
                    UTCDateFromTime(SetMinutes(946684800000,1234567)),
                    LocalDateFromTime(SetMinutes(946684800000,1234567)) );

    addNewTestCase( -2208988800000,59, 59, void 0,
                    "TDATE = new Date(-2208988800000);(TDATE).setMinutes(59,59);TDATE",
                    UTCDateFromTime(SetMinutes(-2208988800000,59,59)),
                    LocalDateFromTime(SetMinutes(-2208988800000,59,59)) );

    addNewTestCase( -2208988800000, 59, 59, 999,
                    "TDATE = new Date(-2208988800000);(TDATE).setMinutes(59,59,999);TDATE",
                    UTCDateFromTime(SetMinutes(-2208988800000,59,59,999)),
                    LocalDateFromTime(SetMinutes(-2208988800000,59,59,999)) );
/*
    addNewTestCase( "TDATE = new Date(-2208988800000);(TDATE).setUTCMilliseconds(123456789);TDATE",
                    UTCDateFromTime(SetUTCMilliseconds(-2208988800000,123456789)),
                    LocalDateFromTime(SetUTCMilliseconds(-2208988800000,123456789)) );

    addNewTestCase( "TDATE = new Date(-2208988800000);(TDATE).setUTCMilliseconds(123456);TDATE",
                    UTCDateFromTime(SetUTCMilliseconds(-2208988800000,123456)),
                    LocalDateFromTime(SetUTCMilliseconds(-2208988800000,123456)) );

    addNewTestCase( "TDATE = new Date(-2208988800000);(TDATE).setUTCMilliseconds(-123456);TDATE",
                    UTCDateFromTime(SetUTCMilliseconds(-2208988800000,-123456)),
                    LocalDateFromTime(SetUTCMilliseconds(-2208988800000,-123456)) );

    addNewTestCase( "TDATE = new Date(0);(TDATE).setUTCMilliseconds(-999);TDATE",
                    UTCDateFromTime(SetUTCMilliseconds(0,-999)),
                    LocalDateFromTime(SetUTCMilliseconds(0,-999)) );
*/

}
function addNewTestCase( time, min, sec, ms, DateString, UTCDate, LocalDate) {
    DateCase = new Date( time );

    if ( sec == void 0 ) {
        DateCase.setMinutes( min );
    } else {
        if ( ms == void 0 ) {
            DateCase.setMinutes( min, sec );
        } else {
            DateCase.setMinutes( min, sec, ms );
        }
    }


    var item = testcases.length;

//    fixed_year = ( ExpectDate.year >=1900 || ExpectDate.year < 2000 ) ? ExpectDate.year - 1900 : ExpectDate.year;

    testcases[item++] = new TestCase( SECTION, DateString+".getTime()",             UTCDate.value,       DateCase.getTime() );
    testcases[item++] = new TestCase( SECTION, DateString+".valueOf()",             UTCDate.value,       DateCase.valueOf() );

    testcases[item++] = new TestCase( SECTION, DateString+".getUTCFullYear()",      UTCDate.year,    DateCase.getUTCFullYear() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCMonth()",         UTCDate.month,  DateCase.getUTCMonth() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCDate()",          UTCDate.date,   DateCase.getUTCDate() );
//    testcases[item++] = new TestCase( SECTION, DateString+".getUTCDay()",           UTCDate.day,    DateCase.getUTCDay() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCHours()",         UTCDate.hours,  DateCase.getUTCHours() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCMinutes()",       UTCDate.minutes,DateCase.getUTCMinutes() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCSeconds()",       UTCDate.seconds,DateCase.getUTCSeconds() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCMilliseconds()",  UTCDate.ms,     DateCase.getUTCMilliseconds() );

    testcases[item++] = new TestCase( SECTION, DateString+".getFullYear()",         LocalDate.year,       DateCase.getFullYear() );
    testcases[item++] = new TestCase( SECTION, DateString+".getMonth()",            LocalDate.month,      DateCase.getMonth() );
    testcases[item++] = new TestCase( SECTION, DateString+".getDate()",             LocalDate.date,       DateCase.getDate() );
//    testcases[item++] = new TestCase( SECTION, DateString+".getDay()",              LocalDate.day,        DateCase.getDay() );
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

function SetMinutes( t, min, sec, ms ) {
    var TIME = LocalTime(t);
    var MIN =  Number(min);
    var SEC  = ( sec == void 0) ? SecFromTime(TIME) : Number(sec);
    var MS   = ( ms == void 0 ) ? msFromTime(TIME)  : Number(ms);
    var RESULT5 = MakeTime( HourFromTime( TIME ),
                            MIN,
                            SEC,
                            MS );
    return ( TimeClip(UTC( MakeDate(Day(TIME),RESULT5))) );
}
