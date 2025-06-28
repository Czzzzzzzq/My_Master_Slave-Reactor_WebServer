## 使用主从Reactor + 线程池 + 主从状态机 实现的高性能Web服务器
使用C++11标准，使用多种锁。
![image](https://github.com/user-attachments/assets/69d2329b-8af8-4158-8293-14597145d416)

## 方法
下载完在webserver目录下
输入 make 之后将自行编译、
输入 ./my_server 将运行服务器
输入 ab -n 100000 -c 1000 http://127.0.0.1:8080/ 进行压测

//目前正在完善
