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
* Contributor(s): igor3@apochta.com, pschwartau@netscape.com
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
* Date:    21 May 2002
* SUMMARY: ECMA conformance of Function.prototype.apply
*
*   Function.prototype.apply(thisArg, argArray)
*
* See ECMA-262 Edition 3 Final, Section 15.3.4.3
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 145791;
var summary = 'Testing ECMA conformance of Function.prototype.apply';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


function F0(a)
{
  return "" + this + arguments.length;
}

function F1(a)
{
  return "" + this + a;
}

function F2()
{
  return "" + this;
}



/*
 * Function.prototype.apply.length should return 2
 */
status = inSection(1);
actual = Function.prototype.apply.length;
expect = 2;
addThis();


/*
 * When |thisArg| is not provided to the apply() method, the
 * called function must be passed the global object as |this|
 */
status = inSection(2);
actual = F0.apply();
expect = "" + this + 0;
addThis();


/*
 * If |argArray| is not provided to the apply() method, the
 * called function should be invoked with an empty argument list
 */
status = inSection(3);
actual = F0.apply("");
expect = "" + "" + 0;
addThis();

status = inSection(4);
actual = F0.apply(true);
expect = "" + true + 0;
addThis();


/*
 * Function.prototype.apply(x) and
 * Function.prototype.apply(x, undefined) should return the same result
 */
status = inSection(5);
actual = F1.apply(0, undefined);
expect = F1.apply(0);
addThis();

status = inSection(6);
actual = F1.apply("", undefined);
expect = F1.apply("");
addThis();

status = inSection(7);
actual = F1.apply(null, undefined);
expect = F1.apply(null);
addThis();

status = inSection(8);
actual = F1.apply(undefined, undefined);
expect = F1.apply(undefined);
addThis();


/*
 * Function.prototype.apply(x) and
 * Function.prototype.apply(x, null) should return the same result
 */
status = inSection(9);
actual = F1.apply(0, null);
expect = F1.apply(0);
addThis();

status = inSection(10);
actual = F1.apply("", null);
expect = F1.apply("");
addThis();

status = inSection(11);
actual = F1.apply(null, null);
expect = F1.apply(null);
addThis();

status = inSection(12);
actual = F1.apply(undefined, null);
expect = F1.apply(undefined);
addThis();


/*
 * Function.prototype.apply() and
 * Function.prototype.apply(undefined) should return the same result
 */
status = inSection(13);
actual = F2.apply(undefined);
expect = F2.apply();
addThis();


/*
 * Function.prototype.apply() and
 * Function.prototype.apply(null) should return the same result
 */
status = inSection(14);
actual = F2.apply(null);
expect = F2.apply();
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
