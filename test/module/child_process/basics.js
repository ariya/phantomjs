// Test basic child process functionality
//

var fs = require('fs');
var process = require('child_process');

var ECHO_SCRIPT = fs.join(TEST_DIR, 'lib', 'fixtures', 'echo.py');
var CAT_SCRIPT = fs.join(TEST_DIR, 'lib', 'fixtures', 'cat.py');


async_test(function(){
    var text = "hello";
    var p = process.spawn(PYTHON, [ECHO_SCRIPT, text]);
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
    var p = process.spawn(PYTHON, [CAT_SCRIPT]);
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
