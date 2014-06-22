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
    File Name:          15.9.5.3.js
    ECMA Section: 15.9.5.3 Date.prototype.toDateString()
    Description:
    This function returns a string value. The contents of the string are
    implementation dependent, but are intended to represent the "date"
    portion of the Date in the current time zone in a convenient,
    human-readable form.   We can't test the content of the string,  
    but can verify that the string is parsable by Date.parse

    The toDateString function is not generic; it generates a runtime error
    if its 'this' value is not a Date object. Therefore it cannot be transferred
    to other kinds of objects for use as a method.

    Author:  pschwartau@netscape.com                             
    Date:      14 november 2000  (adapted from ecma/Date/15.9.5.2.js)
*/

   var SECTION = "15.9.5.3";
   var VERSION = "ECMA_3";  
   var TITLE   = "Date.prototype.toDateString()";  
   
   var status = '';
   var actual = '';  
   var expect = '';


   startTest();
   writeHeaderToLog( SECTION + " "+ TITLE);


//-----------------------------------------------------------------------------------------------------
   var testcases = new Array();
//-----------------------------------------------------------------------------------------------------


   // first, some generic tests -

   status = "typeof (now.toDateString())";  
   actual =   typeof (now.toDateString());
   expect = "string";
   addTestCase();

   status = "Date.prototype.toDateString.length";   
   actual =  Date.prototype.toDateString.length;
   expect =  0;   
   addTestCase();

   /* Date.parse is accurate to the second;  valueOf() to the millisecond.
        Here we expect them to coincide, as we expect a time of exactly midnight -  */
   status = "(Date.parse(now.toDateString()) - (midnight(now)).valueOf()) == 0";   
   actual =   (Date.parse(now.toDateString()) - (midnight(now)).valueOf()) == 0;
   expect = true;
   addTestCase();



   // 1970
   addDateTestCase(0);
   addDateTestCase(TZ_ADJUST);   

   
   // 1900
   addDateTestCase(TIME_1900); 
   addDateTestCase(TIME_1900 - TZ_ADJUST);

   
   // 2000
   addDateTestCase(TIME_2000);
   addDateTestCase(TIME_2000 - TZ_ADJUST);

    
   // 29 Feb 2000
   addDateTestCase(UTC_29_FEB_2000);
   addDateTestCase(UTC_29_FEB_2000 - 1000);    
   addDateTestCase(UTC_29_FEB_2000 - TZ_ADJUST);
 

   // 2005
   addDateTestCase(UTC_1_JAN_2005);
   addDateTestCase(UTC_1_JAN_2005 - 1000);
   addDateTestCase(UTC_1_JAN_2005 - TZ_ADJUST);
   


//-----------------------------------------------------------------------------------------------------
   test();
//-----------------------------------------------------------------------------------------------------


function addTestCase()
{
  testcases[tc++] = new TestCase( SECTION, status, expect, actual); 
}


function addDateTestCase(date_given_in_milliseconds)
{
  var givenDate = new Date(date_given_in_milliseconds);

  status = 'Date.parse('   +   givenDate   +   ').toDateString())';   
  actual =  Date.parse(givenDate.toDateString());
  expect = Date.parse(midnight(givenDate));
  addTestCase();
}


function midnight(givenDate) 
{
  // midnight on the given date -
  return new Date(givenDate.getFullYear(), givenDate.getMonth(), givenDate.getDate());
}


function test() 
{
  for ( tc=0; tc < testcases.length; tc++ ) 
  {
    testcases[tc].passed = writeTestCaseResult(
                                               testcases[tc].expect,
                                               testcases[tc].actual,
                                               testcases[tc].description  +  " = "  +  testcases[tc].actual );

    testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
  }
  stopTest();
  return (testcases);
}