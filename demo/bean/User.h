#ifndef USER_H
#define USER_H

#include <QString>

class User
{
public:
    User();
    QString toString() const;

    int getId() const;
    void setId(int value);

    QString getUsername() const;
    void setUsername(const QString &value);

    QString getPassword() const;
    void setPassword(const QString &value);

    QString getEmail() const;
    void setEmail(const QString &value);

    QString getMobile() const;
    void setMobile(const QString &value);

private:
    int id;
    QString username;
    QString password;
    QString email;
    QString mobile;
};

#endif // USER_H
