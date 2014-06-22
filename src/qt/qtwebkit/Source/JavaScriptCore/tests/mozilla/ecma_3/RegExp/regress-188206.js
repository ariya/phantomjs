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
* Contributor(s): scole@planetweb.com, pschwartau@netscape.com
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
* Date:    21 January 2003
* SUMMARY: Invalid use of regexp quantifiers should generate SyntaxErrors
*
* See http://bugzilla.mozilla.org/show_bug.cgi?id=188206
* and http://bugzilla.mozilla.org/show_bug.cgi?id=85721#c48 etc.
* and http://bugzilla.mozilla.org/show_bug.cgi?id=190685
* and http://bugzilla.mozilla.org/show_bug.cgi?id=197451
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 188206;
var summary = 'Invalid use of regexp quantifiers should generate SyntaxErrors';
var TEST_PASSED = 'SyntaxError';
var TEST_FAILED = 'Generated an error, but NOT a SyntaxError!';
var TEST_FAILED_BADLY = 'Did not generate ANY error!!!';
var CHECK_PASSED = 'Should not generate an error';
var CHECK_FAILED = 'Generated an error!';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


/*
 * All the following are invalid uses of regexp quantifiers and
 * should generate SyntaxErrors. That's what we're testing for.
 *
 * To allow the test to compile and run, we have to hide the errors
 * inside eval strings, and check they are caught at run-time -
 *
 */
status = inSection(1);
testThis(' /a**/ ');

status = inSection(2);
testThis(' /a***/ ');

status = inSection(3);
testThis(' /a++/ ');

status = inSection(4);
testThis(' /a+++/ ');

/*
 * The ? quantifier, unlike * or +, may appear twice in succession.
 * Thus we need at least three in a row to provoke a SyntaxError -
 */

status = inSection(5);
testThis(' /a???/ ');

status = inSection(6);
testThis(' /a????/ ');


/*
 * Now do some weird things on the left side of the regexps -
 */
status = inSection(7);
testThis(' /*a/ ');

status = inSection(8);
testThis(' /**a/ ');

status = inSection(9);
testThis(' /+a/ ');

status = inSection(10);
testThis(' /++a/ ');

status = inSection(11);
testThis(' /?a/ ');

status = inSection(12);
testThis(' /??a/ ');


/*
 * Misusing the {DecmalDigits} quantifier - according to ECMA,
 * but not according to Perl.
 *
 * ECMA-262 Edition 3 prohibits the use of unescaped braces in
 * regexp patterns, unless they form part of a quantifier.
 *
 * Hovever, Perl does not prohibit this. If not used as part
 * of a quantifer, Perl treats braces literally.
 *
 * We decided to follow Perl on this for backward compatibility.
 * See http://bugzilla.mozilla.org/show_bug.cgi?id=190685.
 *
 * Therefore NONE of the following ECMA violations should generate
 * a SyntaxError. Note we use checkThis() instead of testThis().
 */
status = inSection(13);
checkThis(' /a*{/ ');

status = inSection(14);
checkThis(' /a{}/ ');

status = inSection(15);
checkThis(' /{a/ ');

status = inSection(16);
checkThis(' /}a/ ');

status = inSection(17);
checkThis(' /x{abc}/ ');

status = inSection(18);
checkThis(' /{{0}/ ');

status = inSection(19);
checkThis(' /{{1}/ ');

status = inSection(20);
checkThis(' /x{{0}/ ');

status = inSection(21);
checkThis(' /x{{1}/ ');

status = inSection(22);
checkThis(' /x{{0}}/ ');

status = inSection(23);
checkThis(' /x{{1}}/ ');

status = inSection(24);
checkThis(' /x{{0}}/ ');

status = inSection(25);
checkThis(' /x{{1}}/ ');

status = inSection(26);
checkThis(' /x{{0}}/ ');

status = inSection(27);
checkThis(' /x{{1}}/ ');


/*
 * Misusing the {DecmalDigits} quantifier - according to BOTH ECMA and Perl.
 *
 * Just as with the * and + quantifiers above, can't have two {DecmalDigits}
 * quantifiers in succession - it's a SyntaxError.
 */
status = inSection(28);
testThis(' /x{1}{1}/ ');

status = inSection(29);
testThis(' /x{1,}{1}/ ');

status = inSection(30);
testThis(' /x{1,2}{1}/ ');

status = inSection(31);
testThis(' /x{1}{1,}/ ');

status = inSection(32);
testThis(' /x{1,}{1,}/ ');

status = inSection(33);
testThis(' /x{1,2}{1,}/ ');

status = inSection(34);
testThis(' /x{1}{1,2}/ ');

status = inSection(35);
testThis(' /x{1,}{1,2}/ ');

status = inSection(36);
testThis(' /x{1,2}{1,2}/ ');



//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------



/*
 * Invalid syntax should generate a SyntaxError
 */
function testThis(sInvalidSyntax)
{
  expect = TEST_PASSED;
  actual = TEST_FAILED_BADLY;

  try
  {
    eval(sInvalidSyntax);
  }
  catch(e)
  {
    if (e instanceof SyntaxError)
      actual = TEST_PASSED;
    else
      actual = TEST_FAILED;
  }

  statusitems[UBound] = status;
  expectedvalues[UBound] = expect;
  actualvalues[UBound] = actual;
  UBound++;
}


/*
 * Allowed syntax shouldn't generate any errors
 */
function checkThis(sAllowedSyntax)
{
  expect = CHECK_PASSED;
  actual = CHECK_PASSED;

  try
  {
    eval(sAllowedSyntax);
  }
  catch(e)
  {
    actual = CHECK_FAILED;
  }

  statusitems[UBound] = status;
  expectedvalues[UBound] = expect;
  actualvalues[UBound] = actual;
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
