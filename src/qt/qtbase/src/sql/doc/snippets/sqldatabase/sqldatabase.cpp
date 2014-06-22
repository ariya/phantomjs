/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui>
#include <QtSql>

#include <iostream>

using namespace std;

QString tr(const char *text)
{
    return QApplication::translate(text, text);
}

void QSqlDatabase_snippets()
{
    {
//! [0]
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName("acidalia");
    db.setDatabaseName("customdb");
    db.setUserName("mojito");
    db.setPassword("J0a1m8");
    bool ok = db.open();
//! [0]
    Q_UNUSED(ok);
    }

    {
//! [1]
    QSqlDatabase db = QSqlDatabase::database();
//! [1]
    }
}

void QSqlField_snippets()
{
#if 0
    {
//! [2]
    QSqlField field("age", QVariant::Int);
    field.setValue(QPixmap());  // WRONG
//! [2]
    }
#endif

    {
//! [3]
    QSqlField field("age", QVariant::Int);
    field.setValue(QString("123"));  // casts QString to int
//! [3]
    }

    {
//! [4]
    QSqlQuery query;
//! [4] //! [5]
    QSqlRecord record = query.record();
//! [5] //! [6]
    QSqlField field = record.field("country");
//! [6]
    }
}

void doSomething(const QString &)
{
}

void QSqlQuery_snippets()
{
    {
    // typical loop
//! [7]
    QSqlQuery query("SELECT country FROM artist");
    while (query.next()) {
        QString country = query.value(0).toString();
        doSomething(country);
    }
//! [7]
    }

    {
    // field index lookup
//! [8]
    QSqlQuery query("SELECT * FROM artist");
    int fieldNo = query.record().indexOf("country");
    while (query.next()) {
        QString country = query.value(fieldNo).toString();
        doSomething(country);
    }
//! [8]
    }

    {
    // named with named
//! [9]
    QSqlQuery query;
    query.prepare("INSERT INTO person (id, forename, surname) "
                  "VALUES (:id, :forename, :surname)");
    query.bindValue(":id", 1001);
    query.bindValue(":forename", "Bart");
    query.bindValue(":surname", "Simpson");
    query.exec();
//! [9]
    }

    {
    // positional with named
//! [10]
    QSqlQuery query;
    query.prepare("INSERT INTO person (id, forename, surname) "
                  "VALUES (:id, :forename, :surname)");
    query.bindValue(0, 1001);
    query.bindValue(1, "Bart");
    query.bindValue(2, "Simpson");
    query.exec();
//! [10]
    }

    {
    // positional 1
//! [11]
    QSqlQuery query;
    query.prepare("INSERT INTO person (id, forename, surname) "
                  "VALUES (?, ?, ?)");
    query.bindValue(0, 1001);
    query.bindValue(1, "Bart");
    query.bindValue(2, "Simpson");
    query.exec();
//! [11]
    }

    {
    // positional 2
//! [12]
    QSqlQuery query;
    query.prepare("INSERT INTO person (id, forename, surname) "
                  "VALUES (?, ?, ?)");
    query.addBindValue(1001);
    query.addBindValue("Bart");
    query.addBindValue("Simpson");
    query.exec();
//! [12]
    }

    {
    // stored
//! [13]
    QSqlQuery query;
    query.prepare("CALL AsciiToInt(?, ?)");
    query.bindValue(0, "A");
    query.bindValue(1, 0, QSql::Out);
    query.exec();
    int i = query.boundValue(1).toInt(); // i is 65
//! [13]
    Q_UNUSED(i);
    }

    QSqlQuery query;

    {
    // examine with named binding
//! [14]
    QMapIterator<QString, QVariant> i(query.boundValues());
    while (i.hasNext()) {
        i.next();
        cout << i.key().toUtf8().data() << ": "
             << i.value().toString().toUtf8().data() << endl;
    }
//! [14]
    }

    {
    // examine with positional binding
//! [15]
    QList<QVariant> list = query.boundValues().values();
    for (int i = 0; i < list.size(); ++i)
        cout << i << ": " << list.at(i).toString().toUtf8().data() << endl;
//! [15]
    }
}

void QSqlQueryModel_snippets()
{
    {
//! [16]
    QSqlQueryModel *model = new QSqlQueryModel;
    model->setQuery("SELECT name, salary FROM employee");
    model->setHeaderData(0, Qt::Horizontal, tr("Name"));
    model->setHeaderData(1, Qt::Horizontal, tr("Salary"));

//! [17]
    QTableView *view = new QTableView;
//! [17] //! [18]
    view->setModel(model);
//! [18] //! [19]
    view->show();
//! [16] //! [19] //! [20]
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
//! [20]
    }

//! [21]
    QSqlQueryModel model;
    model.setQuery("SELECT * FROM employee");
    int salary = model.record(4).value("salary").toInt();
//! [21]
    Q_UNUSED(salary);

    {
//! [22]
    int salary = model.data(model.index(4, 2)).toInt();
//! [22]
    Q_UNUSED(salary);
    }

    for (int row = 0; row < model.rowCount(); ++row) {
        for (int col = 0; col < model.columnCount(); ++col) {
            qDebug() << model.data(model.index(row, col));
        }
    }
}

class MyModel : public QSqlQueryModel
{
public:
    QVariant data(const QModelIndex &item, int role) const;

    int m_specialColumnNo;
};

//! [23]
QVariant MyModel::data(const QModelIndex &item, int role) const
{
    if (item.column() == m_specialColumnNo) {
        // handle column separately
    }
    return QSqlQueryModel::data(item, role);
}
//! [23]

void QSqlTableModel_snippets()
{
//! [24]
    QSqlTableModel *model = new QSqlTableModel(parentObject, database);
    model->setTable("employee");
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model->select();
    model->setHeaderData(0, Qt::Horizontal, tr("Name"));
    model->setHeaderData(1, Qt::Horizontal, tr("Salary"));

    QTableView *view = new QTableView;
    view->setModel(model);
    view->hideColumn(0); // don't show the ID
    view->show();
//! [24]

    {
//! [25]
    QSqlTableModel model;
    model.setTable("employee");
    QString name = model.record(4).value("name").toString();
//! [25]
    }
}

void sql_intro_snippets()
{
    {
//! [26]
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("bigblue");
    db.setDatabaseName("flightdb");
    db.setUserName("acarlson");
    db.setPassword("1uTbSbAs");
    bool ok = db.open();
//! [26]
    Q_UNUSED(ok);
    }

    {
//! [27]
    QSqlDatabase firstDB = QSqlDatabase::addDatabase("QMYSQL", "first");
    QSqlDatabase secondDB = QSqlDatabase::addDatabase("QMYSQL", "second");
//! [27]
    }

    {
//! [28]
    QSqlDatabase defaultDB = QSqlDatabase::database();
//! [28] //! [29]
    QSqlDatabase firstDB = QSqlDatabase::database("first");
//! [29] //! [30]
    QSqlDatabase secondDB = QSqlDatabase::database("second");
//! [30]
    }

    {
    // SELECT1
//! [31]
    QSqlQuery query;
    query.exec("SELECT name, salary FROM employee WHERE salary > 50000");
//! [31]

//! [32]
    while (query.next()) {
        QString name = query.value(0).toString();
        int salary = query.value(1).toInt();
        qDebug() << name << salary;
    }
//! [32]
    }

    {
    // FEATURE
//! [33]
    QSqlQuery query;
    int numRows;
    query.exec("SELECT name, salary FROM employee WHERE salary > 50000");

    QSqlDatabase defaultDB = QSqlDatabase::database();
    if (defaultDB.driver()->hasFeature(QSqlDriver::QuerySize)) {
        numRows = query.size();
    } else {
        // this can be very slow
        query.last();
        numRows = query.at() + 1;
    }
//! [33]
    }

    {
    // INSERT1
//! [34]
    QSqlQuery query;
    query.exec("INSERT INTO employee (id, name, salary) "
               "VALUES (1001, 'Thad Beaumont', 65000)");
//! [34]
    }

    {
    // NAMED BINDING
//! [35]
    QSqlQuery query;
    query.prepare("INSERT INTO employee (id, name, salary) "
                  "VALUES (:id, :name, :salary)");
    query.bindValue(":id", 1001);
    query.bindValue(":name", "Thad Beaumont");
    query.bindValue(":salary", 65000);
    query.exec();
//! [35]
    }

    {
    // POSITIONAL BINDING
//! [36]
    QSqlQuery query;
    query.prepare("INSERT INTO employee (id, name, salary) "
                  "VALUES (?, ?, ?)");
    query.addBindValue(1001);
    query.addBindValue("Thad Beaumont");
    query.addBindValue(65000);
    query.exec();
//! [36]
    }

    {
    // UPDATE1
//! [37]
    QSqlQuery query;
    query.exec("UPDATE employee SET salary = 70000 WHERE id = 1003");
//! [37]
    }

    {
    // DELETE1
//! [38]
    QSqlQuery query;
    query.exec("DELETE FROM employee WHERE id = 1007");
//! [38]
    }

    {
    // TRANSACTION
//! [39]
    QSqlDatabase::database().transaction();
    QSqlQuery query;
    query.exec("SELECT id FROM employee WHERE name = 'Torild Halvorsen'");
    if (query.next()) {
        int employeeId = query.value(0).toInt();
        query.exec("INSERT INTO project (id, name, ownerid) "
                   "VALUES (201, 'Manhattan Project', "
                   + QString::number(employeeId) + ')');
    }
    QSqlDatabase::database().commit();
//! [39]
    }

    {
    // SQLQUERYMODEL1
//! [40]
    QSqlQueryModel model;
    model.setQuery("SELECT * FROM employee");

    for (int i = 0; i < model.rowCount(); ++i) {
        int id = model.record(i).value("id").toInt();
        QString name = model.record(i).value("name").toString();
        qDebug() << id << name;
    }
//! [40]
    }

    {
    // SQLTABLEMODEL1
//! [41]
    QSqlTableModel model;
    model.setTable("employee");
    model.setFilter("salary > 50000");
    model.setSort(2, Qt::DescendingOrder);
    model.select();

    for (int i = 0; i < model.rowCount(); ++i) {
        QString name = model.record(i).value("name").toString();
        int salary = model.record(i).value("salary").toInt();
        qDebug() << name << salary;
    }
//! [41]
    }

    {
    // SQLTABLEMODEL2
    QSqlTableModel model;
    model.setTable("employee");

//! [42]
    for (int i = 0; i < model.rowCount(); ++i) {
        QSqlRecord record = model.record(i);
        double salary = record.value("salary").toInt();
        salary *= 1.1;
        record.setValue("salary", salary);
        model.setRecord(i, record);
    }
    model.submitAll();
//! [42]

    // SQLTABLEMODEL3
    int row = 1;
    int column = 2;
//! [43]
    model.setData(model.index(row, column), 75000);
    model.submitAll();
//! [43]

    // SQLTABLEMODEL4
//! [44]
    model.insertRows(row, 1);
    model.setData(model.index(row, 0), 1013);
    model.setData(model.index(row, 1), "Peter Gordon");
    model.setData(model.index(row, 2), 68500);
    model.submitAll();
//! [44]

//! [45]
    model.removeRows(row, 5);
//! [45] //! [46]
    model.submitAll();
//! [46]
    }
}

//! [47]
class XyzResult : public QSqlResult
{
public:
    XyzResult(const QSqlDriver *driver)
        : QSqlResult(driver) {}
    ~XyzResult() {}

protected:
    QVariant data(int /* index */) { return QVariant(); }
    bool isNull(int /* index */) { return false; }
    bool reset(const QString & /* query */) { return false; }
    bool fetch(int /* index */) { return false; }
    bool fetchFirst() { return false; }
    bool fetchLast() { return false; }
    int size() { return 0; }
    int numRowsAffected() { return 0; }
    QSqlRecord record() const { return QSqlRecord(); }
};
//! [47]

//! [48]
class XyzDriver : public QSqlDriver
{
public:
    XyzDriver() {}
    ~XyzDriver() {}

    bool hasFeature(DriverFeature /* feature */) const { return false; }
    bool open(const QString & /* db */, const QString & /* user */,
              const QString & /* password */, const QString & /* host */,
              int /* port */, const QString & /* options */)
        { return false; }
    void close() {}
    QSqlResult *createResult() const { return new XyzResult(this); }
};
//! [48]

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QSqlDatabase_snippets();
    QSqlField_snippets();
    QSqlQuery_snippets();
    QSqlQueryModel_snippets();
    QSqlTableModel_snippets();

    XyzDriver driver;
    XyzResult result(&driver);
}
