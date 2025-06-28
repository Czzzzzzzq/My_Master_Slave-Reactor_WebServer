#include "my_webserver.h"

Webserver_reactor::Webserver_reactor()
{
    m_port = 8080;
    m_sql_port = 3306;
    m_sql_user = "root";
    m_sql_pwd = "123456789";
    m_db_name = "test";

    m_subreactor_read_num = 1;
    m_subreactor_write_num = 1;
    m_sql_thread_pool_num = 8;
    m_thread_pool_num = 8;

    my_time_out = false;
    stop_my_server = false;
}

Webserver_reactor::~Webserver_reactor()
{
    stop_Webserver_reactor();
}

void Webserver_reactor::stop_Webserver_reactor(){
    close(m_epoll_fd);
    close(m_listen_fd);

    delete m_task_list;
    delete m_timer;
    
    delete m_thread_pool;
    delete m_sql_thread_pool;

    delete m_log;
}

Webserver_reactor* Webserver_reactor::get_instance(){
    static Webserver_reactor instance;
    return &instance;
}

void Webserver_reactor::init_Webserver_reactor(int port,int sql_port, const char *sql_user,const char *sql_pwd, const char *db_name,
                     int subreactor_read_num,int subreactor_write_num, int sql_thread_pool_num, int thread_pool_num,int timeout,bool log_close){
    m_port = port;
    m_sql_port = sql_port;
    m_sql_user = sql_user;
    m_sql_pwd = sql_pwd;
    m_db_name = db_name;
    m_subreactor_read_num = subreactor_read_num;
    m_subreactor_write_num = subreactor_write_num;
    m_sql_thread_pool_num = sql_thread_pool_num;
    m_thread_pool_num = thread_pool_num;
    m_timeout = timeout;
    m_log_close = log_close;

    m_task_list = my_task_list::get_instance();
    m_log = my_Log::get_instance();
    m_log->init("./web_log/", "log", 10000,m_log_close);
    m_sql_thread_pool = my_sql_thread_pool::get_instance();
    m_sql_thread_pool->init(string("localhost"),m_sql_user,m_sql_pwd,m_db_name, m_sql_port,m_sql_thread_pool_num);
    m_thread_pool = my_thread_pool<my_http_task>::get_instance();
    m_thread_pool->init(m_thread_pool_num,m_sql_thread_pool);
    m_timer = my_timer::get_my_timer();
    m_timer->m_timer_vector.resize(10000);
    for (size_t i = 0; i < m_timer->m_timer_vector.size(); ++i) {
        if (!m_timer->m_timer_vector[i]) {
            m_timer->m_timer_vector[i] = new my_task_list::Webserver_task();
        }
    }
}

void Webserver_reactor::sub_reactor_read(){
    LOG_INFO("sub_reactor_read_active:%d",1);
    int read_bytes = 0;
    char read_buf[READ_BUFFER_SIZE];
    my_task_list::Webserver_task* task = nullptr;
    int i = 0;

    while(!stop_my_server){
        int m_read_idx = 0;
        memset(read_buf,0,READ_BUFFER_SIZE);
        m_task_list->pop_read_task(task);
        task->task_status = my_task_list::Webserver_task::WORK;

        if (task->socket_fd < 0) {
            LOG_ERROR("Invalid socket fd: %d", task->socket_fd);
            continue;
        }
        while(true)
        {
            read_bytes = recv(task->socket_fd, read_buf + m_read_idx, 
                                 READ_BUFFER_SIZE - m_read_idx - 1, 0); // 预留一个位置给'\0'
            if (read_bytes == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break; // 非阻塞模式下没有更多数据
                else {
                    LOG_ERROR("recv error: %d", errno);
                    break;
                }
            }
            else if (read_bytes == 0) {
                LOG_INFO("Connection closed by peer, socket: %d", task->socket_fd);
                break;
            }
            
            m_read_idx += read_bytes;
            
            // 检查缓冲区是否已满
            if (m_read_idx >= READ_BUFFER_SIZE - 1) {
                LOG_WARN("Read buffer full, socket: %d", task->socket_fd);
                break;
            }
        }
        read_buf[m_read_idx] = '\0';
        task->str_in = std::string(read_buf,m_read_idx);
        LOG_INFO("sub_reactor_read_str = %d",i++);        
        m_task_list->push_work_task(task);
    }
    LOG_INFO("sub_reactor_read_stop:%d",1);
}


void Webserver_reactor::sub_reactor_write(){
    LOG_INFO("sub_reactor_write_active:%d",Webserver_reactor::get_instance()->m_subreactor_write_num);
    int i = 0;
    my_task_list::Webserver_task* task = nullptr;
    while(!stop_my_server){
        m_task_list->pop_write_task(task);
        const char* str = task->str_out.c_str();
        send(task->socket_fd,str,strlen(str),0);

        task->task_status = my_task_list::Webserver_task::WAIT;
        close(task->socket_fd);
        //epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, task->socket_fd, 0); //自动完成

        LOG_INFO("sub_reactor_write_str =%d",i++);
    }
    LOG_INFO("sub_reactor_write_stop:%d",Webserver_reactor::get_instance()->m_subreactor_write_num);
}

void Webserver_reactor::start_Webserver_reactor(){
    int ret = 0;
    LOG_INFO("start_Webserver_reactor:%d",Webserver_reactor::get_instance()->m_subreactor_read_num);
    m_epoll_fd = epoll_create(1);
    assert(m_epoll_fd != -1 && "epoll fd error1");
    m_listen_fd = socket(PF_INET, SOCK_STREAM, 0);
    assert(m_listen_fd >= 0 && "listen fd error1");

    struct linger tmp = {1, 1};
    int flag = 1;//复用端口    // 解决 "address already in use" 错误
    setsockopt(m_listen_fd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    setsockopt(m_listen_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    assert(m_listen_fd >= 0 && "listen fd error2");

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr)); //将address清零
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(m_port);

    ret = bind(m_listen_fd, (sockaddr *)&addr, sizeof(addr));
    assert(ret >= 0 && "bind error");
    ret = listen(m_listen_fd, 1024);
    assert(ret >= 0 && "listen error");
    fcntl(m_listen_fd, F_SETFL, O_NONBLOCK);

    epoll_event event_listen = {};
    event_listen.data.fd = m_listen_fd;
    event_listen.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    event_listen.events = EPOLLIN | EPOLLRDHUP;
    epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_listen_fd, &event_listen);
    m_timer->my_timer_init(m_timeout,m_epoll_fd);

    std::vector<std::thread> sub_reactor_read_pool;
    std::vector<std::thread> sub_reactor_write_pool;
    sub_reactor_read_pool.resize(m_subreactor_read_num);
    sub_reactor_write_pool.resize(m_subreactor_write_num);
    
    for (int i = 0; i < m_subreactor_read_num; ++i)
    {
        sub_reactor_read_pool[i] = thread(&Webserver_reactor::sub_reactor_read,this);
        sub_reactor_read_pool[i].detach();
    }
    for (int i = 0; i < m_subreactor_write_num; ++i)
    {
        sub_reactor_write_pool[i] = thread(&Webserver_reactor::sub_reactor_write,this);
        sub_reactor_write_pool[i].detach();
    }

    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipefd);//创建一对socket，用于进程间通信
    assert(ret != -1);
    fcntl(m_pipefd[1], F_SETFL, O_NONBLOCK);

    epoll_event event_pipe = {};
    event_pipe.data.fd = m_pipefd[0];
    event_pipe.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_pipefd[0], &event_pipe);

    my_timer::u_pipefd[0] = m_pipefd[0];
    my_timer::u_pipefd[1] = m_pipefd[1];
    my_timer::get_my_timer()->addsig(SIGALRM, my_timer::sig_handler);
    my_timer::get_my_timer()->addsig(SIGTERM, my_timer::sig_handler);

    epoll_event events[10000];
    int time_tick = m_timeout;

    alarm(m_timeout); //设置定时器，每隔TIMESLOT秒发送SIGALRM信号
    while(!stop_my_server){
        //处理连接
        int event_num = epoll_wait(m_epoll_fd, events, 10000, -1);
        for(int i = 0; i < event_num; i++){
            int socket_fd = events[i].data.fd;
            if(socket_fd == m_listen_fd){
                deal_with_connection(socket_fd);
            }
            else if((socket_fd == m_pipefd[0])) {
                dealwithsignal(my_time_out,stop_my_server);
            }else{
                deal_with_task(socket_fd);
            }
        }
        if(my_time_out){
            deal_with_time(time_tick);
            alarm(time_tick);
            my_time_out = false;
        }
    }
    LOG_INFO("stop_Webserver_reactor");
}


void Webserver_reactor::deal_with_connection(int socket_fd){
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    int accept_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &client_addr_len);

    if (m_timer-> m_timer_vector[accept_fd]->task_status != my_task_list::Webserver_task::WORK)
    {
        epoll_event event = {};
        event.data.fd = accept_fd;
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;

        fcntl(accept_fd, F_SETFL, O_NONBLOCK);
        epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, accept_fd, &event);
        
        my_task_list::Webserver_task* task = m_timer->m_timer_vector[accept_fd];
        task->task_status = my_task_list::Webserver_task::READY;

        task->Webserver_task_reset(accept_fd,client_addr,time(NULL) + m_timeout);
        m_timer->add_timer(task);
    }else{
        LOG_DEBUG("re_accept_fd = %d",accept_fd); //检查task->task_status
    }

    return;
}

void Webserver_reactor::deal_with_task(int socket_fd){
    my_task_list::Webserver_task* task = m_timer->m_timer_vector[socket_fd];
    task->task_status = my_task_list::Webserver_task::WORK;
    m_task_list->push_read_task(task);
}

void Webserver_reactor::deal_with_time(int& time_tick){
    time_tick = m_timer->tick();
    LOG_INFO("deal_with_time time_tick = %d", time_tick);
}

void Webserver_reactor::dealwithsignal(bool &timeout, bool &stop_server){
    int ret = 0;
    char signals[1024];
    ret = recv(m_pipefd[0], signals, sizeof(signals), 0);
    for (int i = 0; i < ret; ++i)
    {
        switch (signals[i])
        {
            case SIGALRM:   
            {
                timeout = true;
                break;
            }
            case SIGTERM || SIGINT:
            {
                stop_server = true;
                break;
            }
        }
    }
}

