
void wrapInFunction()
{

//! [0]
    // ...
    QWebPage *page = new QWebPage;
    // ...

    QWebInspector *inspector = new QWebInspector;
    inspector->setPage(page);
//! [0]

}

