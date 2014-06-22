/*
* The contents of this file are subject to the Netscape Public
* License Version 1.1 (the "License"); you may not use this file
* except in compliance with the License. You may obtain a copy of
* the License at http://www.mozilla.org/NPL/
*
* Software distributed under the License is distributed on an
* "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either expressed
* or implied. See the License for the specific language governing
* rights and limitations under the License.
*
* The Original Code is mozilla.org code.
*
* The Initial Developer of the Original Code is Netscape
* Communications Corporation.  Portions created by Netscape are
* Copyright (C) 1998 Netscape Communications Corporation.
* All Rights Reserved.
*
* Contributor(s): jrgm@netscape.com, pschwartau@netscape.com
* Date: 2001-07-11
*
* SUMMARY: Testing eval scope inside a function.
* See http://bugzilla.mozilla.org/show_bug.cgi?id=77578
*/
//-------------------------------------------------------------------------------------------------
var UBound = 0;
var bug = 77578;
var summary = 'Testing eval scope inside a function';
var cnEquals = '=';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


// various versions of JavaScript -
var JS_VER = [100, 110, 120, 130, 140, 150];

// Note contrast with local variables i,j,k defined below -
var i = 999;
var j = 999;
var k = 999;


//--------------------------------------------------
test();
//--------------------------------------------------


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  // Run tests A,B,C on each version of JS and store results
  for (var n=0; n!=JS_VER.length; n++)
  {
    testA(JS_VER[n]);
  }
  for (var n=0; n!=JS_VER.length; n++)
  {
    testB(JS_VER[n]);
  }
  for (var n=0; n!=JS_VER.length; n++)
  {
    testC(JS_VER[n]);
  }


  // Compare actual values to expected values -
  for (var i=0; i<UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}


function testA(ver)
{
  // Set the version of JS to test -
  version(ver);

  // eval the test, so it compiles AFTER version() has executed -
  var sTestScript = "";

  // Define a local variable i
  sTestScript += "status = 'Section A of test; JS ' + ver/100;";
  sTestScript += "var i=1;";
  sTestScript += "actual = eval('i');";
  sTestScript += "expect = 1;";
  sTestScript += "captureThis('i');";

  eval(sTestScript);
}


function testB(ver)
{
  // Set the version of JS to test -
  version(ver);

  // eval the test, so it compiles AFTER version() has executed -
  var sTestScript = "";

  // Define a local for-loop iterator j
  sTestScript += "status = 'Section B of test; JS ' + ver/100;";
  sTestScript += "for(var j=1; j<2; j++)";
  sTestScript += "{";
  sTestScript += "  actual = eval('j');";
  sTestScript += "};";
  sTestScript += "expect = 1;";
  sTestScript += "captureThis('j');";

  eval(sTestScript);
}


function testC(ver)
{
  // Set the version of JS to test -
  version(ver);

  // eval the test, so it compiles AFTER version() has executed -
  var sTestScript = "";

  // Define a local variable k in a try-catch block -
  sTestScript += "status = 'Section C of test; JS ' + ver/100;";
  sTestScript += "try";
  sTestScript += "{";
  sTestScript += "  var k=1;";
  sTestScript += "  actual = eval('k');";
  sTestScript += "}";
  sTestScript += "catch(e)";
  sTestScript += "{";
  sTestScript += "};";
  sTestScript += "expect = 1;";
  sTestScript += "captureThis('k');";

  eval(sTestScript);
}


function captureThis(varName)
{
  statusitems[UBound] = status;
  actualvalues[UBound] = varName + cnEquals + actual;
  expectedvalues[UBound] = varName + cnEquals + expect;
  UBound++;
}
