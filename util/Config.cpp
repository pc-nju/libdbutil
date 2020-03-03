#include "Config.h"
#include "Json.h"

#include <QString>
#include <QStringList>

//单例宏中已经申明了构造函数和析构函数，这里直接写函数体即可，无需再次申明

Config::Config()
{
    json = new Json("data/config.json", true);
}

Config::~Config()
{
    delete json;
    json = NULL;
}

QString Config::getDatabaseType() const
{
    return json->getString("database.type");
}

QString Config::getDatabaseHost() const
{
    return json->getString("database.host");
}

QString Config::getDatabaseName() const
{
    return json->getString("database.database_name");
}

QString Config::getDatabaseUsername() const
{
    return json->getString("database.username");
}

QString Config::getDatabasePassword() const
{
    return json->getString("database.password");
}

QString Config::getDatabaseTestOnBorrowSql() const
{
    return json->getString("database.test_on_borrow_sql", "SELECT 1");
}

bool Config::getDatabaseTestOnBorrow() const
{
    return json->getBool("database.test_on_borrow", false);
}

int Config::getDatabaseMaxWaitTime() const
{
    return json->getInt("database.max_wait_time", 5000);
}

int Config::getDatabaseWaitInterval() const
{
    return json->getInt("database.wait_interval_time", 200);
}

int Config::getDatabaseMaxConnectionCount() const
{
    return json->getInt("database.max_connection_count", 5);
}

int Config::getDatabaseport() const
{
    return json->getInt("database.port", 0);
}

bool Config::isDatabaseDebug() const
{
    return json->getBool("database.debug", false);
}

QStringList Config::getDatabaseSqlFiles() const
{
    return json->getStringList("database.sql_files");
}

QStringList Config::getQssFiles() const
{
    return json->getStringList("qss_files");
}
