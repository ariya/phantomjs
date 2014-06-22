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
* Contributor(s): shaver@mozilla.org  
* Date: 06 Mar 2001
*
* SUMMARY: Propagate heavyweightness back up the function-nesting
* chain. See http://bugzilla.mozilla.org/show_bug.cgi?id=71107
*
*/
//-------------------------------------------------------------------------------------------------
var bug = 71107;
var summary = 'Propagate heavyweightness back up the function-nesting chain...';


//-------------------------------------------------------------------------------------------------
test(); 
//-------------------------------------------------------------------------------------------------


function test() 
{
  enterFunc ('test'); 
  printBugNumber (bug);
  printStatus (summary);
  
  var actual = outer()()();  //call the return of calling the return of outer()
  var expect = 5;
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}


function outer () {
  var outer_var = 5;

  function inner() {
    function way_inner() {
      return outer_var;
    }
    return way_inner;
  }
  return inner;
}
