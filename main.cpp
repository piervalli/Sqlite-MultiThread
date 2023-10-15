#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QtConcurrent>
#include <QMutex>
QMutex globalMutex;
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("mydatabase.db"); // Replace with your database name
    db.setConnectOptions(QLatin1String("QSQLITE_ENABLE_REGEXP=1;QSQLITE_BUSY_TIMEOUT=10000"));
    if (!db.open()) {
        qDebug() << "Database connection failed";
        return 1;
    }
    auto sql = QString("CREATE TABLE IF NOT EXISTS product (id int PRIMARY KEY,description TEXT);");
    QSqlQuery query(db);
    if(!query.exec(sql)){
        qCritical() << query.lastError();;
        return 1;
    }
    if(!query.exec("PRAGMA journal_mode=WAL;")){
        qCritical() << query.lastError();;
        return 1;
    }
    query.next();
    if(!query.exec("DELETE FROM product;")){
        qCritical() << query.lastError();;
        return 1;
    }

    static auto func = [](int rows,int offset){
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE",QString::number(offset));
        db.setDatabaseName("mydatabase.db"); // Replace with your database name
        db.setConnectOptions(QLatin1String("QSQLITE_ENABLE_REGEXP=1;QSQLITE_THREADSAFE=2;QSQLITE_BUSY_TIMEOUT=0"));
        if (!db.open()) {
            qDebug() << "Database connection failed";
        }
        globalMutex.lock();
        db.transaction();
        while(rows>0){
            --rows;
            ++offset;
            QSqlQuery query(db);
            if(!query.prepare("insert into product(id,description) values(:id,:description)")){
                qCritical() << query.lastError();
            }
            query.bindValue(":id",offset);
            query.bindValue(":description",offset);

            if(!query.exec()){
                qCritical() << query.lastError().databaseText();
            }

            if(!query.exec("update product set description ='1' where id =1;")){
                qCritical() << query.lastError();
            }
        }
        db.commit();
        globalMutex.unlock();
    };
    QtConcurrent::run(func,300000,0);
    QtConcurrent::run(func,300000,600001);
    db.commit();
    db.close();
    return 0;//a.exec();
}
