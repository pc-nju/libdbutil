#include "DbUtil.h"
#include "db/ConnectionPool.h"
#include "util/Config.h"

int DbUtil::insert(const QString &sql, const QVariantMap &params)
{
    int id = -1;
    //id 是引用传递
    executeSql(sql, params, [&id](QSqlQuery *query){
        //插入行的主键
        id = query->lastInsertId().toInt();
    });
    return id;
}

bool DbUtil::update(const QString &sql, const QVariantMap &params)
{
    bool result;
    executeSql(sql, params, [&result](QSqlQuery *query){
        result = query->lastError().type() == QSqlError::NoError;
    });
    return result;
}

QVariantMap DbUtil::selectMap(const QString &sql, const QVariantMap &params)
{
    return selectMaps(sql, params).value(0);
}

QList<QVariantMap> DbUtil::selectMaps(const QString &sql, const QVariantMap &params)
{
    QList<QVariantMap> rowMaps;
    executeSql(sql, params, [&rowMaps](QSqlQuery *query){
        rowMaps = queryToMaps(query);
    });
    return rowMaps;
}

int DbUtil::selectInt(const QString &sql, const QVariantMap &params)
{
    return selectVariant(sql, params).toInt();
}

qint64 DbUtil::selectInt64(const QString &sql, const QVariantMap &params)
{
    return selectVariant(sql, params).toLongLong();
}

QString DbUtil::selectString(const QString &sql, const QVariantMap &params)
{
    return selectVariant(sql, params).toString();
}

QStringList DbUtil::selectStrings(const QString &sql, const QVariantMap &params)
{
    QStringList results;
    executeSql(sql, params, [&results](QSqlQuery *query){
        while (query->next()) {
            results << query->value(0).toString();
        }
    });
    return results;
}

QDate DbUtil::selectDate(const QString &sql, const QVariantMap &params)
{
    return selectVariant(sql, params).toDate();
}

QDateTime DbUtil::selectDateTime(const QString &sql, const QVariantMap &params)
{
    return selectVariant(sql, params).toDateTime();
}

QVariant DbUtil::selectVariant(const QString &sql, const QVariantMap &params)
{
    QVariant result;
    executeSql(sql, params, [&result](QSqlQuery *query){
        if (query->next()) {
            result = query->value(0);
        }
    });
    return result;
}

void DbUtil::executeSql(const QString &sql, const QVariantMap &params, std::function<void (QSqlQuery *)> handleResult)
{
    QSqlDatabase db = Singleton<ConnectionPool>::getInstance().openConnection();
    QSqlQuery query(db);
    query.prepare(sql);
    bindValues(&query, params);
    
    if (query.exec()) {
        handleResult(&query);
    }
    
    debug(query, params);
    Singleton<ConnectionPool>::getInstance().closeConnection(db);
}

QStringList DbUtil::getFieldNames(const QSqlQuery &query)
{
    QSqlRecord record = query.record();
    QStringList names;
    for (int i=0; i<record.count(); i++) {
        names << record.fieldName(i);
    }
    return names;
}

void DbUtil::bindValues(QSqlQuery *query, const QVariantMap &params)
{
    for (QVariantMap::const_iterator i=params.constBegin(); i!=params.constEnd(); i++) {
        query->bindValue(":" + i.key(), i.value());
    }
}

QList<QVariantMap> DbUtil::queryToMaps(QSqlQuery *query)
{
    QList<QVariantMap> rowMaps;
    QStringList fieldNames = getFieldNames(*query);
    while (query->next()) {
        QVariantMap rowMap;
        for (const QString fieldName : fieldNames) {
            rowMap.insert(fieldName, query->value(fieldName));
        }
        rowMaps.append(rowMap);
    }
    return rowMaps;
}

void DbUtil::debug(const QSqlQuery &query, const QVariantMap &params)
{
    if (Singleton<Config>::getInstance().isDatabaseDebug()) {
        if (QSqlError::NoError != query.lastError().type()) {
            qDebug() << "    => SQL Error: " << query.lastError().text().trimmed();
        }
        qDebug() << "    => SQL Query:" << query.lastQuery();
        if (params.size() > 0) {
            qDebug() << "    => SQL Params: " << params;
        }
    }
}
