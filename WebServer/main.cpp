#include "my_webserver.h"
#include "my_thread_pool.h"
#include "my_sql_thread_pool.h"
#include "my_log.h"
#include "my_http.h"

int main(void)
{
    /*
    my_Log::get_instance()->init("./web_log/", "log", 1024);
    for(int i = 0; i < 10; i++){
        LOG_INFO("test %d",i);
    }
    my_Log::get_instance()->flush();
    */
    Webserver_reactor server;
    server.init_Webserver_reactor(8080,3306,"root","123456789","test",1,1,8,8,true);
    server.start_Webserver_reactor();
    cout << "return to main" << endl;
    return 0;
} 
