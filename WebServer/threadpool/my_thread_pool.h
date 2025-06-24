#ifndef MY_THREAD_POOL_H
#define MY_THREAD_POOL_H

#include "my_log.h"
#include "my_sql_thread_pool.h"
#include "my_http.h"
#include "my_task_list.h"

#include <thread>
#include <mutex>
#include <semaphore.h>
#include <list>
#include <mysql/mysql.h>

using namespace std;

template<typename T>
class my_thread_pool
{
public:
    my_thread_pool();
    ~my_thread_pool();
    void init(int thread_number, my_sql_thread_pool* sql_pool);
    static my_thread_pool* get_instance();
private:
    void run();

    int my_thread_pool_number;

    sem_t my_thread_pool_sem;
    mutex my_thread_pool_mutex;

    thread* my_thread_pool_thread;
    my_sql_thread_pool* my_sql_pool;
    my_task_list* my_work_task_list;
};


class connectionRAII
{
public:
    connectionRAII(MYSQL** conn, my_sql_thread_pool* sql_pool){
        *conn = sql_pool->get_Mysql_conn();
        
        my_conn = *conn;
        my_sql_pool = sql_pool;
    }
    ~connectionRAII(){
        my_sql_pool->release_Mysql_conn(my_conn);
    }
private:
    MYSQL* my_conn;
    my_sql_thread_pool* my_sql_pool;
};


#endif