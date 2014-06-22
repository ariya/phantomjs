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
* Contributor(s): pschwartau@netscape.com
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
* Date:    18 July 2002
* SUMMARY: Testing octal sequences in regexps
* See http://bugzilla.mozilla.org/show_bug.cgi?id=141078
*
*/
//-----------------------------------------------------------------------------
var i = 0;
var bug = 141078;
var summary = 'Testing octal sequences in regexps';
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
pattern = /\240/;
string = 'abc';
actualmatch = string.match(pattern);
expectedmatch = null;
addThis();

/*
 * In the following sections, we test the octal escape sequence '\052'.
 * This is character code 42, representing the asterisk character '*'.
 * The Unicode escape for it would be '\u002A', the hex escape '\x2A'.
 */
status = inSection(2);
pattern = /ab\052c/;
string = 'ab*c';
actualmatch = string.match(pattern);
expectedmatch = Array('ab*c');
addThis();

status = inSection(3);
pattern = /ab\052*c/;
string = 'abc';
actualmatch = string.match(pattern);
expectedmatch = Array('abc');
addThis();

status = inSection(4);
pattern = /ab(\052)+c/;
string = 'ab****c';
actualmatch = string.match(pattern);
expectedmatch = Array('ab****c', '*');
addThis();

status = inSection(5);
pattern = /ab((\052)+)c/;
string = 'ab****c';
actualmatch = string.match(pattern);
expectedmatch = Array('ab****c', '****', '*');
addThis();

status = inSection(6);
pattern = /(?:\052)c/;
string = 'ab****c';
actualmatch = string.match(pattern);
expectedmatch = Array('*c');
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
  enterFunc('test');
  printBugNumber(bug);
  printStatus(summary);
  testRegExp(statusmessages, patterns, strings, actualmatches, expectedmatches);
  exitFunc ('test');
}
