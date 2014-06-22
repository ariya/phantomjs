/*
* The contents of this file are subject to the Netscape Public
* License Version 1.1 (the "License"); you may not use this file
* except in compliance with the License. You may obtain a copy of
* the License at http://www.mozilla.org/NPL/
*
* Software distributed under the License is distributed on an "AS IS"
* basis, WITHOUT WARRANTY OF ANY KIND, either expressed
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
* Date: 04 September 2001
*
* SUMMARY: Regression test for Bugzilla bug 98306
* "JS parser crashes in ParseAtom for script using Regexp()"
*
* See http://bugzilla.mozilla.org/show_bug.cgi?id=98306
*/
//-----------------------------------------------------------------------------
var bug = 98306;
var summary = "Testing that we don't crash on this code -";
var cnUBOUND = 10;
var re;
var s;


//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  s = '"Hello".match(/[/]/)';
  tryThis(s);

  s = 're = /[/';
  tryThis(s);

  s = 're = /[/]/';
  tryThis(s);

  s = 're = /[//]/';
  tryThis(s);

  exitFunc ('test');
}


// Try to provoke a crash -
function tryThis(sCode)
{
  // sometimes more than one attempt is necessary -
  for (var i=0; i<cnUBOUND; i++)
  {
    try
    {
      eval(sCode);
    }
    catch(e)
    {
      // do nothing; keep going -
    }
  }
}
