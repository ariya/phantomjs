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
* SUMMARY: ECMA conformance of Function.prototype.call
*
*   Function.prototype.call(thisArg [,arg1 [,arg2, ...]])
*
* See ECMA-262 Edition 3 Final, Section 15.3.4.4
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 145791;
var summary = 'Testing ECMA conformance of Function.prototype.call';
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
 * Function.prototype.call.length should return 1
 */
status = inSection(1);
actual = Function.prototype.call.length;
expect = 1;
addThis();


/*
 * When |thisArg| is not provided to the call() method, the
 * called function must be passed the global object as |this|
 */
status = inSection(2);
actual = F0.call();
expect = "" + this + 0;
addThis();


/*
 * If [,arg1 [,arg2, ...]] are not provided to the call() method,
 * the called function should be invoked with an empty argument list
 */
status = inSection(3);
actual = F0.call("");
expect = "" + "" + 0;
addThis();

status = inSection(4);
actual = F0.call(true);
expect = "" + true + 0;
addThis();


/*
 * Function.prototype.call(x) and
 * Function.prototype.call(x, undefined) should return the same result
 */
status = inSection(5);
actual = F1.call(0, undefined);
expect = F1.call(0);
addThis();

status = inSection(6);
actual = F1.call("", undefined);
expect = F1.call("");
addThis();

status = inSection(7);
actual = F1.call(null, undefined);
expect = F1.call(null);
addThis();

status = inSection(8);
actual = F1.call(undefined, undefined);
expect = F1.call(undefined);
addThis();


/*
 * Function.prototype.call() and
 * Function.prototype.call(undefined) should return the same result
 */
status = inSection(9);
actual = F2.call(undefined);
expect = F2.call();
addThis();


/*
 * Function.prototype.call() and
 * Function.prototype.call(null) should return the same result
 */
status = inSection(10);
actual = F2.call(null);
expect = F2.call();
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
