#include "url.h"
#include "dso.h"

static queue<Surl*> surl_queue;   // 原始url队列
static queue<Url*> ourl_queue;
