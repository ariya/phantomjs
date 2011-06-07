// test the "page.click(QUERY SELECTOR)" api
// NOTE: This test will need to be updated if Google changes it's DOM :-P

var page = new WebPage();

page.open("http://www.google.com/", function(status) {
   if ( status === "success" ) {
       // render before doing anything
       page.render("click_around_google-before.png");
       
       // click on the Google 'settings bolt'
       page.click("#gbg5");
       page.render("click_around_google-clicked_on_gbg5.png");
       
       // click on 'more'
       page.click("#gbztm");
       page.render("click_around_google-clicked_on_gbztm.png");
   }
   phantom.exit();
});
