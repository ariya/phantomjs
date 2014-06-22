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
* Contributor(s): myngs@hotmail.com, pschwartau@netscape.com
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
* Date:    05 June 2003
* SUMMARY: Testing |with (f)| inside the definition of |function f()|
*
* See http://bugzilla.mozilla.org/show_bug.cgi?id=208496
*
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 208496;
var summary = 'Testing |with (f)| inside the definition of |function f()|';
var status = '';
var statusitems = [];
var actual = '(TEST FAILURE)';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


/*
 * GLOBAL SCOPE
 */
function f(par)
{
  var a = par;

  with(f)
  {
    var b = par;
    actual = b;
  }
}

status = inSection(1);
f('abc'); // this sets |actual|
expect = 'abc';
addThis();

status = inSection(2);
f(111 + 222); // sets |actual|
expect = 333;
addThis();


/*
 * EVAL SCOPE
 */
var s = '';
s += 'function F(par)';
s += '{';
s += '  var a = par;';

s += '  with(F)';
s += '  {';
s += '    var b = par;';
s += '    actual = b;';
s += '  }';
s += '}';

s += 'status = inSection(3);';
s += 'F("abc");'; // sets |actual|
s += 'expect = "abc";';
s += 'addThis();';

s += 'status = inSection(4);';
s += 'F(111 + 222);'; // sets |actual|
s += 'expect = 333;';
s += 'addThis();';
eval(s);


/*
 * FUNCTION SCOPE
 */
function g(par)
{
  // Add outer variables to complicate the scope chain -
  var a = '(TEST FAILURE)';
  var b = '(TEST FAILURE)';
  h(par);

  function h(par)
  {
    var a = par;

    with(h)
    {
      var b = par;
      actual = b;
    }
  }
}

status = inSection(5);
g('abc'); // sets |actual|
expect = 'abc';
addThis();

status = inSection(6);
g(111 + 222); // sets |actual|
expect = 333;
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
