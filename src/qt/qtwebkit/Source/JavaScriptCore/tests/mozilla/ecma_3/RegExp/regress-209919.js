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
* Contributor(s): sagdjb@softwareag.com, pschwartau@netscape.com
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
* Date:    19 June 2003
* SUMMARY: Testing regexp submatches with quantifiers
*
* See http://bugzilla.mozilla.org/show_bug.cgi?id=209919
*
*/
//-----------------------------------------------------------------------------
var i = 0;
var bug = 209919;
var summary = 'Testing regexp submatches with quantifiers';
var status = '';
var statusmessages = new Array();
var pattern = '';
var patterns = new Array();
var string = '';
var strings = new Array();
var actualmatch = '';
var actualmatches = new Array();
var expectedmatch = '';
var expectedmatches = new Array();


/*
 * Waldemar: "ECMA-262 15.10.2.5, third algorithm, step 2.1 states that
 * once the minimum repeat count (which is 0 for *, 1 for +, etc.) has
 * been satisfied, an atom being repeated must not match the empty string."
 *
 * In this example, the minimum repeat count is 0, so the last thing the
 * capturing parens is permitted to contain is the 'a'. It may NOT go on
 * to capture the '' at the $ position of 'a', even though '' satifies
 * the condition b*
 *
 */
status = inSection(1);
string = 'a';
pattern = /(a|b*)*/;
actualmatch = string.match(pattern);
expectedmatch = Array(string, 'a');
addThis();


/*
 * In this example, the minimum repeat count is 5, so the capturing parens
 * captures the 'a', then goes on to capture the '' at the $ position of 'a'
 * 4 times before it has to stop. Therefore the last thing it contains is ''.
 */
status = inSection(2);
string = 'a';
pattern = /(a|b*){5,}/;
actualmatch = string.match(pattern);
expectedmatch = Array(string, '');
addThis();


/*
 * Reduction of the above examples to contain only the condition b*
 * inside the capturing parens. This can be even harder to grasp!
 *
 * The global match is the '' at the ^ position of 'a', but the parens
 * is NOT permitted to capture it since the minimum repeat count is 0!
 */
status = inSection(3);
string = 'a';
pattern = /(b*)*/;
actualmatch = string.match(pattern);
expectedmatch = Array('', undefined);
addThis();


/*
 * Here we have used the + quantifier (repeat count 1) outside the parens.
 * Therefore the parens must capture at least once before stopping, so it
 * does capture the '' this time -
 */
status = inSection(4);
string = 'a';
pattern = /(b*)+/;
actualmatch = string.match(pattern);
expectedmatch = Array('', '');
addThis();


/*
 * More complex examples -
 */
pattern = /^\-?(\d{1,}|\.{0,})*(\,\d{1,})?$/;

  status = inSection(5);
  string = '100.00';
  actualmatch = string.match(pattern);
  expectedmatch = Array(string, '00', undefined);
  addThis();

  status = inSection(6);
  string = '100,00';
  actualmatch = string.match(pattern);
  expectedmatch = Array(string, '100', ',00');
  addThis();

  status = inSection(7);
  string = '1.000,00';
  actualmatch = string.match(pattern);
  expectedmatch = Array(string, '000', ',00');
  addThis();




//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------



function addThis()
{
  statusmessages[i] = status;
  patterns[i] = pattern;
  strings[i] = string;
  actualmatches[i] = actualmatch;
  expectedmatches[i] = expectedmatch;
  i++;
}


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  testRegExp(statusmessages, patterns, strings, actualmatches, expectedmatches);
  exitFunc ('test');
}
