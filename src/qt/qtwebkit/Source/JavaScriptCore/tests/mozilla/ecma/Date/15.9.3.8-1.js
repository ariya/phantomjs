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
    File Name:          15.9.3.8.js
    ECMA Section:       15.9.3.8 The Date Constructor
                        new Date( value )
    Description:        The [[Prototype]] property of the newly constructed
                        object is set to the original Date prototype object,
                        the one that is the initial valiue of Date.prototype.

                        The [[Class]] property of the newly constructed object is
                        set to "Date".

                        The [[Value]] property of the newly constructed object is
                        set as follows:

                        1. Call ToPrimitive(value)
                        2. If Type( Result(1) ) is String, then go to step 5.
                        3. Let V be  ToNumber( Result(1) ).
                        4. Set the [[Value]] property of the newly constructed
                            object to TimeClip(V) and return.
                        5. Parse Result(1) as a date, in exactly the same manner
                            as for the parse method.  Let V be the time value for
                            this date.
                        6. Go to step 4.

    Author:             christine@netscape.com
    Date:               28 october 1997
    Version:            9706

*/

    var VERSION = "ECMA_1";
    startTest();
    var SECTION = "15.9.3.8";
    var TYPEOF  = "object";

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


//  for TCMS, the testcases array must be global.
    var tc= 0;
    var TITLE = "Date constructor:  new Date( value )";
    var SECTION = "15.9.3.8";
    var VERSION = "ECMA_1";
    startTest();

    writeHeaderToLog( SECTION +" " + TITLE );

    testcases = new Array();
    getTestCases();

//  all tests must call a function that returns a boolean value
    test();

function getTestCases( ) {
    // all the "ResultArrays" below are hard-coded to Pacific Standard Time values -
    var TZ_ADJUST = -TZ_PST * msPerHour;


    // Dates around 1970
    addNewTestCase( new Date(0),
                    "new Date(0)",
                    [0,1970,0,1,4,0,0,0,0,1969,11,31,3,16,0,0,0] );

    addNewTestCase( new Date(1),
                    "new Date(1)",
                    [1,1970,0,1,4,0,0,0,1,1969,11,31,3,16,0,0,1] );

    addNewTestCase( new Date(true),
                    "new Date(true)",
                    [1,1970,0,1,4,0,0,0,1,1969,11,31,3,16,0,0,1] );

    addNewTestCase( new Date(false),
                    "new Date(false)",
                    [0,1970,0,1,4,0,0,0,0,1969,11,31,3,16,0,0,0] );

    addNewTestCase( new Date( (new Date(0)).toString() ),
                    "new Date(\""+ (new Date(0)).toString()+"\" )",
                    [0,1970,0,1,4,0,0,0,0,1969,11,31,3,16,0,0,0] );
/*
//    addNewTestCase( "new Date(\""+ (new Date(0)).toLocaleString()+"\")", [0,1970,0,1,4,0,0,0,0,1969,11,31,3,16,0,0,0] );

    addNewTestCase( new Date((new Date(0)).toUTCString()),
                    "new Date(\""+ (new Date(0)).toUTCString()+"\" )",
                    [0,1970,0,1,4,0,0,0,0,1969,11,31,3,16,0,0,0] );

    addNewTestCase( new Date((new Date(1)).toString()),
                    "new Date(\""+ (new Date(1)).toString()+"\" )",
                    [0,1970,0,1,4,0,0,0,0,1969,11,31,3,16,0,0,0] );

    addNewTestCase( new Date( TZ_ADJUST ),
                    "new Date(" + TZ_ADJUST+")",
                    [TZ_ADJUST,1970,0,1,4,8,0,0,0,1970,0,1,4,0,0,0,0] );

    addNewTestCase( new Date((new Date(TZ_ADJUST)).toString()),
                    "new Date(\""+ (new Date(TZ_ADJUST)).toString()+"\")",
                    [TZ_ADJUST,1970,0,1,4,8,0,0,0,1970,0,1,4,0,0,0,0] );

//    addNewTestCase( "new Date(\""+ (new Date(TZ_ADJUST)).toLocaleString()+"\")",[TZ_ADJUST,1970,0,1,4,8,0,0,0,1970,0,1,4,0,0,0,0] );

    addNewTestCase( new Date( (new Date(TZ_ADJUST)).toUTCString() ),
                    "new Date(\""+ (new Date(TZ_ADJUST)).toUTCString()+"\")",
                    [TZ_ADJUST,1970,0,1,4,8,0,0,0,1970,0,1,4,0,0,0,0] );

    // Dates around 2000

    addNewTestCase( new Date(TIME_2000+TZ_ADJUST),
                    "new Date(" +(TIME_2000+TZ_ADJUST)+")",
                    [TIME_2000+TZ_ADJUST,2000,0,1,6,8,0,0,0,2000,0,1,6,0,0,0,0] );

    addNewTestCase( new Date(TIME_2000),
                    "new Date(" +TIME_2000+")",
                    [TIME_2000,2000,0,1,6,0,0,0,0,1999,11,31,5,16,0,0,0] );

    addNewTestCase( new Date( (new Date(TIME_2000+TZ_ADJUST)).toString()),
                    "new Date(\"" +(new Date(TIME_2000+TZ_ADJUST)).toString()+"\")",
                    [TIME_2000+TZ_ADJUST,2000,0,1,6,8,0,0,0,2000,0,1,6,0,0,0,0] );

    addNewTestCase( new Date((new Date(TIME_2000)).toString()),
                   "new Date(\"" +(new Date(TIME_2000)).toString()+"\")",
                   [TIME_2000,2000,0,1,6,0,0,0,0,1999,11,31,5,16,0,0,0] );

//    addNewTestCase( "new Date(\"" +(new Date(TIME_2000+TZ_ADJUST)).toLocaleString()+"\")", [TIME_2000+TZ_ADJUST,2000,0,1,6,8,0,0,0,2000,0,1,6,0,0,0,0] );
//    addNewTestCase( "new Date(\"" +(new Date(TIME_2000)).toLocaleString()+"\")",             [TIME_2000,2000,0,1,6,0,0,0,0,1999,11,31,5,16,0,0,0] );

    addNewTestCase(  new Date( (new Date(TIME_2000+TZ_ADJUST)).toUTCString()),
                    "new Date(\"" +(new Date(TIME_2000+TZ_ADJUST)).toUTCString()+"\")",
                    [TIME_2000+TZ_ADJUST,2000,0,1,6,8,0,0,0,2000,0,1,6,0,0,0,0] );

    addNewTestCase( new Date( (new Date(TIME_2000)).toUTCString()),
                    "new Date(\"" +(new Date(TIME_2000)).toUTCString()+"\")",
                    [TIME_2000,2000,0,1,6,0,0,0,0,1999,11,31,5,16,0,0,0] );

    // Dates around Feb 29, 2000

    var UTC_FEB_29_2000 = TIME_2000 + 31*msPerDay + 28*msPerDay;
    var PST_FEB_29_2000 = UTC_FEB_29_2000 + TZ_ADJUST;

    addNewTestCase( new Date(UTC_FEB_29_2000),
                    "new Date("+UTC_FEB_29_2000+")",
                    [UTC_FEB_29_2000,2000,1,29,2,0,0,0,0,2000,1,28,1,16,0,0,0] );

    addNewTestCase( new Date(PST_FEB_29_2000),
                    "new Date("+PST_FEB_29_2000+")",
                    [PST_FEB_29_2000,2000,1,29,2,8,0,0,0,2000,1,29,2,0,0,0,0] );

    addNewTestCase( new Date( (new Date(UTC_FEB_29_2000)).toString() ),
                    "new Date(\""+(new Date(UTC_FEB_29_2000)).toString()+"\")",
                    [UTC_FEB_29_2000,2000,1,29,2,0,0,0,0,2000,1,28,1,16,0,0,0] );

    addNewTestCase( new Date( (new Date(PST_FEB_29_2000)).toString() ),
                    "new Date(\""+(new Date(PST_FEB_29_2000)).toString()+"\")",
                    [PST_FEB_29_2000,2000,1,29,2,8,0,0,0,2000,1,29,2,0,0,0,0] );

//  Parsing toLocaleString() is not guaranteed by ECMA.
//    addNewTestCase( "new Date(\""+(new Date(UTC_FEB_29_2000)).toLocaleString()+"\")",  [UTC_FEB_29_2000,2000,1,29,2,0,0,0,0,2000,1,28,1,16,0,0,0] );
//    addNewTestCase( "new Date(\""+(new Date(PST_FEB_29_2000)).toLocaleString()+"\")",  [PST_FEB_29_2000,2000,1,29,2,8,0,0,0,2000,1,29,2,0,0,0,0] );

    addNewTestCase( new Date( (new Date(UTC_FEB_29_2000)).toGMTString() ),
                    "new Date(\""+(new Date(UTC_FEB_29_2000)).toGMTString()+"\")",
                    [UTC_FEB_29_2000,2000,1,29,2,0,0,0,0,2000,1,28,1,16,0,0,0] );

    addNewTestCase( new Date( (new Date(PST_FEB_29_2000)).toGMTString() ),
                    "new Date(\""+(new Date(PST_FEB_29_2000)).toGMTString()+"\")",
                     [PST_FEB_29_2000,2000,1,29,2,8,0,0,0,2000,1,29,2,0,0,0,0] );

    // Dates around 1900

    var PST_1900 = TIME_1900 + 8*msPerHour;

    addNewTestCase( new Date( TIME_1900 ),
                    "new Date("+TIME_1900+")",
                    [TIME_1900,1900,0,1,1,0,0,0,0,1899,11,31,0,16,0,0,0] );

    addNewTestCase( new Date(PST_1900),
                    "new Date("+PST_1900+")",
                    [ PST_1900,1900,0,1,1,8,0,0,0,1900,0,1,1,0,0,0,0] );

    addNewTestCase( new Date( (new Date(TIME_1900)).toString() ),
                    "new Date(\""+(new Date(TIME_1900)).toString()+"\")",
                    [TIME_1900,1900,0,1,1,0,0,0,0,1899,11,31,0,16,0,0,0] );

    addNewTestCase( new Date( (new Date(PST_1900)).toString() ),
                    "new Date(\""+(new Date(PST_1900 )).toString()+"\")",
                    [ PST_1900,1900,0,1,1,8,0,0,0,1900,0,1,1,0,0,0,0] );

    addNewTestCase( new Date( (new Date(TIME_1900)).toUTCString() ),
                    "new Date(\""+(new Date(TIME_1900)).toUTCString()+"\")",
                    [TIME_1900,1900,0,1,1,0,0,0,0,1899,11,31,0,16,0,0,0] );

    addNewTestCase( new Date( (new Date(PST_1900)).toUTCString() ),
                    "new Date(\""+(new Date(PST_1900 )).toUTCString()+"\")",
                    [ PST_1900,1900,0,1,1,8,0,0,0,1900,0,1,1,0,0,0,0] );

//    addNewTestCase( "new Date(\""+(new Date(TIME_1900)).toLocaleString()+"\")",   [TIME_1900,1900,0,1,1,0,0,0,0,1899,11,31,0,16,0,0,0] );
//    addNewTestCase( "new Date(\""+(new Date(PST_1900 )).toLocaleString()+"\")",   [ PST_1900,1900,0,1,1,8,0,0,0,1900,0,1,1,0,0,0,0] );
*/
/*
   This test case is incorrect.  Need to fix the DaylightSavings functions in
   shell.js for this to work properly.

    var DST_START_1998 = UTC( GetSecondSundayInMarch(TimeFromYear(1998)) + 2*msPerHour )

    addNewTestCase( new Date(DST_START_1998-1),
                    "new Date("+(DST_START_1998-1)+")",
                    [DST_START_1998-1,1998,3,5,0,9,59,59,999,1998,3,5,0,1,59,59,999] );

    addNewTestCase( new Date(DST_START_1998),
                    "new Date("+DST_START_1998+")",
                    [DST_START_1998,1998,3,5,0,10,0,0,0,1998,3,5,0,3,0,0,0]);

    var DST_END_1998 = UTC( GetFirstSundayInNovember(TimeFromYear(1998)) + 2*msPerHour );

    addNewTestCase ( new Date(DST_END_1998-1),
                    "new Date("+(DST_END_1998-1)+")",
                    [DST_END_1998-1,1998,9,25,0,8,59,59,999,1998,9,25,0,1,59,59,999] );

    addNewTestCase ( new Date(DST_END_1998),
                    "new Date("+DST_END_1998+")",
                    [DST_END_1998,1998,9,25,0,9,0,0,0,1998,9,25,0,1,0,0,0] );
*/
}

function addNewTestCase( DateCase, DateString, ResultArray ) {
    //adjust hard-coded ResultArray for tester's timezone instead of PST
    adjustResultArray(ResultArray, 'msMode'); 

    item = testcases.length;

    testcases[item++] = new TestCase( SECTION, DateString+".getTime()", ResultArray[TIME],       DateCase.getTime() );
    testcases[item++] = new TestCase( SECTION, DateString+".valueOf()", ResultArray[TIME],       DateCase.valueOf() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCFullYear()",      ResultArray[UTC_YEAR], DateCase.getUTCFullYear() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCMonth()",         ResultArray[UTC_MONTH],  DateCase.getUTCMonth() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCDate()",          ResultArray[UTC_DATE],   DateCase.getUTCDate() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCDay()",           ResultArray[UTC_DAY],    DateCase.getUTCDay() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCHours()",         ResultArray[UTC_HOURS],  DateCase.getUTCHours() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCMinutes()",       ResultArray[UTC_MINUTES],DateCase.getUTCMinutes() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCSeconds()",       ResultArray[UTC_SECONDS],DateCase.getUTCSeconds() );
    testcases[item++] = new TestCase( SECTION, DateString+".getUTCMilliseconds()",  ResultArray[UTC_MS],     DateCase.getUTCMilliseconds() );
    testcases[item++] = new TestCase( SECTION, DateString+".getFullYear()",         ResultArray[YEAR],       DateCase.getFullYear() );
    testcases[item++] = new TestCase( SECTION, DateString+".getMonth()",            ResultArray[MONTH],      DateCase.getMonth() );
    testcases[item++] = new TestCase( SECTION, DateString+".getDate()",             ResultArray[DATE],       DateCase.getDate() );
    testcases[item++] = new TestCase( SECTION, DateString+".getDay()",              ResultArray[DAY],        DateCase.getDay() );
    testcases[item++] = new TestCase( SECTION, DateString+".getHours()",            ResultArray[HOURS],      DateCase.getHours() );
    testcases[item++] = new TestCase( SECTION, DateString+".getMinutes()",          ResultArray[MINUTES],    DateCase.getMinutes() );
    testcases[item++] = new TestCase( SECTION, DateString+".getSeconds()",          ResultArray[SECONDS],    DateCase.getSeconds() );
    testcases[item++] = new TestCase( SECTION, DateString+".getMilliseconds()",     ResultArray[MS],         DateCase.getMilliseconds() );
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

    //  all tests must return a boolean value
        return ( testcases );
}
