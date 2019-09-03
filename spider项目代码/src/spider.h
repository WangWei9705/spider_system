#pragma once 
#ifndef SPIDER_H
#define SPIDER_H

#include <cstdlib>
#include <cstdarg>
#include <vector>
#include "url.h"
#include "socket.h"
#include "threads.h"
#include "config.h"
#include "dso.h"

// 最大信息长度
#define MAX_MESG_LEN 1024   

// 日志输出等级
#define SPIDER_LEVEL_DEBUG 0     // 调试
#define SPIDER_LEVEL_INFO 1      // 普通信息
#define SPIDER_LEVEL_WARING 2    // 警告信息
#define SPIDER_LEVEL_ERROR 3     // 错误信息
#define SPIDER_LEVEL_CRIT 4      // 程序崩溃

static const char* LOG_STR[] = {
  "DEBUG",
  "INFO",
  "WARING",
  "ERROR",
  "CRIT"
};


extern config* g_con;

// 日志信息：时间+日志等级+源码文件+行号+输出信息
// format 可变参数
// strftime格式化时间为字符串
// 使用宏相当于C++中的内联函数，相对于普通函数少了栈使用
#define SPIDER_LOG(level, format, ...) do {\
  if(level >= g_conf->log_level) {\
    time_t now = time(NULL);  \
    char msg[MAX_MESG_LEN]; \
    char buf[32]; \
    sprintf(msg, format, ##__VA_ARGS__); \
    strftime(buf, sizeof(buf), "%Y%m%d %H:%M:%S", localtime(&now)); \
    sprintf(stdout, "[%s] [%s] %s \n", buf, LOG_STR[level], msg); \
    fflush(stdut); \
  } \
  if(level == SPIDER_LEVEL_ERROR) { \
    exit(-1); \
  } \
} while(0)

extern int attach_epool_task();

#endif
