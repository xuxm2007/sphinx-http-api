#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include "./buffer.h"

struct buffer * buffer_new(void) {
  struct buffer *buffer = calloc(1, sizeof(struct buffer));
  return (buffer);
}

void buffer_free(struct buffer *buffer) {
  if (buffer->orig_buffer != NULL) free(buffer->orig_buffer);
  free(buffer);
}

/**
 * 把数据从一个缓存移动到另一个缓存
 */
#define SWAP(x,y) do { \
  (x)->buffer = (y)->buffer; \
  (x)->orig_buffer = (y)->orig_buffer; \
  (x)->misalign = (y)->misalign; \
  (x)->totallen = (y)->totallen; \
  (x)->len = (y)->len; \
} while (0)

int buffer_add_buffer(struct buffer *outbuf, struct buffer *inbuf) {

  // 对于空的目标缓存，高效的做法，直接交换它们
  if (outbuf->len == 0) {
    struct buffer tmp;

    SWAP(&tmp, outbuf);
    SWAP(outbuf, inbuf);
    SWAP(inbuf, &tmp);

    return (0);
  }

  int res = buffer_add(outbuf, inbuf->buffer, inbuf->len);
  // 成功的话，清理输入的数据缓存
  if (res == 0) buffer_drain(inbuf, inbuf->len);
  return (res);
}

int buffer_add_vprintf(struct buffer *buf, const char *fmt, va_list ap) {
  char *buffer;
  size_t space;
  int sz;
  va_list aq;

  /* make sure that at least some space is available */
  buffer_expand(buf, 64);
  for (;;) {
    size_t used = buf->misalign + buf->len;
    buffer = (char *)buf->buffer + buf->len;
    assert(buf->totallen >= used);
    space = buf->totallen - used;

#ifndef va_copy
#define	va_copy(dst, src)	memcpy(&(dst), &(src), sizeof(va_list))
#endif
    va_copy(aq, ap);
    sz = vsnprintf(buffer, space, fmt, aq);

    va_end(aq);

    if (sz < 0) return (-1);
    if (sz < space) {
      buf->len += sz;
      return (sz);
    }
    if (buffer_expand(buf, sz + 1) == -1) return (-1);
  }
}

int buffer_add_printf(struct buffer *buf, const char *fmt, ...) {
  int res = -1;
  va_list ap;

  va_start(ap, fmt);
  res = buffer_add_vprintf(buf, fmt, ap);
  va_end(ap);

  return (res);
}

int buffer_remove(struct buffer *buf, void *data, size_t datlen) {
  size_t nread = datlen;
  if (nread >= buf->len) nread = buf->len;

  memcpy(data, buf->buffer, nread);
  buffer_drain(buf, nread);

  return (nread);
}

// 返回的行申请了内存, 调用者负责释放
char * buffer_readline(struct buffer *buffer) {
  u_char *data = buffer->buffer;
  size_t len = buffer->len;
  char *line = NULL;
  unsigned int i;

  for (i = 0; i < len; i++) {
    if (data[i] == '\r' || data[i] == '\n')
    break;
  }

  if (i == len) return (NULL);

  if ((line = malloc(i + 1)) == NULL) {
    return NULL;
  }

  memcpy(line, data, i);
  line[i] = '\0';

  /*
   * Some protocols terminate a line with '\r\n', so check for
   * that, too.
   */
  if ( i < len - 1 ) {
    char fch = data[i], sch = data[i+1];

    /* Drain one more character if needed */
    if ( (sch == '\r' || sch == '\n') && sch != fch )
    i += 1;
  }

  buffer_drain(buffer, i + 1);

  return (line);
}

/**
 * 数据对齐
 */
static void buffer_align(struct buffer *buf) {
  memmove(buf->orig_buffer, buf->buffer, buf->len);
  buf->buffer = buf->orig_buffer;
  buf->misalign = 0;
}

int buffer_expand(struct buffer *buf, size_t datlen) {
  size_t need = buf->misalign + buf->len + datlen;

  if (buf->totallen >= need) return (0);

  // 如果开始的空余空间就足够，发动一次数据对齐
  if (buf->misalign >= datlen) {
    buffer_align(buf);
    return 0;
  }

  void *newbuf;
  size_t length = buf->totallen;

  if (length < 256) length = 256;
  while (length < need) length <<= 1;  // 成倍的扩充空间

  if (buf->orig_buffer != buf->buffer) buffer_align(buf);
  if ((newbuf = realloc(buf->buffer, length)) == NULL) return (-1);

  buf->orig_buffer = buf->buffer = newbuf;
  buf->totallen = length;

  return (0);
}

int buffer_add(struct buffer *buf, const void *data, size_t datlen) {
  size_t need = buf->misalign + buf->len + datlen;

  if (buf->totallen < need) {
    if (buffer_expand(buf, datlen) == -1) return (-1);
  }

  memcpy(buf->buffer + buf->len, data, datlen);
  buf->len += datlen;

  return (0);
}

// 移除数据
void buffer_drain(struct buffer *buf, size_t len) {
  if (len >= buf->len) {
    buf->len = 0;
    buf->buffer = buf->orig_buffer;
    buf->misalign = 0;
    return;
  }

  buf->buffer += len;
  buf->misalign += len;
  buf->len -= len;
}

#define BUFFER_MAX_READ	4096
int buffer_read(struct buffer *buf, int fd, int howmuch) {
  int n = BUFFER_MAX_READ;

#if defined(FIONREAD)
  if (ioctl(fd, FIONREAD, &n) == -1 || n == 0) {
    n = BUFFER_MAX_READ;
  } else if (n > BUFFER_MAX_READ && n > howmuch) {
    if (n > buf->totallen << 2) n = buf->totallen << 2;
    if (n < BUFFER_MAX_READ) n = BUFFER_MAX_READ;
  }
#endif
  if (howmuch < 0 || howmuch > n) howmuch = n;
  if (buffer_expand(buf, howmuch) == -1) return (-1);

  // 从这里开始添加新的数据
  u_char *p = buf->buffer + buf->len;

  n = recv(fd, p, howmuch, 0);
  if (n == -1) return (-1);
  if (n == 0) return (0);

  buf->len += n;
  return (n);
}

int buffer_write(struct buffer *buffer, int fd) {
  int n;
  n = write(fd, buffer->buffer, buffer->len);
  if (n == -1) return (-1);
  if (n == 0) return (0);
  buffer_drain(buffer, n);
  return (n);
}

u_char * buffer_find(struct buffer *buffer, const u_char *what, size_t len) {
  u_char *search = buffer->buffer, *end = search + buffer->len;
  u_char *p;

  while (search < end && (p = memchr(search, *what, end - search)) != NULL) {
    if (p + len > end) break;
    if (memcmp(p, what, len) == 0) return (p);
    search = p + 1;
  }

  return (NULL);
}

