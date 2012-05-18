// Copyright 2012 zhaigy hontlong@gmail.com

#ifndef SRC_HTTP_H_
#define SRC_HTTP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/queue.h>

  struct http_header {
    char *name;
    char *value;
    // 队列
    TAILQ_ENTRY(http_header) next;
  };

  typedef struct http_header keyval;

  /** http request type. */
  enum { HTTP_GET, HTTP_HEAD, HTTP_UNKNOWN };

  /** http protocol version. */
  struct http_pro_version {
    int major;
    int minor;
  };

  /** http request. */
  struct http_request {
    /** request type : GET / HEAD */
    int type;
    /** request resource */
    char *uri;
    /** http version */
    struct http_pro_version ver;
    /** http headers */
    struct http_header_head *headers;
    /** optional bodies, currently it's not implemented. */
  };

  /** response status code. */
  enum {
    HTTP_OK = 200,
    HTTP_NOTFOUND = 404,
    HTTP_BADREQUEST = 400,
    HTTP_NOTIMPLEMENT = 501,
    HTTP_SERVUNAVAIL = 503,
  };

  /** response status readable code. */
#define HTTP_OK_STR "OK"
#define HTTP_NOTFOUND_STR "NOT FOUND"
#define HTTP_BADREQUEST_STR "BAD REQUEST"
#define HTTP_NOTIMPLEMENT_STR "NOT IMPLEMENTED"
#define HTTP_SERVUNAVAIL_STR "SERVER ERROR"

#define CODE_STR(code) code##_STR

  struct buffer;

  // http header queue head definition.
  TAILQ_HEAD(http_header_head, http_header);
  typedef struct http_header_head keyvalq;

  struct http_header_head *http_header_new();
  void http_clear_headers(keyvalq *headers);
  void http_header_free(struct http_header_head *header_queue);
  void add_time_header(struct buffer *buf);
  int http_add_header_line(struct http_header_head *header_queue, char *line);
  int http_add_header(struct http_header_head *headers, const char *name,
        const char *value);
  const char *http_get_header_value(struct http_header_head *header_queue,
        const char *header_name);

  // parse the http request from a raw buffer from net layer.
  struct http_request *http_request_parse(struct buffer *buf);

  // 解析URL的查询部分
  int http_parse_query_str(const char *str, keyvalq *headers);
  // 在使用http_parse_request之后，必须调用它来释放资源
  void http_request_free(struct http_request *request);

  void http_response_error(struct buffer *buf, int status,
        const char *status_str, const char *more_info);

#ifdef __cplusplus
}
#endif

#endif  // SRC_HTTP_H_

