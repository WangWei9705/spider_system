#pragma once
#ifndef URL_H
#define URL_H

#include <event.h>    // libevent库
#include <evdns.h>    // libevent库中用于进行DNS解析
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <regex.h>   // C语言中的正则表达式
#include <queue>
#include <map>
#include <string>
using namespace std;

#include "spider.h"
#include "bloomfilter.h"

// 最大链接数
#define MAX_LINK_LEN 128  

#define TYPE_HTML 0
#define TYPE_IMAGE 1

typedef struct Surl {
  char* url;   // 原始url
  int level;   // 深度
  int type;    // 类型：HTML或者IMAGE

}Surl;


// 解析后的url
typedef struct Url {
  char* domain;   // 域名
  char* path;     // 路径
  int port;      // 端口
  char* ip;      // ip
  int level;    // 深度
} Url;

// 将url通信的套接字与url关联起来
typedef struct evso_arg{
  int fd;
  Url* Url;
}evso_arg;

extern void* urlparser(void* arg);    // 原始字符串解析
extern int extract_url(regex_t* re, char* str, Url* domain);   // 提取页面中的新url

// surl 与 ourl的操作
extern void push_surlqueue(Surl* url);   // 将原始url加入到url队列中
extern int is_surlqueue_empty();         // 判断原始Url队列是否为空
extern int is_ourlqueue_empty();        // 判断解析之后的队列知否为空
extern int get_surlqueue_size();
extern int get_ourlqueue_size();
extern Url* pop_ourlqueue();     // 解析之后的url出队

extern char* attach_domain(char* url, const char* domain);     // 检测域名是否完整
extern int iscrawled(char* url);    // 爬蛋是否被抓取过
extern char* url2fn(const Url* ourl);     // url关联句柄
extern char* url_normalized(char* url);   // url切割

extern void free_url(Url* ourl);          // 释放url


#endif
