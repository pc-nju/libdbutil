#include "SqlUtil.h"
#include "util/Config.h"

#include <QDebug>
#include <QString>
#include <QHash>
#include <QXmlParseException>
#include <QFile>
#include <QXmlInputSource>
#include <QXmlSimpleReader>
#include <QXmlDefaultHandler>



// static 全局变量作用域为当前文件
static const QString SQL_TAGNAME_SQLS = "sqls";
static const QString SQL_TAGNAME_DEFINE = "define";
static const QString SQL_TAGNAME_SQL = "sql";
static const QString SQL_TAGNAME_INCLUDE = "include";
static const QString SQL_NAMESPACE = "namespace";
static const QString SQL_ID = "id";
static const QString SQL_INCLUDE_DEFINE_ID = "defineId";

/*-----------------------------------------------------------------------------|
 |                         d指针 implementation                          |
 |----------------------------------------------------------------------------*/

class SqlUtil::Private : public QXmlDefaultHandler {
public:
    // Key 是 id, value 是 SQL 语句

    Private();
    QString buildKey(const QString &sqlNameSpace, const QString &sqlId);

    QHash<QString, QString> getSqls() const;

protected:
    bool startElement(const QString& namespaceURI, const QString& localName, const QString& qName, const QXmlAttributes& atts) Q_DECL_OVERRIDE;
    bool endElement(const QString& namespaceURI, const QString& localName, const QString& qName) Q_DECL_OVERRIDE;
    bool characters(const QString& ch) Q_DECL_OVERRIDE;
    bool fatalError(const QXmlParseException& exception) Q_DECL_OVERRIDE;

private:
    QHash<QString, QString> sqls;
    QHash<QString, QString> defines;
    QString sqlNameSpace;
    QString currentText;
    QString currentSqlId;
    QString currentDefineId;
    QString currentIncludedDefineId;
};

SqlUtil::Private::Private()
{
    //读取配置文件中配置项：sql_files
    QStringList sqlFiles = Singleton<Config>::getInstance().getDatabaseSqlFiles();
    if (!sqlFiles.isEmpty()) {
        for (QString fileName : sqlFiles) {
            qDebug() << QString("Loading SQL file：%1").arg(fileName);

            //解析配置文件
            QFile file(fileName);
            QXmlInputSource inputSource(&file);
            QXmlSimpleReader reader;
            reader.setContentHandler(this);
            reader.setErrorHandler(this);
            reader.parse(inputSource);

            this->defines.clear();
        }
    } else {
        qDebug() << "Cannot find sql_files in app.ini";
    }
}

QString SqlUtil::Private::buildKey(const QString &sqlNameSpace, const QString &sqlId)
{
    return sqlNameSpace + "::" + sqlId;
}

QHash<QString, QString> SqlUtil::Private::getSqls() const
{
    return this->sqls;
}

/**
 * 1. 取得 SQL 得 xml 文档中得 namespace, sql id, include 的 defineId, include 的 id
 * 2. 如果是 <sql> 标签，清空 currentText
 * 3. 如果是 <define> 标签，清空 currentText
 * @brief 参数都是解析 xml 之后得到的
 * @param namespaceUri
 * @param localName
 * @param qName 标签类型
 * @param attributes 属性
 * @return
 */
bool SqlUtil::Private::startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts)
{
    Q_UNUSED(namespaceURI);
    Q_UNUSED(localName);
    
    if (SQL_TAGNAME_SQLS == qName) {
        this->sqlNameSpace = atts.value(SQL_NAMESPACE);
    } else if (SQL_TAGNAME_DEFINE == qName) {
        this->currentDefineId = atts.value(SQL_ID);
        //置空
        this->currentText = "";
    } else if (SQL_TAGNAME_SQL == qName) {
        this->currentSqlId = atts.value(SQL_ID);
        //置空
        this->currentText = "";
    } else if (SQL_TAGNAME_INCLUDE == qName) {
        this->currentIncludedDefineId = atts.value(SQL_INCLUDE_DEFINE_ID);
    }
    return true;
}
/**
 * 1. 如果是 <sql> 标签，则插入 sqls
 * 2. 如果是 <include> 标签，则从 defines 里取其内容加入 sql
 * 3. 如果是 <define> 标签，则存入 defines
 * @param namespaceURI
 * @param localName
 * @param qName 标签类型
 * @return
 */
bool SqlUtil::Private::endElement(const QString &namespaceURI, const QString &localName, const QString &qName)
{
    Q_UNUSED(namespaceURI);
    Q_UNUSED(localName);

    if (SQL_TAGNAME_DEFINE == qName) {
        this->defines.insert(buildKey(this->sqlNameSpace, this->currentDefineId), currentText.simplified());
    } else if (SQL_TAGNAME_SQL == qName) {
        this->sqls.insert(buildKey(this->sqlNameSpace, this->currentSqlId), currentText.simplified());
        //重置
        currentText = "";
    } else if (SQL_TAGNAME_INCLUDE == qName) {
        QString defineKey = buildKey(this->sqlNameSpace, this->currentIncludedDefineId);
        QString defineValue = this->defines.value(defineKey);
        if (!defineValue.isEmpty()) {
            //将定义的片段拼接进 sql
            currentText += defineValue;
        } else {
            qDebug() << "Cannot find define：" << defineKey;
        }
    }
    return true;
}
/**
 * @brief 取出标签内的内容，拼接sql
 * @param ch 标签内的内容
 * @return
 */
bool SqlUtil::Private::characters(const QString &ch)
{
    currentText += ch;
    return true;
}

bool SqlUtil::Private::fatalError(const QXmlParseException &exception)
{
    qDebug() << QString("Parse error at line %1, column %2, message：%3")
                .arg(exception.lineNumber())
                .arg(exception.columnNumber())
                .arg(exception.message());
    return false;
}



SqlUtil::SqlUtil() : d(new SqlUtil::Private)
{
}

SqlUtil::~SqlUtil()
{
    delete d;
    d = NULL;
}

QString SqlUtil::getSql(const QString &sqlNameSpace, const QString &sqlId) const
{
    QString sql = d->getSqls().value(d->buildKey(sqlNameSpace, sqlId));
    if (sql.isEmpty()) {
        qDebug() << QString("Cannot find SQL for %1::%2").arg(sqlNameSpace).arg(sqlId);
    }
    return sql;
}


