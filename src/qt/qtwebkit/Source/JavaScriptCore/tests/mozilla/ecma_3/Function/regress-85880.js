/*
* The contents of this file are subject to the Netscape Public
* License Version 1.1 (the "License"); you may not use this file
* except in compliance with the License. You may obtain a copy of
* the License at http://www.mozilla.org/NPL/
*
* Software distributed under the License is distributed on an "AS
* IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
* implied. See the License for the specific language governing
* rights and limitations under the License.
*
* The Original Code is mozilla.org code.
*
* The Initial Developer of the Original Code is Netscape
* Communications Corporation.  Portions created by Netscape are
* Copyright (C) 1998 Netscape Communications Corporation.
* All Rights Reserved.
*
* Contributor(s): pschwartau@netscape.com
* Date: 2001-06-14
*
* SUMMARY: Regression test for Bugzilla bug 85880
*
* Rhino interpreted mode was nulling out the arguments object of a
* function if it happened to call another function inside its body.
*
* See http://bugzilla.mozilla.org/show_bug.cgi?id=85880
*
*/
//-------------------------------------------------------------------------------------------------
var UBound = 0;
var bug = 85880;
var summary = 'Arguments object of g(){f()} should not be null';
var cnNonNull = 'Arguments != null';
var cnNull = 'Arguments == null';
var cnRecurse = true;
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


function f1(x)
{
}


function f2()
{
  return f2.arguments;
}
status = 'Section A of test';
actual = (f2() == null);
expect = false;
addThis();

status = 'Section B of test';
actual = (f2(0) == null);
expect = false;
addThis();


function f3()
{
  f1();
  return f3.arguments;
}
status = 'Section C of test';
actual = (f3() == null);
expect = false;
addThis();

status = 'Section D of test';
actual = (f3(0) == null);
expect = false;
addThis();


function f4()
{
  f1();
  f2();
  f3();
  return f4.arguments;
}
status = 'Section E of test';
actual = (f4() == null);
expect = false;
addThis();

status = 'Section F of test';
actual = (f4(0) == null);
expect = false;
addThis();


function f5()
{
  if (cnRecurse)
  {
    cnRecurse = false;
    f5();
  }
  return f5.arguments;
}
status = 'Section G of test';
actual = (f5() == null);
expect = false;
addThis();

status = 'Section H of test';
actual = (f5(0) == null);
expect = false;
addThis();



//-------------------------------------------------------------------------------------------------
test();
//-------------------------------------------------------------------------------------------------


function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = isThisNull(actual);
  expectedvalues[UBound] = isThisNull(expect);
  UBound++;
}


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  for (var i = 0; i < UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}


function isThisNull(bool)
{
  return bool? cnNull : cnNonNull
}
