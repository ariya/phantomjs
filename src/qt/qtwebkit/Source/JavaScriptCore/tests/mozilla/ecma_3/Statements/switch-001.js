/*
* The contents of this file are subject to the Netscape Public
* License Version 1.1 (the "License"); you may not use this file
* except in compliance with the License. You may obtain a copy of
* the License at http://www.mozilla.org/NPL/
*
* Software distributed under the License is distributed on an "AS IS" 
* basis, WITHOUT WARRANTY OF ANY KIND, either expressed
* or implied. See the License for the specific language governing
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
* Date: 07 May 2001
*
* SUMMARY: Testing the switch statement
*
* See ECMA3  Section 12.11,  "The switch Statement"
*/
//-------------------------------------------------------------------------------------------------
var UBound = 0;
var bug = '(none)';
var summary = 'Testing the switch statement';
var cnMatch = 'Match';
var cnNoMatch = 'NoMatch';
var status = '';
var statusitems = [ ];
var actual = '';
var actualvalues = [ ];
var expect= '';
var expectedvalues = [ ];


status = 'Section A of test';
actual = match(17, f(fInverse(17)), f, fInverse);
expect = cnMatch;
addThis();

status = 'Section B of test';
actual = match(17, 18, f, fInverse);
expect = cnNoMatch;
addThis();

status = 'Section C of test';
actual = match(1, 1, Math.exp, Math.log);
expect = cnMatch;
addThis();

status = 'Section D of test';
actual = match(1, 2, Math.exp, Math.log);
expect = cnNoMatch;
addThis();

status = 'Section E of test';
actual = match(1, 1, Math.sin, Math.cos);
expect = cnNoMatch;
addThis();



//---------------------------------------------------------------------------------
test();
//---------------------------------------------------------------------------------



/*
 * If F,G are inverse functions and x==y, this should return cnMatch -
 */
function match(x, y, F, G)
{
  switch (x)
  {
    case F(G(y)):
      return cnMatch;

    default:
      return cnNoMatch;
  }
}


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
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}


function f(m)
{
  return 2*(m+1);
}


function fInverse(n)
{
  return (n-2)/2;
}
