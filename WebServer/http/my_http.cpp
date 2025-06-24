#include <my_http.h>

my_http_task::my_http_task()
{
    mysql = nullptr;
    my_work_task_list = nullptr;

    my_method = METHOD::GET;
    my_check_state = CHECK_STATE::CHECK_STATE_REQUESTLINE;
    my_http_code = HTTP_CODE::NO_REQUEST;
    my_line_status = LINE_STATUS::LINE_OK;
}

my_http_task::~my_http_task()
{    
    delete m_url;
    delete m_version;
    delete m_host;
}

void my_http_task::close_http_task()
{

}

void my_http_task::process_http_task(my_task_list::Webserver_task* task)
{
    HTTP_CODE res = HTTP_CODE::NO_REQUEST;
    read_http_task(res,task);
    //process

    write_http_task(res,task);
}

void my_http_task::read_http_task(HTTP_CODE& res,my_task_list::Webserver_task* task)
{
    //process

    if(task->str_in.size() >= 0)
    {
        res = GET_REQUEST;
    }    
}

void my_http_task::write_http_task(HTTP_CODE& res,my_task_list::Webserver_task* task)
{
    if(res == GET_REQUEST)
    {
        task->str_out = "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/plain\r\n"
                        "Content-Length: 13\r\n"
                        "Connection: close\r\n"
                        "\r\n"
                        "Hello, World!";
    }
    else
    {
        task->str_out = "HTTP/1.1 400 Bad Request\r\n"
                        "Connection: close\r\n"
                        "\r\n";
    }
    my_task_list::get_instance()->push_write_task(task);
}


void my_http_task::reset()
{
    mysql = nullptr;
    my_method = METHOD::GET;
    my_check_state = CHECK_STATE::CHECK_STATE_REQUESTLINE;
    my_http_code = HTTP_CODE::NO_REQUEST;
    my_line_status = LINE_STATUS::LINE_OK;
}
