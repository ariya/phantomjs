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
* Contributor(s): jband@netscape.com, pschwartau@netscape.com
* Date: 29 Aug 2001
*
* SUMMARY: Negative test that JS infinite recursion protection works.
* We expect the code here to fail (i.e. exit code 3), but NOT crash.
*
* See http://bugzilla.mozilla.org/show_bug.cgi?id=96128
*/
//-----------------------------------------------------------------------------
var bug = 96128;
var summary = 'Testing that JS infinite recursion protection works';


function objRecurse()
{
  /*
   * jband:
   *
   * Causes a stack overflow crash in debug builds of both the browser
   * and the shell. In the release builds this is safely caught by the
   * "too much recursion" mechanism. If I remove the 'new' from the code below
   * this is safely caught in both debug and release builds. The 'new' causes a
   * lookup for the Constructor name and seems to (at least) double the number
   * of items on the C stack for the given interpLevel depth.
   */
  return new objRecurse();
}



//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------


function test() 
{
  enterFunc ('test'); 
  printBugNumber (bug);
  printStatus (summary);

  // we expect this to fail (exit code 3), but NOT crash. -
  var obj = new objRecurse();
 
  exitFunc ('test');
}
