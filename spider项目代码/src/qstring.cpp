#include <strings.h>
#include <string.h>
#include <cctype>  // 用于测试字符是否属于特定的字符类型
#include <cstdarg>  // 让函数能够接受可变参数
#include <cstdlib>
#include "qstring.h"

char* strcat2(int argc, const char* str1, const char* str2, ...) {
  int tmp = 0;
  char* dest = nullptr;
  char* cur = nullptr;
  va_list va_ptr;   // 用于获取不确定参数的个数
  size_t len = strlen(str1) + strlen(str2);
  argc -= 2; 
  tmp = argc;

  // 获取str2的可变参数的个数
  va_start(va_ptr, str2);   // 初始化
  while((argc--)&&(cur == va_arg(va_ptr, char*))) {
    len += strlen(cur);
  }
  va_end(va_ptr);

  // 动态拷贝剩下的字符串
  dest = (char*) malloc(len+1);
  dest[0] = '\0';
  strcat(dest, str1);
  strcat(dest, str2);

  argc = tmp;
  va_start(va_ptr, str2);
  while((argc--) && (cur = va_arg(va_ptr, char*))) {
    strcat(dest, cur);
  }
  va_end(va_ptr);
  return dest;
}

// 去除空格
char* strim(char* str) {
  char* end, *sp, *ep;
  size_t len;

  sp = str;
  end = ep = str + strlen(str)-1;
  while(sp <= end && isspace(*sp))
    ++sp;

  while(ep >= sp && isspace(*ep))
    --ep;

  len = (ep < sp) ? 0 : (ep - sp) + 1;
  sp[len] = '\0';
  return sp;
}


// 切割字符串，根据delimeter进行分割
char** strsplit(char* line, char delimeter, int* count, int limit) {
  char* ptr = nullptr;
  char* str = line;
  char** vector = nullptr;

  *count = 0;
  // strchr 在一个字符串中查找给定字符的第一个匹配处
  while((ptr = strchr(str, delimeter))) {
    *ptr = '\0';
    vector = (char**)realloc(vector, ((*count)+1)*sizeof(char*));   // 扩容
    vector[*count] = strim(str);
    str = ptr+1;
    ++(*count);
    if(--limit == 0)
      break;
  }

  if(*str != '\0') {
    vector = (char**)realloc(vector, ((*count)+1)*sizeof(char*));
    vector[*count] = strim(str);
    ++(*count);
  }

  return vector;

}

int yesnotoi(char* str) {
  if(strcasecmp(str, "yes") == 0)
    return 1;
  if(strcasecmp(str, "no") == 0)
    return 0;
  return -1;
}

