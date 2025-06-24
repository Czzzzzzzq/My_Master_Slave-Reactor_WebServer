#include <my_thread_pool.h>
template class my_thread_pool<my_http_task>;

template<typename T>
my_thread_pool<T>::my_thread_pool()
{
    my_thread_pool_number = 0;
    my_thread_pool_thread = nullptr;
    my_sql_pool = nullptr;
    my_work_task_list = my_task_list::get_instance();
}

template<typename T>
my_thread_pool<T>::~my_thread_pool()
{
    delete[] my_thread_pool_thread;
    my_sql_pool = nullptr;
}

template<typename T>
void my_thread_pool<T>::init(int thread_number, my_sql_thread_pool* sql_pool)
{
    my_thread_pool_number = thread_number;
    my_sql_pool = sql_pool;
    my_thread_pool_thread = new thread[thread_number];
    my_work_task_list = my_task_list::get_instance();
    
    sem_init(&my_thread_pool_sem, 0, thread_number);
    for (int i = 0; i < thread_number; ++i)
    {
        my_thread_pool_thread[i] = thread(&my_thread_pool<T>::run, this);
        my_thread_pool_thread[i].detach();
    }
    
}

template<typename T>
my_thread_pool<T>* my_thread_pool<T>::get_instance()
{
    static my_thread_pool pool;
    return &pool;
}

template<typename T>
void my_thread_pool<T>::run()
{
    T worker;
    my_task_list::Webserver_task* request = nullptr;

    while (true)
    {
        my_work_task_list->pop_work_task(request);    
        {
            connectionRAII mysqlcon(&worker.mysql, my_sql_pool);
            worker.process_http_task(request);
        }
        worker.reset();
    }
}





