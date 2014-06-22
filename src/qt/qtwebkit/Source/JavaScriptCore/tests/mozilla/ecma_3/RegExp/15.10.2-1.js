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
* Contributor(s): rogerl@netscape.com, pschwartau@netscape.com
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
* Date:    09 July 2002
* SUMMARY: RegExp conformance test
*
*   These testcases are derived from the examples in the ECMA-262 Ed.3 spec
*   scattered through section 15.10.2.
*
*/
//-----------------------------------------------------------------------------
var i = 0;
var bug = '(none)';
var summary = 'RegExp conformance test';
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
pattern = /a|ab/;
string = 'abc';
actualmatch = string.match(pattern);
expectedmatch = Array('a');
addThis();

status = inSection(2);
pattern = /((a)|(ab))((c)|(bc))/;
string = 'abc';
actualmatch = string.match(pattern);
expectedmatch = Array('abc', 'a', 'a', undefined, 'bc', undefined, 'bc');
addThis();

status = inSection(3);
pattern = /a[a-z]{2,4}/;
string = 'abcdefghi';
actualmatch = string.match(pattern);
expectedmatch = Array('abcde');
addThis();

status = inSection(4);
pattern = /a[a-z]{2,4}?/;
string = 'abcdefghi';
actualmatch = string.match(pattern);
expectedmatch = Array('abc');
addThis();

status = inSection(5);
pattern = /(aa|aabaac|ba|b|c)*/;
string = 'aabaac';
actualmatch = string.match(pattern);
expectedmatch = Array('aaba', 'ba');
addThis();

status = inSection(6);
pattern = /^(a+)\1*,\1+$/;
string = 'aaaaaaaaaa,aaaaaaaaaaaaaaa';
actualmatch = string.match(pattern);
expectedmatch = Array('aaaaaaaaaa,aaaaaaaaaaaaaaa', 'aaaaa');
addThis();

status = inSection(7);
pattern = /(z)((a+)?(b+)?(c))*/;
string = 'zaacbbbcac';
actualmatch = string.match(pattern);
expectedmatch = Array('zaacbbbcac', 'z', 'ac', 'a', undefined, 'c');
addThis();

status = inSection(8);
pattern = /(a*)*/;
string = 'b';
actualmatch = string.match(pattern);
expectedmatch = Array('', undefined);
addThis();

status = inSection(9);
pattern = /(a*)b\1+/;
string = 'baaaac';
actualmatch = string.match(pattern);
expectedmatch = Array('b', '');
addThis();

status = inSection(10);
pattern = /(?=(a+))/;
string = 'baaabac';
actualmatch = string.match(pattern);
expectedmatch = Array('', 'aaa');
addThis();

status = inSection(11);
pattern = /(?=(a+))a*b\1/;
string = 'baaabac';
actualmatch = string.match(pattern);
expectedmatch = Array('aba', 'a');
addThis();

status = inSection(12);
pattern = /(.*?)a(?!(a+)b\2c)\2(.*)/;
string = 'baaabaac';
actualmatch = string.match(pattern);
expectedmatch = Array('baaabaac', 'ba', undefined, 'abaac');
addThis();

status = inSection(13);
pattern = /(?=(a+))/;
string = 'baaabac';
actualmatch = string.match(pattern);
expectedmatch = Array('', 'aaa');
addThis();



//-------------------------------------------------------------------------------------------------
test();
//-------------------------------------------------------------------------------------------------


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
