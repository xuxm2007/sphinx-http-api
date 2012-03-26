/*******************************
 * utf - 8
 * sphinx http api
 * hontlong@gmail.com 2012 - 02
 * Copyright [2012] zhaigy
 * *****************************/
// 搜索功能补全
// 错误异常返回值检查
// 配置文件，记录日志
// 统计、调试功能
// 连接池
// 增加信息、统计、调试页面
// 调试
// 测试
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <string>
#include <map>

#include "../libsphinxclient/sphinxclient.h"
#include "../libthreadpool/CThread.h"
#include "../libjson/json.h"
#include "../liblog/log.h"
#include "./utils.h"
#include "./config.h"
#include "./sphinx_query_data.h"
#include "./search_result.h"
#include "./buffer.h"
#include "./http.h"

using namespace std;

/**
 * 记录用户的输入参数
 */
class GlobalParameter {
  public:
    GlobalParameter(): is_daemon(false),
    host("0.0.0.0"),
    port(2046),
    proxyed_host("localhost"),
    proxyed_port(9312),
    log_path_name("./logs/proxy.log"),
    pid_path_name("./logs/proxy.pid"),
    thread_pool_min(3),
    thread_pool_max(100),
    log_level("INFO") {
    }

    // 是否是后台执行
    bool is_daemon;
    string host;
    int port;
    // 代理的sphinx的ip
    string proxyed_host;
    // dialing的sphinx的端口
    int proxyed_port;
    string log_path_name;
    string pid_path_name;
    int thread_pool_min;
    int thread_pool_max;
    string log_level;
};

/**
 * 记录程序运行统计信息
 */
class Statistic {
  public:
    Statistic():query_count(0),
    query_error_count(0),
    start_time(time(NULL)) {
    }
    // 查询总次数
    // volatile int64_t query_count;  // why can't use volatile?
    int64_t query_count;
    // 查询出错次数
    // volatile int64_t query_error_count;
    int64_t query_error_count;
    // 启动时间
    time_t start_time;
};

struct SockConnection {
  int fd;
  // buffer 需要自己定义
  struct buffer *inbuf;
  struct buffer *outbuf;
};

/**
 * 自定义结构保存client信息
 */
struct ClientInfo {
  char ip[50];
  struct SockConnection conn;
};

/********************全局变量**********************/
static bool g_running = false;
static Statistic * gp_statistic = NULL;
static GlobalParameter * gp_parameter = NULL;
static CThreadPool * gp_thread_pool = NULL;
static map<pthread_t, sphinx_client *> g_sphinx_client_map;
// sphinx client池的互斥锁
// 锁的包装，或许更好
static pthread_mutex_t g_sphinx_client_mutex = PTHREAD_MUTEX_INITIALIZER;
/***********************LOG************************/
CLog * gp_log = NULL;
#define LOG(level, printout, fmt...) \
  do { \
    if (gp_log) gp_log->level(fmt); \
    if (strcmp(#level, "error") == 0) { \
      gp_log->level("file:%s, func:%s, line:%d", \
            __FILE__, __FUNCTION__, __LINE__); \
    } \
    if (gp_parameter != NULL && gp_parameter->is_daemon) { \
      fprintf(printout, fmt); \
      fprintf(printout, "\n"); \
    } \
  }while (0)
#define ERROR_LOG(fmt...) LOG(error, stderr, fmt)
#define WARN_LOG(fmt...) LOG(warn, stderr, fmt)
#define INFO_LOG(fmt...) LOG(info, stdout, fmt)
#define DEBUG_LOG(fmt...) LOG(debug, stdout, fmt)
#define TRACE_F DEBUG_LOG("function:%s, line:%d", __FUNCTION__, __LINE__)
// #define TRACE_F
/**************************************************/

/**
 * 实现从输入到查询数据的初步转换
 */
int parserQuery(keyvalq & http_query, QueryData & qd) {
  // 已经处理全部预定义Key
  for (keyval * kv = http_query.tqh_first, *kvEnd = *http_query.tqh_last;
        kv != 0; kv = kv->next.tqe_next) {
    DEBUG_LOG("kv->name:%s, kv->value:%s.", kv->name, kv->value);
#define NO_SUPPORT_QUERY_PARAM \
        WARN_LOG("不支持的输入值, key:%s, value:%s", kv->name, kv->value);
    switch (kv->name[0]) {
      case 'c':
        if (strcmp("connecttimeout", kv->name) == 0) {
          qd.connecttimeout = atoi(kv->value);
        } else {
          NO_SUPPORT_QUERY_PARAM;
        }
        break;
      case 'f':  // filter
        if (strcmp("filter", kv->name) == 0) {
          qd.filters.push_back(kv->value);
        } else if (strcmp("fieldweights", kv->name) == 0) {
          qd.fieldweights = kv->value;
        } else {
          NO_SUPPORT_QUERY_PARAM;
        }
        break;
      case 'g':
        if (strcmp("groupby", kv->name) == 0) {
          qd.groupby = kv->value;
        } else if (strcmp("groupdistinct", kv->name) == 0) {
          qd.groupdistinct = kv->value;
        } else {
          NO_SUPPORT_QUERY_PARAM;
        }
        break;
      case 'i':  // index, idrange
        if (strcmp("index", kv->name) == 0) {
          qd.index = kv->value;
        } else if (strcmp("idrange", kv->name) == 0) {
          qd.idrange = kv->value;
        } else {
          NO_SUPPORT_QUERY_PARAM;
        }
        break;
      case 'm':
        if (strcmp("maxquerytime", kv->name) == 0) {
          qd.maxquerytime = atoi(kv->value);
        } else if (strcmp("matchmode", kv->name) == 0) {
          qd.matchmode = kv->value;
        } else {
          NO_SUPPORT_QUERY_PARAM;
        }
        break;
      case 'q':  // q
        if (strlen(kv->name) == 1) {
          qd.q = kv->value;
        } else {
          NO_SUPPORT_QUERY_PARAM;
        }
        break;
      case 'r':  // rows
        if (strcmp("rows", kv->name) == 0) {
          qd.rows = atoi(kv->value);
        } else if (strcmp("retries", kv->name) == 0) {
          qd.retries = atoi(kv->value);
        } else if (strcmp("rankingmode", kv->name) == 0) {
          qd.rankingmode = kv->value;
        } else {
          NO_SUPPORT_QUERY_PARAM;
        }
        break;
      case 's':  // start
        if (strcmp("start", kv->name) == 0) {
          qd.start = atoi(kv->value);
        } else if (strcmp("sortmode", kv->name) == 0) {
          qd.sortmode = kv->value;
        } else if (strcmp("select", kv->name) == 0) {
          qd.select = kv->value;
        } else {
          NO_SUPPORT_QUERY_PARAM;
        }
        break;
      default:
        NO_SUPPORT_QUERY_PARAM;
    }
#undef NO_SUPPORT_QUERY_PARAM

    if (kv == kvEnd) {
      break;
    }
  }
  return 0;
}

void disconnect_conn(struct SockConnection *conn) {
  close(conn->fd);
  if (conn->inbuf) buffer_free(conn->inbuf);
  if (conn->outbuf) buffer_free(conn->outbuf);
}

/**
 * 创建Sphinx Client, 并打开长连接.
 */
static sphinx_client * create_sphinx_client() {
  DEBUG_LOG("%s", __FUNCTION__);
  sphinx_client * client = sphinx_create(SPH_TRUE);
  if (client != NULL) {
    if (sphinx_set_server(client, gp_parameter->proxyed_host.c_str(),
            gp_parameter->proxyed_port) && sphinx_open(client)) {
      return client;
    }
    // 默认1秒钟超时
    // sphinx_set_connect_timeout(client, 5);
    WARN_LOG("sphinx open fail:%s", sphinx_error(client));
    sphinx_destroy(client);
  }
  return NULL;
}

/**
 * 加锁，获取有效的Sphinx Client。
 */
static sphinx_client * get_sphinx_client(pthread_t tid) {
  TRACE_F;
  if (pthread_mutex_lock(&g_sphinx_client_mutex) != 0) {
    WARN_LOG("Fial to lock sphinx client mutex.");
    return NULL;
  }
  map<pthread_t, sphinx_client *>::iterator it = g_sphinx_client_map.find(tid);
  sphinx_client * client = NULL;
  if (it != g_sphinx_client_map.end()) {
    client = it->second;
  } else {
    client = create_sphinx_client();
    if (client != NULL) {
      g_sphinx_client_map[tid]=client;
      DEBUG_LOG("create sphinx client:tid=%lu, client pool num=%lu, client=%p",
            tid, g_sphinx_client_map.size(), client);
    }
  }
  if (pthread_mutex_unlock(&g_sphinx_client_mutex) != 0) {
    WARN_LOG("Fial to unlock sphinx client mutex.");
  }
  return client;
}

static sphinx_client * get_thread_sphinx_client() {
  pthread_t tid = pthread_self();
  sphinx_client * client = get_sphinx_client(tid);
  return client;
}

/**
 * 释放并关闭一个sphinx client
 */
static int release_sphinx_client(pthread_t tid) {
  DEBUG_LOG("function:%s, line:%d", __FUNCTION__, __LINE__);
  if (pthread_mutex_lock(&g_sphinx_client_mutex) != 0) {
    WARN_LOG("Fial to lock sphinx client mutex.");
    return -1;
  }
  map<pthread_t, sphinx_client *>::iterator it = g_sphinx_client_map.find(tid);
  sphinx_client * client = NULL;
  if (it != g_sphinx_client_map.end()) {
    client = it->second;
    g_sphinx_client_map.erase(it);
  }
  if (pthread_mutex_unlock(&g_sphinx_client_mutex) != 0) {
    WARN_LOG("Fial to lock sphinx client mutex.");
  }
  if (client != NULL) {
    sphinx_destroy(client);
    INFO_LOG("release_sphinx_client, tid=%lu, client=%p", tid, client);
  } else {
    // WARN_LOG("[%lu] why client is null ?", tid);
  }
  return 0;
}

static void release_thread_sphinx_client() {
  pthread_t tid = pthread_self();
  release_sphinx_client(tid);
}

// 关闭全部连接，退出前释放资源
static void close_all_sphinx_client() {
  if (pthread_mutex_lock(&g_sphinx_client_mutex) != 0) {
    WARN_LOG("Fial to lock sphinx client mutex.");
    return;
  }
  map<pthread_t, sphinx_client *>::iterator it = g_sphinx_client_map.begin();
  for (; it != g_sphinx_client_map.end(); ++it) {
    sphinx_client * client = it->second;
    if (client != NULL) {
      sphinx_destroy(client);
    }
  }
  if (pthread_mutex_unlock(&g_sphinx_client_mutex) != 0) {
    WARN_LOG("Fial to unlock sphinx client mutex.");
  }
}

/**
 * 调用客户端接口查询
 * 抛出异常, string类型, 描述错误原因
 */
void search(const SphinxQueryData & sqd, SearchResult * sr) {
  TRACE_F;
  sphinx_client * const client = get_thread_sphinx_client();
  if (client == NULL) throw string("sphinx client is NULL");

  sphinx_reset_filters(client);
  sphinx_reset_groupby(client);

  sr->set_client(client);

  // filter - 属性值，属性范围，属性范围float, id范围（仅可以设置一个值）
  bool set_ok = false;
  do {
    if (!sqd.filters.empty()) {
      for (list<SphinxQueryData::Filter>::const_iterator
            it = sqd.filters.begin(); it != sqd.filters.end(); ++it) {
        const SphinxQueryData::Filter & filter = *it;
        if (filter.type == SphinxQueryData::Filter::kValue) {
          sphinx_int64_t *values = new sphinx_int64_t[filter.values.size()];
          if (values == NULL) throw string("mem of out");
          for (int i = 0, len = filter.values.size(); i < len; ++i) {
            values[i] = filter.values[i];
          }
          if (sphinx_add_filter(client, filter.attr_name.c_str(),
                  filter.values.size(), values, filter.exclude) != SPH_TRUE) {
            delete [] values;
            break;
          }
          delete [] values;
        } else if (filter.type == SphinxQueryData::Filter::kIntRange) {
          if (sphinx_add_filter_range (client, filter.attr_name.c_str(),
                  filter.int_min, filter.int_max, filter.exclude) != SPH_TRUE) {
            break;
          }
        } else if (filter.type == SphinxQueryData::Filter::kFloatRange) {
          if (sphinx_add_filter_float_range (client, filter.attr_name.c_str(),
                  filter.float_min, filter.float_max, filter.exclude)
                != SPH_TRUE) {
            break;
          }
        } else {
          throw "Filter 未知的设置类型";
        }
      }
    }
    if (sqd.idrange_min > 0L) {
      // id = 0 这个在sphinx中是不合法的
      if (sphinx_set_id_range (client, sqd.idrange_min, sqd.idrange_max)
            != SPH_TRUE) {
        break;
      }
    }
    // 第4个参数是否可以设置为rows*10000?
    if (sphinx_set_limits(client, sqd.start, sqd.rows, sqd.start + sqd.rows, 0)
          != SPH_TRUE) {
      break;
    }
    // fieldweight - 字段数，字段名数组，权重数组（整数）
    if (sqd.fieldweights_num > 0) {
      if (sphinx_set_field_weights(client, sqd.fieldweights_num,
              sqd.fieldweights_fields, sqd.fieldweights_weights) != SPH_TRUE) {
        break;
      }
    }

    if (sqd.retries > 0) {
      // 间隔毫秒
      if (sphinx_set_retries(client, sqd.retries, 100) != SPH_TRUE) {
        break;
      }
    }

    if (sqd.maxquerytime > 0) {
      // 毫秒
      if (sphinx_set_max_query_time(client, sqd.maxquerytime) != SPH_TRUE) {
        break;
      }
    }
    if (sqd.connecttimeout > 0) {
      // 转换为秒
      if (sphinx_set_connect_timeout(client,
              static_cast<float>(sqd.connecttimeout) / 1000) != SPH_TRUE) {
        break;
      }
    }
    // groupby - groupfun , sort字符串
    if (!sqd.groupby_attr.empty()) {
      if (sphinx_set_groupby(client, sqd.groupby_attr.c_str(),
              sqd.groupby_func, sqd.groupby_sort.c_str()) != SPH_TRUE) {
        break;
      }
      if (!sqd.groupdistinct.empty()) {
        if (sphinx_set_groupby_distinct (client, sqd.groupdistinct.c_str())
              != SPH_TRUE) {
          break;
        }
      }
    }

    if (sphinx_set_match_mode(client, sqd.matchmode) != SPH_TRUE
          || sphinx_set_ranking_mode(client, sqd.rankingmode) != SPH_TRUE
          || sphinx_set_sort_mode(client, sqd.sortmode,
            sqd.sortclause.c_str()) != SPH_TRUE) {
      break;
    }

    if (!sqd.select.empty()) {
      if (sphinx_set_select(client, sqd.select.c_str()) != SPH_TRUE) {
        break;
      }
    }
    set_ok = true;
  } while (0);
  if (!set_ok) {
    throw string(sphinx_error(client));
  }
  sphinx_result *res = sphinx_query(client, sqd.q.c_str(),
        sqd.index.c_str(), NULL);
  if (res == NULL) {
    WARN_LOG("query error:%s", sphinx_error(client));
    release_thread_sphinx_client();
    // 可能存在错误的连接
    throw string(sphinx_error(client));
  }
  sr->set_result(res);
}

/**使用jsoncpp实现的转换*/
string convert_to_json_string(const SearchResult & sr) {
  Json::Value root;
  // 拼输出
  root["status_info"]="0 = SEARCHD_OK, 1 = SEARCHD_ERROR, 2 = SEARCHD_RETRY, "
    "3 = SEARCHD_WARNING";
  root["status"] = sr.getStatus();
  if (sr.getStatus() != SEARCHD_OK) {
    WARN_LOG("search result status:%d", sr.getStatus());
  }
  if (sr.getStatus() == SEARCHD_ERROR) {
    root["error"] = sr.getError();
    WARN_LOG("error=[%s]", sr.getError());
  } else if (sr.getStatus() == SEARCHD_RETRY) {
    // 无任何说明
  } else {
    if (sr.getStatus() == SEARCHD_WARNING) {
      root["warning"] = sr.getWarning();
    }
    root["total"] = sr.getTotal();
    root["time"] = sr.getTime();
    int matchNumb = sr.getMatchNumb();
    root["matchNumb"] = matchNumb;
    // 匹配集合 root["matches"];
    for (int i = 0; i < matchNumb; ++i) {
      Json::Value match;
      // match["id"] = static_cast<int64_t>(sr.getId(i));
      match["id"] = sr.getId(i);
      int attrNumb = sr.getAttrNumb();
      for (int j = 0; j < attrNumb; ++j) {
        if (sr.getAttrType(j) == (SPH_ATTR_MULTI | SPH_ATTR_INTEGER)) {
          unsigned int *mva = sr.getAttrMultiIntValue(i, j);
          Json::Value array_temp;
          for (unsigned int k = 0; k < mva[0]; ++k) {
            array_temp.append(mva[1 + k]);
          }
          match[sr.getAttrName(j)] = array_temp;
        } else if (sr.getAttrType(j) == SPH_ATTR_FLOAT) {
          match[sr.getAttrName(j)] = sr.getAttrFloatValue(i, j);
        } else {
          // int
          match[sr.getAttrName(j)] = sr.getAttrIntValue(i, j);
        }
      }
      // 这里添加进来
      root["matches"].append(match);
    }

    // for words
    int wordNumb = sr.getWordNumb();
    root["wordNumb"] = wordNumb;
    // words
    for (int i = 0; i < wordNumb; ++i) {
      Json::Value word;
      word["word"] = sr.getWord(i);
      word["hits"] = sr.getHits(i);
      word["docs"] = sr.getDocs(i);
      root["words"].append(word);
    }
  }
  Json::StyledWriter writer;
  return writer.write(root);
}

/**
 * 构造一个简单的错误信息的Json字符串
 */
string make_inner_error_json_string(string e) {
  Json::Value root;
  root["status_info"] = "0 = SEARCHD_OK, 1 = SEARCHD_ERROR; 2 = SEARCHD_RETRY;"
    " 3 = SEARCHD_WARNING";
  root["status"] = SEARCHD_ERROR;
  root["error"] = e;
  Json::StyledWriter json_writer;
  return json_writer.write(root);
}

/**
 * 返回结果必须是json格式，不能抛出异常
 */
bool deal_search_handler(keyvalq & http_query, string & json) {
  try {
    QueryData query_data;
    if (parserQuery(http_query, query_data) != 0 || query_data.q.empty()) {
      ERROR_LOG("查询语句是空的");
      json = make_inner_error_json_string("查询语句是空的");
      return false;
    }
    // 转换为sphinx可用的查询数据格式
    SphinxQueryData sqd(query_data);
    SearchResult searchResult;
    search(sqd, &searchResult);
    json = convert_to_json_string(searchResult);
    return true;
  } catch(const char *e) {
    ERROR_LOG("异常:%s", e);
    json = make_inner_error_json_string(string("异常：") + e);
  } catch(const string & e) {
    ERROR_LOG("异常:%s", e.c_str());
    json = make_inner_error_json_string("异常：" + e);
  } catch(...) {
    ERROR_LOG("未知的异常");
    json = make_inner_error_json_string("未知的异常");
  }
  return false;
}

/**
 * 处理状态请求, 不允许抛出异常
 */
bool deal_status_handler(string * pjson) {
  TRACE_F;
  Json::Value root;
  root["status_info"] = "0 = SEARCHD_OK, 1 = SEARCHD_ERROR; 2 = SEARCHD_RETRY;"
    " 3 = SEARCHD_WARNING";

  string & json = *pjson;
  sphinx_client * const client = get_thread_sphinx_client();
  if (client == NULL) {
    const char * err = "sphinx client 异常";
    json = make_inner_error_json_string(err);
    return false;
  }

  int row_num = 0;
  int col_num = 0;
  char **status = sphinx_status(client, &row_num, &col_num);
  if (!status) {
    root["status"] = SEARCHD_ERROR;
    root["error"] = sphinx_error(client);
  } else {
    root["status"] = SEARCHD_OK;
    for (int i = row_num * col_num - 1; i >= 0; i -= 2) {
      root[status[i - 1]] = status[i];
    }
  }
  sphinx_status_destroy(status, row_num, col_num);

  Json::StyledWriter json_writer;
  json = json_writer.write(root);
  return true;
}

/**
 * 处理统计请求
 */
string deal_statistic_handler(void) {
  Json::Value root;
  root["query_count"] = (long long)gp_statistic->query_count;  // NOLINT
  root["query_error_count"] = (long long)gp_statistic->query_error_count;  // NOLINT
  char time[50];
  root["start_time"] = ctime_r(&gp_statistic->start_time, time);
  Json::StyledWriter json_writer;
  string json = json_writer.write(root);
  return json;
}

static int64_t get_file_size(FILE *fp) {
  int64_t file_size=-1;
  fseek(fp, 0, SEEK_END);
  file_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  return file_size;
}

/**
 * 工具方法：读取文本文件内容到内存, 文本文件要求是小的，最大不能操作1M，
 * 超过仅返回1M内容。
 */
int read_small_text_file(const char * file_name, struct buffer * buf) {
  string file("./web/");
  file += file_name;
  FILE * fp = fopen(file.c_str(), "rb");
  if (fp == NULL) {
    ERROR_LOG("open file %s error.",file.c_str()); 
    return -1;
  }
  int64_t file_size = get_file_size(fp);
  DEBUG_LOG("file:%s, size:%ld", file.c_str(), file_size);
  int size=-1;
  if (file_size < 1024L*1024) {
    char * data = static_cast<char*>(malloc(static_cast<int>(file_size)));
    if (data != NULL) {
      size = fread(data, 1, file_size, fp);
      if (size > 0) {
        buffer_add(buf, data, size);
      }
      free(data);
    }
  }
  fclose(fp);
  DEBUG_LOG("%s:%d", __FUNCTION__, size);
  return size;
}

#define RESP_HEAD(buf, status, status_str, keep_alive, data_length, is_html) \
  do { \
  buffer_add_printf(buf, "HTTP/1.1 %d %s\r\n", status, status_str); \
  buffer_add_printf(buf, "Server: sphinx_http_api\r\n"); \
  add_time_header(buf); \
  if (is_html) {\
    buffer_add_printf(buf, "Content-Type: text/html;charset=UTF-8\r\n"); \
  } else {\
    buffer_add_printf(buf, "Content-Type: text/plain;charset=UTF-8\r\n"); \
  }\
  buffer_add_printf(buf, "Content-Length: %d\r\n", data_length); \
  if (keep_alive) { \
    buffer_add_printf(buf, "Connection: keep-alive\r\n"); \
  } else { \
    buffer_add_printf(buf, "Connection: close\r\n"); \
  } \
  buffer_add(buf, "\r\n", 2); \
}while (0)

static void set_http_response(struct buffer *buf, int status,
      const char *status_str, bool keep_alive, const char *more_info,
      int data_length, bool is_html = false) {
  RESP_HEAD(buf, status, status_str, keep_alive, data_length, is_html);
  buffer_add(buf, more_info, data_length);
}

static void set_http_response_buf(struct buffer *buf, int status,
      const char *status_str, bool keep_alive, struct buffer * body_data,
      bool is_html = false) {
  int data_length = static_cast<int>(body_data->len);
  RESP_HEAD(buf, status, status_str, keep_alive, data_length, is_html);
  buffer_add_buffer(buf, body_data);
}
#undef RESP_HEAD

/**
 * 检查请求是否有效
 @return 有效返回0.
 */
int check_request_valid(const struct http_request *request) {
  if (request->type == HTTP_UNKNOWN) {
    return -1;
  }

  /* HTTP 1.1 needs Host header */
  if (request->ver.major == 1 && request->ver.minor == 1) {
    const char *host = http_get_header_value(request->headers, "Host");
    if (host == 0) {
      return -2;
    }
  }
  return 0;
}

// 返回http请求中路径或者NULL, 调用者负责删除数据
static char * uri_get_path(const char * uri) {
  if (uri ==NULL || strlen(uri) == 0) return NULL;
  const char * s = uri;
  const char * p = strstr(s, "://");
  if (p != NULL) s = p + 3;
  p = strchr(s, '/');
  if (p == NULL) return NULL;
  s = p;
  p = strchr(s, '?');
  if (p == NULL) {
    int len = strlen(s);
    char * data = reinterpret_cast<char *>(malloc(len + 1));
    snprintf(data, len + 1, "%s", s);  // add 0 at last
    return data;
  }

  int len = p - s;
  char * data = reinterpret_cast<char *>(malloc(len + 1));
  strncpy(data, s, len);
  data[len] = 0;

  return data;
}

/**
 * 处理器, 注意, 此函数内部如无系统崩溃，不会报错，所有异常应控制在本函数内部
 */
void http_handler(struct ClientInfo * socket_client, bool * keep_alive) {
  TRACE_F;
  struct http_request *request = NULL;
  char * path = NULL;
  buffer * body_data = NULL;
  try {
    struct SockConnection *conn = &socket_client->conn;
    struct buffer *buf = conn->outbuf;
    request = http_request_parse(conn->inbuf);
    if (request == NULL) {
      const char * info = "ERROR: malloc fail";
      WARN_LOG("%s", info);
      set_http_response(buf, HTTP_BADREQUEST, CODE_STR(HTTP_BADREQUEST), false,
            info, strlen(info));
      goto BUF_OUT;
    }

    // for goto cross variant
    {
      // 对是否保持连接进行处理
      *keep_alive = false;
      http_header * tmp = NULL;
      TAILQ_FOREACH(tmp, request->headers, next) {
        if (strcasecmp(tmp->name, "Connection") == 0) {
          if (strcasecmp(tmp->value, "keep-alive") == 0) {
            *keep_alive = true;
          }
          break;
        }
      }

      {
        int ret = check_request_valid(request);
        if (ret == -1) {
          const char * info = "ERROR: Not Implemented Method";
          WARN_LOG("%s", info);
          set_http_response(buf, HTTP_NOTIMPLEMENT, CODE_STR(HTTP_NOTIMPLEMENT),
                *keep_alive, info, strlen(info));
          goto BUF_OUT;
        } else if (ret == -2) {
          const char * info = "ERROR: Bad Request: needs Host header";
          WARN_LOG("%s", info);
          set_http_response(buf, HTTP_BADREQUEST, CODE_STR(HTTP_BADREQUEST),
                *keep_alive, info, strlen(info));
          goto BUF_OUT;
        }
      }

      path = uri_get_path(request->uri);
      DEBUG_LOG("path:%s, uri:%s", path, request->uri);  // path can is NULL

      body_data = buffer_new();
      if (body_data == NULL) {
        const char * info = "ERROR: malloc fail";
        set_http_response(buf, HTTP_BADREQUEST, CODE_STR(HTTP_BADREQUEST),
              *keep_alive, info, strlen(info));
        goto BUF_OUT;
      }

      bool is_ok = false;
      bool is_html = false;

      if (path != NULL && strcmp(path, "/search") == 0) {
        // 处理检索
        gp_statistic->query_count++;
        char query_str[256];
        // FIXME 这里可以和获取路径的函数合并优化
        {
          // 获取查询语句
          const char * s = request->uri;
          s = strchr(s, '?');
          if (s == NULL) {
            const char * info = "ERROR: parse uri fail";
            WARN_LOG("%s", info);
            set_http_response(buf, HTTP_BADREQUEST, CODE_STR(HTTP_BADREQUEST),
                  *keep_alive, info, strlen(info));
            goto BUF_OUT;
          }
          s += 1;
          const char * e = strrchr(s, '#');
          if (e == NULL) e = s + strlen(s);
          int len = e - s;
          if (len > 255) {
            const char * info = "ERROR: parse uri fail";
            WARN_LOG("%s", info);
            set_http_response(buf, HTTP_BADREQUEST, CODE_STR(HTTP_BADREQUEST),
                  *keep_alive, info, strlen(info));
            goto BUF_OUT;
          }
          memcpy(query_str, s, len);
          query_str[len] = 0;
        }

        keyvalq http_query;
        if (http_parse_query_str(query_str, &http_query) != 0) {
          const char * info = "ERROR: parse uri fail";
          WARN_LOG("%s", info);
          set_http_response(buf, HTTP_BADREQUEST, CODE_STR(HTTP_BADREQUEST),
                *keep_alive, info, strlen(info));
          goto BUF_OUT;
        }

        string json;
        is_ok = deal_search_handler(http_query, json);
        http_clear_headers(&http_query);
        const char * data = json.c_str();
        buffer_add(body_data, data, strlen(data));
      } else {
        if (path == NULL || strlen(path) == 0 ||  strcmp(path, "/") == 0) {
          DEBUG_LOG("/info.html");
          int res = read_small_text_file("info.html", body_data);
          if (res > 0) is_ok = true;
          is_html = true;
        } else if (strcmp(path, "/status") == 0) {
          DEBUG_LOG("/status");
          // 代理的统计信息
          string json;
          is_ok = deal_status_handler(&json);
          const char * data = json.c_str();
          buffer_add(body_data, data, strlen(data));
        } else if (strcmp(path, "/statistic") == 0) {
          DEBUG_LOG("/statistic");
          string json = deal_statistic_handler();
          const char * data = json.c_str();
          buffer_add(body_data, data, strlen(data));
          is_ok = true;
        } else if (strcmp(path, "/debug") == 0) {
          DEBUG_LOG("/debug");
          int res = read_small_text_file("debug.html", body_data);
          if (res > 0) {
            is_ok = true;
          }
          is_html = true;
        } else {
          WARN_LOG("未知的路径");
          // 不支持的请求
          const char * info = "ERROR: HTTP_NOTIMPLEMENT";
          set_http_response(buf, HTTP_NOTIMPLEMENT, CODE_STR(HTTP_NOTIMPLEMENT),
                *keep_alive, info, strlen(info));
          goto BUF_OUT;
        }
      }
      if (is_ok) {
        INFO_LOG("[%s] OK, [%s]", path, request->uri);
        set_http_response_buf(buf, HTTP_OK, CODE_STR(HTTP_OK),
              *keep_alive, body_data, is_html);
      } else {
        WARN_LOG("[%s] Fail, [%s]", path, request->uri);
        *keep_alive = false;
        set_http_response_buf(buf, HTTP_BADREQUEST, CODE_STR(HTTP_BADREQUEST),
              *keep_alive, body_data);
      }
    }
 BUF_OUT:
    // 输出数据
    if (buf->len > 0) {
      DEBUG_LOG("out put response ...");
      buffer_write(buf, conn->fd);
    }
    if (body_data) buffer_free(body_data);
    if (path) free(path);
    if (request) http_request_free(request);
    // FIXME:需要更稳妥的处理异常
    return;
  } catch(string err) {
    ERROR_LOG("%s", err.c_str());
  } catch(...) {
    ERROR_LOG("未知错误");
  }
  gp_statistic->query_error_count++;
  if (body_data) buffer_free(body_data);
  if (path) free(path);
  if (request) http_request_free(request);
}

/**
 * 加载配置文件
 */
bool load_config_file() {
  try {
    const char * config_file = "./etc/proxy.ini";
    Config configSettings(config_file);

    gp_parameter->is_daemon = configSettings.Read("is_daemon",
          gp_parameter->is_daemon);
    gp_parameter->host = configSettings.Read("host",
          gp_parameter->host);
    gp_parameter->port = configSettings.Read("port",
          gp_parameter->port);
    gp_parameter->proxyed_host = configSettings.Read("proxyed_host",
          gp_parameter->proxyed_host);
    gp_parameter->proxyed_port = configSettings.Read("proxyed_port",
          gp_parameter->proxyed_port);
    gp_parameter->pid_path_name = configSettings.Read("pid_path_name",
          gp_parameter->pid_path_name);
    gp_parameter->log_path_name = configSettings.Read("log_path_name",
          gp_parameter->log_path_name);
    gp_parameter->thread_pool_min = configSettings.Read("thread_pool_min",
          gp_parameter->thread_pool_min);
    gp_parameter->thread_pool_max = configSettings.Read("thread_pool_max",
          gp_parameter->thread_pool_max);
    gp_parameter->log_level = configSettings.Read("log_level",
          gp_parameter->log_level);
    return true;
  } catch(Config::File_not_found fnf) {
    fprintf(stderr, "配置文件没有找到\n");
  } catch(...) {
    fprintf(stderr, "未知错误\n");
  }
  return false;
}

/**
 * 检查参数配置，失败就退出程序
 */
static int check_config_parameter() {
  cout << "is_daemon: " << gp_parameter->is_daemon << endl;
  cout << "host: " << gp_parameter->host << ":" << gp_parameter-> port << endl;
  cout << "proxyed_host: " << gp_parameter-> proxyed_host << ":"
    << gp_parameter->proxyed_port << endl;

  if (gp_parameter->host.empty() || gp_parameter->proxyed_host.empty()) {
    cerr << "配置不正确" << endl;
    return 1;
  }
  return 0;
}

void ExecSafeExit(int) {
  // struct timeval tv;
  // tv.tv_sec = 1;
  // tv.tv_usec = 0;
  g_running = false;
}

/**
 * 处理终端消息通知
 */
void SetSignalHandle(void) {
  signal(SIGALRM, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);
  signal(SIGHUP, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
  signal(SIGUSR1, SIG_IGN);
  signal(SIGINT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);

  signal(SIGINT, ExecSafeExit);
  signal(SIGTERM, ExecSafeExit);
  signal(SIGQUIT, ExecSafeExit);
}

// 文件不存在并且创建新的PID文件成功返回0
static int check_pid_file() {
  if (access(gp_parameter->pid_path_name.c_str(), F_OK) == 0) {
    ERROR_LOG("PID[%s]文件已经存在", gp_parameter->pid_path_name.c_str());
    return 1;
  }
  pid_t pid = getpid();
  FILE * pidFile = fopen(gp_parameter->pid_path_name.c_str(), "w");
  if (pidFile == NULL) {
    ERROR_LOG("PID文件创建失败:%s", strerror(errno));
    return 1;
  }
  fprintf(pidFile, "%d", pid);
  fclose(pidFile);
  return 0;
}

void delet_pid_file() {
  if (remove(gp_parameter->pid_path_name.c_str()) < 0) {
    WARN_LOG("PID[%s]文件删除失败", gp_parameter->pid_path_name.c_str());
  }
}

// 函数：设置sock为non - blocking mode
inline int setSockNonBlock(int sock) {
  int flags = fcntl(sock, F_GETFL, 0);
  if (flags >= 0) {
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) >= 0) {
      return 0;
    }
  }
  return -1;
}

inline int set_reuse_addr(int sock) {
  int opt = 1;
  return setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}

void send_bad_req(int fd) {
  const char * bad_msg="error query";
  send(fd, bad_msg, strlen(bad_msg), 0);
}

class SimpleWorkTask: public CTask {
  public:
    explicit SimpleWorkTask(struct ClientInfo * socket_client) {
      this->socket_client_ = socket_client;
    }

    int Run() {
      TRACE_F;
      DEBUG_LOG("%s", "SimpleWorkTask Run: begin...");
      struct SockConnection *conn = &socket_client_->conn;
      DEBUG_LOG("sock fd=%d", conn->fd);
      // 还不能处理成非阻塞的，掌握的知识还不够
      // setSockNonBlock(conn->fd);
      // 必须显式设置为阻塞
      fcntl(conn->fd, F_SETFL, 0);

      for (; true; ) {
        conn->inbuf = buffer_new();
        conn->outbuf = buffer_new();

        bool keep_alive = false;
        int ret = buffer_read(conn->inbuf, conn->fd, 4096);
        DEBUG_LOG("buffer_read:%d", ret);
        if (ret > 0) {
          http_handler(this->socket_client_, &keep_alive);
        }

        buffer_free(conn->inbuf);
        conn->inbuf = NULL;
        buffer_free(conn->outbuf);
        conn->outbuf = NULL;

        if (ret <= 0) {
          break;
        }

        if (!keep_alive) {
          break;
        }
      }
      disconnect_conn(conn);
      free(socket_client_);
      DEBUG_LOG("%s", "SimpleWorkTask Run: end");
      return 0;
    }

  private:
    struct ClientInfo * socket_client_;
};

inline static void fast_close(int fd) {
  if (fd >= 0) {
    if (close(fd) != 0) {
      ERROR_LOG("close fd %d error, errno:%d", fd, errno);
    }
  }
}

/**
 * 返回服务器忙的信息
 */
int response_busynow(struct ClientInfo* socket_client) {
  struct SockConnection *conn = &socket_client->conn;
  conn->inbuf = NULL;
  conn->outbuf = buffer_new();
  const char * info ="service is too busy now!";
  set_http_response(conn->outbuf, HTTP_OK, CODE_STR(HTTP_OK) , false,
        info, strlen(info));
  buffer_write(conn->outbuf, conn->fd);
  buffer_free(conn->outbuf);
  conn->outbuf = NULL;
  disconnect_conn(&socket_client->conn);
  free(socket_client);
  return 0;
}

static void accept_conn_cb(int fd, struct sockaddr_in *address) {
  TRACE_F;
  if (address == NULL) return;
  // 保存client端信息到自定义结构体中
  struct ClientInfo * socket_client = (struct ClientInfo*)malloc(
      sizeof(struct ClientInfo));
  if (socket_client == NULL) {
    WARN_LOG("malloc failed");
    close(fd);
    return;
  }
  /*
     struct sockaddr_in * address_in = address;
     if (!inet_ntop(AF_INET, &(address_in->sin_addr),
     socket_client->ip, sizeof(socket_client->ip))) {
     WARN_LOG("inet_ntop failed");
     close(fd);
     return;
     }
  */
  struct SockConnection *conn = &socket_client->conn;
  conn->fd = fd;
  conn->inbuf = NULL;
  conn->outbuf = NULL;

  SimpleWorkTask * task = new SimpleWorkTask(socket_client);
  int res = gp_thread_pool->AddTask(task);
  if (res < 0) {
    WARN_LOG("server is busy now!");
    response_busynow(socket_client);
    delete task;
  }
}

void service() {
  INFO_LOG("启动服务");
  int epfd            = -1;
  int listenfd        = -1;

#define CLEAN  do { \
  fast_close(listenfd); \
  fast_close(epfd); \
} while (0)

#define CLEAN_R do { \
  CLEAN; \
  return; \
} while (0)

  const int EP_MAX_SIZE  = 10240;
  epfd = epoll_create(EP_MAX_SIZE);
  if (epfd < 0) {
    ERROR_LOG("epoll create fail:%d, errno:%d", epfd, errno);
    CLEAN_R;
  }

  // 创建server的listen socket
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd < 0) {
    ERROR_LOG("socket failed:%d, errno:%d", listenfd, errno);
    CLEAN_R;
  }

  // in case of 'address already in use' error message
  if (set_reuse_addr(listenfd) != 0) {
    ERROR_LOG("setsockopt failed, errno:%d", errno);
    CLEAN_R;
  }

  // 设置sock为non - blocking
  if (setSockNonBlock(listenfd) < 0) CLEAN_R;

  struct epoll_event ev;
  ev.data.fd = listenfd;
  ev.events = EPOLLIN|EPOLLET;
  int ret             = 0;
  ret = epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);
  if (ret != 0) {
    ERROR_LOG("epoll_ctl add fail:%d,errno:%d", ret, errno);
    CLEAN_R;
  }
  // 创建要bind的socket address
  struct sockaddr_in bind_addr;
  memset(&bind_addr, 0, sizeof(bind_addr));
  bind_addr.sin_family = AF_INET;
  // bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  bind_addr.sin_addr.s_addr = inet_addr(gp_parameter->host.c_str());
  bind_addr.sin_port = htons(gp_parameter->port);

  // bind sock到创建的socket address上
  if (bind(listenfd, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) != 0) {
    ERROR_LOG("bind listenfd failed.");
    CLEAN_R;
  }

  // listen
  if (listen(listenfd, 5) != 0) {
    ERROR_LOG("listen listenfd failed.");
    CLEAN_R;
  }

  const int MAXEVENTS = 1024;
  struct epoll_event events[MAXEVENTS];
  struct sockaddr_in cliaddr;
  g_running = true;
  while (g_running) {
    int nfds = epoll_wait(epfd, events, MAXEVENTS, 1000);
    for (int i = 0; i < nfds && g_running; ++i) {
      if (events[i].data.fd == listenfd) {
        DEBUG_LOG("accept connect ...");
      }
      if (events[i].events & EPOLLIN) {
        do {
          socklen_t clilen = sizeof(struct sockaddr);
          int connfd = accept(listenfd,
                reinterpret_cast<sockaddr *>(&cliaddr), &clilen);
          if (connfd >= 0) {
            INFO_LOG("accept:%d, connect:%s:%d", connfd,
                  inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
            // accept conn
            accept_conn_cb(connfd, &cliaddr);
          } else {
            if (errno == EAGAIN) {
              // ERROR_LOG("accept:%d,errno:%d = EAGAIN", connfd, errno);
              // 非阻塞 无数据可读
              break;
            } else if (errno == EINTR) {
              // ERROR_LOG("accept:%d,errno:%d = EINTR", connfd, errno);
              // 调用被信号中断，可简单忽略它
              // retry
            } else {
              ERROR_LOG("accept:%d,errno:%d", connfd, errno);
              // 真正的错误
              // error, closeing
              close(listenfd);
              epoll_ctl(epfd, EPOLL_CTL_DEL, listenfd, &ev);
              g_running = false;
              CLEAN_R;
            }
          }
        } while (g_running);
      } else if ((events[i].events & EPOLLERR)
            || (events[i].events & EPOLLHUP)) {
        close(listenfd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, listenfd, &ev);
        CLEAN_R;
      }
    }
  }
  CLEAN;
#undef CLEAN
#undef CLEAN_R
}

// -1 表示失败
int daemonize(int chrt, int noclose) {
    int fd;
    switch (fork()) {
        case -1:
            return -1;
        case 0:
            break;
        default:
            _exit(EXIT_SUCCESS);
    }

    if (setsid() == -1) {
        return -1;
    }

    if (chrt != 0) {
        if (chdir("/") != 0) {
            ERROR_LOG("Failed to chdir.");
            return -1;
        }
    }

    if (noclose == 0 && (fd = open("/dev/null", O_RDWR, 0)) != -1) {
        if (dup2(fd, STDIN_FILENO) < 0) {
            ERROR_LOG("Failed to dup2 stdin.");
            return -1;
        }
        if (dup2(fd, STDOUT_FILENO) < 0) {
            ERROR_LOG("Failed to dup2 stdout.");
            return -1;
        }
        if (dup2(fd, STDERR_FILENO) < 0) {
            ERROR_LOG("Failed to dup2 stderr.");
            return -1;
        }

        if (fd > STDERR_FILENO) {
            if (close(fd) < 0) {
                ERROR_LOG("Failed to close.");
                return -1;
            }
        }
    }
    return 0;
}


int main(int argc, char **argv) {
  if (argc > 0) {
    printf("input params:");
    for (int i=0; i< argc; ++i) {
        char *s = argv[0];
        printf("\t%s\n", s);
    }
    printf("\n");
  }
  gp_parameter = new GlobalParameter;
  // 配置文件解析
  if (!load_config_file()) {
    fprintf(stderr, "加载配置文件失败\n");
    return -1;
  }
  // 全部参数从配置文件读取, 不再接受命令行参数
  if (check_config_parameter()) {
    fprintf(stderr, "配置参数检查失败\n");
    return -1;
  }
  // 全局的日志
  gp_log = new CLog(gp_parameter->log_path_name.c_str());
  if (gp_parameter->log_level == "INFO") {
    gp_log->set_log_level(CLog::LOG_INFO);
  } else if (gp_parameter->log_level == "DEBUG") {
    gp_log->set_log_level(CLog::LOG_DEBUG);
  } else if (gp_parameter->log_level == "WARN") {
    gp_log->set_log_level(CLog::LOG_WARN);
  } else if (gp_parameter->log_level == "ERROR") {
    gp_log->set_log_level(CLog::LOG_ERROR);
  } else {
    gp_log->set_log_level(CLog::LOG_NO);
  }

  if (gp_parameter->is_daemon) {
    if (daemonize(0, 0) == -1) {
      fprintf(stderr, "Failed to daemon() in order to daemonize.\n");
      ERROR_LOG("Failed to daemon() in order to daemonize.");
      exit(EXIT_FAILURE);
    }
    INFO_LOG("Daemon mode start.");
  }

  INFO_LOG("开始");

  // for pid
  if (check_pid_file() == 0) {
    SetSignalHandle();

    gp_statistic = new Statistic;
    gp_thread_pool = new CThreadPool(gp_parameter->thread_pool_min,
          gp_parameter->thread_pool_max);
    gp_thread_pool->set_thread_exit_cb(release_sphinx_client);

    service();
    // gp_thread_pool->StopAll();

    delet_pid_file();
    close_all_sphinx_client();

    delete gp_thread_pool;
    delete gp_statistic;
  }

  INFO_LOG("结束");

  gp_log->close();
  delete gp_log;
  delete gp_parameter;

  return 0;
}

