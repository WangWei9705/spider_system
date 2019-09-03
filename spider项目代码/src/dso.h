#pragma once

#ifndef DSO_H
#define DSO_H

#include <vector>
using namespace std;

#define MODULE_OK 0
#define MODULE_ERR 1

#define MAXJOR_NUM 20190820    // 最大任务数
#define MINJOB_NUM 0           // 最小任务数

// 用于统计信息的时间戳
#define  STANDARD_MODULE_STUFF MAXJOR_NUM, \
                               MINJOB_NUM, \
                               __FILE__

// 模块信息
typedef struct Module{
  int varsion;   // 版本
  int minor_version;
  const char* name;   // 模块名
  void (*init)(Module*);  // 每个模块的初始化函数，用于在载入模块时对其进行初始化
  int (*handle)(void*);  // 模块入口
} Module;

// 处理url模块
extern vector<Module*> modules_pre_surl; 
// 初始化
#define SPIDER_ADD_MODULE_PRE_SURL(module) do{\
  modules_pre_surl.push_back(module);   \
} while(0)

// 这个模块用于处理http的报头
extern vector<Module*> modules_post_header;
#define SPIDER_ADD_MODULE_POST_HEADER(module) do {\
  modules_post_header.push_backback(module); \
} while(0)

// 这个模块用于处理完成后的html
extern vector<Module*> modules_post_html;

#define SPIDER_ADD_MODULE_POST_HTML(module) do{\
  modules_post_html.push_back(module); \
} while(0)

// 当程序启动后动态加载的模块
extern Module* dso_load(const char* path, const char* name);
#endif
