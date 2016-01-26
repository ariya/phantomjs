// Test basic child process functionality
//

async_test(function(){
    var text = "hello";
    var process = require('child_process');
    var p = process.spawn("echo", ["-n", text]);
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
    var p = process.spawn("cat", []);
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
