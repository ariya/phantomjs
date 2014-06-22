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
* Date:    16 Dec 2002
* SUMMARY: Testing |with (x) {function f() {}}| when |x.f| already exists
* See http://bugzilla.mozilla.org/show_bug.cgi?id=185485
*
* The idea is this: if |x| does not already have a property named |f|,
* a |with| statement cannot be used to define one. See, for example,
*
*       http://bugzilla.mozilla.org/show_bug.cgi?id=159849#c11
*       http://bugzilla.mozilla.org/show_bug.cgi?id=184107
*
*
* However, if |x| already has a property |f|, a |with| statement can be
* used to modify the value it contains:
*
*                 with (x) {f = 1;}
*
* This should work even if we use a |var| statement, like this:
*
*                 with (x) {var f = 1;}
*
* However, it should NOT work if we use a |function| statement, like this:
*
*                 with (x) {function f() {}}
*
* Instead, this should newly define a function f in global scope.
* See http://bugzilla.mozilla.org/show_bug.cgi?id=185485
*
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 185485;
var summary = 'Testing |with (x) {function f() {}}| when |x.f| already exists';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];

var x = { f:0, g:0 };

with (x)
{
  f = 1;
}
status = inSection(1);
actual = x.f;
expect = 1;
addThis();

with (x)
{
  var f = 2;
}
status = inSection(2);
actual = x.f;
expect = 2;
addThis();

/*
 * Use of a function statement under the with-block should not affect
 * the local property |f|, but define a function |f| in global scope -
 */
with (x)
{
  function f() {}
}
status = inSection(3);
actual = x.f;
expect = 2;
addThis();

status = inSection(4);
actual = typeof this.f;
expect = 'function';
addThis();


/*
 * Compare use of function expression instead of function statement.
 * Note it is important that |x.g| already exists. Otherwise, this
 * would newly define |g| in global scope -
 */
with (x)
{
  var g = function() {}
}
status = inSection(5);
actual = x.g.toString();
expect = (function () {}).toString();
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
