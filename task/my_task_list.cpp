#include "my_task_list.h"

my_task_list::my_task_list()
{
    m_subreactor_read_list.clear();
    m_subreactor_write_list.clear();
    m_task_list.clear();
}

my_task_list::~my_task_list()
{
    m_subreactor_read_list.clear();
    m_subreactor_write_list.clear();
    m_task_list.clear();
}

my_task_list* my_task_list::get_instance()
{
    static my_task_list instance;
    return &instance;
}

//读队列
void my_task_list::push_read_task(Webserver_task* task)
{
    lock_guard<mutex> lock(reactor_read_mutex);
    m_subreactor_read_list.push_back(task);
    read_task_num++;
    subreactor_read_cond.notify_all();
}
void my_task_list::pop_read_task(Webserver_task* &task)
{
    unique_lock<mutex> lock(reactor_read_mutex);
    subreactor_read_cond.wait(lock, [this] { return read_task_num > 0; });      
    task = m_subreactor_read_list.front();
    m_subreactor_read_list.pop_front();
    read_task_num--;
    lock.unlock();
}


//写队列
void my_task_list::push_write_task(Webserver_task* task)
{
    lock_guard<mutex> lock(reactor_write_mutex);
    m_subreactor_write_list.push_back(task);
    write_task_num++;
    subreactor_write_cond.notify_all();
}
void my_task_list::pop_write_task(Webserver_task* &task)
{
    unique_lock<mutex> lock(reactor_write_mutex);       
    subreactor_write_cond.wait(lock, [this] { return write_task_num > 0; });
    task = m_subreactor_write_list.front();
    m_subreactor_write_list.pop_front();
    write_task_num--;
    lock.unlock();
}

//任务队列
void my_task_list::push_work_task(Webserver_task* task)
{
    lock_guard<mutex> lock(task_mutex);
    m_task_list.push_back(task);
    work_task_num++;
    task_cond.notify_all();
}

void my_task_list::pop_work_task(Webserver_task* &task)
{
    unique_lock<mutex> lock(task_mutex);
    task_cond.wait(lock,[this]{return m_task_list.size() > 0;});
    task = m_task_list.front();
    m_task_list.pop_front();
    work_task_num--;
    lock.unlock();
}   




