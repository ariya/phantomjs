// This allows creating a new web page using the construct "new WebPage",
// which feels more natural than "phantom.createWebPage()".
window.WebPage = function() {
    var page = phantom.createWebPage();

    // deep copy
    page.settings = JSON.parse(JSON.stringify(phantom.defaultPageSettings));

    page.open = function () {
        if (arguments.length === 2) {
            this.loadStatusChanged.connect(arguments[1]);
            this.openUrl(arguments[0], 'get', this.settings);
            return;
        } else if (arguments.length === 3) {
            this.loadStatusChanged.connect(arguments[2]);
            this.openUrl(arguments[0], arguments[1], this.settings);
            return;
        } else if (arguments.length === 4) {
            this.loadStatusChanged.connect(arguments[3]);
            this.openUrl(arguments[0], {
                operation: arguments[1],
                data: arguments[2]
                }, this.settings);
            return;
        }
        throw "Wrong use of WebPage#open";
    };

    return page;
}
