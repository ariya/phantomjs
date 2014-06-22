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
* Contributor(s): deneen@alum.bucknell.edu, shaver@mozilla.org
* Date: 08 August 2001
*
* SUMMARY: When we invoke a function, the arguments object should take
*          a back seat to any local identifier named "arguments".
*
* See http://bugzilla.mozilla.org/show_bug.cgi?id=94506
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 94506;
var summary = 'Testing functions employing identifiers named "arguments"';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var TYPE_OBJECT = typeof new Object();
var arguments = 5555;


// use a parameter named "arguments"
function F1(arguments)
{
  return arguments;
}


// use a local variable named "arguments"
function F2()
{
  var arguments = 55;
  return arguments;
}


// same thing in a different order. CHANGES THE RESULT!
function F3()
{
  return arguments;
  var arguments = 555;
}


// use the global variable above named "arguments"
function F4()
{
  return arguments;
}



/*
 * In Sections 1 and 2, expect the local identifier, not the arguments object.
 * In Sections 3 and 4, expect the arguments object, not the the identifier.
 */

status = 'Section 1 of test';
actual = F1(5);
expect = 5;
addThis();


status = 'Section 2 of test';
actual = F2();
expect = 55;
addThis();


status = 'Section 3 of test';
actual = typeof F3();
expect = TYPE_OBJECT;
addThis();


status = 'Section 4 of test';
actual = typeof F4();
expect = TYPE_OBJECT;
addThis();


// Let's try calling F1 without providing a parameter -
status = 'Section 5 of test';
actual = F1();
expect = undefined;
addThis();


// Let's try calling F1 with too many parameters -
status = 'Section 6 of test';
actual = F1(3,33,333);
expect = 3;
addThis();



//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------


function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = actual;
  expectedvalues[UBound] = expect;
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
