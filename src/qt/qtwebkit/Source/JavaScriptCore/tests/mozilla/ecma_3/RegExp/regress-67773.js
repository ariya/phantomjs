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
* Date: 06 February 2001
*
* SUMMARY:  Arose from Bugzilla bug 67773:
* "Regular subexpressions followed by + failing to run to completion"
*
* See http://bugzilla.mozilla.org/show_bug.cgi?id=67773
* See http://bugzilla.mozilla.org/show_bug.cgi?id=69989
*/
//-------------------------------------------------------------------------------------------------
var i = 0;
var bug = 67773;
var summary = 'Testing regular subexpressions followed by ? or +\n';
var cnSingleSpace = ' ';
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


pattern = /^(\S+)?( ?)(B+)$/;  //single space before second ? character
    status = inSection(1);
    string = 'AAABBB AAABBB ';  //single space at middle and at end -
    actualmatch = string.match(pattern);
    expectedmatch = null;
    addThis();

    status = inSection(2);
    string = 'AAABBB BBB';  //single space in the middle
    actualmatch = string.match(pattern);
    expectedmatch = Array(string,  'AAABBB', cnSingleSpace,  'BBB');
    addThis();

    status = inSection(3);
    string = 'AAABBB AAABBB';  //single space in the middle
    actualmatch = string.match(pattern);
    expectedmatch = null;
    addThis();


pattern = /^(A+B)+$/;
    status = inSection(4);
    string = 'AABAAB';
    actualmatch = string.match(pattern);
    expectedmatch = Array(string,  'AAB');
    addThis();

    status = inSection(5);
    string = 'ABAABAAAAAAB';
    actualmatch = string.match(pattern);
    expectedmatch = Array(string,  'AAAAAAB');
    addThis();

    status = inSection(6);
    string = 'ABAABAABAB';
    actualmatch = string.match(pattern);
    expectedmatch = Array(string,  'AB');
    addThis();

    status = inSection(7);
    string = 'ABAABAABABB';
    actualmatch = string.match(pattern);
    expectedmatch = null;   // because string doesn't match at end
    addThis();


pattern = /^(A+1)+$/;
    status = inSection(8);
    string = 'AA1AA1';
    actualmatch = string.match(pattern);
    expectedmatch = Array(string,  'AA1');
    addThis();


pattern = /^(\w+\-)+$/;
    status = inSection(9);
    string = '';
    actualmatch = string.match(pattern);
    expectedmatch = null;
    addThis();

    status = inSection(10);
    string = 'bla-';
    actualmatch = string.match(pattern);
    expectedmatch = Array(string, string);
    addThis();

    status = inSection(11);
    string = 'bla-bla';  // hyphen missing at end -
    actualmatch = string.match(pattern);
    expectedmatch = null;  //because string doesn't match at end
    addThis();

    status = inSection(12);
    string = 'bla-bla-';
    actualmatch = string.match(pattern);
    expectedmatch = Array(string, 'bla-');
    addThis();


pattern = /^(\S+)+(A+)$/;
    status = inSection(13);
    string = 'asdldflkjAAA';
    actualmatch = string.match(pattern);
    expectedmatch = Array(string, 'asdldflkjAA', 'A');
    addThis();

    status = inSection(14);
    string = 'asdldflkj AAA'; // space in middle
    actualmatch = string.match(pattern);
    expectedmatch = null;  //because of the space
    addThis();


pattern = /^(\S+)+(\d+)$/;
    status = inSection(15);
    string = 'asdldflkj122211';
    actualmatch = string.match(pattern);
    expectedmatch = Array(string, 'asdldflkj12221', '1');
    addThis();

    status = inSection(16);
    string = 'asdldflkj1111111aaa1';
    actualmatch = string.match(pattern);
    expectedmatch = Array(string, 'asdldflkj1111111aaa', '1');
    addThis();


/* 
 * This one comes from Stephen Ostermiller.
 * See http://bugzilla.mozilla.org/show_bug.cgi?id=69989
 */
pattern = /^[A-Za-z0-9]+((\.|-)[A-Za-z0-9]+)+$/;
    status = inSection(17);
    string = 'some.host.tld';
    actualmatch = string.match(pattern);
    expectedmatch = Array(string, '.tld', '.');
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
