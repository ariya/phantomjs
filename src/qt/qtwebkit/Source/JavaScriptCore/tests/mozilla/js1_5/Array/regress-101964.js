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
* Date: 27 September 2001
*
* SUMMARY: Performance: truncating even very large arrays should be fast!
* See http://bugzilla.mozilla.org/show_bug.cgi?id=101964
*
* Adjust this testcase if necessary. The FAST constant defines
* an upper bound in milliseconds for any truncation to take.
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 101964;
var summary = 'Performance: truncating even very large arrays should be fast!';
var BIG = 10000000;
var LITTLE = 10;
var FAST = 50; // array truncation should be 50 ms or less to pass the test
var MSG_FAST = 'Truncation took less than ' + FAST + ' ms';
var MSG_SLOW = 'Truncation took ';
var MSG_MS = ' ms';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];



status = inSection(1);
var arr = Array(BIG);
var start = new Date();
arr.length = LITTLE;
actual = elapsedTime(start);
expect = FAST;
addThis();



//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------



function elapsedTime(startTime)
{
  return new Date() - startTime;
}


function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = isThisFast(actual);
  expectedvalues[UBound] = isThisFast(expect);
  UBound++;
}


function isThisFast(ms)
{
  if (ms <= FAST)
    return MSG_FAST;
  return MSG_SLOW + ms + MSG_MS;
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
