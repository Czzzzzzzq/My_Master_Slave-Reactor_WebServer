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

}

void my_http_task::process_http_task(my_task_list::Webserver_task* task)
{
    read_http_task(task);
    write_http_task(task);
}

my_http_task::LINE_STATUS my_http_task::read_one_line(my_task_list::Webserver_task* task) {
    int pos = task->str_in.find("\r\n", buffer_read_pos);    // 查找 \r\n 序列
    if(pos == -1)
    {
        return LINE_STATUS::LINE_BAD;
    }
    task->str_in[pos] = '\0';
    task->str_in[pos + 1] = '\0';
    buffer_read_pos = pos + 2;
    return LINE_STATUS::LINE_OK;
}

void my_http_task::read_http_task(my_task_list::Webserver_task* task) {
    buffer_read_start = 0;
    my_check_state = CHECK_STATE::CHECK_STATE_REQUESTLINE;
    my_http_code = HTTP_CODE::NO_REQUEST;
    
    while (my_http_code == HTTP_CODE::NO_REQUEST || (my_line_status = read_one_line(task)) == LINE_STATUS::LINE_OK)
    {
        if(my_http_code == HTTP_CODE::BAD_REQUEST)
        {
            break;
        }

        if(buffer_read_start == buffer_read_pos)
        {
            buffer_read_line = "";
        }else{
            buffer_read_line = task->str_in.substr(buffer_read_start, buffer_read_pos - buffer_read_start - 1);
            buffer_read_start = buffer_read_pos;
        }

        switch (my_check_state) {

            case CHECK_STATE::CHECK_STATE_REQUESTLINE: {
                my_http_code = read_request();
                break;
            }
            case CHECK_STATE::CHECK_STATE_HEADER: {
                my_http_code = read_header();
                break;
            }
            case CHECK_STATE::CHECK_STATE_CONTENT: {
                my_http_code = read_content(task);
                break;
            }
            default: {
                my_http_code = HTTP_CODE::BAD_REQUEST;
            }
        }
    }
}

my_http_task::HTTP_CODE my_http_task::read_request() {
    int first_space = buffer_read_line.find(' ');
    if (first_space == -1) {
        return HTTP_CODE::BAD_REQUEST;
    }
    string method_str = buffer_read_line.substr(0, first_space);   
    
    if (method_str == "GET") {
        my_method = METHOD::GET;
    } else if (method_str == "POST") {
        my_method = METHOD::POST;
    } else{
        my_method = METHOD::ERROR;
    }
    
    int second_space = buffer_read_line.find(' ', first_space + 1);
    if (second_space == -1) {
        return HTTP_CODE::BAD_REQUEST;
    }
    int third_space = buffer_read_line.find('\0', second_space + 1);
    if (third_space == -1) {
        return HTTP_CODE::BAD_REQUEST;
    }
    if(buffer_read_line[second_space] == ' ')
    {
        second_space++;
    }

    my_version = buffer_read_line.substr(second_space, third_space - second_space);
    
    // 检查版本格式
    if (my_version != "HTTP/1.0" && my_version != "HTTP/1.1") {
        return HTTP_CODE::BAD_REQUEST;
    }
    
    my_check_state = CHECK_STATE::CHECK_STATE_HEADER;
    return HTTP_CODE::NO_REQUEST;
}

my_http_task::HTTP_CODE my_http_task::read_header() {
    if (buffer_read_line.empty()) {
        my_check_state = CHECK_STATE::CHECK_STATE_CONTENT;
        return HTTP_CODE::NO_REQUEST;
    }
    
    int colon_pos = buffer_read_line.find(':');
    if (colon_pos == -1) {
        return HTTP_CODE::BAD_REQUEST;
    }
    int colon_pos_end = buffer_read_line.find('\0', colon_pos + 1);
    if (colon_pos_end == -1) {
        return HTTP_CODE::BAD_REQUEST;
    }
    std::string header_name = buffer_read_line.substr(0, colon_pos);
    std::string header_value = buffer_read_line.substr(colon_pos + 1, colon_pos_end - colon_pos);

    // 处理 Host 头部
    if (header_name == "Host" || header_name == "host") {
        my_host = header_value;
    }
    // 处理 Content-Type 头部
    if (header_name == "Content-Type" || header_name == "content-type") {
        my_content_type = header_value;
    }
    // 处理 Content-Length 头部
    if (header_name == "Content-Length" || header_name == "content-length") {
        my_content_length = std::stoi(header_value);
    }
    // 处理 Connection 头部
    if (header_name == "Connection" || header_name == "connection") {
        if (header_value == "keep-alive" || header_value == "Keep-Alive") {
            my_keep_alive = true;
        }
    }
    return HTTP_CODE::NO_REQUEST;
}
my_http_task::HTTP_CODE my_http_task::read_content(my_task_list::Webserver_task* task) {
    my_content = task->str_in.substr(buffer_read_pos);
    return HTTP_CODE::GET_REQUEST;
}


void my_http_task::write_http_task(my_task_list::Webserver_task* task)
{
    if(my_http_code == GET_REQUEST)
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
}

void my_http_task::reset()
{
    mysql = nullptr;
    my_method = METHOD::GET;
    my_check_state = CHECK_STATE::CHECK_STATE_REQUESTLINE;
    my_http_code = HTTP_CODE::NO_REQUEST;
    my_line_status = LINE_STATUS::LINE_OK;

}

