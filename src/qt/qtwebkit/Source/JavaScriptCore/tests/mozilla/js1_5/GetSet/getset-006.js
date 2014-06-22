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
* Date: 14 April 2001
*
* SUMMARY: Testing  obj.__lookupGetter__(), obj.__lookupSetter__()
* See http://bugzilla.mozilla.org/show_bug.cgi?id=71992 
*
* Brendan: "I see no need to provide more than the minimum: 
* o.__lookupGetter__('p') returns the getter function for o.p, 
* or undefined if o.p has no getter.  Users can wrap and layer."
*/
//-------------------------------------------------------------------------------------------------
var UBound = 0;
var bug = 71992;
var summary = 'Testing  obj.__lookupGetter__(), obj.__lookupSetter__()';
var statprefix = 'Status: ';
var status = '';
var statusitems = [ ];
var actual = '';
var actualvalues = [ ];
var expect= '';
var expectedvalues = [ ];
var cnName = 'name';
var cnColor = 'color';
var cnNonExistingProp = 'ASDF_#_$%';
var cnDEFAULT = 'default name';
var cnFRED = 'Fred';
var cnRED = 'red';
var obj = {};
var obj2 = {};
var s;

 
// The only setter and getter functions we'll use in the three sections below -
var cnNameSetter = function(newValue) {this._name=newValue; this.nameSETS++;};
var cnNameGetter = function() {this.nameGETS++; return this._name;};



// SECTION1: define getter/setter directly on an object (not its prototype)
obj = new Object();
obj.nameSETS = 0;
obj.nameGETS = 0;
obj.__defineSetter__(cnName, cnNameSetter);
obj.__defineGetter__(cnName, cnNameGetter);
obj.name = cnFRED;
obj.color = cnRED;

status ='In SECTION1 of test; looking up extant getter/setter';
actual = [obj.__lookupSetter__(cnName), obj.__lookupGetter__(cnName)];
expect = [cnNameSetter, cnNameGetter];
addThis();

status = 'In SECTION1 of test; looking up nonexistent getter/setter';
actual = [obj.__lookupSetter__(cnColor), obj.__lookupGetter__(cnColor)];
expect = [undefined, undefined];
addThis();

status = 'In SECTION1 of test; looking up getter/setter on nonexistent property';
actual = [obj.__lookupSetter__(cnNonExistingProp), obj.__lookupGetter__(cnNonExistingProp)];
expect = [undefined, undefined];
addThis();



// SECTION2: define getter/setter in Object.prototype
Object.prototype.nameSETS = 0;
Object.prototype.nameGETS = 0;
Object.prototype.__defineSetter__(cnName, cnNameSetter);
Object.prototype.__defineGetter__(cnName, cnNameGetter);

obj = new Object();
obj.name = cnFRED;
obj.color = cnRED;

status = 'In SECTION2 of test looking up extant getter/setter';
actual = [obj.__lookupSetter__(cnName), obj.__lookupGetter__(cnName)];
expect = [cnNameSetter, cnNameGetter];
addThis();

status = 'In SECTION2 of test; looking up nonexistent getter/setter';
actual = [obj.__lookupSetter__(cnColor), obj.__lookupGetter__(cnColor)];
expect = [undefined, undefined];
addThis();

status = 'In SECTION2 of test; looking up getter/setter on nonexistent property';
actual = [obj.__lookupSetter__(cnNonExistingProp), obj.__lookupGetter__(cnNonExistingProp)];
expect = [undefined, undefined];
addThis();



// SECTION 3: define getter/setter in prototype of user-defined constructor
function TestObject()
{
}
TestObject.prototype.nameSETS = 0;
TestObject.prototype.nameGETS = 0;
TestObject.prototype.__defineSetter__(cnName, cnNameSetter);
TestObject.prototype.__defineGetter__(cnName, cnNameGetter);
TestObject.prototype.name = cnDEFAULT;

obj = new TestObject();
obj.name = cnFRED;
obj.color = cnRED;

status = 'In SECTION3 of test looking up extant getter/setter';
actual = [obj.__lookupSetter__(cnName), obj.__lookupGetter__(cnName)];
expect = [cnNameSetter, cnNameGetter];
addThis();

status = 'In SECTION3 of test; looking up non-existent getter/setter';
actual = [obj.__lookupSetter__(cnColor), obj.__lookupGetter__(cnColor)];
expect = [undefined, undefined];
addThis();

status = 'In SECTION3 of test; looking up getter/setter on nonexistent property';
actual = [obj.__lookupSetter__(cnNonExistingProp), obj.__lookupGetter__(cnNonExistingProp)];
expect = [undefined, undefined];
addThis();



//---------------------------------------------------------------------------------
test();
//---------------------------------------------------------------------------------


function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = actual.toString();
  expectedvalues[UBound] = expect.toString();
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
