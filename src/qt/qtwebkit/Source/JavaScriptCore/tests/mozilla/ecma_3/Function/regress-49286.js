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
* Contributors: jlaprise@delanotech.com,pschwartau@netscape.com
* Date: 2001-07-10
*
* SUMMARY:  Invoking try...catch through Function.call
* See  http://bugzilla.mozilla.org/show_bug.cgi?id=49286
*
* 1) Define a function with a try...catch block in it
* 2) Invoke the function via the call method of Function
* 3) Pass bad syntax to the try...catch block
* 4) We should catch the error!
*/
//-------------------------------------------------------------------------------------------------
var UBound = 0;
var bug = 49286;
var summary = 'Invoking try...catch through Function.call';
var cnErrorCaught = 'Error caught';
var cnErrorNotCaught = 'Error NOT caught';
var cnGoodSyntax = '1==2';
var cnBadSyntax = '1=2';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


var obj = new testObject();

status = 'Section A of test: direct call of f';
actual = f.call(obj);
expect = cnErrorCaught;
addThis();

status = 'Section B of test: indirect call of f';
actual = g.call(obj);
expect = cnErrorCaught;
addThis();



//-----------------------------------------
test();
//-----------------------------------------


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


// An object storing bad syntax as a property -
function testObject()
{
  this.badSyntax = cnBadSyntax;
  this.goodSyntax = cnGoodSyntax;
}


// A function wrapping a try...catch block
function f()
{
  try
  {
    eval(this.badSyntax);
  }
  catch(e)
  {
    return cnErrorCaught;
  }
  return cnErrorNotCaught;
}


// A function wrapping a call to f -
function g()
{
  return f.call(this);
}


function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = actual;
  expectedvalues[UBound] = expect;
  UBound++;
}
