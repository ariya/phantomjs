// Upload an image to imagebin.org

var page = require('webpage').create(),
	fname;

if (phantom.args.length !== 1) {
    console.log('Usage: imagebin.js filename');
    phantom.exit();
} else {
    fname = phantom.args[0];
    page.open("http://imagebin.org/index.php?page=add", function () {
        page.uploadFile('input[name=image]', fname);
        page.evaluate(function () {
            document.querySelector('input[name=nickname]').value = 'phantom';
            document.querySelector('input[name=disclaimer_agree]').click()
            document.querySelector('form').submit();
        });
        window.setTimeout(function () {
            phantom.exit();
        }, 3000);
    });
}
