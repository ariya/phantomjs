/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is JavaScript Engine testing utilities.
 *
 * The Initial Developer of the Original Code is
 * Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s): Mike Shaver
 *                 Bob Clary
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
//-----------------------------------------------------------------------------
var bug = 290592;
var summary = 'Array extras: forEach, indexOf, filter, map';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

// Utility functions

function identity(v, index, array)
{
  reportCompare(v, array[index], 'identity: check callback argument consistency');
  return v;
}  

function mutate(v, index, array)
{
  reportCompare(v, array[index], 'mutate: check callback argument consistency');
  if (index == 0)
  {
    array[1] = 'mutated';
    delete array[2];
    array.push('not visited');
  }
  return v;
}

function mutateForEach(v, index, array)
{
  reportCompare(v, array[index], 'mutateForEach: check callback argument consistency');
  if (index == 0)
  {
    array[1] = 'mutated';
    delete array[2];
    array.push('not visited');
  }
  actual += v + ',';
}

function makeUpperCase(v, index, array)
{
  reportCompare(v, array[index], 'makeUpperCase: check callback argument consistency');
  try
  {
    return v.toUpperCase();
  }
  catch(e)
  {
  }
  return v;
}


function concat(v, index, array)
{
  reportCompare(v, array[index], 'concat: check callback argument consistency');
  actual += v + ',';
}


function isUpperCase(v, index, array)
{
  reportCompare(v, array[index], 'isUpperCase: check callback argument consistency');
  try
  {
    return v == v.toUpperCase();
  }
  catch(e)
  {
  }
  return false;
}

function isString(v, index, array)
{
  reportCompare(v, array[index], 'isString: check callback argument consistency');
  return typeof v == 'string';
}


// callback object.
function ArrayCallback(state)
{
  this.state = state;
}

ArrayCallback.prototype.makeUpperCase = function (v, index, array)
{
  reportCompare(v, array[index], 'ArrayCallback.prototype.makeUpperCase: check callback argument consistency');
  try
  {
    return this.state ? v.toUpperCase() : v.toLowerCase();
  }
  catch(e)
  {
  }
  return v;
};

ArrayCallback.prototype.concat = function(v, index, array)
{
  reportCompare(v, array[index], 'ArrayCallback.prototype.concat: check callback argument consistency');
  actual += v + ',';
};

ArrayCallback.prototype.isUpperCase = function(v, index, array)
{
  reportCompare(v, array[index], 'ArrayCallback.prototype.isUpperCase: check callback argument consistency');
  try
  {
    return this.state ? true : (v == v.toUpperCase());
  }
  catch(e)
  {
  }
  return false;
};

ArrayCallback.prototype.isString = function(v, index, array)
{
  reportCompare(v, array[index], 'ArrayCallback.prototype.isString: check callback argument consistency');
  return this.state ? true : (typeof v == 'string');
};

function dumpError(e)
{
  var s = e.name + ': ' + e.message + 
    ' File: ' + e.fileName + 
    ', Line: ' + e.lineNumber + 
    ', Stack: ' + e.stack;
  return s;
}

var obj;
var strings = ['hello', 'Array', 'WORLD'];
var mixed   = [0, '0', 0];
var sparsestrings = new Array();
sparsestrings[2] = 'sparse';

if ('map' in Array.prototype)
{
// see http://developer-test.mozilla.org/docs/Core_JavaScript_1.5_Reference:Objects:Array:map

  // test Array.map

  // map has 1 required argument
  expect = 1;
  actual = Array.prototype.map.length;
  reportCompare(expect, actual, 'Array.prototype.map.length == 1');

  // throw TypeError if no callback function specified
  expect = 'TypeError';
  try
  {
    strings.map();
    actual = 'no error';
  }
  catch(e)
  {
    actual = e.name;
  }
  reportCompare(expect, actual, 'Array.map(undefined) throws TypeError');  

  try
  {
    // identity map
    expect = 'hello,Array,WORLD';
    actual = strings.map(identity).toString();
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'Array.map: identity');  


  try
  {
    expect = 'hello,mutated,';
    actual = strings.map(mutate).toString();
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'Array.map: mutate');  

  strings = ['hello', 'Array', 'WORLD'];

  try
  {
    // general map
    expect = 'HELLO,ARRAY,WORLD';
    actual = strings.map(makeUpperCase).toString();
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'Array.map: uppercase');  

  try
  {
    // pass object method as map callback
    expect = 'HELLO,ARRAY,WORLD';
    var obj = new ArrayCallback(true);
    actual  = strings.map(obj.makeUpperCase, obj).toString();
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'Array.map: uppercase with object callback');  

  try
  {
    expect = 'hello,array,world';
    obj = new ArrayCallback(false);
    actual = strings.map(obj.makeUpperCase, obj).toString();
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'Array.map: lowercase with object callback');  

  try
  {
    // map on sparse arrays
    expect = ',,SPARSE';
    actual = sparsestrings.map(makeUpperCase).toString();
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'Array.map: uppercase on sparse array');  
}

if ('forEach' in Array.prototype)
{
// see http://developer-test.mozilla.org/docs/Core_JavaScript_1.5_Reference:Objects:Array:forEach

  // test Array.forEach

  // forEach has 1 required argument
  expect = 1;
  actual = Array.prototype.forEach.length;
  reportCompare(expect, actual, 'Array.prototype.forEach.length == 1');

  // throw TypeError if no callback function specified
  expect = 'TypeError';
  try
  {
    strings.forEach();
    actual = 'no error';
  }
  catch(e)
  {
    actual = e.name;
  }
  reportCompare(expect, actual, 'Array.forEach(undefined) throws TypeError');  

  try
  {
    // general forEach
    expect = 'hello,Array,WORLD,';
    actual = '';
    strings.forEach(concat);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'Array.forEach');  

  try
  {
    expect = 'hello,mutated,';
    actual = '';
    strings.forEach(mutateForEach);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'Array.forEach: mutate');  

  strings = ['hello', 'Array', 'WORLD'];



  try
  {
    // pass object method as forEach callback
    expect = 'hello,Array,WORLD,';
    actual = '';
    obj = new ArrayCallback(true);
    strings.forEach(obj.concat, obj);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'Array.forEach with object callback 1');  

  try
  {
    expect = 'hello,Array,WORLD,';
    actual = '';
    obj = new ArrayCallback(false);
    strings.forEach(obj.concat, obj);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'Array.forEach with object callback 2');  

  try
  {
    // test forEach on sparse arrays
    // see https://bugzilla.mozilla.org/show_bug.cgi?id=311082
    expect = 'sparse,';
    actual = '';
    sparsestrings.forEach(concat);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'Array.forEach on sparse array');  
}

if ('filter' in Array.prototype)
{
// see http://developer-test.mozilla.org/docs/Core_JavaScript_1.5_Reference:Objects:Array:filter

  // test Array.filter

  // filter has 1 required argument
  expect = 1;
  actual = Array.prototype.filter.length;
  reportCompare(expect, actual, 'Array.prototype.filter.length == 1');

  // throw TypeError if no callback function specified
  expect = 'TypeError';
  try
  {
    strings.filter();
    actual = 'no error';
  }
  catch(e)
  {
    actual = e.name;
  }
  reportCompare(expect, actual, 'Array.filter(undefined) throws TypeError');  

  try
  {
    // test general filter
    expect = 'WORLD';
    actual = strings.filter(isUpperCase).toString();
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'Array.filter');

  try
  {
    expect = 'WORLD';
    obj = new ArrayCallback(false);
    actual = strings.filter(obj.isUpperCase, obj).toString();
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'Array.filter object callback 1');

  try
  {
    expect = 'hello,Array,WORLD';
    obj = new ArrayCallback(true);
    actual = strings.filter(obj.isUpperCase, obj).toString();
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'Array.filter object callback 2');
}

if ('every' in Array.prototype)
{
// see http://developer-test.mozilla.org/docs/Core_JavaScript_1.5_Reference:Objects:Array:every

  // test Array.every

  // every has 1 required argument

  expect = 1;
  actual = Array.prototype.every.length;
  reportCompare(expect, actual, 'Array.prototype.every.length == 1');

  // throw TypeError if no every callback function specified
  expect = 'TypeError';
  try
  {
    strings.every();
    actual = 'no error';
  }
  catch(e)
  {
    actual = e.name;
  }
  reportCompare(expect, actual, 'Array.every(undefined) throws TypeError');  

  // test general every

  try
  {
    expect = true;
    actual = strings.every(isString);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'strings: every element is a string');

  try
  {
    expect = false;
    actual = mixed.every(isString);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'mixed: every element is a string');

  try
  {
    // see https://bugzilla.mozilla.org/show_bug.cgi?id=311082
    expect = true;
    actual = sparsestrings.every(isString);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'sparsestrings: every element is a string');

  // pass object method as map callback

  obj = new ArrayCallback(false);

  try
  {
    expect = true;
    actual = strings.every(obj.isString, obj);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'strings: every element is a string, via object callback');

  try
  {
    expect = false;
    actual = mixed.every(obj.isString, obj);
  }
  catch(e)
  {
    actual = dumpError(e) ;
  }
  reportCompare(expect, actual, 'mixed: every element is a string, via object callback');

  try
  {
    // see https://bugzilla.mozilla.org/show_bug.cgi?id=311082
    expect = true;
    actual = sparsestrings.every(obj.isString, obj);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'sparsestrings: every element is a string, via object callback');

}

if ('some' in Array.prototype)
{
// see http://developer-test.mozilla.org/docs/Core_JavaScript_1.5_Reference:Objects:Array:some

  // test Array.some

  // some has 1 required argument

  expect = 1;
  actual = Array.prototype.some.length;
  reportCompare(expect, actual, 'Array.prototype.some.length == 1');

  // throw TypeError if no some callback function specified
  expect = 'TypeError';
  try
  {
    strings.some();
    actual = 'no error';
  }
  catch(e)
  {
    actual = e.name;
  }
  reportCompare(expect, actual, 'Array.some(undefined) throws TypeError');  

  // test general some

  try
  {
    expect = true;
    actual = strings.some(isString);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'strings: some element is a string');

  try
  {
    expect = true;
    actual = mixed.some(isString);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'mixed: some element is a string');

  try
  {
    expect = true;
    actual = sparsestrings.some(isString);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'sparsestrings: some element is a string');

  // pass object method as map callback

  obj = new ArrayCallback(false);

  try
  {
    expect = true;
    actual = strings.some(obj.isString, obj);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'strings: some element is a string, via object callback');

  try
  {
    expect = true;
    actual = mixed.some(obj.isString, obj);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'mixed: some element is a string, via object callback');

  try
  {
    expect = true;
    actual = sparsestrings.some(obj.isString, obj);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'sparsestrings: some element is a string, via object callback');

}

if ('indexOf' in Array.prototype)
{
// see http://developer-test.mozilla.org/docs/Core_JavaScript_1.5_Reference:Objects:Array:indexOf

  // test Array.indexOf

  // indexOf has 1 required argument

  expect = 1;
  actual = Array.prototype.indexOf.length;
  reportCompare(expect, actual, 'Array.prototype.indexOf.length == 1');

  // test general indexOf

  try
  {
    expect = -1;
    actual = mixed.indexOf('not found');
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'indexOf returns -1 if value not found');

  try
  {
    expect = 0;
    actual = mixed.indexOf(0);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'indexOf matches using strict equality');

  try
  {
    expect = 1;
    actual = mixed.indexOf('0');
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'indexOf matches using strict equality');

  try
  {
    expect = 2;
    actual = mixed.indexOf(0, 1);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'indexOf begins searching at fromIndex');
}

