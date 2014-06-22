/*
* The contents of this file are subject to the Netscape Public
* License Version 1.1 (the "License"); you may not use this file
* except in compliance with the License. You may obtain a copy of
* the License at http://www.mozilla.org/NPL/
*
* Software distributed under the License is distributed on an "AS  IS"
* basis, WITHOUT WARRANTY OF ANY KIND, either expressed
* or implied. See the License for the specific language governing
* rights and limitations under the License.
*
* The Original Code is mozilla.org code.
*
* The Initial Developer of the Original Code is Netscape
* Communications Corporation.  Portions created by Netscape are
* Copyright (C) 1998 Netscape Communications Corporation.
* All Rights Reserved.
*
* Contributor(s): pschwartau@netscape.com
* Date: 28 December 2000
*
* SUMMARY: Testing regular expressions containing the ? character.
* Arose from Bugzilla bug 57572: "RegExp with ? matches incorrectly"
*
* See http://bugzilla.mozilla.org/show_bug.cgi?id=57572
*
*/
//-----------------------------------------------------------------------------
var i = 0;
var bug = 57572;
var summary = 'Testing regular expressions containing "?"';
var cnEmptyString = ''; var cnSingleSpace = ' ';
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
pattern = /(\S+)?(.*)/;
string = 'Test this';
actualmatch = string.match(pattern);
expectedmatch = Array(string, 'Test', ' this');  //single space in front of 'this'
addThis();

status = inSection(2);
pattern = /(\S+)? ?(.*)/;  //single space between the ? characters
string= 'Test this';
actualmatch = string.match(pattern);
expectedmatch = Array(string, 'Test', 'this');  //NO space in front of 'this'
addThis();

status = inSection(3);
pattern = /(\S+)?(.*)/;
string = 'Stupid phrase, with six - (short) words';
actualmatch = string.match(pattern);
expectedmatch = Array(string, 'Stupid', ' phrase, with six - (short) words');  //single space in front of 'phrase'
addThis();

status = inSection(4);
pattern = /(\S+)? ?(.*)/;  //single space between the ? characters
string = 'Stupid phrase, with six - (short) words';
actualmatch = string.match(pattern);
expectedmatch = Array(string, 'Stupid', 'phrase, with six - (short) words');  //NO space in front of 'phrase'
addThis();


// let's add an extra back-reference this time - three instead of two -
status = inSection(5);
pattern = /(\S+)?( ?)(.*)/;  //single space before second ? character
string = 'Stupid phrase, with six - (short) words';
actualmatch = string.match(pattern);
expectedmatch = Array(string, 'Stupid', cnSingleSpace, 'phrase, with six - (short) words');
addThis();

status = inSection(6);
pattern = /^(\S+)?( ?)(B+)$/;  //single space before second ? character
string = 'AAABBB';
actualmatch = string.match(pattern);
expectedmatch = Array(string, 'AAABB', cnEmptyString, 'B');
addThis();

status = inSection(7);
pattern = /(\S+)?(!?)(.*)/;
string = 'WOW !!! !!!';
actualmatch = string.match(pattern);
expectedmatch = Array(string, 'WOW', cnEmptyString, ' !!! !!!');
addThis();

status = inSection(8);
pattern = /(.+)?(!?)(!+)/;
string = 'WOW !!! !!!';
actualmatch = string.match(pattern);
expectedmatch = Array(string, 'WOW !!! !!', cnEmptyString, '!');
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
