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
* Date:    09 December 2002
* SUMMARY: with(...) { function f ...} should set f in the global scope
* See http://bugzilla.mozilla.org/show_bug.cgi?id=184107
*
* In fact, any variable defined in a with-block should be created
* in global scope, i.e. should be a property of the global object.
*
* The with-block syntax allows existing local variables to be SET,
* but does not allow new local variables to be CREATED.
*
* See http://bugzilla.mozilla.org/show_bug.cgi?id=159849#c11
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 184107;
var summary = 'with(...) { function f ...} should set f in the global scope';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];

y = 1;
var obj = {y:10};
with (obj)
{
  // function statement
  function f()
  {
    return y;
  }

  // function expression
  g = function() {return y;}
}

status = inSection(1);
actual = obj.f;
expect = undefined;
addThis();

status = inSection(2);
actual = f();
// Mozilla result, which contradicts IE and the ECMA spec: expect = obj.y;
expect = y;
addThis();

status = inSection(3);
actual = obj.g;
expect = undefined;
addThis();

status = inSection(4);
actual = g();
expect = obj.y;
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
