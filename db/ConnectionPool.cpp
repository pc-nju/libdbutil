#include "ConnectionPool.h"
#include "util/Config.h"

#include <QString>
#include <QQueue>
#include <QDebug>
#include <QMutex>
#include <QWaitCondition>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

/*-----------------------------------------------------------------------------|
 |                          d指针 的定义                                        |
 |----------------------------------------------------------------------------*/
class ConnectionPool::Private {
public:
    //数据库信息
    QString hostName;
    QString databaseName;
    QString databaseType;
    QString userName;
    QString password;
    int port;

    // 取得连接的时候验证连接有效
    bool testOnBorrow;
    // 测试访问数据库的 SQL
    QString testOnBorrowSql;
    // 获取连接最大等待时间
    int maxWaitTime;
    int waitInterval;
    // 最大连接数
    int maxConnectionCount;

    static QMutex mutex;
    static QWaitCondition waitCondition;

    QQueue<QString> usedConnectionNames;
    QQueue<QString> unUsedConnectionNames;

    Private();
    ~Private();

    QSqlDatabase createConnection(const QString &connectionName);

};

ConnectionPool::Private::Private()
{
    //获取配置实例
    Config &config = Singleton<Config>::getInstance();

    //从配置中获取数据库信息
    hostName = config.getDatabaseHost();
    databaseName = config.getDatabaseName();
    databaseType = config.getDatabaseType();
    userName = config.getDatabaseUsername();
    password = config.getDatabasePassword();

    port = config.getDatabaseport();
    testOnBorrow = config.getDatabaseTestOnBorrow();
    testOnBorrowSql = config.getDatabaseTestOnBorrowSql();
    maxWaitTime = config.getDatabaseMaxWaitTime();
    waitInterval = config.getDatabaseWaitInterval();
    maxConnectionCount = config.getDatabaseMaxConnectionCount();
}

ConnectionPool::Private::~Private()
{
    //销毁连接池的时候删除所有的连接
    for(QString connectionName : usedConnectionNames) {
        QSqlDatabase::removeDatabase(connectionName);
    }
    for(QString connectionName : unUsedConnectionNames) {
        QSqlDatabase::removeDatabase(connectionName);
    }
}

QSqlDatabase ConnectionPool::Private::createConnection(const QString &connectionName)
{
    Q_ASSERT(!connectionName.isEmpty());
    // 连接已经创建过了，复用它，而不是重新创建
    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase unUsedDb = QSqlDatabase::database(connectionName);
        if (testOnBorrow) {
            // 返回连接前访问数据库，如果连接断开，重新建立连接
            qDebug() << "Test connection on borrow, execute："
                     << testOnBorrowSql << ", for" << connectionName;
            QSqlQuery query(testOnBorrowSql, unUsedDb);
            if (query.lastError().type() != QSqlError::NoError && !unUsedDb.open()) {
                qDebug() << "Open databse error：" << query.lastError().text();
                return QSqlDatabase();
            }
        }
        return unUsedDb;
    }

    // 创建一个新的连接
    QSqlDatabase newDb = QSqlDatabase::addDatabase(databaseType, connectionName);
    newDb.setHostName(hostName);
    newDb.setDatabaseName(databaseName);
    newDb.setUserName(userName);
    newDb.setPassword(password);
    if (port != 0) {
        newDb.setPort(port);
    }

    if (!newDb.open()) {
        qDebug() << "Open database error：" << newDb.lastError().text();
        return QSqlDatabase();
    }
    return newDb;
}

QMutex ConnectionPool::Private::mutex;
QWaitCondition ConnectionPool::Private::waitCondition;

/*-----------------------------------------------------------------------------|
 |                             ConnectionPool 的定义                            |
 |----------------------------------------------------------------------------*/

ConnectionPool::ConnectionPool() : d(new ConnectionPool::Private)
{

}

ConnectionPool::~ConnectionPool()
{
}

void ConnectionPool::release()
{
    QMutexLocker locker(&ConnectionPool::Private::mutex);
    //删除私有指针，会调用 Private 析构函数，删除池中所有的连接
    delete d;
    d = NULL;
    qDebug() << "Destroy connection pool";
}

QSqlDatabase ConnectionPool::openConnection()
{
    QString connectionName;
    //加锁
    ConnectionPool::Private::mutex.lock();
    // 已创建连接数
    int connectionCount = d->usedConnectionNames.size() + d->unUsedConnectionNames.size();
    //需要一直阻塞等待的条件：未到最大等待时间；未使用的连接为0；当前连接总数等于最大连接数
    //满足所有条件，则阻塞当前线程，等待其他线程释放连接
    for (int i=0; i<d->maxWaitTime && d->unUsedConnectionNames.size() == 0
         && connectionCount == d->maxConnectionCount ; i+=d->waitInterval) {
        //阻塞，期间其他线程可以使用 mutex 锁，等待唤醒，唤醒后继续加锁，执行后续代码
        ConnectionPool::Private::waitCondition.wait(&ConnectionPool::Private::mutex, d->waitInterval);
        // 重新计算已创建连接数
        connectionCount = d->usedConnectionNames.size() + d->unUsedConnectionNames.size();
    }
    if (d->unUsedConnectionNames.size() > 0) {
        // 有已经回收的连接，复用它们
        connectionName = d->unUsedConnectionNames.dequeue();
    } else if (connectionCount < d->maxConnectionCount) {
        // 没有已经回收的连接，但是没有达到最大连接数，则创建新的连接
        connectionName = QString("Connection-%1").arg(connectionCount + 1);
    } else {
        // 已经达到最大连接数，且在最长等待时间内其他线程无释放连接
        qDebug() << "Cannot create more connections";
        // 创建连接超时，返回一个无效连接
        return QSqlDatabase();
    }
    //解锁
    ConnectionPool::Private::mutex.unlock();
    // 创建连接，因为创建连接很耗时，所以不放在 lock 的范围内，提高并发效率
    QSqlDatabase db = d->createConnection(connectionName);

    if (db.isOpen()) {
        ConnectionPool::Private::mutex.lock();
        // 有效的连接才放入 usedConnectionNames
        d->usedConnectionNames.enqueue(connectionName);
        ConnectionPool::Private::mutex.unlock();
    }
    return db;
}

void ConnectionPool::closeConnection(const QSqlDatabase &connection)
{
    QString connectionName = connection.connectionName();
    // 如果是我们创建的连接，从 used 里删除，放入 unused 里
    if (d->usedConnectionNames.contains(connectionName)) {
        QMutexLocker locker(&ConnectionPool::Private::mutex);
        d->usedConnectionNames.removeOne(connectionName);
        d->unUsedConnectionNames.enqueue(connectionName);
        //唤醒所有线程，此时阻塞的 QWaitCondition 会被唤醒
        ConnectionPool::Private::waitCondition.wakeOne();
    }
}


