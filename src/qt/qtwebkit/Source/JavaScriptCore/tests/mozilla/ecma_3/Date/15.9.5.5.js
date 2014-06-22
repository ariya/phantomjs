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
    File Name:          15.9.5.5.js
    ECMA Section: 15.9.5.5 Date.prototype.toLocaleString()
    Description:
    This function returns a string value. The contents of the string are
    implementation dependent, but are intended to represent the "date"
    portion of the Date in the current time zone in a convenient,
    human-readable form.   We can't test the content of the string,  
    but can verify that the object returned is a string.

    The toLocaleString function is not generic; it generates a runtime error
    if its 'this' value is not a Date object. Therefore it cannot be transferred
    to other kinds of objects for use as a method.

    Author:  pschwartau@netscape.com                             
    Date:      14 november 2000
*/

   var SECTION = "15.9.5.5";
   var VERSION = "ECMA_3";  
   var TITLE   = "Date.prototype.toLocaleString()";  
   
   var status = '';
   var actual = '';  
   var expect = '';


   startTest();
   writeHeaderToLog( SECTION + " "+ TITLE);


//-----------------------------------------------------------------------------------------------------
   var testcases = new Array();
//-----------------------------------------------------------------------------------------------------


   // first, some generic tests -

   status = "typeof (now.toLocaleString())";  
   actual =   typeof (now.toLocaleString());
   expect = "string";
   addTestCase();

   status = "Date.prototype.toLocaleString.length";   
   actual =  Date.prototype.toLocaleString.length;
   expect =  0;   
   addTestCase();

//-----------------------------------------------------------------------------------------------------
   test();
//-----------------------------------------------------------------------------------------------------


function addTestCase()
{
  testcases[tc++] = new TestCase( SECTION, status, expect, actual); 
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