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
* Copyright (C) 1998 Netscape Communications Corporation. All
* Rights Reserved.
*
* Contributor(s): georg@bioshop.de, pschwartau@netscape.com
* Date: 10 September 2001
*
* SUMMARY: Testing with() statement with nested functions
* See http://bugzilla.mozilla.org/show_bug.cgi?id=97921
*
* Brendan: "The bug is peculiar to functions that have formal parameters,
* but that are called with fewer actual arguments than the declared number
* of formal parameters."
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 97921;
var summary = 'Testing with() statement with nested functions';
var cnYES = 'Inner value === outer value';
var cnNO = "Inner value !== outer value!";
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var outerValue = '';
var innerValue = '';
var useWith = '';


function F(i)
{
  i = 0;
  if(useWith) with(1){i;}
  i++;

  outerValue = i; // capture value of i in outer function
  F1 = function() {innerValue = i;}; // capture value of i in inner function
  F1();
}


status = inSection(1);
useWith=false;
F(); // call F without supplying the argument
actual = innerValue === outerValue;
expect = true;
addThis();

status = inSection(2);
useWith=true;
F(); // call F without supplying the argument
actual = innerValue === outerValue;
expect = true;
addThis();


function G(i)
{
  i = 0;
  with (new Object()) {i=100};
  i++;

  outerValue = i; // capture value of i in outer function
  G1 = function() {innerValue = i;}; // capture value of i in inner function
  G1();
}


status = inSection(3);
G(); // call G without supplying the argument
actual = innerValue === 101;
expect = true;
addThis();

status = inSection(4);
G(); // call G without supplying the argument
actual = innerValue === outerValue;
expect = true;
addThis();



//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------


function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = areTheseEqual(actual);
  expectedvalues[UBound] = areTheseEqual(expect);
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


function areTheseEqual(yes)
{
  return yes? cnYES : cnNO
}
