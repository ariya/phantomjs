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
/*
 * JavaScript shared functions file for running the tests in either
 * stand-alone JavaScript engine.  To run a test, first load this file,
 * then load the test script.
 */

var completed = false;
var testcases;
var tc = 0;

SECTION = "";
VERSION = "";
BUGNUMBER = "";

/*
 * constant strings
 */
var GLOBAL = "[object global]";
var PASSED = " PASSED!"
var FAILED = " FAILED! expected: ";
var DEBUG = false;


/*
* Wrapper for test case constructor that doesn't require the SECTION argument.
 */
function AddTestCase( description, expect, actual ) 
{
  testcases[tc++] = new TestCase( SECTION, description, expect, actual );
}


/*
 * TestCase constructor
*/
function TestCase( n, d, e, a ) 
{
  this.name = n;
  this.description = d;
  this.expect = e;
  this.actual = a;
  this.passed = true;
  this.reason = "";
  this.bugnumber = BUGNUMBER;
  this.passed = getTestCaseResult( this.expect, this.actual );
  if ( DEBUG ) {writeLineToLog("added " + this.description);}
}


/*
 * Set up test environment.
*/
function startTest() 
{
  if ( version ) 
  {
    // JavaScript 1.3 is supposed to be compliant ECMA version 1.0
    if (VERSION == "ECMA_1" ) {version ("130");}
    if (VERSION == "JS_1.3" ) {version ( "130");}
    if (VERSION == "JS_1.2" ) {version ( "120");}
    if (VERSION == "JS_1.1" ) {version( "110");}

   // for ECMA version 2.0, we will leave the JavaScript version 
   // to the default ( for now ).
 }

  // print out bugnumber
  if ( BUGNUMBER ) 
  {
    writeLineToLog ("BUGNUMBER: " + BUGNUMBER );
  }

  testcases = new Array();
  tc = 0;
}


function test() 
{
  for ( tc=0; tc < testcases.length; tc++ ) 
  {
    testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+ testcases[tc].actual );

    testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }

  stopTest();
  return ( testcases );
}


/*
 * Compare expected result to the actual result and figure out whether
 * the test case passed.
 */
function getTestCaseResult(expect, actual ) 
{
  //because ( NaN == NaN ) always returns false, need to do
  //a special compare to see if we got the right result.
  if ( actual != actual ) 
  {
    if ( typeof actual == "object" ) {actual = "NaN object";} 
    else {actual = "NaN number";}
  }

  if ( expect != expect ) 
  {
    if ( typeof expect == "object" ) {expect = "NaN object";} 
    else {expect = "NaN number";}
  }

  var passed = ( expect == actual ) ?  true : false;

  // if both objects are numbers, need to replace w/ IEEE standard for rounding
  if ( !passed  &&  typeof(actual) == "number"  &&  typeof(expect) == "number" ) 
  {
    if ( Math.abs(actual-expect) < 0.0000001 ) {passed = true;}
  }

  //verify type is the same
  if ( typeof(expect) != typeof(actual) ) {passed = false;}

  return passed;
}


/*
 * Begin printing functions.  These functions use the shell's print function. 
*  When running tests in the browser, override these functions with functions 
*  that use document.write.
 */
function writeTestCaseResult( expect, actual, string ) 
{
  var passed = getTestCaseResult(expect, actual );
  writeFormattedResult( expect, actual, string, passed );
  return passed;
}


function writeFormattedResult( expect, actual, string, passed ) 
{
  var s = string ;
  s += ( passed ) ? PASSED : FAILED + expect;
  writeLineToLog( s);
  return passed;
}


function writeLineToLog( string ) 
{
  print( string );
}


function writeHeaderToLog( string ) 
{
  print( string );
}
/* End of printing functions */


/*
 * When running in the shell, run the garbage collector after the test has completed.
 */
function stopTest() 
{
  var gc;
  if ( gc != undefined ) 
  {
    gc();
  }
}


/*
 * Convenience function for displaying failed test cases.  
 * Useful when running tests manually.
*/
function getFailedCases() 
{
  for (var i = 0; i < testcases.length; i++ ) 
  {
    if ( !testcases[i].passed ) 
    {
      print( testcases[i].description  +  " = " +  testcases[i].actual  +  " expected: "  +  testcases[i].expect );
    }
  }
}


 /*
  * Date constants and functions used by tests in Date suite
*/
var msPerDay = 86400000;
var HoursPerDay = 24;
var MinutesPerHour = 60;
var SecondsPerMinute = 60;
var msPerSecond = 1000;
var msPerMinute = 60000;   // msPerSecond * SecondsPerMinute
var msPerHour = 3600000;   // msPerMinute * MinutesPerHour
var TZ_DIFF = getTimeZoneDiff();
var TZ_ADJUST = TZ_DIFF * msPerHour;
var TIME_1970 = 0;
var TIME_2000 = 946684800000;
var TIME_1900 = -2208988800000;
var UTC_29_FEB_2000 = TIME_2000 + 31*msPerDay + 28*msPerDay;
var UTC_1_JAN_2005 = TIME_2000 + TimeInYear(2000) + TimeInYear(2001) +
       TimeInYear(2002) + TimeInYear(2003) + TimeInYear(2004);
var now = new Date();
var TIME_NOW = now.valueOf();  //valueOf() is to accurate to the millisecond
                                                              //Date.parse() is accurate only to the second



/*
 * Originally, the test suite used a hard-coded value TZ_DIFF = -8. 
 * But that was only valid for testers in the Pacific Standard Time Zone! 
 * We calculate the proper number dynamically for any tester. We just
 * have to be careful to use a date not subject to Daylight Savings Time...
*/
function getTimeZoneDiff()
{
  return -((new Date(2000, 1, 1)).getTimezoneOffset())/60;
}


function Day( t) 
{
  return ( Math.floor( t/msPerDay ) );
}


function DaysInYear( y ) 
{
  if ( y % 4 != 0 ) {return 365;}

  if ( (y%4 == 0) && (y%100 != 0) ) {return 366;}

  if ( (y%100 == 0) && (y%400 != 0) ) {return 365;}

  if ( (y%400 == 0)){return 366;} 
  else {return "ERROR: DaysInYear("  +  y  +  ") case not covered";}
}


function TimeInYear( y ) 
{
  return ( DaysInYear(y) * msPerDay );
}


function DayNumber( t ) 
{
  return ( Math.floor( t / msPerDay ) );
}


function TimeWithinDay( t ) 
{
  if ( t < 0 ) {return ( (t%msPerDay) + msPerDay );} 
  else {return ( t % msPerDay );}
}


function YearNumber( t ) 
{
}


function TimeFromYear( y ) 
{
  return ( msPerDay * DayFromYear(y) );
}


function DayFromYear( y ) 
{
  return ( 365*(y-1970)  +  Math.floor((y-1969)/4)  -  Math.floor((y-1901)/100) 
                + Math.floor((y-1601)/400) );
}


function InLeapYear( t ) 
{
  if ( DaysInYear(YearFromTime(t)) == 365 ) {return 0;}

  if ( DaysInYear(YearFromTime(t)) == 366 ) {return 1;} 
  else {return "ERROR: InLeapYear("  +  t  +  ") case not covered";}
}


function YearFromTime( t ) 
{
  t =Number( t );
  var sign = ( t < 0 ) ? -1 : 1;
  var year = ( sign < 0 ) ? 1969 : 1970;

  for (var timeToTimeZero = t; ;  ) 
  {
    // subtract the current year's time from the time that's left.
    timeToTimeZero -= sign * TimeInYear(year)

    // if there's less than the current year's worth of time left, then break.
    if ( sign < 0 ) 
    {
      if ( sign * timeToTimeZero <= 0 ) {break;} 
      else {year += sign;}
    } 
    else 
    {
      if ( sign * timeToTimeZero < 0 ) {break;} 
      else {year += sign;}
    }
  }

  return ( year );
}


function MonthFromTime( t ) 
{
  var day = DayWithinYear( t );
  var leap = InLeapYear(t);

  // I know I could use switch but I'd rather not until it's part of ECMA
  if ( (0 <= day) && (day < 31) ) {return 0;}
  if ( (31 <= day) && (day < (59+leap) )) {return 1;}
  if ( ((59+leap) <= day) && (day < (90+leap) )) {return 2;}
  if ( ((90+leap) <= day) && (day < (120+leap) )) {return 3;}
  if ( ((120+leap) <= day) && (day < (151+leap) )) {return 4;}
  if ( ((151+leap) <= day) && (day < (181+leap) )) {return 5;}
  if ( ((181+leap) <= day) && (day < (212+leap) )) {return 6;}
  if ( ((212+leap) <= day) && (day < (243+leap)) ) {return 7;}
  if ( ((243+leap) <= day) && (day < (273+leap) )) {return 8;}
  if ( ((273+leap) <= day) && (day < (304+leap)) ) {return 9;}
  if ( ((304+leap) <= day) && (day < (334+leap)) ) {return 10;}
  if ( ((334+leap) <= day) && (day < (365+leap)) ) {return 11;} 
  else {return "ERROR: MonthFromTime("  +  t  +  ") not known";}
}


function DayWithinYear( t ) 
{
  return(Day(t) - DayFromYear(YearFromTime(t)) );
}


function DateFromTime( t ) 
{
  var day = DayWithinYear(t);
  var month = MonthFromTime(t);

  if ( month == 0) {return ( day + 1 );}
  if ( month == 1) {return ( day - 30 );}
  if ( month == 2) {return ( day - 58 - InLeapYear(t) );}
  if ( month == 3) {return ( day - 89 - InLeapYear(t));}
  if ( month == 4) {return ( day - 119 - InLeapYear(t));}
  if ( month == 5) {return ( day - 150 - InLeapYear(t));}
  if ( month == 6) {return ( day - 180 - InLeapYear(t));}
  if ( month == 7) {return ( day - 211 - InLeapYear(t));}
  if ( month == 8) {return ( day - 242 - InLeapYear(t));}
  if ( month == 9) {return ( day - 272 - InLeapYear(t));}
  if ( month == 10) {return ( day - 303 - InLeapYear(t));}
  if ( month == 11) {return ( day - 333 - InLeapYear(t));}
  return ("ERROR: DateFromTime("+t+") not known" );
}


function WeekDay( t ) 
{
  var weekday = (Day(t)+4)%7;
  return( weekday < 0 ?  7+weekday : weekday );
}


// missing daylight savings time adjustment


function HourFromTime( t ) 
{
  var h = Math.floor( t / msPerHour )%HoursPerDay;
  return ( (h<0) ? HoursPerDay + h : h  );
}


function MinFromTime( t ) 
{
  var min = Math.floor( t / msPerMinute )%MinutesPerHour;
  return( (min < 0 ) ?  MinutesPerHour + min : min );
}


function SecFromTime( t ) 
{
  var sec = Math.floor( t / msPerSecond )%SecondsPerMinute;
  return ( (sec < 0 ) ?  SecondsPerMinute + sec : sec );
}


function msFromTime( t ) 
{
  var ms = t%msPerSecond;
  return ( (ms < 0 ) ? msPerSecond + ms : ms );
}


function LocalTZA() 
{
  return ( TZ_DIFF * msPerHour );
}


function UTC( t ) 
{
  return ( t - LocalTZA() - DaylightSavingTA(t  - LocalTZA()) );
}


function DaylightSavingTA( t ) 
{
  t = t - LocalTZA();

  var dst_start = GetSecondSundayInMarch(t) +  2*msPerHour;
  var dst_end = GetFirstSundayInNovember(t) +  2*msPerHour;

  if ( t >= dst_start  &&  t < dst_end ) {return msPerHour;} 
  else {return 0;}

  // Daylight Savings Time starts on the first Sunday in April at 2:00AM in PST.  
  // Other time zones will need to override this function.

print( new Date( UTC(dst_start + LocalTZA())) );
return UTC(dst_start + LocalTZA());
}

function GetFirstSundayInApril( t ) {
    var year = YearFromTime(t);
    var leap = InLeapYear(t);

    var april = TimeFromYear(year) + TimeInMonth(0, leap) + TimeInMonth(1,leap) +
    TimeInMonth(2,leap);

    for ( var first_sunday = april; WeekDay(first_sunday) > 0;
        first_sunday += msPerDay )
    {
        ;
    }

    return first_sunday;
}
function GetLastSundayInOctober( t ) {
    var year = YearFromTime(t);
    var leap = InLeapYear(t);

    for ( var oct = TimeFromYear(year), m = 0; m < 9; m++ ) {
        oct += TimeInMonth(m, leap);
    }
    for ( var last_sunday = oct + 30*msPerDay; WeekDay(last_sunday) > 0;
        last_sunday -= msPerDay )
    {
        ;
    }
    return last_sunday;
}

// Added these two functions because DST rules changed for the US.
function GetSecondSundayInMarch( t ) {
	var	year = YearFromTime(t);
	var	leap = InLeapYear(t);

	var	march =	TimeFromYear(year) + TimeInMonth(0, leap) + TimeInMonth(1,leap);

	var sundayCount = 0;
	var flag = true;
	for ( var second_sunday = march; flag; second_sunday += msPerDay )
	{
		if (WeekDay(second_sunday) == 0) {
			if(++sundayCount == 2)
				flag = false;
		}
	}

	return second_sunday;
}
function GetFirstSundayInNovember( t ) {
	var year = YearFromTime(t);
	var leap = InLeapYear(t);

	for ( var nov = TimeFromYear(year), m =	0; m < 10; m++ ) {
		nov += TimeInMonth(m, leap);
	}
	for ( var first_sunday = nov; WeekDay(first_sunday) > 0;
		first_sunday += msPerDay	)
	{
		;
	}
	return first_sunday;
}


function LocalTime( t ) 
{
  return ( t + LocalTZA() + DaylightSavingTA(t) );
}


function MakeTime( hour, min, sec, ms ) 
{
  if ( isNaN(hour) || isNaN(min) || isNaN(sec) || isNaN(ms) ){return Number.NaN;}

  hour = ToInteger(hour);
  min  = ToInteger( min);
  sec  = ToInteger( sec);
  ms = ToInteger( ms );

  return( (hour*msPerHour) + (min*msPerMinute) + (sec*msPerSecond) + ms );
}


function MakeDay( year, month, date ) 
{
  if ( isNaN(year) || isNaN(month) || isNaN(date)) {return Number.NaN;}

  year = ToInteger(year);
  month = ToInteger(month);
  date = ToInteger(date );

  var sign = ( year < 1970 ) ?  -1 : 1;
  var t = ( year < 1970 ) ? 1 :  0;
  var y = ( year < 1970 ) ? 1969 : 1970;

  var result5 = year + Math.floor( month/12 );
  var result6= month%12;

  if ( year < 1970 ) 
  {
    for ( y = 1969; y >= year;  y += sign ) 
    {
      t += sign * TimeInYear(y);
    }
  } 
  else 
  {
    for ( y = 1970 ; y < year; y += sign ) 
    {
      t += sign * TimeInYear(y);
    }
  }

  var leap = InLeapYear( t );

  for ( var m = 0; m < month; m++) 
  {
    t += TimeInMonth( m, leap );
  }

  if ( YearFromTime(t) != result5 ) {return Number.NaN;} 
  if ( MonthFromTime(t) != result6 ) {return Number.NaN;}
  if ( DateFromTime(t) != 1 ){return Number.NaN;}

  return ( (Day(t)) + date - 1 );
}


function TimeInMonth( month, leap ) 
{  
  // Jan 0  Feb 1  Mar 2  Apr 3   May 4  June 5  Jul 6 Aug 7  Sep 8 Oct 9  Nov 10  Dec11

  // April  June  September November
  if ( month == 3 || month == 5 || month == 8 || month == 10 ) {return ( 30*msPerDay );}

  // all the rest
  if ( month == 0 || month == 2 || month == 4  || month == 6 ||
       month == 7  || month == 9 || month == 11 ) {return ( 31*msPerDay );}

  // save February
  return ( (leap == 0) ?  28*msPerDay : 29*msPerDay );
}


function MakeDate( day, time ) 
{
  if (day == Number.POSITIVE_INFINITY ||
       day == Number.NEGATIVE_INFINITY ||
       day == Number.NaN ) 
  {
    return Number.NaN;
  }

  if ( time == Number.POSITIVE_INFINITY ||
        time == Number.POSITIVE_INFINITY ||
        day == Number.NaN) 
  {
    return Number.NaN;
  }

  return ( day * msPerDay ) + time;
}


function TimeClip( t ) 
{
  if ( isNaN( t )) {return ( Number.NaN);}
  if ( Math.abs( t ) > 8.64e15 ) {return ( Number.NaN);}

  return ( ToInteger( t ) );
}


function ToInteger( t ) 
{
  t = Number( t );

  if ( isNaN( t )) {return ( Number.NaN);}

  if ( t == 0 || t == -0 || 
        t == Number.POSITIVE_INFINITY || 
        t == Number.NEGATIVE_INFINITY) 
  {
    return 0;
  }

  var sign = ( t < 0 ) ?  -1 : 1;

  return ( sign * Math.floor( Math.abs( t ) ) );
}


function Enumerate( o ) 
{
  var p;
  for ( p in o ) {print( p +  ": "  +  o[p] );}
}


/* these functions are useful for running tests manually in Rhino */

function GetContext() 
{
  return Packages.com.netscape.javascript.Context.getCurrentContext();
}


function OptLevel( i ) 
{
  i = Number(i);
  var cx = GetContext();
  cx.setOptimizationLevel(i);
}

/* end of Rhino functions */

