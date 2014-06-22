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
* Contributor(s): brendan@mozilla.org, pschwartau@netscape.com  
* Date: 15 Feb 2001
*
* SUMMARY: calling obj.eval(str)
*
* See http://bugzilla.mozilla.org/show_bug.cgi?id=68498
* See http://bugzilla.mozilla.org/showattachment.cgi?attach_id=25251
*
* Brendan:
*
* "Backward compatibility: support calling obj.eval(str), which evaluates
*   str using obj as the scope chain and variable object."
*/
//-------------------------------------------------------------------------------------------------
var bug = 68498;
var summary = 'Testing calling obj.eval(str)';
var statprefix = '; currently at expect[';
var statsuffix = '] within test -';
var actual = [ ];
var expect = [ ];


// Capture a reference to the global object -
var self = this;

// This function is the heart of the test -
function f(s) {self.eval(s); return y;}


// Set the actual-results array -
actual[0] = f('var y = 43');
actual[1] = 'y' in self && y;
actual[2] = delete y;
actual[3] = 'y' in self;

// Set the expected-results array -
expect[0] = 43;
expect[1] = 43;
expect[2] = true;
expect[3] = false;


//-------------------------------------------------------------------------------------------------
test();
//-------------------------------------------------------------------------------------------------


function test() 
{
  enterFunc ('test'); 
  printBugNumber (bug);
  printStatus (summary);
  
  for (var i in expect)
  {
    reportCompare(expect[i], actual[i], getStatus(i));
  }

  exitFunc ('test');
}


function getStatus(i)
{
  return (summary  +  statprefix  +  i  +  statsuffix);
}
