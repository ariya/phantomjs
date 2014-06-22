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
* Date: 12 Mar 2001
*
*
* SUMMARY: Testing Array.prototype.toLocaleString()
* See http://bugzilla.mozilla.org/show_bug.cgi?id=56883
* See http://bugzilla.mozilla.org/show_bug.cgi?id=58031
*
* By ECMA3 15.4.4.3, myArray.toLocaleString() means that toLocaleString()
* should be applied to each element of the array, and the results should be
* concatenated with an implementation-specific delimiter. For example:
*
*  myArray[0].toLocaleString()  +  ','  +  myArray[1].toLocaleString()  +  etc.
*
* In this testcase toLocaleString is a user-defined property of each array element;
* therefore it is the function that should be invoked. This function increments a
* global variable. Therefore the end value of this variable should be myArray.length.
*/
//-------------------------------------------------------------------------------------------------
var bug = 56883;
var summary = 'Testing Array.prototype.toLocaleString() -';
var actual = '';
var expect = '';
var n = 0;
var obj = {toLocaleString: function() {n++}};
var myArray = [obj, obj, obj];


myArray.toLocaleString();
actual = n;
expect = 3; // (see explanation above)


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
