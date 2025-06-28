#ifndef MY_TASK_LIST_H
#define MY_TASK_LIST_H  

#include <mutex>
#include <list>
#include <string>
#include <condition_variable>
#include <semaphore.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <mysql/mysql.h>
#include <iostream>
#include <unistd.h>

using namespace std;
class my_task_list
{
public:
    struct Webserver_task{
        public:
        enum Task_status{
            WAIT,
            READY,
            WORK,
            CLOSE
        };
        Task_status task_status;

        int socket_fd;
        sockaddr_in client_addr;

        string str_in;
        string str_out;
        MYSQL* mysql;

        time_t timeout;

        Webserver_task(int socket_fd = -1,sockaddr_in client_addr = {},time_t timeout = 0,string str_in = "",string str_out = "",MYSQL* mysql = nullptr,Task_status task_status = WAIT){
            this->socket_fd = socket_fd;
            this->client_addr = client_addr;
            this->timeout = timeout;
            this->str_in = str_in;
            this->str_out = str_out;
            this->mysql = mysql;
            this->task_status = task_status;
        }
        ~Webserver_task(){
            if(socket_fd != -1){
                close(socket_fd);
            }
        };
        void Webserver_task_reset(int socket_fd = -1,sockaddr_in client_addr = {},time_t timeout = 0,string str_in = "",string str_out = "",MYSQL* mysql = nullptr,Task_status task_status = READY){
            this->socket_fd = socket_fd;
            this->client_addr = client_addr;
            this->timeout = timeout;
            this->str_in = str_in;
            this->str_out = str_out;
            this->mysql = mysql;
            this->task_status = task_status;
        }
    };
public:
    my_task_list();
    ~my_task_list();

    static my_task_list* get_instance();

    void push_read_task(Webserver_task* task);
    void push_write_task(Webserver_task* task);

    void pop_read_task(Webserver_task* &task);
    void pop_write_task(Webserver_task* &task);

    void push_work_task(Webserver_task* task);
    void pop_work_task(Webserver_task* &task);

private:
    //IO队列
    int read_task_num;
    int write_task_num;
    int work_task_num;

    mutex reactor_read_mutex;
    mutex reactor_write_mutex;

    condition_variable subreactor_read_cond;
    condition_variable subreactor_write_cond;

    list<Webserver_task*> m_subreactor_read_list;
    list<Webserver_task*> m_subreactor_write_list;

    //任务队列
    mutex task_mutex;
    condition_variable task_cond;
    list<Webserver_task*> m_task_list;
};







#endif