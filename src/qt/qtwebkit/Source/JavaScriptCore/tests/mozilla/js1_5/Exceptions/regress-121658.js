/* ***** BEGIN LICENSE BLOCK *****
* Version: NPL 1.1/GPL 2.0/LGPL 2.1
*
* The contents of this file are subject to the Netscape Public License
* Version 1.1 (the "License"); you may not use this file except in
* compliance with the License. You may obtain a copy of the License at
* http://www.mozilla.org/NPL/
*
* Software distributed under the License is distributed on an "AS IS" basis,
* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
* for the specific language governing rights and limitations under the
* License.
*
* The Original Code is JavaScript Engine testing utilities.
*
* The Initial Developer of the Original Code is Netscape Communications Corp.
* Portions created by the Initial Developer are Copyright (C) 2002
* the Initial Developer. All Rights Reserved.
*
* Contributor(s): timeless@mac.com,pschwartau@netscape.com
*
* Alternatively, the contents of this file may be used under the terms of
* either the GNU General Public License Version 2 or later (the "GPL"), or
* the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
* in which case the provisions of the GPL or the LGPL are applicable instead
* of those above. If you wish to allow use of your version of this file only
* under the terms of either the GPL or the LGPL, and not to allow others to
* use your version of this file under the terms of the NPL, indicate your
* decision by deleting the provisions above and replace them with the notice
* and other provisions required by the GPL or the LGPL. If you do not delete
* the provisions above, a recipient may use your version of this file under
* the terms of any one of the NPL, the GPL or the LGPL.
*
* ***** END LICENSE BLOCK *****
*
*
* Date:    24 Jan 2002
* SUMMARY: "Too much recursion" errors should be safely caught by try...catch
* See http://bugzilla.mozilla.org/show_bug.cgi?id=121658
*
* In the cases below, we expect i>0. The bug was filed because we were getting
* i===0; i.e. |i| did not retain the value it had at the location of the error.
*
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 121658;
var msg = '"Too much recursion" errors should be safely caught by try...catch';
var TEST_PASSED = 'i retained the value it had at location of error';
var TEST_FAILED = 'i did NOT retain this value';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var i;


function f()
{
  ++i;

  // try...catch should catch the "too much recursion" error to ensue
  try
  {
    f();
  }
  catch(e)
  {
  }
}

i=0; 
f();
status = inSection(1);
actual = (i>0);
expect = true;
addThis();



// Now try in function scope -
function g()
{
  f();
}

i=0; 
g();
status = inSection(2);
actual = (i>0);
expect = true;
addThis();



// Now try in eval scope -
var sEval = 'function h(){++i; try{h();} catch(e){}}; i=0; h();';
eval(sEval);
status = inSection(3);
actual = (i>0);
expect = true;
addThis();



// Try in eval scope and mix functions up -
sEval = 'function a(){++i; try{h();} catch(e){}}; i=0; a();';
eval(sEval);
status = inSection(4);
actual = (i>0);
expect = true;
addThis();




//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------



function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = formatThis(actual);
  expectedvalues[UBound] = formatThis(expect);
  UBound++;
}


function formatThis(bool)
{
  return bool? TEST_PASSED : TEST_FAILED;
}


function test()
{
  enterFunc('test');
  printBugNumber(bug);
  printStatus(msg);
 
  for (var i=0; i<UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
