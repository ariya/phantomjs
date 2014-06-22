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
* Contributor(s): brendan@mozilla.org, pschwartau@netscape.com
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
* Date:    29 Oct 2002
* SUMMARY: Testing "use" and "set" operations on expressions like a[i][j][k]
* See http://bugzilla.mozilla.org/show_bug.cgi?id=96526#c52
*
* Brendan: "The idea is to cover all the 'use' and 'set' (as in modify)
* operations you can do on an expression like a[i][j][k], including variations
* where you replace |a| with arguments (literally) and |i| with 0, 1, 2, etc.
* (to hit the optimization for arguments[0]... that uses JSOP_ARGSUB)."
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 96526;
var summary = 'Testing "use" and "set" ops on expressions like a[i][j][k]';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];

var z='magic';
Number.prototype.magic=42;

status = inSection(1);
actual = f(2,1,[-1,0,[1,2,[3,4]]]);
expect = 42;
addThis();


function f(j,k,a)
{
  status = inSection(2);
  actual = formatArray(a[2]);
  expect = formatArray([1,2,[3,4]]);
  addThis();

  status = inSection(3);
  actual = formatArray(a[2][j]);
  expect = formatArray([3,4]);
  addThis();

  status = inSection(4);
  actual = a[2][j][k];
  expect = 4;
  addThis();

  status = inSection(5);
  actual = a[2][j][k][z];
  expect = 42;
  addThis();

  return a[2][j][k][z];
}



//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------



function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = actual;
  expectedvalues[UBound] = expect;
  UBound++;
}


function test()
{
  enterFunc('test');
  printBugNumber(bug);
  printStatus(summary);

  for (var i=0; i<UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
