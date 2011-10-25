// This is a wrapper for the script to be executed.
// When remote debugging, there's no way to reload the page
// to be able to run the script being debugged, because there's
// no user-facing way to interact with phantomjs.
// This provides a function wrapper around the script in order to
// make the whole script callable from the console on the remote debugger.

var __run = function() {

%1

};