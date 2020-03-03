#include "User.h"

User::User()
{
    //作为判断 User 是否为空的标志位
    id = -1;
}

QString User::toString() const
{
    return QString("[User]{ID: %1, Username: %2, Password: %3, Email: %4, Mobile: %5}")
            .arg(id).arg(username).arg(password).arg(email).arg(mobile);
}

int User::getId() const
{
    return id;
}

void User::setId(int value)
{
    id = value;
}

QString User::getUsername() const
{
    return username;
}

void User::setUsername(const QString &value)
{
    username = value;
}

QString User::getPassword() const
{
    return password;
}

void User::setPassword(const QString &value)
{
    password = value;
}

QString User::getEmail() const
{
    return email;
}

void User::setEmail(const QString &value)
{
    email = value;
}

QString User::getMobile() const
{
    return mobile;
}

void User::setMobile(const QString &value)
{
    mobile = value;
}
