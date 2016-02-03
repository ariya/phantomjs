// Test basic child process functionality
//

var ECHO_SCRIPT = 'echo.py';
var CAT_SCRIPT = 'cat.py';

setup(function(){
    var f;
    var fs = require('fs');

    // Platform-agnostic drop-in replacements for 'echo -n' and 'cat'.
    // Using sys.stdout.write rather than print here to make the
    // python code 2/3 agnostic.

    f = fs.open(ECHO_SCRIPT, 'w');
    f.writeLine('import sys');
    f.writeLine('');
    f.writeLine('sys.stdout.write(" ".join(sys.argv[1:]))');
    f.close();

    f = fs.open(CAT_SCRIPT, 'w');
    f.writeLine('import sys');
    f.writeLine('');
    f.writeLine('sys.stdout.write(sys.stdin.read())');
    f.close();
});

async_test(function(){
    var text = "hello";
    var process = require('child_process');
    var p = process.spawn("python", [ECHO_SCRIPT, text]);
    var out = '';
    p.stdout.on('data', function(data) { out += data; });
    p.on('exit', this.step_func_done(
                    function(exitCode) {
                        assert_equals(exitCode, 0);
                        assert_equals(out, text);
                    }));
}, "call a simple subprocess");

async_test(function(){
    var text = "hello";
    var process = require('child_process');
    var p = process.spawn("python", [CAT_SCRIPT]);
    var out = '';
    p.stdout.on('data', function(data) { out += data; });
    p.on('exit', this.step_func_done(
                    function(exitCode) {
                        assert_equals(exitCode, 0);
                        assert_equals(out, text);
                    }));
    p.stdin.write(text);
    p.stdin.close();
}, "call a subprocess that reads input");
