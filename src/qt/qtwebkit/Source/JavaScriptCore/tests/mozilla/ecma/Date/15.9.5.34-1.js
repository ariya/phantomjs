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
    File Name:          15.9.5.34-1.js
    ECMA Section:       15.9.5.34 Date.prototype.setMonth(mon [, date ] )
    Description:
    If date is not specified, this behaves as if date were specified with the
    value getDate( ).

    1.  Let t be the result of LocalTime(this time value).
    2.  Call ToNumber(date).
    3.  If date is not specified, compute DateFromTime(t); otherwise, call ToNumber(date).
    4.  Compute MakeDay(YearFromTime(t), Result(2), Result(3)).
    5.  Compute UTC(MakeDate(Result(4), TimeWithinDay(t))).
    6.  Set the [[Value]] property of the this value to TimeClip(Result(5)).
    7.  Return the value of the [[Value]] property of the this value.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "15.9.5.34-1";
    var VERSION = "ECMA_1";
    startTest();

    writeHeaderToLog( SECTION + " Date.prototype.setMonth(mon [, date ] )");

    var now =  (new Date()).valueOf();

    getFunctionCases();
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

function getFunctionCases() {
    // some tests for all functions
    testcases[testcases.length] = new TestCase(
                                    SECTION,
                                    "Date.prototype.setMonth.length",
                                    2,
                                    Date.prototype.setMonth.length );

    testcases[testcases.length] = new TestCase(
                                    SECTION,
                                    "typeof Date.prototype.setMonth",
                                    "function",
                                    typeof Date.prototype.setMonth );


/*

    testcases[testcases.length] = new TestCase(
                                    SECTION,
                                    "delete Date.prototype.setMonth",
                                    false,
                                    delete Date.prototype.setMonth );
*/

}


function getTestCases() {
    // regression test for http://scopus.mcom.com/bugsplat/show_bug.cgi?id=112404
    d = new Date(0);
    d.setMonth(1,1,1,1,1,1);

        addNewTestCase(
        "TDATE = new Date(0); TDATE.setMonth(1,1,1,1,1,1); TDATE",
        UTCDateFromTime(SetMonth(0,1,1)),
        LocalDateFromTime(SetMonth(0,1,1)) );


    // whatever today is

    addNewTestCase( "TDATE = new Date(now); (TDATE).setMonth(11,31); TDATE",
                    UTCDateFromTime(SetMonth(now,11,31)),
                    LocalDateFromTime(SetMonth(now,11,31)) );

    // 1970

    addNewTestCase( "TDATE = new Date(0);(TDATE).setMonth(0,1);TDATE",
                    UTCDateFromTime(SetMonth(0,0,1)),
                    LocalDateFromTime(SetMonth(0,0,1)) );

    addNewTestCase( "TDATE = new Date("+TIME_1900+"); "+
                    "(TDATE).setMonth(11,31); TDATE",
                    UTCDateFromTime( SetMonth(TIME_1900,11,31) ),
                    LocalDateFromTime( SetMonth(TIME_1900,11,31) ) );




/*
    addNewTestCase( "TDATE = new Date(28800000);(TDATE).setMonth(11,23,59,999);TDATE",
                    UTCDateFromTime(SetMonth(28800000,11,23,59,999)),
                    LocalDateFromTime(SetMonth(28800000,11,23,59,999)) );

    addNewTestCase( "TDATE = new Date(28800000);(TDATE).setMonth(99,99);TDATE",
                    UTCDateFromTime(SetMonth(28800000,99,99)),
                    LocalDateFromTime(SetMonth(28800000,99,99)) );

    addNewTestCase( "TDATE = new Date(28800000);(TDATE).setMonth(11);TDATE",
                    UTCDateFromTime(SetMonth(28800000,11,0)),
                    LocalDateFromTime(SetMonth(28800000,11,0)) );

    addNewTestCase( "TDATE = new Date(28800000);(TDATE).setMonth(-11);TDATE",
                    UTCDateFromTime(SetMonth(28800000,-11)),
                    LocalDateFromTime(SetMonth(28800000,-11)) );

    // 1900

//    addNewTestCase( "TDATE = new Date(); (TDATE).setMonth(11,31); TDATE;"
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
function SetMonth( t, mon, date ) {
    var TIME = LocalTime(t);
    var MONTH = Number( mon );
    var DATE = ( date == void 0 ) ? DateFromTime(TIME) : Number( date );
    var DAY = MakeDay( YearFromTime(TIME), MONTH, DATE );
    return ( TimeClip (UTC(MakeDate( DAY, TimeWithinDay(TIME) ))) );
}
