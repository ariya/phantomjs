/*
* The contents of this file are subject to the Netscape Public
* License Version 1.1 (the "License"); you may not use this file
* except in compliance with the License. You may obtain a copy of
* the License at http://www.mozilla.org/NPL/
*
* Software distributed under the License is distributed on an "AS
* IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
* implied. See the License for the specific language governing
* rights and limitations under the License.
*
* The Original Code is mozilla.org code.
*
* The Initial Developer of the Original Code is Netscape
* Communications Corporation.  Portions created by Netscape are
* Copyright (C) 1998 Netscape Communications Corporation. All
* Rights Reserved.
*
* Contributor(s): pschwartau@netscape.com
* Date: 16 May 2001
*
* SUMMARY: Regression test for bug 76054
*
* See http://bugzilla.mozilla.org/show_bug.cgi?id=76054
* See http://bugzilla.mozilla.org/show_bug.cgi?id=78706
* All String HTML methods should be LOWER case -
*/
//-------------------------------------------------------------------------------------------------
var UBound = 0;
var bug = 76054;
var summary = 'Testing that String HTML methods produce all lower-case';
var statprefix = 'Currently testing String.';
var status = '';
var statusitems = [ ];
var actual = '';
var actualvalues = [ ];
var expect= '';
var expectedvalues = [ ];
var s = 'xyz';

status = 'anchor()';
actual = s.anchor();
expect = actual.toLowerCase();
addThis();

status = 'big()';
actual = s.big();
expect = actual.toLowerCase();
addThis();

status = 'blink()';
actual = s.blink();
expect = actual.toLowerCase();
addThis();

status = 'bold()';
actual = s.bold();
expect = actual.toLowerCase();
addThis();

status = 'italics()';
actual = s.italics();
expect = actual.toLowerCase();
addThis();

status = 'fixed()';
actual = s.fixed();
expect = actual.toLowerCase();
addThis();

status = 'fontcolor()';
actual = s.fontcolor();
expect = actual.toLowerCase();
addThis();

status = 'fontsize()';
actual = s.fontsize();
expect = actual.toLowerCase();
addThis();

status = 'link()';
actual = s.link();
expect = actual.toLowerCase();
addThis();

status = 'small()';
actual = s.small();
expect = actual.toLowerCase();
addThis();

status = 'strike()';
actual = s.strike();
expect = actual.toLowerCase();
addThis();

status = 'sub()';
actual = s.sub();
expect = actual.toLowerCase();
addThis();

status = 'sup()';
actual = s.sup();
expect = actual.toLowerCase();
addThis();


//-------------------------------------------------------------------------------------------------
test();
//-------------------------------------------------------------------------------------------------


function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = actual;
  expectedvalues[UBound] = expect;
  UBound++;
}


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
 
  for (var i = 0; i < UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], getStatus(i));
  }

  exitFunc ('test');
}


function getStatus(i)
{
  return statprefix + statusitems[i];
}
