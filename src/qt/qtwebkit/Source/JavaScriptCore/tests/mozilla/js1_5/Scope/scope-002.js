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
* Contributor(s): pschwartau@netscape.com, crock@veilnetworks.com
* Date: 2001-07-02
*
* SUMMARY:  Testing visibility of outer function from inner function. 
*
*/
//-------------------------------------------------------------------------------------------------
var UBound = 0;
var bug = '(none)';
var summary = 'Testing visibility of outer function from inner function';
var cnCousin = 'Fred';
var cnColor = 'red';
var cnMake = 'Toyota';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


// TEST 1
function Outer()
{

  function inner()
  {
    Outer.cousin = cnCousin;
    return Outer.cousin;
  }

  status = 'Section 1 of test';
  actual = inner();
  expect = cnCousin;
  addThis();
}


Outer();
status = 'Section 2 of test';
actual = Outer.cousin;
expect = cnCousin;
addThis();



// TEST 2
function Car(make)
{
  this.make = make;
  Car.prototype.paint = paint;

  function paint()
  {
    Car.color = cnColor;
    Car.prototype.color = Car.color;
  }
}


var myCar = new Car(cnMake);
status = 'Section 3 of test';
actual = myCar.make;
expect = cnMake;
addThis();


myCar.paint();
status = 'Section 4 of test';
actual = myCar.color;
expect = cnColor;
addThis();



//--------------------------------------------------
test();
//--------------------------------------------------



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
