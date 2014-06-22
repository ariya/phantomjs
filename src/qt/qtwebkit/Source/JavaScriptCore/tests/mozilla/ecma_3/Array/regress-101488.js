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
* Copyright (C) 1998 Netscape Communications Corporation.
* All Rights Reserved.
*
* Contributor(s): igor@icesoft.no, pschwartau@netscape.com
* Date: 24 September 2001
*
* SUMMARY: Try assigning arr.length = new Number(n)
* From correspondence with Igor Bukanov <igor@icesoft.no>
* See http://bugzilla.mozilla.org/show_bug.cgi?id=101488
*
* Without the "new" keyword, assigning arr.length = Number(n) worked.
* But with it, Rhino was giving an error "Inappropriate array length"
* and SpiderMonkey was exiting without giving any error or return value -
*
* Comments on the Rhino code by igor@icesoft.no:
*
* jsSet_length requires that the new length value should be an instance
* of Number. But according to Ecma 15.4.5.1, item 12-13, an error should
* be thrown only if ToUint32(length_value) != ToNumber(length_value)
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 101488;
var summary = 'Try assigning arr.length = new Number(n)';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var arr = [];


status = inSection(1);
arr = Array();
tryThis('arr.length = new Number(1);');
actual = arr.length;
expect = 1;
addThis();

status = inSection(2);
arr = Array(5);
tryThis('arr.length = new Number(1);');
actual = arr.length;
expect = 1;
addThis();

status = inSection(3);
arr = Array();
tryThis('arr.length = new Number(17);');
actual = arr.length;
expect = 17;
addThis();

status = inSection(4);
arr = Array(5);
tryThis('arr.length = new Number(17);');
actual = arr.length;
expect = 17;
addThis();


/*
 * Also try the above with the "new" keyword before Array().
 * Array() and new Array() should be equivalent, by ECMA 15.4.1.1
 */
status = inSection(5);
arr = new Array();
tryThis('arr.length = new Number(1);');
actual = arr.length;
expect = 1;
addThis();

status = inSection(6);
arr = new Array(5);
tryThis('arr.length = new Number(1);');
actual = arr.length;
expect = 1;
addThis();

arr = new Array();
tryThis('arr.length = new Number(17);');
actual = arr.length;
expect = 17;
addThis();

status = inSection(7);
arr = new Array(5);
tryThis('arr.length = new Number(17);');
actual = arr.length;
expect = 17;
addThis();



//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------



function tryThis(s)
{
  try 
  {
    eval(s);
  }
  catch(e)
  {
    // keep going
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

  for (var i=0; i<UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
