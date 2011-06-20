// Find pizza in New York using Google Local

var page = new WebPage();

page.open('http://www.google.com/m/local?site=local&q=pizza+in+new+york', function (status) {
    if (status !== 'success') {
        console.log('Unable to access network');
    } else {
        var results = page.evaluate(function() {
            var list = document.querySelectorAll('div.bf'), pizza = [], i;
            for (i = 0; i < list.length; i++) {
                pizza.push(list[i].innerText);
            }
            return pizza;
        });
        console.log(results.join('\n'));
    }
    phantom.exit();
});
