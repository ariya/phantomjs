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
* Contributor(s): bzbarsky@mit.edu, pschwartau@netscape.com
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
* Date:    14 Mar 2003
* SUMMARY: Testing left-associativity of the + operator
*
* See ECMA-262 Ed.3, Section 11.6.1, "The Addition operator"
* See http://bugzilla.mozilla.org/show_bug.cgi?id=196290
*
* The upshot: |a + b + c| should always equal |(a + b) + c|
*
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 196290;
var summary = 'Testing left-associativity of the + operator';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


status = inSection(1);
actual = 1 + 1 + 'px';
expect = '2px';
addThis();

status = inSection(2);
actual = 'px' + 1 + 1;
expect = 'px11';
addThis();

status = inSection(3);
actual = 1 + 1 + 1 + 'px';
expect = '3px';
addThis();

status = inSection(4);
actual = 1 + 1 + 'a' + 1 + 1 + 'b';
expect = '2a11b';
addThis();

/*
 * The next sections test the + operator via eval()
 */
status = inSection(5);
actual = sumThese(1, 1, 'a', 1, 1, 'b');
expect = '2a11b';
addThis();

status = inSection(6);
actual = sumThese(new Number(1), new Number(1), 'a');
expect = '2a';
addThis();

status = inSection(7);
actual = sumThese('a', new Number(1), new Number(1));
expect = 'a11';
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

/*
 * Applies the + operator to the provided arguments via eval().
 *
 * Form an eval string of the form 'arg1 + arg2 + arg3', but
 * remember to add double-quotes inside the eval string around
 * any argument that is of string type. For example, suppose the
 * arguments were 11, 'a', 22. Then the eval string should be
 *
 *              arg1 + quoteThis(arg2) + arg3
 *
 * If we didn't put double-quotes around the string argument,
 * we'd get this for an eval string:
 *
 *                     '11 + a + 22'
 *
 * If we eval() this, we get 'ReferenceError: a is not defined'.
 * With proper quoting, we get eval('11 + "a" + 22') as desired.
 */
function sumThese()
{
  var sEval = '';
  var arg;
  var i;

  var L = arguments.length;
  for (i=0; i<L; i++)
  {
    arg = arguments[i];
    if (typeof arg === 'string')
      arg = quoteThis(arg);

    if (i < L-1)
      sEval += arg + ' + ';
    else
      sEval += arg;
  }

  return eval(sEval);
}


function quoteThis(x)
{
  return '"' + x + '"';
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
