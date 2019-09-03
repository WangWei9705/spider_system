#pragma once
#ifndef CONFIG_H
#define CONFIG_H
#include <vector>
using namespace std;

#define MAX_CON_LEN 1024
#define CON_FILE "../speider.conf"

// 日志解析
typedef struct configparse{
  int max_job_num;  // 最大任务数
  char* url_seed;  // url种子
  char* logfile;   // 日志文件名称
  int log_level;    // 日志等级
  int max_deepth;   // 最大抓取深度
  char* module_path;  // 模块路径
  vector<char*> module_name;   // 模块名称
  vector<char*> file_types;    // 文件类型
  int status;    // 抓取状态  0未抓取   1 抓取成功  -1 抓取失败
}config;

// 初始化配置文件
extern config* initconfigparse();    

// 加载配置文件
extern void loadconfig(config* conf);
#endif
