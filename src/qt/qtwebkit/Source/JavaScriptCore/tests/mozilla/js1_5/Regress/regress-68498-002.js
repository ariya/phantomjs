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
* SUMMARY: create a Deletable local variable using eval
*
* See http://bugzilla.mozilla.org/show_bug.cgi?id=68498
* See http://bugzilla.mozilla.org/showattachment.cgi?attach_id=25251
*
* Brendan:
*
* "Demonstrate the creation of a Deletable local variable using eval"
*/
//-------------------------------------------------------------------------------------------------
var bug = 68498;
var summary = 'Creating a Deletable local variable using eval';
var statprefix = '; currently at expect[';
var statsuffix = '] within test -';
var actual = [ ];
var expect = [ ];


// Capture a reference to the global object -
var self = this;

// This function is the heart of the test -
function f(s) {eval(s); actual[0]=y; return delete y;}


// Set the actual-results array. The next line will set actual[0] and actual[1] in one shot
actual[1] = f('var y = 42');
actual[2] = 'y' in self && y;

// Set the expected-results array -
expect[0] = 42;
expect[1] = true;
expect[2] = false;


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
