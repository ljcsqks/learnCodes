#include "ConnectionPool.h"
#include <json/json.h>
#include <thread>

ConnectionPool* ConnectionPool::getInstance()
{
    static ConnectionPool pool;
    return &pool;
}

bool ConnectionPool::parseJsonFile()
{
    ifstream ifs("connConfig.json");
    // 1. 检查文件是否能打开
    if (!ifs.is_open()) {
        cerr << "无法打开配置文件 connConfig.json，请检查文件路径是否正确！" << endl;
        return false;
    }

    Reader rd;
    Value root;
    // 2. 检查JSON格式是否正确
    if (!rd.parse(ifs, root)) {
        cerr << "配置文件格式错误！解析失败：" << rd.getFormattedErrorMessages() << endl;
        return false;
    }

    // 3. 检查是否为合法的JSON对象
    if (!root.isObject()) {
        cerr << "配置文件不是合法的JSON对象！" << endl;
        return false;
    }

    // 4. 读取配置参数
    m_user = root["user"].asString();
    m_password = root["password"].asString();
    m_ip = root["ip"].asString();
    m_dbName = root["dbName"].asString();
    m_port = root["port"].asUInt();
    m_minSize = root["minSize"].asInt();
    m_maxSize = root["maxSize"].asInt();
    m_maxIdleTime = root["maxIdleTime"].asInt();
    m_timeOut = root["timeOut"].asInt();

    cout << "配置文件解析成功，参数如下：" << endl;
    cout << "user: " << m_user << ", password: " << m_password << endl;
    cout << "ip: " << m_ip << ", port: " << m_port << ", dbName: " << m_dbName << endl;
    return true;
}

ConnectionPool::ConnectionPool()
{
    if(!parseJsonFile())
    {
        cerr << "Parse connConfig.json failed!" << endl;
        return;
    }
    cout << "配置文件参数：user " << m_user << ", password " << m_password << ", ip " << m_ip << ", dbName " << m_dbName << ", port " << m_port << endl;
    for (int i = 0; i < m_minSize; i++)
    {
        MysqlConn* myConn = new MysqlConn;
        if(myConn -> connect(m_user, m_password, m_dbName, m_ip, m_port))
        {
            myConn -> refreshAliveTime();
            m_connectionQ.push(myConn);
            cout << "MySQL Connection Pool Initialization Successed! Current Connection Num: " << m_connectionQ.size() << endl;
        }
        else
        {
            delete myConn;
            cerr << "MySQL Connection Pool Initialization Failed!" << endl;
        }
    }
    thread producer(&ConnectionPool::produceConnection, this);
    thread recycler(&ConnectionPool::recycleConnection, this);
    producer.detach();
    recycler.detach();
}

ConnectionPool::~ConnectionPool()
{
    while(!m_connectionQ.empty())
    {
        MysqlConn* conn = m_connectionQ.front();
        m_connectionQ.pop();
        delete conn;
    }
}

void ConnectionPool::produceConnection()
{
    while(true)
    {
        unique_lock<mutex> locker(m_mutexQ);
        while(m_connectionQ.size() >= m_maxSize)
        {
            m_condition.wait(locker);
        }
        addConnection();
        m_condition.notify_all();
    }
}

void ConnectionPool::recycleConnection()
{
    while(true)
    {
        this_thread::sleep_for(chrono::seconds(1));
        unique_lock<mutex> locker(m_mutexQ);
        while(m_connectionQ.size() > m_minSize)
        {
            MysqlConn* conn = m_connectionQ.front();
            auto aliveTime = conn->getAliveTime();
            if(aliveTime > m_maxIdleTime)
            {
                m_connectionQ.pop();
                delete conn;
                cout << "回收超时连接, 当前连接数: " << m_connectionQ.size() << endl;
            }
            else
            {
                break;
            }
        }
    }
}

void ConnectionPool::addConnection()
{
    MysqlConn* myConn = new MysqlConn;
    if(myConn -> connect(m_user, m_password, m_dbName, m_ip, m_port))
    {
        cout << "新增连接成功, 当前连接数: " << m_connectionQ.size() + 1 << endl;
    }
    else
    {
        delete myConn;
        cerr << "新增连接失败!" << endl;
        return;
    }
    myConn -> refreshAliveTime();
    m_connectionQ.push(myConn);
}

shared_ptr<MysqlConn> ConnectionPool::getConnection()
{
    unique_lock<mutex> locker(m_mutexQ);
    while(m_connectionQ.empty())
    {
        if(cv_status::timeout == m_condition.wait_for(locker, chrono::milliseconds(m_timeOut)))
        {
            if(m_connectionQ.empty())
            {
                cerr << "Get connection timeout!" << endl;
                // return nullptr;
                continue;
            }
        }
    }
    shared_ptr<MysqlConn> shared_conn(m_connectionQ.front(), [this](MysqlConn* conn){
        conn -> refreshAliveTime();
        lock_guard<mutex> lock(m_mutexQ);
        m_connectionQ.push(conn);
    });
    m_connectionQ.pop();
    m_condition.notify_all();
    return shared_conn;
}