#include <QCoreApplication>

#include "db/SqlUtil.h"
#include "db/DbUtil.h"
#include "db/ConnectionPool.h"
#include "demo/bean/User.h"
#include "demo/dao/UserDao.h"

#include <QDebug>
#include <QDir>
#include <QCache>

void useDbUtil();
void useSqlFromFile();
void useDao();
void testCache();
void testUpdate();


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
//    useDbUtil();
//    useSqlFromFile();
//    useDao();
//    testCache();
//    testQCache();
    testUpdate();
    //必须手动释放，否则程序会崩溃
    Singleton<ConnectionPool>::getInstance().release();
    return a.exec();
}

void testUpdate() {
    //必须是new出来的对象，否则 QCache 拿不到所有权，就无法自动删除该对象，导致内存泄漏
    User *user = new User();
    user->setId(87);
    user->setUsername("Alice2");
    user->setPassword("5666");
    user->setEmail("23423@164.com");
    user->setMobile("1234241234");
    UserDao::update(user);
}
void testCache() {
    User *user1 = new User();
    User *user2 = new User();
    user1->setUsername("Alice");
    user1->setPassword("123123");
    user1->setEmail("23423@qq.com");
    user1->setMobile("1234241234");

    user2->setUsername("Bob");
    user2->setPassword("4564654");
    user2->setEmail("23423@163.com");
    user2->setPassword("dfhfdhgdfgh");
    user2->setMobile("54674576546");

    UserDao::insert(user1);
    UserDao::insert(user2);
    int startTime = QTime::currentTime().second();
    for (int i=0; i<1000; i++) {
        UserDao::findAll();
    }
    int endTime = QTime::currentTime().second();
    qDebug() << (endTime - startTime) << "秒";
}

void useDbUtil() {
    // 1. 查找 Alice 的 ID
    qDebug() << "\n1. 查找 Alice 的 ID";
    qDebug() << DbUtil::selectInt("select id from user where username='Alice'");
    qDebug() << DbUtil::selectVariant("select id from user where username='Alice'").toInt();

    // 2. 查找 Alice 的密码
    qDebug() << "\n2. 查找 Alice 的密码";
    qDebug() << DbUtil::selectString("select password from user where username='Alice'");
    qDebug() << DbUtil::selectMap("select password from user where username='Alice'")["password"].toString();

    // 3. 查找 Alice 的所有信息，如名字，密码，邮件等
    qDebug() << "\n3. 查找 Alice 的所有信息，如名字，密码，邮件等";
    qDebug() << DbUtil::selectMap("select * from user where username='Alice'");

    // 4. 查找 Alice 和 Bob 的所有信息，如名字，密码，邮件等
    qDebug() << "\n4. 查找 Alice 和 Bob 的所有信息，如名字，密码，邮件等";
    qDebug() << DbUtil::selectMaps("select * from user where username='Alice' or username='Bob'");

    // 5. 查找 Alice 和 Bob 的密码
    qDebug() << "\n5. 查找 Alice 和 Bob 的密码";
    qDebug() << DbUtil::selectStrings("select password from user where username='Alice' or username='Bob'");

    // 6. 查询时使用命名参数
    qDebug() << "\n6. 查询时使用命名参数";
    QMap<QString, QVariant> params;
    params["id"] = 1;

    qDebug() << DbUtil::selectMap("select * from user where id=:id", params);
    qDebug() << DbUtil::selectString("select username from user where id=:id", params);
}

void useSqlFromFile() {
    // 读取 namespace 为 User 下，id 为 findByUserId 的 SQL 语句
    qDebug() << Singleton<SqlUtil>::getInstance().getSql("User", "findByUserId");
    qDebug() << Singleton<SqlUtil>::getInstance().getSql("User", "findByUserId-1"); // 找不到这条 SQL 语句会有提示
    qDebug() << DbUtil::selectMap(Singleton<SqlUtil>::getInstance().getSql("User", "findByUserId").arg(2));
}

void useDao() {
    // 使用基于 DbUtil 封装好的 DAO 查询数据库
    User user = UserDao::findUserById(2);
    qDebug() << user.getUsername();
    qDebug() << user.toString();

    // 更新数据库
    user.setEmail("bob@gmail.com");
//    qDebug() << "Update: " << UserDao::update(user);

    QList<User> users = UserDao::findAll();
    foreach (const User &u, users) {
        qDebug() << u.toString();
    }
}
