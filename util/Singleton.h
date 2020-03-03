#ifndef SINGLETON_H
#define SINGLETON_H

#include <QMutex>
#include <QScopedPointer>

/**
 * 使用方法:
 * 1. 定义类为单例:
 *     class ConnectionPool {
 *         SINGLETON(ConnectionPool) // Here
 *     public:
 *
 * 2. 获取单例类的对象:
 *     Singleton<ConnectionPool>::getInstance();
 *     ConnectionPool &pool = Singleton<ConnectionPool>::getInstance();
 * 注意: 如果单例的类需要释放的资源和 Qt 底层的信号系统有关系，例如 QSettings，QSqlDatabase 等，
 *     需要在程序结束前手动释放(也就是在 main() 函数返回前调用释放资源的函数，参考 ConnectionPool 的调用)，
 *     否则有可能在程序退出时报系统底层的信号错误，导致如 QSettings 的数据没有保存。
 */
template <typename T>
class Singleton {
public:
    static T& getInstance();

    Singleton(const Singleton &other);
    Singleton<T>& operator=(const Singleton &other);

private:
    //同步锁
    static QMutex mutex;
    //智能指针，只在该作用域存在，一旦出该作用域，就会自动销毁（这也是单例模式在创建的时候，无需析构的原因，出了作用域自动删除）
    static QScopedPointer<T> instance;
};

/*-----------------------------------------------------------------------------|
 |                          Singleton implementation                           |
 |----------------------------------------------------------------------------*/
template <typename T> QMutex Singleton<T>::mutex;
template <typename T> QScopedPointer<T> Singleton<T>::instance;

template <typename T>
T &Singleton<T>::getInstance()
{
    if (instance.isNull()) {
        mutex.lock();
        if (instance.isNull()) {
            //reset()：delete目前指向的对象，调用其析构函数，将指针指向另一个对象other，所有权转移到other
            instance.reset(new T());
        }
        mutex.unlock();
    }
    /**
     * 获取智能指针动态创建的对象的指针有两种方法：QScopedPointer#data() 和 QScopedPointer#take()
     *     T *QScopedPointer::data() const返回指向对象的常量指针，QScopedPointer仍拥有对象所有权。
     * 所以通过data()返回过后就被自动删除了，从而导致mian函数中的p1变成了野指针，程序崩溃.
     *     使用T *QScopedPointer::take()也是返回对象指针，但QScopedPointer不再拥有对象所有权，而
     * 是转移到调用这个函数的caller，同时QScopePointer对象指针置为NULL
     */
    return *instance.data();
}

/*-----------------------------------------------------------------------------|
 |                               Singleton Macro                               |
 |----------------------------------------------------------------------------*/

/**
  * QScopedPointerDeleter 是 QScopedPointer 的默认实现，使用delete删除指针
  * 宏中不能使用 // 添加注释，换行续添加换行符 “\”
  *
  * 宏中已经申明了构造函数和析构函数，使用该宏的类直接写函数体即可，无需再次申明
  */
#define SINGLETON(Class)                            \
private:                                            \
    Class();                                        \
    ~Class();                                       \
    Class(const Class &other);                      \
    Class& operator=(const Class &other);           \
    friend class Singleton<Class>;  /*注释必须写在续行符（"\"）的前面 */ \
    friend struct QScopedPointerDeleter<Class>;/* 末尾注释不需要加续行符 */
#endif // DBUTIL_H


