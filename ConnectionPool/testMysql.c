#include <stdio.h>
#include <mysql.h>

#define HOST "localhost"
#define USER "root"
#define PASSWORD "Ljc@522918"
#define DATABASE "demo"
#define PORT 3306

int main()
{
    //初始化MySQL连接对象
    MYSQL* conn = mysql_init(NULL);
    if (conn == NULL)
    {
        printf("mysql_init() error\n");
        return -1;
    }

    //连接
}