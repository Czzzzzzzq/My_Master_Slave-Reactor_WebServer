#ifndef MY_TIMER_H
#define MY_TIMER_H

#include <my_task_list.h>
#include <my_log.h>

#include <time.h>
#include <list>
#include <unordered_map>
#include <vector>

#include <sys/epoll.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>


class my_timer
{
public:
    my_timer();
    ~my_timer();
    void my_timer_init(int max_time,int epoll_fd);

    static my_timer* get_my_timer();

    void add_timer(my_task_list::Webserver_task* task);
    void del_timer(my_task_list::Webserver_task* task);
    void adjust_timer(my_task_list::Webserver_task* task);
    int tick();

    void addfd(int epollfd, int fd);
    static void sig_handler(int sig);
    void addsig(int sig, void(handler)(int));


    std::list<my_task_list::Webserver_task*> m_sorted_timer;
    std::vector<my_task_list::Webserver_task*> m_timer_vector;

    int MAX_TIME;
    int m_epoll_fd;
    static int u_pipefd[2];
};


#endif

