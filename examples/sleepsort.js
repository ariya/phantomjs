// sleepsort.js - Sort integers from the commandline in a very ridiculous way: leveraging timeouts :P

function sleepSort(array, callback) {
    var sortedCount = 0,
        i, len;
    for ( i = 0, len = array.length; i < len; ++i ) {
        setTimeout((function(j){
            return function() {
                console.log(array[j]);
                ++sortedCount;
                (len === sortedCount) && callback();
            };
        }(i)), array[i]);
    }
}

if ( phantom.args < 1 ) {
    console.log("Usage: phantomjs sleepsort.js PUT YOUR INTEGERS HERE SEPARATED BY SPACES");
    phantom.exit();
} else {
    sleepSort(phantom.args, function() {
        phantom.exit();
    });
}