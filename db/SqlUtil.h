#ifndef SQLUTIL_H
#define SQLUTIL_H

#include "util/Singleton.h"

class QString;

/**
SQL 文件的定义
1. <sqls> 必须有 namespace
2. [<define>]*: <define> 必须在 <sql> 前定义，必须有 id 属性才有意义，否则不能被引用
3. [<sql>]*: <sql> 必须有 id 属性才有意义，<sql> 里可以用 <include defineId="define_id"> 引用 <define> 的内容

SQL 文件定义 Demo:
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

/**
 * 用于加载 SQL 语句，用法.
 * Sqls::getSql("User", "selectById");
 *
 * SQL 文件的路径定义在 app.ini 的 [Database] 下的 sql_files，可以指定多个 SQL 文件，
 * 路径可以是绝对路径，也可以是相对与可执行文件的路径，如
 * sql_files = resources/sql/user.sql, resources/sql/product.sql
 */

class SqlUtil
{
    SINGLETON(SqlUtil)

public:
    // 取得 SQL 语句
    QString getSql(const QString &sqlNameSpace, const QString &sqlId) const;

private:
    class Private;
    friend class Private;
    Private *d;
};

#endif // SQLUTIL_H
