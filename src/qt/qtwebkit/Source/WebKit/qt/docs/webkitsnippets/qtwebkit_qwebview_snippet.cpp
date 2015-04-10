
void wrapInFunction()
{

//! [0]
    view->page()->history();
//! [0]


//! [1]
    view->page()->settings();
//! [1]


//! [2]
    view->triggerAction(QWebPage::Copy);
//! [2]


//! [3]
    view->page()->triggerPageAction(QWebPage::Stop);
//! [3]


//! [4]
    view->page()->triggerPageAction(QWebPage::GoBack);
//! [4]


//! [5]
    view->page()->triggerPageAction(QWebPage::GoForward);
//! [5]

}

