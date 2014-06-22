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
* Contributor(s): wtam@bigfoot.com, pschwartau@netscape.com
* Date: 30 October 2001
*
* SUMMARY: Regression test for bug 94257
* See http://bugzilla.mozilla.org/show_bug.cgi?id=94257
*
* Rhino used to crash on this code; specifically, on the line
*
*                       arr[1+1] += 2;
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 94257;
var summary = "Making sure we don't crash on this code -";
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


var arr = new Array(6);
arr[1+1] = 1;
arr[1+1] += 2;


status = inSection(1);
actual = arr[1+1];
expect = 3;
addThis();

status = inSection(2);
actual = arr[1+1+1];
expect = undefined;
addThis();

status = inSection(3);
actual = arr[1];
expect = undefined;
addThis();


arr[1+2] = 'Hello';


status = inSection(4);
actual = arr[1+1+1];
expect = 'Hello';
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
