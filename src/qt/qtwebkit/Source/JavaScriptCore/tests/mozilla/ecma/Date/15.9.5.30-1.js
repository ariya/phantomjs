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
    File Name:          15.9.5.30-1.js
    ECMA Section:       15.9.5.30 Date.prototype.setHours(hour [, min [, sec [, ms ]]] )
    Description:
    If min is not specified, this behaves as if min were specified with the
    value getMinutes( ). If sec is not specified, this behaves as if sec were
    specified with the value getSeconds ( ). If ms is not specified, this
    behaves as if ms were specified with the value getMilliseconds( ).

    1.  Let t be the result of LocalTime(this time value).
    2.  Call ToNumber(hour).
    3.  If min is not specified, compute MinFromTime(t); otherwise, call
        ToNumber(min).
    4.  If sec is not specified, compute SecFromTime(t); otherwise, call
        ToNumber(sec).
    5.  If ms is not specified, compute msFromTime(t); otherwise, call
        ToNumber(ms).
    6.  Compute MakeTime(Result(2), Result(3), Result(4), Result(5)).
    7.  Compute UTC(MakeDate(Day(t), Result(6))).
    8.  Set the [[Value]] property of the this value to TimeClip(Result(7)).
    9.  Return the value of the [[Value]] property of the this value.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "15.9.5.30-1";
    var VERSION = "ECMA_1";
    startTest();

    writeHeaderToLog( SECTION + " Date.prototype.setHours( hour [, min, sec, ms] )");

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
    addNewTestCase( 0,0,0,0,void 0,
                    "TDATE = new Date(0);(TDATE).setHours(0);TDATE" );

    addNewTestCase( 28800000, 23, 59, 999,void 0,
                    "TDATE = new Date(28800000);(TDATE).setHours(23,59,999);TDATE" );

    addNewTestCase( 28800000, 999, 999, void 0, void 0,
                    "TDATE = new Date(28800000);(TDATE).setHours(999,999);TDATE" );

    addNewTestCase( 28800000,999,0, void 0, void 0,
                    "TDATE = new Date(28800000);(TDATE).setHours(999);TDATE" );

    addNewTestCase( 28800000,-8, void 0, void 0, void 0,
                    "TDATE = new Date(28800000);(TDATE).setHours(-8);TDATE" );

    addNewTestCase( 946684800000,8760, void 0, void 0, void 0,
                "TDATE = new Date(946684800000);(TDATE).setHours(8760);TDATE" );

    addNewTestCase( TIME_2000 - msPerDay, 23, 59, 59, 999,
                    "d = new Date( " + (TIME_2000-msPerDay) +"); d.setHours(23,59,59,999)" );

    addNewTestCase( TIME_2000 - msPerDay, 23, 59, 59, 1000,
                    "d = new Date( " + (TIME_2000-msPerDay) +"); d.setHours(23,59,59,1000)" );


/*
    addNewTestCase( "TDATE = new Date(-2208988800000);(TDATE).setHours(59,999);TDATE",
                    UTCDateFromTime(SetHours(-2208988800000,59,999)),
                    LocalDateFromTime(SetHours(-2208988800000,59,999)) );

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
function addNewTestCase( time, hours, min, sec, ms, DateString) {
    var UTCDate =   UTCDateFromTime( SetHours( time, hours, min, sec, ms ));
    var LocalDate = LocalDateFromTime( SetHours( time, hours, min, sec, ms ));

    var DateCase = new Date( time );

    if ( min == void 0 ) {
        DateCase.setHours( hours );
    } else {
        if ( sec == void 0 ) {
            DateCase.setHours( hours, min );
        } else {
            if ( ms == void 0 ) {
                DateCase.setHours( hours, min, sec );
            } else {
                DateCase.setHours( hours, min, sec, ms );
            }
        }
    }


    var item = testcases.length;

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

    d.day = WeekDay( t );
    d.time = MakeTime( d.hours, d.minutes, d.seconds, d.ms );
    d.value = TimeClip( MakeDate( MakeDay( d.year, d.month, d.date ), d.time ) );

    return (d);
}
function SetHours( t, hour, min, sec, ms ) {
    var TIME = LocalTime(t);
    var HOUR = Number(hour);
    var MIN =  ( min == void 0) ? MinFromTime(TIME) : Number(min);
    var SEC  = ( sec == void 0) ? SecFromTime(TIME) : Number(sec);
    var MS   = ( ms == void 0 ) ? msFromTime(TIME)  : Number(ms);
    var RESULT6 = MakeTime( HOUR,
                            MIN,
                            SEC,
                            MS );
    var UTC_TIME = UTC(  MakeDate(Day(TIME), RESULT6) );
    return ( TimeClip(UTC_TIME) );
}
