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
 * Contributor(s):
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
var bug = 304828;
var summary = 'Array Generic Methods';
var actual = '';
var expect = '';
printBugNumber (bug);
printStatus (summary);

var value;

// use Array methods on a String
// join
value  = '123';
expect = '1,2,3';
try
{
  actual = Array.prototype.join.call(value);
}
catch(e)
{
  actual = e + '';
}
reportCompare(expect, actual, summary + ': join');

// reverse
value  = '123';
expect = 'TypeError: Attempted to assign to readonly property.';
try
{
  actual = Array.prototype.reverse.call(value) + '';
}
catch(e)
{
  actual = e + '';
}
reportCompare(expect, actual, summary + ': reverse');

// sort
value  = 'cba';
expect = 'TypeError: Attempted to assign to readonly property.';
try
{
  actual = Array.prototype.sort.call(value) + '';
}
catch(e)
{
  actual = e + '';
}
reportCompare(expect, actual, summary + ': sort');

// push
value  = 'abc';
expect = 6;
try
{
  actual = Array.prototype.push.call(value, 'd', 'e', 'f');
}
catch(e)
{
  actual = e + '';
}
reportCompare(expect, actual, summary + ': push');
reportCompare('abc', value, summary + ': push');

// pop
value  = 'abc';
expect = 'TypeError: Unable to delete property.';
try
{
  actual = Array.prototype.pop.call(value);
}
catch(e)
{
  actual = e + '';
}
reportCompare(expect, actual, summary + ': pop');
reportCompare('abc', value, summary + ': pop');

// unshift
value  = 'def';
expect = "TypeError: Attempted to assign to readonly property.";
try
{
  actual = Array.prototype.unshift.call(value, 'a', 'b', 'c');
}
catch(e)
{
  actual = e + '';
}
reportCompare(expect, actual, summary + ': unshift');
reportCompare('def', value, summary + ': unshift');

// shift
value  = 'abc';
expect = 'TypeError: Attempted to assign to readonly property.';
try
{
  actual = Array.prototype.shift.call(value);
}
catch(e)
{
  actual = e + '';
}
reportCompare(expect, actual, summary + ': shift');
reportCompare('abc', value, summary + ': shift');

// splice
value  = 'abc';
expect = 'TypeError: Attempted to assign to readonly property.';
try
{
  actual = Array.prototype.splice.call(value, 1, 1) + '';
}
catch(e)
{
  actual = e + '';
}
reportCompare(expect, actual, summary + ': splice');

// concat
value  = 'abc';
expect = 'abc,d,e,f';
try
{
  actual = Array.prototype.concat.call(value, 'd', 'e', 'f') + '';
}
catch(e)
{
  actual = e + '';
}
reportCompare(expect, actual, summary + ': concat');

// slice
value  = 'abc';
expect = 'b';
try
{
  actual = Array.prototype.slice.call(value, 1, 2) + '';
}
catch(e)
{
  actual = e + '';
}
reportCompare(expect, actual, summary + ': slice');

// indexOf
value  = 'abc';
expect = 1;
try
{
  actual = Array.prototype.indexOf.call(value, 'b');
}
catch(e)
{
  actual = e + '';
}
reportCompare(expect, actual, summary + ': indexOf');

// lastIndexOf
value  = 'abcabc';
expect = 4;
try
{
  actual = Array.prototype.lastIndexOf.call(value, 'b');
}
catch(e)
{
  actual = e + '';
}
reportCompare(expect, actual, summary + ': lastIndexOf');

// forEach
value  = 'abc';
expect = 'ABC';
actual = '';
try
{
  Array.prototype.forEach.call(value, 
                               function (v, index, array) 
  {actual += array[index].toUpperCase();});
}
catch(e)
{
  actual = e + '';
}
reportCompare(expect, actual, summary + ': forEach');

// map
value  = 'abc';
expect = 'A,B,C';
try
{
  actual = Array.prototype.map.call(value, 
                                    function (v, index, array) 
    {return v.toUpperCase();}) + '';
}
catch(e)
{
  actual = e + '';
}
reportCompare(expect, actual, summary + ': map');

// filter
value  = '1234567890';
expect = '2,4,6,8,0';
try
{
  actual = Array.prototype.filter.call(value, 
                                        function (v, index, array) 
    {return array[index] % 2 == 0;}) + '';
}
catch(e)
{
  actual = e + '';
}
reportCompare(expect, actual, summary + ': filter');

// every
value  = '1234567890';
expect = false;
try
{
  actual = Array.prototype.every.call(value, 
                                        function (v, index, array) 
    {return array[index] % 2 == 0;});
}
catch(e)
{
  actual = e + '';
}
reportCompare(expect, actual, summary + ': every');



