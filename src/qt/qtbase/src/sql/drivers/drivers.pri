contains(sql-drivers, all):sql-driver += psql mysql odbc oci tds db2 sqlite ibase

contains(sql-drivers, psql):include($$PWD/psql/qsql_psql.pri)
contains(sql-drivers, mysql):include($$PWD/mysql/qsql_mysql.pri)
contains(sql-drivers, odbc):include($$PWD/odbc/qsql_odbc.pri)
contains(sql-drivers, oci):include($$PWD/oci/qsql_oci.pri)
contains(sql-drivers, tds):include($$PWD/tds/qsql_tds.pri)
contains(sql-drivers, db2):include($$PWD/db2/qsql_db2.pri)
contains(sql-drivers, ibase):include($$PWD/ibase/qsql_ibase.pri)
contains(sql-drivers, sqlite2):include($$PWD/sqlite2/qsql_sqlite2.pri)
contains(sql-drivers, sqlite):include($$PWD/sqlite/qsql_sqlite.pri)
