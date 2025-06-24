#ifndef MY_HTTP_H
#define MY_HTTP_H

#include "my_log.h"
#include "my_sql_thread_pool.h"
#include "my_task_list.h"


#include <mysql/mysql.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>

using namespace std;
class my_http_task
{
public:
    enum METHOD     //http请求方法
    {
        GET = 0,    // GET 请求方法，用于获取资源
        POST,       // POST 请求方法，用于向服务器提交数据
    };
    enum CHECK_STATE    //主状态机的状态
    {
        CHECK_STATE_REQUESTLINE = 0,    //当前正在分析请求行
        CHECK_STATE_HEADER,             //当前正在分析头部字段
        CHECK_STATE_CONTENT             //当前正在解析请求体
    };
    enum HTTP_CODE    // 从状态机的可能状态，报文解析的结果
    {
        NO_REQUEST = 0,     // 未收到完整的 HTTP 请求
        GET_REQUEST,    // 成功解析出一个完整的 HTTP GET 请求
        POST_REQUEST,   // 成功解析出一个完整的 HTTP POST 请求
        BAD_REQUEST,    // 请求格式错误，不符合 HTTP 协议规范
    };
    enum LINE_STATUS    //从状态机的可能状态，行的读取状态
    {
        LINE_OK = 0, //值为 0，表示当前行已经完整读取，并且格式正确
        LINE_BAD, //表示当前行的格式错误，不符合 HTTP 请求的规范。可能是在读取过程中发现了非法字符
        LINE_OPEN //表示当前行还未读取完整，可能是因为数据还没有全部到达，需要继续等待更多的数据
    };

public:
    
    my_http_task();
    ~my_http_task();

    void close_http_task();

    void process_http_task(my_task_list::Webserver_task* task);

    void read_http_task(HTTP_CODE& res,my_task_list::Webserver_task* task);
    void write_http_task(HTTP_CODE& res,my_task_list::Webserver_task* task);

    void reset();
    
    MYSQL* mysql;

private:
    static int my_http_task_num;

    METHOD my_method;

    CHECK_STATE my_check_state;
    HTTP_CODE my_http_code;
    LINE_STATUS my_line_status;

    char sql_user[100];//数据库用户名
    char sql_passwd[100];//数据库密码
    char sql_name[100];//数据库名称

    char *m_url;//请求的URL
    char *m_version;//HTTP协议版本
    char *m_host;//主机名

    my_task_list* my_work_task_list;
};

#endif