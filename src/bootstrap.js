// This allows creating a new web page using the construct "new WebPage",
// which feels more natural than "phantom.createWebPage()".
window.WebPage = function() {
    var page = phantom.createWebPage();

    // deep copy
    page.settings = JSON.parse(JSON.stringify(phantom.defaultPageSettings));

    page.open = function (url, callback) {
        this.loadStatusChanged.connect(callback);
        this.openUrl(url, this.settings);
    };

    return page;
}
