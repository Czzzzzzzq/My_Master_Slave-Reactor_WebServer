CXX = g++
CXXFLAGS = -I. -Ihttp -Ilog -ICGImysql -Ithreadpool -Itask -Iwebserver_reactor -Itimer -std=c++11 -Wall -Wextra -pedantic -g
LDFLAGS = -lpthread -lmysqlclient

SRCS = main.cpp \
       http/my_http.cpp \
       log/my_log.cpp \
       log/my_log_task.cpp \
       CGImysql/my_sql_thread_pool.cpp \
       threadpool/my_thread_pool.cpp \
       task/my_task_list.cpp \
       timer/my_timer.cpp \
       webserver_reactor/my_webserver.cpp

OBJS = $(SRCS:.cpp=.o)
TARGET = myserver

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
