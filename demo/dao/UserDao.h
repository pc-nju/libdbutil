#ifndef USERDAO_H
#define USERDAO_H

#include <QList>
#include <QString>
#include <QVariant>
#include <QVariantMap>

class User;

class UserDao
{
public:
    static User findUserById(int id);
    static QList<User> findAll();
    static int insert(User *user);
    static bool update(User *user);
    static bool deleteUser(int id);

private:
    /**
     * @brief 将 QVariantMap 对象转换成 User
     * @param rowMap 数据库查询到的结果转换成的 QVariantMap 对象
     * @return User
     */
    static User mapToUser(const QVariantMap &rowMap);
    static QString getSql(const QString &functionName);
    //根据函数名和参数构造缓存的key
    static QString buildKey(std::initializer_list<QString> params);

    //单条记录缓存，key:"id"，value:"User"
    static QCache<QString, User> userCache;
    //缓存多条记录，key:"方法名+参数"，value：id集合
    static QCache<QString, QList<int>> usersCache;
};

#endif // USERDAO_H
