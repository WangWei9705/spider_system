#include <cstdio>
#include <sys/epoll.h>
#include <sys/resource.h>   // 资源操作
#include <sys/time.h>
#include <getopt.h>   // 用于解析命令行参数
#include <fcntl.h>
#include <signal.h>

#include "spider.h"
#include "threads.h"
#include "qstring.h"

int g_epfd;   // epoll 句柄
config* g_conf;   // 配置文件句柄
extern int g_cur_thread_num;  // 当前任务数

static int set_nofile(rlim_t limit);

// 设置守护进程
// 创建子进程、父进程退出
static void daemonize();

// 当前url状态
static void stat(int sig);
static int set_ticker(int second);

// 版本号信息
static void version() {
  printf("Version: spider 1.0 \n");
  exit(1);
}

// 帮助手册
static void usage() {
  printf("Usage: ./spider [Option] \n"
      "\nOptions:\n"
      " -h\t: this is help"
      " -v\t: print spider's version\n"
      " -d\t: run program as a daemon process\n\n"
      );
  exit(1);
}


int main(int argc, char* argv[]) {
  struct epoll_event events[10];   // epoll 时间监控数组
  int daemonized = 0;
  char ch;

  // 主程序流程
  // 1、 解析命令行参数
  
  while((ch = getopt(argc, (char* const*)argv, "vhd")) != -1) {
    switch(ch) {
      case 'v':
        version();
        break;
      case 'd':
        daemonized = 1;
        break;
      case 'h':
      case '?':
      default:
        usage();
    }
  }

  // 2、读取配置文件，提取url种子
  g_conf = initconfigparse();
  loadconfig(g_conf);

  // 设置最大能打开的文件数
  set_nofile(1024);

  // 3、载入动态模块
  vector<char*>::iterator it = g_con->module_name.begin();
  for(; it != g_con->module_name.end(); it++) {
    dso_load(g_con->module_path, *it);
  }

  // 4、添加爬虫种子
  if(g_conf->url_seed == nullptr) {
    SPIDER_LOG(SPIDER_LEVEL_ERROR, "There have no seeds !");
  } else {
    int c = 0;
    char** splits = strsplit(g_conf->url_seed, '.', &c, 0);
    while(--c) {
      Surl* srul = (Surl*)malloc(sizeof(Surl));
      surl->url = url_normalized(strdup(splits[c]));
      surl->level = 0;
      surl->type = TYPE_HTML;
      if(surl->url != nullptr)
        push_surlqueue(surl);
    }
  }

  // 守护进程模式
  if(daemonized)
    daemonize();

  // 设定下载路径
  chdir("download");

  // 启动用于解析DNS的线程
  int err = -1;
  if((err = create_thread(urlparse, NULL, NULL, NULL)) < 0) {
    SPIDER_LOG(SPIDER_LEVEL_ERROR, "Create urlparse thread fail: %s", strerror(err));
  }

  // 一直从url管理器取url种子，进行DNS解析,直到url队列为空
  int try_num = 1;
  while(try_num < 8 && is_ourlqueue_empty())
    usleep((1000 << try_num++));

  if(try_num >= 8)
    SPIDER_LOG(SPIDER_LEVEL_ERROR, "NO url, DNS parse error !");


  
  if(g_conf->status > 0) {
    signal(SIGALRM, stat);
    set_ticker(g_conf->status);
  }

  // 开始创建epoll,从url队列中取出一个url重建socket通信，将创建的socket用epoll进行事件监控
  int url_num = 0;
  g_epfd = epoll_create(g_conf->max_job_num);

  while((url_num++) < g_conf->max_job_num) {
    if(attach_epool_task() < 0)
      break;
  }

  // epoll wait
  while(1) {
    // 10 为最大监控事件数， 2000为监控时间
    int n = epoll_wait(g_epfd, events, 10 , 2000);
    printf("epoll: %d\n", \n);
    if(n == -1)
      printf("epoll errno: %s\n", strerror(errno));
    fflush(stdout);

    if(n <= 0) {
      if(g_cur_thread_num <= 0 && is_ourlqueue_empty() && is_surlqueue_empty()) {
        sleep(1);
        if(g_cur_thread_num <= 0 &&is_ourlqueue_empty() && is_surlqueue_empty())
          break;
      }
    }

    // 循环判断是否为关心事件
    for(int i = 0; i < n; i++) {
      evso_arg* arg = (evso_arg*)(events[i].data.ptr);
      if((events[i].events & EPOLLERR) || 
          (events[i].events & EPOLLHUP) || 
          (!(events[i].events & EPOLLIN))) {
        SPIDER_LOG(SPIDER_LEVEL_WARING, "epoll fail, close socket %d", arg->fd);
        close(arg->fd);
        continue;
      }

      
      // 删除非关心事件
      epoll_ctl(g_epfd, EPOLL_CTL_DEL, arg->fd, &events[i]);
      
      printf("hello epoll: event = %d\n", events[i].events);
      fflush(stdout);
      create_thread(recv_response, arg, NULL, NULL);
    }
  }

  SPIDER_LOG(SPIDER_LEVEL_DEBUG, "Task done !");
  close(g_epfd);   // 使用完毕之后必须关闭
  return 0;
}



int attach_epoll_task() {
  struct epoll_event ev;
  int sock_rv;
  int sockfd;
  // 从url队列中取url
  Url* ourl = pop_ourlqueue;
  if(ourl == nullptr) {
    SPIDER_LOG(SPIDER_LEVEL_WARING, "Pop ourlqueue fail !");
    return -1;
  }
  // 同得到的url创建socket通信
  if((sock_rv = build_connect(&sockfd, ourl->ip, ourl->port)) < 0) {
    SPIDER_LOG(SPIDER_LEVEL_WARING, "Build socket connect fail: %s", ourl->ip);
    return -1;
  }

  // 设置非阻塞
  set_nonblocking(sockfd);

  if((sock_rv = send_request(sockfd, ourl)) < 0) {
    SPIDER_LOG(SPIDER_LEVEL_WARING, "Send socket request fail: %s", ourl->ip);
    return -1;
  }

  evso_arg* arg = (evso_arg*)calloc(1, sizeof(evso_arg));
  arg->fd = sockfd;
  arg->fd = sockfd;
  arg->url = ourl;
  ev.data.ptr = arg;
  ev.events = EPOLLIN | EPOLLET;   // 读事件、边缘触发
  // 添加关心事件
  if(epoll_ctl(g_epfd, EPOLL_CTL_ADD, sockfd, &ev) == 0)
    SPIDER_LOG(SPIDER_LEVEL_DEBUG,"ATTACH an epoll event success !");
  else {
    SPIDER_LOG(SPIDER_LEVEL_WARING, "Attach an epoll event fail !");
    return -1;
  }

  // 关注一个事件就创建一个线程去处理
  ++g_cur_thread_num;
  return 0;
} 


static int set_nofile(rlim_t limit) {
  struct rlimit r1;
  if(getrlimit(RLIMIT_NOFILE, &r1) < 0) {
    SPIDER_LOG(SPIDER_LEVEL_WARING, "getrlimit fail !");
    return -1;
  }

  if(limit > r1.rlim_max) {
    SPIDER_LOG(SPIDER_LEVEL_WARING, "limit should NOT be greater than %lu", r1.rlim_max);
    return -1;
  }

  r1.rlim_cur = limit;
  if(setrlimit(RLIMIT_NOFILE, &r1) < 0) {
    SPIDER_LOG(SPIDER_LEVEL_WARING, "setrlimit fail !");
    return -1;
  }
  return 0;
}

// 设置守护进程
// 创建子进程、父进程退出
static void daemonize() {
  int fd;
  if(fork() != 0)
    exit(0);
  setsid();
  SPIDER_LOG(SPIDER_LEVEL_INFO, "守护进程 pid = %d", (int)getpid());

  // 重定向stdin，stdout,stderr到dev/null
  if((fd = open("/dev/null", O_RDWR, 0)) != -1) {
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);

    // 除了标准输入、输出、错误，其他都关闭
    if(fd > 2)
      close(fd);
  }

  // 重定向stdout到日志文件
  if(g_conf->logfile != nullptr && (fd = open(g_conf->logfile, O_RDWR | O_APPEND | O_CREAT, 0)) != -1) {
    dup2(fd, STDOUT_FILENO);
    if(fd > 2)
      close(fd);
  }
}

// 当前url状态
static void stat(int sig) {
  SPIDER_LOG(SPIDER_LEVEL_DEBUG, 
      "cur_thread_num = %d\tsurl_num = %d\tourl_num = %d",
      g_cur_thread_num,
      get_surl_queue_size(),
      get_ourl_queue_size()
      );
}
static int set_ticker(int second) {
  // 周期性事件设置
  struct itimerval itimer;
  itimer.it_interval.tv_sec = (long)second;
  itimer.it_interval.tv_usec = 0;
  itimer.it_value.tv_sec = (long)second;
  itmier.it_value.tv_usec = 0;

  return setitimer(ITIMER_REAL, &itimer, NULL);

}
