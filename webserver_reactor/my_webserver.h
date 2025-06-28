#ifndef MY_WEB_SERVER_H
#define MY_WEB_SERVER_H

#include "my_thread_pool.h"
#include "my_sql_thread_pool.h"
#include "my_log.h"
#include "my_http.h"
#include "my_task_list.h"
#include "my_timer.h"

#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/uio.h>


#include <mutex>
#include <list>
#include <assert.h>
#include <fcntl.h>
#include <unordered_map>
#include <thread>


using namespace std;

const int MAX_FD = 65536;           //最大文件描述符

class Webserver_reactor
{
public:
    Webserver_reactor();
    ~Webserver_reactor();

    static Webserver_reactor* get_instance();

    void init_Webserver_reactor(int port,int sql_port, const char *sql_user,const char *sql_pwd, const char *db_name,
                     int subreactor_read_num,int subreactor_write_num, int conn_pool_num, int thread_pool_num,int timeout,bool log_close);

    void sub_reactor_read();
    void sub_reactor_write();

    void start_Webserver_reactor();
    void stop_Webserver_reactor();

    void deal_with_connection(int socket_listen_fd);
    void deal_with_task(int socket_fd);
    void deal_with_time(int& time_tick);
    void dealwithsignal(bool &my_time_out, bool &stop_my_server);

private:
    static const int READ_BUFFER_SIZE = 4096;
    int m_timeout;

    bool my_time_out;
    bool stop_my_server;
    bool m_log_close;
private:
    int m_port;
    int m_sql_port;
    string m_sql_user;
    string m_sql_pwd;
    string m_db_name;
    int m_subreactor_read_num;
    int m_subreactor_write_num;

    int m_sql_thread_pool_num;
    int m_thread_pool_num;

    int m_epoll_fd;
    int m_listen_fd;
    int m_pipefd[2];

    int pre_time;

    my_thread_pool<my_http_task> *m_thread_pool;
    my_sql_thread_pool *m_sql_thread_pool;
    my_Log *m_log;
    my_task_list *m_task_list;
    my_timer *m_timer;


};


#endif