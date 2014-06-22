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
* SUMMARY: var self = global JS object, outside any eval, is DontDelete
*
* See http://bugzilla.mozilla.org/show_bug.cgi?id=68498
* See http://bugzilla.mozilla.org/showattachment.cgi?attach_id=25251
*
* Brendan:
*
* "Demonstrate that variable statement outside any eval creates a 
*   DontDelete property of the global object"
*/
//-------------------------------------------------------------------------------------------------
var bug = 68498;
var summary ='Testing that variable statement outside any eval creates'  + 
                          ' a DontDelete property of the global object';


// To be pedantic, use a variable named 'self' to capture the global object -
var self = this;
var actual = (delete self);
var expect =false; 


//-------------------------------------------------------------------------------------------------
test();
//-------------------------------------------------------------------------------------------------


function test() 
{
  enterFunc ('test'); 
  printBugNumber (bug);
  printStatus (summary);  
  reportCompare(expect, actual, summary);
  exitFunc ('test');
}
