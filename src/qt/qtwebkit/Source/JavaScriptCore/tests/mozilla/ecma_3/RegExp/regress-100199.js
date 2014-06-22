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
* Date: 17 September 2001
*
* SUMMARY: Regression test for Bugzilla bug 100199
* See http://bugzilla.mozilla.org/show_bug.cgi?id=100199
*
* The empty character class [] is a valid RegExp construct: the condition
* that a given character belong to a set containing no characters. As such,
* it can never be met and is always FALSE. Similarly, [^] is a condition
* that matches any given character and is always TRUE.
*
* Neither one of these conditions should cause syntax errors in a RegExp.
*/
//-----------------------------------------------------------------------------
var i = 0;
var bug = 100199;
var summary = '[], [^] are valid RegExp conditions. Should not cause errors -';
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


pattern = /[]/;
  string = 'abc';
  status = inSection(1);
  actualmatch = string.match(pattern);
  expectedmatch = null;
  addThis();

  string = '';
  status = inSection(2);
  actualmatch = string.match(pattern);
  expectedmatch = null;
  addThis();

  string = '[';
  status = inSection(3);
  actualmatch = string.match(pattern);
  expectedmatch = null;
  addThis();

  string = '/';
  status = inSection(4);
  actualmatch = string.match(pattern);
  expectedmatch = null;
  addThis();

  string = '[';
  status = inSection(5);
  actualmatch = string.match(pattern);
  expectedmatch = null;
  addThis();

  string = ']';
  status = inSection(6);
  actualmatch = string.match(pattern);
  expectedmatch = null;
  addThis();

  string = '[]';
  status = inSection(7);
  actualmatch = string.match(pattern);
  expectedmatch = null;
  addThis();

  string = '[ ]';
  status = inSection(8);
  actualmatch = string.match(pattern);
  expectedmatch = null;
  addThis();

  string = '][';
  status = inSection(9);
  actualmatch = string.match(pattern);
  expectedmatch = null;
  addThis();


pattern = /a[]/;
  string = 'abc';
  status = inSection(10);
  actualmatch = string.match(pattern);
  expectedmatch = null;
  addThis();

  string = '';
  status = inSection(11);
  actualmatch = string.match(pattern);
  expectedmatch = null;
  addThis();

  string = 'a[';
  status = inSection(12);
  actualmatch = string.match(pattern);
  expectedmatch = null;
  addThis();

  string = 'a[]';
  status = inSection(13);
  actualmatch = string.match(pattern);
  expectedmatch = null;
  addThis();

  string = '[';
  status = inSection(14);
  actualmatch = string.match(pattern);
  expectedmatch = null;
  addThis();

  string = ']';
  status = inSection(15);
  actualmatch = string.match(pattern);
  expectedmatch = null;
  addThis();

  string = '[]';
  status = inSection(16);
  actualmatch = string.match(pattern);
  expectedmatch = null;
  addThis();

  string = '[ ]';
  status = inSection(17);
  actualmatch = string.match(pattern);
  expectedmatch = null;
  addThis();

  string = '][';
  status = inSection(18);
  actualmatch = string.match(pattern);
  expectedmatch = null;
  addThis();


pattern = /[^]/;
  string = 'abc';
  status = inSection(19);
  actualmatch = string.match(pattern);
  expectedmatch = Array('a');
  addThis();

  string = '';
  status = inSection(20);
  actualmatch = string.match(pattern);
  expectedmatch = null; //there are no characters to test against the condition
  addThis();

  string = '\/';
  status = inSection(21);
  actualmatch = string.match(pattern);
  expectedmatch = Array('/');
  addThis();

  string = '\[';
  status = inSection(22);
  actualmatch = string.match(pattern);
  expectedmatch = Array('[');
  addThis();
  
  string = '[';
  status = inSection(23);
  actualmatch = string.match(pattern);
  expectedmatch = Array('[');
  addThis();

  string = ']';
  status = inSection(24);
  actualmatch = string.match(pattern);
  expectedmatch = Array(']');
  addThis();

  string = '[]';
  status = inSection(25);
  actualmatch = string.match(pattern);
  expectedmatch = Array('[');
  addThis();

  string = '[ ]';
  status = inSection(26);
  actualmatch = string.match(pattern);
  expectedmatch = Array('[');
  addThis();

  string = '][';
  status = inSection(27);
  actualmatch = string.match(pattern);
  expectedmatch = Array(']');
  addThis();


pattern = /a[^]/;
  string = 'abc';
  status = inSection(28);
  actualmatch = string.match(pattern);
  expectedmatch = Array('ab');
  addThis();

  string = '';
  status = inSection(29);
  actualmatch = string.match(pattern);
  expectedmatch = null; //there are no characters to test against the condition
  addThis();

  string = 'a[';
  status = inSection(30);
  actualmatch = string.match(pattern);
  expectedmatch = Array('a[');
  addThis();

  string = 'a]';
  status = inSection(31);
  actualmatch = string.match(pattern);
  expectedmatch = Array('a]');
  addThis();

  string = 'a[]';
  status = inSection(32);
  actualmatch = string.match(pattern);
  expectedmatch = Array('a[');
  addThis();

  string = 'a[ ]';
  status = inSection(33);
  actualmatch = string.match(pattern);
  expectedmatch = Array('a[');
  addThis();

  string = 'a][';
  status = inSection(34);
  actualmatch = string.match(pattern);
  expectedmatch = Array('a]');
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
