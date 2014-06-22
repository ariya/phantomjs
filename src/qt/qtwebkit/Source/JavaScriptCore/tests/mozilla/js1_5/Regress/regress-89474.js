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
* Contributors:darren.deridder@icarusproject.com,
*                     pschwartau@netscape.com
* Date: 07 July 2001
*
* SUMMARY: Regression test for Bugzilla bug 89474
* See http://bugzilla.mozilla.org/show_bug.cgi?id=89474
*
* This test used to crash the JS shell. This was discovered
* by Darren DeRidder <darren.deridder@icarusproject.com
*/
//-------------------------------------------------------------------------------------------------
var bug = 89474;
var summary = "Testing the JS shell doesn't crash on it.item()";
var cnTest = 'it.item()';


//-------------------------------------------------------------------------------------------------
test();
//-------------------------------------------------------------------------------------------------


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  tryThis(cnTest); // Just testing that we don't crash on this

  exitFunc ('test');
}


function tryThis(sEval)
{
  try
  {
    eval(sEval);
  }
  catch(e)
  {
    // If we get here, we didn't crash.
  }
}
