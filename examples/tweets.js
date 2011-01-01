// Get twitter status from someone

if (phantom.storage.length === 0) {
    phantom.storage = 'tweets';
    phantom.open('http://mobile.twitter.com/mr3bc');
} else {
    var list = document.querySelectorAll('span.status');
    for (var i = 0; i < list.length; ++i) {
        console.log((i + 1) + ': ' + list[i].innerHTML.replace(/<.*?>/g, ''));
    }
    phantom.exit();
}
