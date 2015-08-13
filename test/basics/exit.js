//! no-harness

var sys = require('system');
sys.stdout.write("1..1\nok 1 we are alive\n");
phantom.exit();
sys.stdout.write("not ok 2 control passed beyond phantom.exit # TODO");
