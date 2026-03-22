#ifndef _CONNECTIONPOOL
#define _CONNECTIONPOOL
#include <iostream>
#include <queue>
#include <mutex>
#include "MysqlConn.h"
#include <json/json.h>
#include <fstream>
#include <condition_variable>

using namespace std;
using namespace Json;

class ConnectionPool
{
public:
    static ConnectionPool* getInstance();
    shared_ptr<MysqlConn> getConnection();
    ~ConnectionPool();

private:
    ConnectionPool();
    ConnectionPool(const ConnectionPool& connPool) = delete;
    ConnectionPool& operator=(const ConnectionPool& connPool) = delete;

    bool parseJsonFile(); //解析json文件
    void produceConnection(); //生产连接
    void recycleConnection(); //回收连接
    void addConnection(); //增加连接

    queue<MysqlConn*> m_connectionQ;
    mutex m_mutexQ;
    condition_variable m_condition;
    string m_user;
    string m_password;
    string m_ip;
    string m_dbName;
    unsigned short m_port;
    int m_minSize;
    int m_maxSize;
    int m_maxIdleTime;
    int m_timeOut;
};
#endif