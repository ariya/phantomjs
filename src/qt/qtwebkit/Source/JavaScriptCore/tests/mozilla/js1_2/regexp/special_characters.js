/* The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, released March
 * 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 * 
 */
/**
	Filename:     special_characters.js
	Description:  'Tests regular expressions containing special characters'

	Author:       Nick Lerissa
	Date:         March 10, 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE   = 'RegExp: special_charaters';

	writeHeaderToLog('Executing script: special_characters.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

    // testing backslash '\'
	testcases[count++] = new TestCase ( SECTION, "'^abcdefghi'.match(/\^abc/)", String(["^abc"]), String('^abcdefghi'.match(/\^abc/)));

	// testing beginning of line '^'
	testcases[count++] = new TestCase ( SECTION, "'abcdefghi'.match(/^abc/)", String(["abc"]), String('abcdefghi'.match(/^abc/)));

	// testing end of line '$'
	testcases[count++] = new TestCase ( SECTION, "'abcdefghi'.match(/fghi$/)", String(["ghi"]), String('abcdefghi'.match(/ghi$/)));

	// testing repeat '*'
	testcases[count++] = new TestCase ( SECTION, "'eeeefghi'.match(/e*/)", String(["eeee"]), String('eeeefghi'.match(/e*/)));

	// testing repeat 1 or more times '+'
	testcases[count++] = new TestCase ( SECTION, "'abcdeeeefghi'.match(/e+/)", String(["eeee"]), String('abcdeeeefghi'.match(/e+/)));

	// testing repeat 0 or 1 time '?'
	testcases[count++] = new TestCase ( SECTION, "'abcdefghi'.match(/abc?de/)", String(["abcde"]), String('abcdefghi'.match(/abc?de/)));

	// testing any character '.'
	testcases[count++] = new TestCase ( SECTION, "'abcdefghi'.match(/c.e/)", String(["cde"]), String('abcdefghi'.match(/c.e/)));

	// testing remembering ()
	testcases[count++] = new TestCase ( SECTION, "'abcewirjskjdabciewjsdf'.match(/(abc).+\\1'/)",
	                                    String(["abcewirjskjdabc","abc"]), String('abcewirjskjdabciewjsdf'.match(/(abc).+\1/)));

	// testing or match '|'
	testcases[count++] = new TestCase ( SECTION, "'abcdefghi'.match(/xyz|def/)", String(["def"]), String('abcdefghi'.match(/xyz|def/)));

	// testing repeat n {n}
	testcases[count++] = new TestCase ( SECTION, "'abcdeeeefghi'.match(/e{3}/)", String(["eee"]), String('abcdeeeefghi'.match(/e{3}/)));

	// testing min repeat n {n,}
	testcases[count++] = new TestCase ( SECTION, "'abcdeeeefghi'.match(/e{3,}/)", String(["eeee"]), String('abcdeeeefghi'.match(/e{3,}/)));

	// testing min/max repeat {min, max}
	testcases[count++] = new TestCase ( SECTION, "'abcdeeeefghi'.match(/e{2,8}/)", String(["eeee"]), String('abcdeeeefghi'.match(/e{2,8}/)));

	// testing any in set [abc...]
	testcases[count++] = new TestCase ( SECTION, "'abcdefghi'.match(/cd[xey]fgh/)", String(["cdefgh"]), String('abcdefghi'.match(/cd[xey]fgh/)));

	// testing any in set [a-z]
	testcases[count++] = new TestCase ( SECTION, "'netscape inc'.match(/t[r-v]ca/)", String(["tsca"]), String('netscape inc'.match(/t[r-v]ca/)));

	// testing any not in set [^abc...]
	testcases[count++] = new TestCase ( SECTION, "'abcdefghi'.match(/cd[^xy]fgh/)", String(["cdefgh"]), String('abcdefghi'.match(/cd[^xy]fgh/)));

	// testing any not in set [^a-z]
	testcases[count++] = new TestCase ( SECTION, "'netscape inc'.match(/t[^a-c]ca/)", String(["tsca"]), String('netscape inc'.match(/t[^a-c]ca/)));

	// testing backspace [\b]
	testcases[count++] = new TestCase ( SECTION, "'this is b\ba test'.match(/is b[\b]a test/)",
	                                    String(["is b\ba test"]), String('this is b\ba test'.match(/is b[\b]a test/)));

	// testing word boundary \b
	testcases[count++] = new TestCase ( SECTION, "'today is now - day is not now'.match(/\bday.*now/)",
	                                    String(["day is not now"]), String('today is now - day is not now'.match(/\bday.*now/)));

	// control characters???

	// testing any digit \d
	testcases[count++] = new TestCase ( SECTION, "'a dog - 1 dog'.match(/\d dog/)", String(["1 dog"]), String('a dog - 1 dog'.match(/\d dog/)));

	// testing any non digit \d
	testcases[count++] = new TestCase ( SECTION, "'a dog - 1 dog'.match(/\D dog/)", String(["a dog"]), String('a dog - 1 dog'.match(/\D dog/)));

	// testing form feed '\f'
	testcases[count++] = new TestCase ( SECTION, "'a b a\fb'.match(/a\fb/)", String(["a\fb"]), String('a b a\fb'.match(/a\fb/)));

	// testing line feed '\n'
	testcases[count++] = new TestCase ( SECTION, "'a b a\nb'.match(/a\nb/)", String(["a\nb"]), String('a b a\nb'.match(/a\nb/)));

	// testing carriage return '\r'
	testcases[count++] = new TestCase ( SECTION, "'a b a\rb'.match(/a\rb/)", String(["a\rb"]), String('a b a\rb'.match(/a\rb/)));

	// testing whitespace '\s'
	testcases[count++] = new TestCase ( SECTION, "'xa\f\n\r\t\vbz'.match(/a\s+b/)", String(["a\f\n\r\t\vb"]), String('xa\f\n\r\t\vbz'.match(/a\s+b/)));

	// testing non whitespace '\S'
	testcases[count++] = new TestCase ( SECTION, "'a\tb a b a-b'.match(/a\Sb/)", String(["a-b"]), String('a\tb a b a-b'.match(/a\Sb/)));

	// testing tab '\t'
	testcases[count++] = new TestCase ( SECTION, "'a\t\tb a  b'.match(/a\t{2}/)", String(["a\t\t"]), String('a\t\tb a  b'.match(/a\t{2}/)));

	// testing vertical tab '\v'
	testcases[count++] = new TestCase ( SECTION, "'a\v\vb a  b'.match(/a\v{2}/)", String(["a\v\v"]), String('a\v\vb a  b'.match(/a\v{2}/)));

	// testing alphnumeric characters '\w'
	testcases[count++] = new TestCase ( SECTION, "'%AZaz09_$'.match(/\w+/)", String(["AZaz09_"]), String('%AZaz09_$'.match(/\w+/)));

	// testing non alphnumeric characters '\W'
	testcases[count++] = new TestCase ( SECTION, "'azx$%#@*4534'.match(/\W+/)", String(["$%#@*"]), String('azx$%#@*4534'.match(/\W+/)));

	// testing back references '\<number>'
	testcases[count++] = new TestCase ( SECTION, "'test'.match(/(t)es\\1/)", String(["test","t"]), String('test'.match(/(t)es\1/)));

	// testing hex excaping with '\'
	testcases[count++] = new TestCase ( SECTION, "'abcdef'.match(/\x63\x64/)", String(["cd"]), String('abcdef'.match(/\x63\x64/)));

	// testing oct excaping with '\'
	testcases[count++] = new TestCase ( SECTION, "'abcdef'.match(/\\143\\144/)", String(["cd"]), String('abcdef'.match(/\143\144/)));

	function test()
	{
	   for ( tc=0; tc < testcases.length; tc++ ) {
	        testcases[tc].passed = writeTestCaseResult(
	        testcases[tc].expect,
	        testcases[tc].actual,
	        testcases[tc].description +" = "+
	        testcases[tc].actual );
	        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
	   }
	   stopTest();
	   return ( testcases );
	}

	test();

