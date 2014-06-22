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
* Contributor(s): pschwartau@netscape.com  
* Date: 26 Feb 2001
* See http://bugzilla.mozilla.org/show_bug.cgi?id=44009
*
* SUMMARY:  Testing that we don't crash on obj.toSource()
*/
//-------------------------------------------------------------------------------------------------
var bug = 44009;
var summary = "Testing that we don't crash on obj.toSource()";
var obj1 = {};
var sToSource = ''; 
var self = this;  //capture a reference to the global JS object - 



//-------------------------------------------------------------------------------------------------
test();
//-------------------------------------------------------------------------------------------------


function test() 
{
  enterFunc ('test'); 
  printBugNumber (bug);
  printStatus (summary);

  var obj2 = {};

  // test various objects and scopes - 
  testThis(self);
  testThis(this);
  testThis(obj1);
  testThis(obj2);
 
  exitFunc ('test');
}


// We're just testing that we don't crash by doing this - 
function testThis(obj)
{
  sToSource = obj.toSource();
  obj.prop = obj; 
  sToSource = obj.toSource();
}