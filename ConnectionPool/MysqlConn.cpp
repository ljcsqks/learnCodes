#include "MysqlConn.h"

MysqlConn::MysqlConn()
{
    m_conn = mysql_init(nullptr);
    mysql_set_character_set(m_conn, "utf8");
}

MysqlConn::~MysqlConn()
{
    if (m_conn != nullptr)
    {
        mysql_close(m_conn);
    }
    freeResult();
}

bool MysqlConn::connect(string user, string passwd, string dbName, string ip, unsigned short port)
{
    MYSQL* ptr = mysql_real_connect(m_conn, ip.c_str(), user.c_str(), passwd.c_str(), dbName.c_str(), port, nullptr, 0);
    return ptr != nullptr;
}

bool MysqlConn::update(string sql)
{
    if (mysql_query(m_conn, sql.c_str()))
    {
        return false;
    }
    return true;
}

bool MysqlConn::query(string sql)
{
    freeResult();
    if (mysql_query(m_conn, sql.c_str()))
    {
        return false;
    }
    m_res = mysql_store_result(m_conn);
    return m_res != nullptr;
}

bool MysqlConn::next()
{
    if (m_res != nullptr)
    {
        m_row = mysql_fetch_row(m_res);
        return m_row != nullptr;
    }
    return false;
    
}

string MysqlConn::value(int index)
{
    int colNum = mysql_num_fields(m_res);
    if (index >= colNum || index < 0)
    {
        return string();
    }
    char* val = m_row[index];
    unsigned long length = mysql_fetch_lengths(m_res)[index];
    return string(val, length);

}

bool MysqlConn::transcation()
{
    return mysql_autocommit(m_conn, false);
}

bool MysqlConn::commit()
{
   return mysql_commit(m_conn);
}

bool MysqlConn::rollback()
{
    return mysql_rollback(m_conn);
}

void MysqlConn::freeResult()
{
    if (m_res != nullptr)
    {
        mysql_free_result(m_res);
        m_res = nullptr;
    }
}

void MysqlConn::refreshAliveTime()
{
    m_aliveTime = steady_clock::now();
}

long long MysqlConn::getAliveTime()
{
    nanoseconds res = steady_clock::now() - m_aliveTime;
    auto milliRes = duration_cast<milliseconds>(res);
    return milliRes.count();
}
