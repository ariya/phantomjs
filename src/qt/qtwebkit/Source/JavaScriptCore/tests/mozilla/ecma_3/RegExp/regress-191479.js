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
* Contributor(s): flying@dom.natm.ru, pschwartau@netscape.com
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
* Date:    31 January 2003
* SUMMARY: Testing regular expressions of form /(x|y){n,}/
*
* See http://bugzilla.mozilla.org/show_bug.cgi?id=191479
*
*/
//-----------------------------------------------------------------------------
var i = 0;
var bug = 191479;
var summary = 'Testing regular expressions of form /(x|y){n,}/';
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


status = inSection(1);
string = '12 3 45';
pattern = /(\d|\d\s){2,}/;
actualmatch = string.match(pattern);
expectedmatch = Array('12', '2');
addThis();

status = inSection(2);
string = '12 3 45';
pattern = /(\d|\d\s){4,}/;
actualmatch = string.match(pattern);
expectedmatch = Array(string, '5');
addThis();

status = inSection(3);
string = '12 3 45';
pattern = /(\d|\d\s)+/;
actualmatch = string.match(pattern);
expectedmatch = Array('12', '2');
addThis();

status = inSection(4);
string = '12 3 45';
pattern = /(\d\s?){4,}/;
actualmatch = string.match(pattern);
expectedmatch = Array(string, '5');
addThis();

/*
 * Let's reverse the operands in Sections 1-3 above -
 */
status = inSection(5);
string = '12 3 45';
pattern = /(\d\s|\d){2,}/;
actualmatch = string.match(pattern);
expectedmatch = Array(string, '5');
addThis();

status = inSection(6);
string = '12 3 45';
pattern = /(\d\s|\d){4,}/;
actualmatch = string.match(pattern);
expectedmatch = Array(string, '5');
addThis();

status = inSection(7);
string = '12 3 45';
pattern = /(\d\s|\d)+/;
actualmatch = string.match(pattern);
expectedmatch = Array(string, '5');
addThis();


/*
 * Let's take all 7 sections above and make each quantifer non-greedy.
 *
 * This is done by appending ? to it. It doesn't change the meaning of
 * the quantifier, but makes it non-greedy, which affects the results -
 */
status = inSection(8);
string = '12 3 45';
pattern = /(\d|\d\s){2,}?/;
actualmatch = string.match(pattern);
expectedmatch = Array('12', '2');
addThis();

status = inSection(9);
string = '12 3 45';
pattern = /(\d|\d\s){4,}?/;
actualmatch = string.match(pattern);
expectedmatch = Array('12 3 4', '4');
addThis();

status = inSection(10);
string = '12 3 45';
pattern = /(\d|\d\s)+?/;
actualmatch = string.match(pattern);
expectedmatch = Array('1', '1');
addThis();

status = inSection(11);
string = '12 3 45';
pattern = /(\d\s?){4,}?/;
actualmatch = string.match(pattern);
expectedmatch = Array('12 3 4', '4');
addThis();

status = inSection(12);
string = '12 3 45';
pattern = /(\d\s|\d){2,}?/;
actualmatch = string.match(pattern);
expectedmatch = Array('12 ', '2 ');
addThis();

status = inSection(13);
string = '12 3 45';
pattern = /(\d\s|\d){4,}?/;
actualmatch = string.match(pattern);
expectedmatch = Array('12 3 4', '4');
addThis();

status = inSection(14);
string = '12 3 45';
pattern = /(\d\s|\d)+?/;
actualmatch = string.match(pattern);
expectedmatch = Array('1', '1');
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
