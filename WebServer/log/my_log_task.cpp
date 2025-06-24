#include <my_log_task.h>
template class my_log_task<std::string>;


template<typename T>
my_log_task<T>::my_log_task(){
    log_task_max_size = 1000;
    log_task_front = 0;
    log_task_back = 0;
    task_queue.reserve(log_task_max_size);
}


template<typename T>
my_log_task<T>::my_log_task(int max_size){
    log_task_max_size = max_size;
    log_task_front = 0;
    log_task_back = 0;
    task_queue.reserve(log_task_max_size);
}

template<typename T>
my_log_task<T>::~my_log_task(){
    task_queue.clear();
}

template<typename T>
int my_log_task<T>::log_task_size(){
    return (log_task_back - log_task_front + log_task_max_size) % log_task_max_size;
}

template<typename T>
int my_log_task<T>::log_task_capacity(){
    return log_task_max_size;
}

template<typename T>
void my_log_task<T>::log_task_resize(int size){
    std::lock_guard<std::mutex> lock(log_task_mutex);
    if(size <= log_task_max_size) {
        return;
    }
    
    std::vector<T> new_queue(size);
    int count = log_task_size();
    
    for(int i = 0; i < count; i++) {
        new_queue[i] = task_queue[(log_task_front + i) % log_task_max_size];
    }
    
    task_queue = new_queue;
    log_task_front = 0;
    log_task_back = count;
    log_task_max_size = size;
}

template<typename T>
bool my_log_task<T>::if_log_task_empty(){
    return log_task_front == log_task_back;
}



template<typename T>
bool my_log_task<T>::if_log_task_full(){
    return (log_task_back + 1) % log_task_max_size == log_task_front;
}

template<typename T>
void my_log_task<T>::log_task_push(const T& task){
    std::unique_lock<std::mutex> lock(log_task_mutex);
    log_task_cond.wait(lock, [this] { return !if_log_task_full(); });
    task_queue[log_task_back] = task;
    log_task_back = (log_task_back + 1) % log_task_max_size;
    lock.unlock();
    log_task_cond.notify_one();
}

template<typename T>
void my_log_task<T>::log_task_pop(T& task){
    std::unique_lock<std::mutex> lock(log_task_mutex);
    log_task_cond.wait(lock, [this] { return !if_log_task_empty(); });     
    task = task_queue[log_task_front];
    log_task_front = (log_task_front + 1) % log_task_max_size;  
    lock.unlock();
    log_task_cond.notify_all(); 
}
