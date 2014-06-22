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
* Contributor(s): ajvincent@hotmail.com, pschwartau@netscape.com
* Date: 31 October 2001
*
* SUMMARY: Regression test for bug 107771
* See http://bugzilla.mozilla.org/show_bug.cgi?id=107771
*
* The bug: Section 1 passed, but Sections 2-5 all failed with |actual| == 12
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 107771;
var summary = "Regression test for bug 107771";
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var str = '';
var k = -9999;


status = inSection(1);
str = "AAA//BBB/CCC/";
k = str.lastIndexOf('/');
actual = k;
expect = 12;

status = inSection(2);
str = str.substring(0, k);
k = str.lastIndexOf('/');
actual = k;
expect = 8;
addThis();

status = inSection(3);
str = str.substring(0, k);
k = str.lastIndexOf('/');
actual = k;
expect = 4;
addThis();

status = inSection(4);
str = str.substring(0, k);
k = str.lastIndexOf('/');
actual = k;
expect = 3;
addThis();

status = inSection(5);
str = str.substring(0, k);
k = str.lastIndexOf('/');
actual = k;
expect = -1;
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

  for (var i=0; i<UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
