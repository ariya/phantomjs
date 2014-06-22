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
* Portions created by the Initial Developer are Copyright (C) 2003
* the Initial Developer. All Rights Reserved.
*
* Contributor(s): igor@icesoft.no, pschwartau@netscape.com
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
* Date:    21 February 2003
* SUMMARY: Testing eval statements containing conditional function expressions
*
* See http://bugzilla.mozilla.org/show_bug.cgi?id=194364
*
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 194364;
var summary = 'Testing eval statements with conditional function expressions';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


status = inSection(1);
actual = eval('1; function() {}');
expect = 1;
addThis();

status = inSection(2);
actual = eval('2; function f() {}');
expect = 2;
addThis();

status = inSection(3);
actual = eval('3; if (true) function() {}');
expect = 3;
addThis();

status = inSection(4);
actual = eval('4; if (true) function f() {}');
expect = 4;
addThis();

status = inSection(5);
actual = eval('5; if (false) function() {}');
expect = 5;
addThis();

status = inSection(6);
actual = eval('6; if (false) function f() {}');
expect = 6;
addThis();

status = inSection(7);
actual = eval('7; switch(true) { case true: function() {} }');
expect = 7;
addThis();

status = inSection(8);
actual = eval('8; switch(true) { case true: function f() {} }');
expect = 8;
addThis();

status = inSection(9);
actual = eval('9; switch(false) { case false: function() {} }');
expect = 9;
addThis();

status = inSection(10);
actual = eval('10; switch(false) { case false: function f() {} }');
expect = 10;
addThis();



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
