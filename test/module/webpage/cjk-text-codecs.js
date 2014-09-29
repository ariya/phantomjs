var assert = require('../../assert');
var webpage = require('webpage');

function Text(codec, base64, reference) {
    this.codec = codec;
    this.base64 = base64;
    this.reference = reference;
    this.url = 'data:text/plain;charset=' + this.codec + ';base64,' + this.base64;
}

var texts = [
    new Text('Shift_JIS', 'g3SDQIOTg2eDgA==', 'ファントム'),
    new Text('EUC-JP', 'pdWloaXzpcil4A0K', 'ファントム'),
    new Text('ISO-2022-JP', 'GyRCJVUlISVzJUglYBsoQg0K', 'ファントム'),
    new Text('Big5', 'pNu2SA0K', '幻象'),
    new Text('GBK', 'u8PP8w0K', '幻象'),
    new Text('EUC-KR', 'yK+/tQ==', '환영'),
];

texts.forEach(function (text) {
    var page = webpage.create();
    page.open(text.url, function() {
        var decodedText = page.evaluate(function() {
            return document.querySelector('pre').innerText;
        });
        var regex = '^' + text.reference;
        assert.equal(decodedText.match(regex), text.reference);
    });
});

