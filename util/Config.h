#ifndef CONFIG_H
#define CONFIG_H

#include "util/Singleton.h"

class QString;
class QStringList;
class Json;


/**
 * 用于读写配置文件:
 * 1. data/config.json: 存储配置的信息，例如数据库信息，QSS 文件的路径
 */
class Config
{
    SINGLETON(Config)
public:
    //获取数据库配置信息

    // 数据库的类型, 如QPSQL, QSQLITE, QMYSQL
    QString getDatabaseType() const;
    // 数据库主机的IP
    QString getDatabaseHost() const;
    // 数据库名
    QString getDatabaseName() const;
    // 登录数据库的用户名
    QString getDatabaseUsername() const;
    // 登录数据库的密码
    QString getDatabasePassword() const;
    // 验证连接的 SQL
    QString getDatabaseTestOnBorrowSql() const;
    // 是否验证连接
    bool getDatabaseTestOnBorrow() const;
    // 线程获取连接最大等待时间
    int getDatabaseMaxWaitTime() const;
    //等待测试连接间隔
    int getDatabaseWaitInterval() const;
    // 最大连接数
    int getDatabaseMaxConnectionCount() const;
    // 数据库的端口号
    int getDatabaseport() const;
    // 是否打印出执行的 SQL 语句和参数
    bool isDatabaseDebug() const;
    // SQL 语句文件, 可以是多个
    QStringList getDatabaseSqlFiles() const;

    //QSS 样式表文件, 可以是多个
    QStringList getQssFiles() const;

private:
    Json *json;

};

#endif // CONFIG_H
