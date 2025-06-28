#include <my_timer.h>
int my_timer::u_pipefd[2] = {0};

my_timer::my_timer()
{
    my_timer_init(10,0);
}

void my_timer::my_timer_init(int max_time,int epoll_fd){
    MAX_TIME = max_time;
    m_epoll_fd = epoll_fd;
}

//任务关闭放在定时器中
my_timer::~my_timer()
{
    for(auto iter = m_sorted_timer.begin(); iter != m_sorted_timer.end(); iter++){
        if((*iter)->socket_fd != -1){
            close((*iter)->socket_fd);
        }
    }
    m_sorted_timer.clear();
}

my_timer* my_timer::get_my_timer(){
    static my_timer timer;
    return &timer;
}

void my_timer::add_timer(my_task_list::Webserver_task* task){
    //m_timer_vector[task->socket_fd] = task;
    m_sorted_timer.push_back(task);
}

void my_timer::del_timer(my_task_list::Webserver_task* task){
    if(task->socket_fd != -1){
        epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, task->socket_fd, 0);
        close(task->socket_fd);
    }
}

int my_timer::tick(){
    time_t cur = time(NULL);
    while(!m_sorted_timer.empty()){
        my_task_list::Webserver_task* task = m_sorted_timer.front();
        if(task == nullptr){
            break;
        }
        if(task->timeout >= cur){
            break;
        }else{
            m_sorted_timer.pop_front();
            if(task->task_status == my_task_list::Webserver_task::CLOSE){
                LOG_INFO("close task:%d",task->socket_fd);
                del_timer(task);
            }
            else if(task->task_status == my_task_list::Webserver_task::READY){
                task->timeout = cur + MAX_TIME;
                m_sorted_timer.push_back(task);           
                task->task_status = my_task_list::Webserver_task::CLOSE;
            }else if(task->task_status == my_task_list::Webserver_task::WORK){
                task->timeout = cur + MAX_TIME;
                m_sorted_timer.push_back(task);                      
            }
        }
    }
    if (!m_sorted_timer.empty() && m_sorted_timer.front()->timeout - cur > 0){
        return m_sorted_timer.front()->timeout - cur;
    }else{
        return MAX_TIME;
    }
}

//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void my_timer::addfd(int epollfd, int fd)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;

    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    fcntl(fd, F_SETFL, O_NONBLOCK);
}

//信号处理函数
void my_timer::sig_handler(int sig)
{
    int msg = sig;
    send(u_pipefd[1], (char *)&msg, 1, 0);
}

//设置信号函数
void my_timer::addsig(int sig, void(handler)(int))
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    sigfillset(&sa.sa_mask);
    sigaction(sig, &sa, NULL);
}