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
    m_task_list = my_task_list::get_instance();
    m_conn_information = new conn_informantion[MAX_FD];
    m_log = my_Log::get_instance();
}

Webserver_reactor::~Webserver_reactor()
{
    close(m_epoll_fd);
    close(m_listen_fd);
    delete m_thread_pool;
    delete m_sql_thread_pool;
    delete m_log;
    delete m_task_list;
}

Webserver_reactor* Webserver_reactor::get_instance(){
    static Webserver_reactor instance;
    return &instance;
}

void Webserver_reactor::init_Webserver_reactor(int port,int sql_port, const char *sql_user,const char *sql_pwd, const char *db_name,
                     int subreactor_read_num,int subreactor_write_num, int sql_thread_pool_num, int thread_pool_num,bool log_close){
    m_port = port;
    m_sql_port = sql_port;
    m_sql_user = sql_user;
    m_sql_pwd = sql_pwd;
    m_db_name = db_name;
    m_subreactor_read_num = subreactor_read_num;
    m_subreactor_write_num = subreactor_write_num;
    m_sql_thread_pool_num = sql_thread_pool_num;
    m_thread_pool_num = thread_pool_num;
    m_task_list = my_task_list::get_instance();

    m_log = my_Log::get_instance();
    m_log->init("./web_log/", "log", 1024,log_close);
}

void Webserver_reactor::sub_reactor_read(){
    LOG_INFO("sub_reactor_read_active:%d",1);
    int read_bytes = 0;
    char read_buf[READ_BUFFER_SIZE];
    memset(read_buf,0,READ_BUFFER_SIZE);
    my_task_list::Webserver_task* task = nullptr;
    while(1){
        int m_read_idx = 0;
        //memset(read_buf,0,READ_BUFFER_SIZE);
        m_task_list->pop_read_task(task);
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
        //LOG_INFO("sub_reactor_read_str = %d",i++);
        LOG_INFO("sub_reactor_read_str =%s",task->str_in.c_str());
        task->close_flag = false;
        
        m_task_list->push_work_task(task);
        task = nullptr;
    }
}


void Webserver_reactor::sub_reactor_write(){
    LOG_INFO("sub_reactor_write_active:%d",Webserver_reactor::get_instance()->m_subreactor_write_num);
    int i = 0;
    my_task_list::Webserver_task* task = nullptr;
    while(1){
        m_task_list->pop_write_task(task);
        if(task->close_flag){
            close(task->socket_fd);
            delete task;
            task = nullptr;
            continue;
        }
        send(task->socket_fd,task->str_out.c_str(), task->str_out.size(),0);
        LOG_INFO("sub_reactor_write_str =%d",i++);
        LOG_INFO("sub_reactor_write_str_size =%s",task->str_out.c_str());

        epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, task->socket_fd, 0);
        delete task;
        task = nullptr;
    }
}

void Webserver_reactor::start_Webserver_reactor(){

    int ret = 0;
    LOG_INFO("start_Webserver_reactor:%d",Webserver_reactor::get_instance()->m_subreactor_read_num);
    
    m_epoll_fd = epoll_create(1);
    assert(m_epoll_fd != -1 && "epoll fd error1");

    m_listen_fd = socket(PF_INET, SOCK_STREAM, 0);
    assert(m_listen_fd >= 0 && "listen fd error1");

    struct linger tmp = {0, 1}; //长链接
    int flag = 1;//复用端口

    setsockopt(m_listen_fd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    setsockopt(m_listen_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    assert(m_listen_fd >= 0 && "listen fd error2");

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr)); //将address清零
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(m_port);

    ret = bind(m_listen_fd, (sockaddr *)&addr, sizeof(addr));
    assert(ret >= 0 && "bind error");
    ret = listen(m_listen_fd, 10);
    assert(ret >= 0 && "listen error");

    epoll_event event;
    event.data.fd = m_listen_fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;

    epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_listen_fd, &event);

    int old_option = fcntl(m_listen_fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(m_listen_fd, F_SETFL, new_option);

    m_sql_thread_pool = my_sql_thread_pool::get_instance();
    m_sql_thread_pool->init(string("localhost"),m_sql_user,m_sql_pwd,m_db_name, m_sql_port,m_sql_thread_pool_num);

    m_thread_pool = my_thread_pool<my_http_task>::get_instance();
    m_thread_pool->init(m_thread_pool_num,m_sql_thread_pool);

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

    epoll_event events[10000];
    while(1){
        //处理连接
        int event_num = epoll_wait(m_epoll_fd, events, 10000, -1);
        for(int i = 0; i < event_num; i++){
            int socket_fd = events[i].data.fd;
            if(socket_fd == m_listen_fd){
                deal_with_connection(socket_fd);
            }
            else {
                deal_with_task(socket_fd);
            }
        }
    }
}
void Webserver_reactor::deal_with_connection(int socket_fd){
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    int accept_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &client_addr_len);

    epoll_event event;
    event.data.fd = accept_fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;

    int old_option = fcntl(accept_fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(accept_fd, F_SETFL, new_option);
    epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, accept_fd, &event);

    m_conn_information[accept_fd].socket_fd = accept_fd;
    m_conn_information[accept_fd].client_addr = client_addr;
}

void Webserver_reactor::deal_with_task(int socket_fd){
    my_task_list::Webserver_task* task = new my_task_list::Webserver_task(socket_fd,m_conn_information[socket_fd].client_addr);
    m_task_list->push_read_task(task);
}

void Webserver_reactor::deal_with_time(int socket_fd){

}


void Webserver_reactor::stop_Webserver_reactor(){
    close(m_epoll_fd);
    close(m_listen_fd);
    delete m_thread_pool;
    delete m_sql_thread_pool;
    delete m_log;
    delete m_task_list;
}
