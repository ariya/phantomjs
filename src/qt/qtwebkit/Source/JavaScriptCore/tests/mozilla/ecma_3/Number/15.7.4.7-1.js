/*
* The contents of this file are subject to the Netscape Public
* License Version 1.1 (the "License"); you may not use this file
* except in compliance with the License. You may obtain a copy of
* the License at http://www.mozilla.org/NPL/
*
* Software distributed under the License is distributed on an
* "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either expressed
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
* Date: 2001-07-15
*
* SUMMARY: Testing Number.prototype.toPrecision(precision)
* See EMCA 262 Edition 3 Section 15.7.4.7
*
* Also see http://bugzilla.mozilla.org/show_bug.cgi?id=90551
*
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = '(none)';
var summary = 'Testing Number.prototype.toPrecision(precision)';
var cnIsRangeError = 'instanceof RangeError';
var cnNotRangeError = 'NOT instanceof RangeError';
var cnNoErrorCaught = 'NO ERROR CAUGHT...';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var testNum = 5.123456;


status = 'Section A of test: no error intended!';
actual = testNum.toPrecision(4);
expect = '5.123';
captureThis();


///////////////////////////    OOPS....    ///////////////////////////////
/*************************************************************************
 * 15.7.4.7 Number.prototype.toPrecision(precision)
 *
 * An implementation is permitted to extend the behaviour of toPrecision
 * for values of precision less than 1 or greater than 21. In this
 * case toPrecision would not necessarily throw RangeError for such values.

status = 'Section B of test: expect RangeError because precision < 1';
actual = catchError('testNum.toPrecision(0)');
expect = cnIsRangeError;
captureThis();

status = 'Section C of test: expect RangeError because precision < 1';
actual = catchError('testNum.toPrecision(-4)');
expect = cnIsRangeError;
captureThis();

status = 'Section D of test: expect RangeError because precision > 21 ';
actual = catchError('testNum.toPrecision(22)');
expect = cnIsRangeError;
captureThis();
*************************************************************************/



//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------


function captureThis()
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


function catchError(sEval)
{
  try {eval(sEval);}
  catch(e) {return isRangeError(e);}
  return cnNoErrorCaught;
}


function isRangeError(obj)
{
  if (obj instanceof RangeError)
    return cnIsRangeError;
  return cnNotRangeError;
}
