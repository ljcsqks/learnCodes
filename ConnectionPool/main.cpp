#include "MysqlConn.h"
#include "ConnectionPool.h"
#include <iostream>
#include <memory>

using namespace std;

void op1(int begin, int end)
{
    for(int i = begin; i < end; i++)
    {
        MysqlConn conn;
        conn.connect("root", "Ljc@522918", "demo", "localhost", 3306);
        char sql[1024] = {0};
        sprintf_s(sql, "insert into user values(%d,'zhangsan', 11, '1111@gmail.com', 111111)", i); 
        conn.update(sql);
    }
}

void op2(ConnectionPool* connPool, int begin, int end)
{
    for(int i = begin; i < end; i++)
    {
        shared_ptr<MysqlConn> conn =  connPool -> getConnection();
        char sql[1024] = {0};
        sprintf_s(sql, "insert into user values(%d,'zhangsan', 11, '1111@gmail.com', 111111)", i); 
        conn -> update(sql);
    }
}

void test()
{
#if 0
//单线程用时： 67796毫秒
    steady_clock::time_point begin = steady_clock::now();
    op1(0, 5000);
    steady_clock::time_point end = steady_clock::now();
    auto milliLength = duration_cast<milliseconds>(end - begin);
    cout << "单线程用时： " << milliLength.count() << "毫秒" << endl;
#else
    ConnectionPool* connPool = ConnectionPool::getInstance();
    steady_clock::time_point begin = steady_clock::now();
    op2( connPool, 0, 5000);
    steady_clock::time_point end = steady_clock::now();
    auto milliLength = duration_cast<milliseconds>(end - begin);
    cout << "连接池用时： " << milliLength.count() << "毫秒" << endl;

#endif
}

int main()
{
    test();
    return 0;
}


/*
int query()
{
    MysqlConn conn;
    conn.connect("root", "Ljc@522918", "demo", "localhost", 3306);
    string sql = "insert into user values(4,'zhangsan', 11, '1111@gmail.com', 111111)";
    bool flag = conn.update(sql);
    cout << "flag value: " << flag <<endl;
    
    sql = "select * from user";
    conn.query(sql);
    while(conn.next())
    {
        cout << conn.value(0) << ", "
        <<conn.value(1) << ", "
        <<conn.value(2) << ", "
        <<conn.value(3) << endl;
    }
    return 0;
}
*/

