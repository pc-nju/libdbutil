#include "Json.h"

#include <QDebug>
#include <QObject>
#include <QFile>
#include <QRegularExpression>
#include <QJsonArray>

/*-----------------------------------------------------------------------------|
 |                             Private implementation                          |
 |----------------------------------------------------------------------------*/

class Json::Private
{
public:
    // Json 根节点
    QJsonObject root;

    /**
     * @brief 实例化根节点 root
     * @param jsonOrJsonFilePath Json 的字符串内容或者 Json 文件的路径
     * @param fromFile 为 true，则 jsonOrJsonFilePath 为文件的路径，为 false 则 jsonOrJsonFilePath 为 Json 的字符串内容
     */
    Private(const QString &jsonOrJsonFilePath, bool fromFile);

    /**
     * @brief 用递归+引用设置 Json 的值，因为 toObject() 等返回的是对象的副本，对其修改不会改变原来的对象，所以需要用引用来实现
     * @param parent 父节点
     * @param path 带 "." 的路径格
     * @param newValue 待设置值
     * @return
     */
    void setValue(QJsonObject &parent, const QString &path, const QJsonValue &newValue);

    /**
     * @brief 读取属性的值，如果 fromNode 为空，则从跟节点开始访问
     * @param path 带 "." 的路径格
     * @param fromNode 寻找的开始节点
     * @return 值
     */
    QJsonValue getValue(const QString &path, const QJsonObject &fromNode) const;
};

Json::Private::Private(const QString &jsonOrJsonFilePath, bool fromFile)
{
    QByteArray json("{}");

    //如果传入的是 Json 文件的路径，则读取内容
    if (fromFile) {
        QFile file(jsonOrJsonFilePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            json = file.readAll();
        }
        else{
            qDebug() << QString("Cannot open the file：%1").arg(jsonOrJsonFilePath);
        }
    } else {
        json = jsonOrJsonFilePath.toUtf8();
    }

    // 解析 Json
    QJsonParseError parseError;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(json, &parseError);

    if (QJsonParseError::NoError == parseError.error) {
        root = jsonDocument.object();
    } else {
        root = QJsonObject();
        qDebug() << parseError.errorString() << ", Offset：" << parseError.offset;
    }
}

void Json::Private::setValue(QJsonObject &parent, const QString &path, const QJsonValue &newValue)
{
    if (!path.isEmpty()) {
        const int indexOfDot = path.indexOf(".");
        // 第一个 . 之前的内容，如果 indexOfDot 是 -1 则返回整个字符串
        const QString property = path.left(indexOfDot);
        //获取路径中的余下字符串
        const QString subPath = (indexOfDot > 0) ? path.mid(indexOfDot + 1) : QString();
        QJsonValue subValue = parent[property];
        if (!subPath.isEmpty()) {
            QJsonObject subObject = subValue.toObject();
            //递归
            setValue(subObject, subPath, newValue);
            //在找到最后的根节点之后，我们更改其值，同时该节点所在的父节点也要更改，把修改的父节点写回根节点
            subValue = subObject;
        } else {
            //这里只是给最后的节点赋值的
            subValue = newValue;
        }
        //这里也会被递归调用，最后的节点被写回父节点，父节点再被写回祖节点，直到写回根节点
        parent[property] = subValue;
    } else {
        qDebug() << QString("路径为空，请检查路径!");
    }
}

QJsonValue Json::Private::getValue(const QString &path, const QJsonObject &fromNode) const
{
    QJsonValue result;
    if (!path.isEmpty()) {
        QJsonObject parent(fromNode.isEmpty() ? root : fromNode);
        //\\会转义成反斜杠，反斜杠本身就是转义符，所有就成了“\.”，在进行转义就是.，所以\\.实际上是“.”。
        QStringList tokens = path.split(QRegularExpression("\\."));

        for (int i=0; i<tokens.count()-1; i++) {
            if (parent.isEmpty()) {
                return QJsonValue();
            }
            //[] 和 value() 都是一种取值方式
            parent = parent.value(tokens.at(i)).toObject();
        }

        return parent.value(tokens.last());
    } else {
        qDebug() << QString("路径为空，请检查路径!");
    }
    return result;
}

/*-----------------------------------------------------------------------------|
 |                                Json implementation                          |
 |----------------------------------------------------------------------------*/

Json::Json(const QString &jsonOrJsonFilePath, bool fromFile) :
    d(new Json::Private(jsonOrJsonFilePath, fromFile))
{

}

Json::~Json()
{
    delete d;
    d = NULL;
}

int Json::getInt(const QString &path, int def, const QJsonObject &fromNode) const
{
    return d->getValue(path, fromNode).toInt(def);
}

bool Json::getBool(const QString &path, bool def, const QJsonObject &fromNode) const
{
    return d->getValue(path, fromNode).toBool(def);
}

double Json::getDouble(const QString &path, double def, const QJsonObject &fromNode) const
{
    return d->getValue(path, fromNode).toDouble(def);
}

QString Json::getString(const QString &path, const QString &def, const QJsonObject &fromNode) const
{
    return d->getValue(path, fromNode).toString(def);
}

QStringList Json::getStringList(const QString &path, const QJsonObject &fromNode) const
{
    QStringList result;
    QJsonArray array = d->getValue(path, fromNode).toArray();
    for (QJsonValue value : array) {
        result << value.toString();
    }
    return result;
}

QJsonArray Json::getJsonArray(const QString &path, const QJsonObject &fromNode) const
{
    return d->getValue(path, fromNode).toArray();
}

QJsonValue Json::getJsonValue(const QString &path, const QJsonObject &fromNode) const
{
    return d->getValue(path, fromNode);
}

QJsonObject Json::getJsonObject(const QString &path, const QJsonObject &fromNode) const
{
    return d->getValue(path, fromNode).toObject();
}



void Json::set(const QString &path, const QJsonValue &value)
{
    d->setValue(d->root, path, value);
}

void Json::set(const QString &path, const QStringList &strings)
{
    QJsonArray arr;
    for(QString str : strings) {
        arr.append(str);
    }
    d->setValue(d->root, path, arr);
}

void Json::save(const QString &filePath, QJsonDocument::JsonFormat format)
{
    QFile file(filePath);
    if (!(file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))) {
        return;
    }

    QTextStream out(&file);
    out << QJsonDocument(d->root).toJson(format);
}

QString Json::toString(QJsonDocument::JsonFormat format) const
{
    return QJsonDocument(d->root).toJson(format);
}

