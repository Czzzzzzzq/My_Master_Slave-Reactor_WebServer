#include "my_webserver.h"
#include "my_thread_pool.h"
#include "my_sql_thread_pool.h"
#include "my_log.h"
#include "my_http.h"

int main(void)
{
    Webserver_reactor server;

    server.init_Webserver_reactor(8080,3306,"root","123456789","test",1,1,8,8,5,false);

    server.start_Webserver_reactor();
    
    return 0;
} 
