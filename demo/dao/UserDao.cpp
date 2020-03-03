#include "UserDao.h"
#include "db/SqlUtil.h"
#include "db/DbUtil.h"
#include "demo/bean/User.h"

#include <QCache>

/**
 * <?xml version="1.0" encoding="UTF-8"?>
    <sqls namespace="User">
        <define id="fields">id, username, password, email, mobile</define>

        <sql id="findByUserId">
            SELECT <include defineId="fields"/> FROM user WHERE id=%1
        </sql>

        <sql id="findAll">
            SELECT id, username, password, email, mobile FROM user
        </sql>

        <sql id="insert">
            INSERT INTO user (username, password, email, mobile)
            VALUES (:username, :password, :email, :mobile)
        </sql>

        <sql id="update">
            UPDATE user SET username=:username, password=:password,
                email=:email, mobile=:mobile
            WHERE id=:id
        </sql>
    </sqls>
 */

static const QString SQL_NAMESPACE_USER = "User";
/*
 * 缓存基本策略：
 *
 * 单个对象缓存：key：就是对象id；value：就是对象
 * 多个对象缓存（比如分页查询）： key：就是“函数名+参数1+参数2+...”；value：就是“对象id集合”
 *
 * 1、更新策略：只更新单个对象缓存
 * 2、删除策略：只删除单个对象缓存
 * 3、查询策略：查询策略又分为单个对象查询和多个对象查询
 *  （1）单个对象查询：基本一致
 *  （2）多个对象查询：获取缓存，取出id集合，然后遍历id集合，再去单个对象缓存里去找：
 *                   a.若全部找到，则返回对象集合
 *                   b.若未找到全部，则说明有对象已被删除，删除该缓存，重新查询数据库，更新缓存，返回
 *
 * 备注：若是自己写的局部缓存，就按上述策略；若使用像redis这种全局缓存，则重点需要构建key：对象的全局唯一id:id
 */

//在.h文件中定义了静态变量，在.cpp文件中使用，必须再次申明，否则报错
QCache<QString, User> UserDao::userCache;
QCache<QString, QList<int>> UserDao::usersCache;

User UserDao::findUserById(int id)
{
    QString key = "id";
    if (userCache.contains(key)) {
        return *userCache.object(key);
    } else {
        User user = DbUtil::selectBean(mapToUser, getSql("findUserById").arg(id));
        userCache.insert(key, &user);
        return user;
    }
}

/**
 * 查询所有数据，基本思路：
 * 1、根据key获取id集合
 * 2、遍历id集合，判断缓存是否存在该id对应的对象，一旦没有，则中断循环，清空对象集合
 * 3、判断对象集合是否为空，非空则说明缓存中对应的对象都在，返回缓存中对象即可
 * 4、若此时程序还未终止，说明缓存中数据有问题，查询数据库，更新缓存
 */
QList<User> UserDao::findAll()
{
    QString key = "findUserById";
    QList<User> users;
    //QCache自动获得被插入对象的所有权，并在需要的时候自动释放他们来为新插入的对象腾出空间，所以不用担心内存泄漏的问题
    //而且这里必须使用new,否则在该函数结束以后，集合变量就被删除了，导致缓存中的指针成为野指针！！！！！
    QList<int> *ids = new QList<int>();
    if (usersCache.contains(key)) {
        for (int id : *usersCache.object(key)) {
            if (!userCache.contains(QString::number(id))) {
                users.clear();
                break;
            }
            users.append(*userCache.object(QString::number(id)));
        }
        if (!users.isEmpty()) {
            return users;
        }
    }
    users = DbUtil::selectBeans(mapToUser, getSql("findAll"));
    for (User user : users) {
        ids->append(user.getId());
    }
    usersCache.insert(key, ids);
    return users;
}

int UserDao::insert(User *user)
{
    //构造参数
    QVariantMap params;
    params["username"] = user->getUsername();
    params["password"] = user->getPassword();
    params["email"] = user->getEmail();
    params["mobile"] = user->getMobile();
    int newId = DbUtil::insert(getSql("insert"), params);
    //-1作为判断 User 是否为空的标志位
    if (newId != -1) {
        userCache.insert(QString::number(newId), user);
        //因为新插入数据，所以需要更新数据集合（分页查询、全部查询），若不更新，则永远找不到新数据
        //现在采用的策略是删除所有的集合缓存，后续有什么好的方法再改进吧
        usersCache.clear();
    }

    return newId;
}

bool UserDao::update(User *user)
{
    //构造参数
    QVariantMap params;
    params["id"] = user->getId();
    params["username"] = user->getUsername();
    params["password"] = user->getPassword();
    params["email"] = user->getEmail();
    params["mobile"] = user->getMobile();
    bool result = DbUtil::update(getSql("update"), params);
    if (result) {
        //修改缓存
        userCache.insert(QString::number(user->getId()), user);
    }
    return result;
}

bool UserDao::deleteUser(int id)
{
    //更新缓存
    QVariantMap params;
    params["id"] = id;
    bool result = DbUtil::update(getSql("delete"), params);
    if (result) {
        userCache.remove(QString::number(id));
    }
    return result;
}

/**
 * @brief 将 QVariantMap 对象转换成 User
 * @param rowMap 数据库查询到的结果转换成的 QVariantMap 对象
 * @return User
 */
User UserDao::mapToUser(const QVariantMap &rowMap)
{
    User user;
    user.setId(rowMap.value("id", -1).toInt());
    user.setUsername(rowMap.value("username").toString());
    user.setPassword(rowMap.value("password").toString());
    user.setEmail(rowMap.value("email").toString());
    user.setMobile(rowMap.value("mobile").toString());
    return user;
}
/**
 * @brief 从配置文件中取出sql
 * @param 函数名
 * @return sql
 */
QString UserDao::getSql(const QString &functionName)
{
    return Singleton<SqlUtil>::getInstance().getSql(SQL_NAMESPACE_USER, functionName);
}

QString UserDao::buildKey(std::initializer_list<QString> params)
{
    QString key;
    for (auto param : params) {
        key += param + ":";
    }
    return key;
}

