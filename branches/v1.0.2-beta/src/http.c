/**
 *    Description:  简单的http解析
 *        Version:  1.0
 *        Created:  2012年03月04日 21时35分13秒
 *         Author:  zhaigy hontlong@gmail.com
 */
#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "http.h"
#include "buffer.h"

#define NEW(type) (type*)malloc(sizeof(type))

void add_time_header(struct buffer *buf) {
  char date[50];
  struct tm cur;
  struct tm *cur_p;
  time_t t = time( NULL );
  gmtime_r( &t, &cur );
  cur_p = &cur;
  strftime( date, sizeof( date ), "%a, %d %b %Y %H:%M:%S GMT", cur_p);
  buffer_add_printf( buf, "Date: %s\r\n", date );
}

/**
 * parse http request initial line.
 */
static int parse_init_line(struct http_request *request, char *line) {
  char *ptr = NULL;
  char *token = strtok_r(line, " ", &ptr);
  if (token == NULL) {
    request->type = HTTP_UNKNOWN;
  } else if (strcmp(token, "GET") == 0) {
    request->type = HTTP_GET;
  } else if (strcmp(token, "HEAD") == 0) {
    request->type = HTTP_HEAD;
  } else {
    request->type = HTTP_UNKNOWN;
  }

  /* uri */
  token = strtok_r(NULL, " ", &ptr);
  request->uri = (char*)malloc(strlen(token) + 1);
  strcpy(request->uri, token);

  /* http protocol version */
  token = strtok_r(NULL, " ", &ptr);
  if (strcmp( token, "HTTP/1.0") == 0) {
    request->ver.major = 1;
    request->ver.minor = 0;
  } else if (strcmp( token, "HTTP/1.1") == 0) {
    request->ver.major = 1;
    request->ver.minor = 1;
  } else {
    request->ver.major = 1;
    request->ver.major = 0;
  }

  return 0;
}

/**
 * @param decode_plus 1=把+转码为空格
 */
static char * http_decode_uri(const char *uri, int decode_plus,
      size_t *size_out) {
  char *ret = malloc(strlen(uri) + 1);
  if (ret == NULL) return (NULL);

  decode_plus = !!decode_plus;

  int j = 0;
  int length = strlen(uri);
  unsigned i = 0;
  for (; i < length; i++) {
    char c = uri[i];
    if (c == '+' && decode_plus) {
      c = ' ';
    } else if (c == '%' && isxdigit(uri[i+1]) && isxdigit(uri[i+2])) {
      char tmp[3];
      tmp[0] = uri[i+1];
      tmp[1] = uri[i+2];
      tmp[2] = '\0';
      c = (char)strtol(tmp, NULL, 16);
      i += 2;
    }
    ret[j++] = c;
  }
  ret[j] = '\0';

  if (size_out) *size_out = (size_t)j;
  return (ret);
}

static const char uri_chars[256] = {
  /* 0 */
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 1, 1, 0,
  1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 0, 0, 0, 0, 0, 0,
  /* 64 */
  0, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 0, 0, 1,
  0, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 0, 1, 0,
  /* 128 */
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
  /* 192 */
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
};

#define CHAR_IS_UNRESERVED(c)   (uri_chars[(unsigned char)(c)])

char * http_encode_uri(const char *uri, size_t len, int space_as_plus) {
  struct buffer *buf = buffer_new();
  if (buf == NULL) return (NULL);

  const char *p, *end;
  char *result;

  if (len >= 0) end = uri+len;
  else end = uri+strlen(uri);

  for (p = uri; p < end; p++) {
    if (CHAR_IS_UNRESERVED(*p)) {
      buffer_add(buf, p, 1);
    } else if (*p == ' ' && space_as_plus) {
      buffer_add(buf, "+", 1);
    } else {
      buffer_add_printf(buf, "%%%02X", (unsigned char)(*p));
    }
  }
  buffer_add(buf, "", 1); /* NUL-terminator. */
  result = malloc(buf->len);
  if (!result) return NULL;
  buffer_remove(buf, result, buf->len);
  buffer_free(buf);

  return (result);
}

void http_clear_headers(keyvalq *headers) {
  keyval *header = NULL;
  for (header = TAILQ_FIRST(headers); header != NULL;
        header = TAILQ_FIRST(headers)) {
    TAILQ_REMOVE(headers, header, next);
    free(header->name);
    free(header->value);
    free(header);
  }
}

void http_header_free(struct http_header_head *header_queue) {
  struct http_header *header, *prev = 0;
  TAILQ_FOREACH( header, header_queue, next ) {
    if( prev != 0 ) {
      free( prev );
      prev = 0;
    }
    free( header->name );
    free( header->value );
    prev = header;
  }
  if (prev) free(prev);
  free( header_queue );
}

/**
 * 仅解析查询部分，会进行编码转换
 * 返回0=成功
 */
int http_parse_query_str(const char *str, keyvalq *headers) {
  char *line=NULL;
  char *argument;
  char *p;
  const char *query_part;
  int result = -1;

  TAILQ_INIT(headers);

  query_part = str;

  /* No arguments - we are done */
  if (!query_part || !strlen(query_part)) {
    result = 0;
    goto done;
  }

  if ((line = strdup(query_part)) == NULL) {
    goto error;
  }

  p = argument = line;
  while (p != NULL && *p != '\0') {
    char *key, *value, *decoded_value;
    argument = strsep(&p, "&");

    value = argument;
    key = strsep(&value, "=");
    if (value == NULL || *key == '\0') {
      goto error;
    }

    decoded_value = http_decode_uri(value, 1, NULL);
    if ( decoded_value == NULL ) {
      goto error;
    }
    http_add_header(headers, key, decoded_value);
    free(decoded_value);
  }

  result = 0;
  goto done;

error:
  http_clear_headers(headers);

done:
  if (line) free(line);

  return result;
}

// 0 成功
int http_add_header(struct http_header_head *headers, const char *name,
      const char *value) {
  struct http_header *header = NEW( struct http_header );
  if (header == NULL) return -1;

  header->name = (char*) malloc( strlen( name ) + 1 );
  if (header->name == NULL) return -1;

  header->value = (char*) malloc( strlen( value ) + 1 );
  if (header->value == NULL) return -1;

  strcpy( header->name, name );
  strcpy( header->value, value );

  TAILQ_INSERT_TAIL( headers, header, next );
  return 0;
}

struct http_header_head *http_header_new() {
  struct http_header_head *queue = NEW( struct http_header_head );
  if( queue == 0 ) return 0;
  TAILQ_INIT( queue );
  return queue;
}



int http_add_header_line( struct http_header_head *header_queue, char *line ) {
  char *value ;
  char *name = strchr( line, ':' );
  struct http_header *header = NEW( struct http_header );
  header->name = (char*) malloc( name - line + 1 );
  strncpy( header->name, line, name - line );
  header->name[name-line] = '\0';

  for( value = name + 1; *value == ' '; ++ value ) {
    ;
  }

  header->value = (char*) malloc( strlen( line ) - ( value - line ) + 1 );
  strcpy( header->value, value );
  TAILQ_INSERT_TAIL( header_queue, header, next );

  return 0;
}

const char *http_get_header_value( struct http_header_head *header_queue,
      const char *header_name ) {
  struct http_header *header;
  TAILQ_FOREACH( header, header_queue, next ) {
    if( strcmp( header->name, header_name ) == 0 ) return header->value;
  }
  return 0;
}

// NULL=失败
struct http_request *http_request_parse( struct buffer *buf ) {
  struct http_request *request = NEW(struct http_request);
  if (request == NULL) return NULL;
  memset(request, 0, sizeof(request));
  {
    // 请求行处理
    char *line = buffer_readline(buf);
    if (line == NULL) {
      http_request_free(request);
      return NULL; 
    }
    int ret = parse_init_line(request, line);
    free(line);
    line = NULL;
    if(ret != 0) {
      http_request_free(request);
      return NULL;
    }
  }

  request->headers = http_header_new();
  // parse headers
  char *line = buffer_readline(buf) ;
  int ret = 0;
  // 头和Body之间有一个空行
  while(line != NULL && *line != '\0') {
    ret = http_add_header_line(request->headers, line);
    if(ret != 0) break;
    free(line);
    line = buffer_readline(buf);
  }
  if (line) free(line);
  // 忽略body数据，现在还不支持
  return request;
}

void http_request_free(struct http_request *request) {
  if (request->uri) free(request->uri);
  if (request->headers) http_header_free(request->headers);
  free( request );
}

