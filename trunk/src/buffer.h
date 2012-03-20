// Copyright 2012 zhaigy hontlong@gmail.com
#ifndef SRC_BUFFER_H_
#define SRC_BUFFER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>

  typedef unsigned char u_char;

  struct buffer {
    u_char *buffer;
    u_char *orig_buffer;

    size_t misalign;  // 对不齐
    size_t totallen;
    size_t len;
  };

  struct buffer *buffer_new(void);
  void buffer_free(struct buffer *);

  /**
   * 扩展空间
   * @return 0 if successful, or -1 if an error occurred
   */
  int buffer_expand(struct buffer *, size_t);

  int buffer_add(struct buffer *, const void *, size_t);

  /**
   * 读取数据从缓存中,返回读取的数量
   */
  int buffer_remove(struct buffer *, void *, size_t);

  /**
   * 从缓存中读取一行.
   * 行结束符是'\r\n', '\n\r' or '\r' or '\n'.
   * 返回的数据需要调用者释放
   */
  char *buffer_readline(struct buffer *);

  /**
   * 0=成功,-1=失败
   */
  int buffer_add_buffer(struct buffer *, struct buffer *);


  /**
   * 返回添加的字节数，-1=错误产生
   */
  int buffer_add_printf(struct buffer *, const char *fmt, ...);
  int buffer_add_vprintf(struct buffer *, const char *fmt, va_list ap);

  /**
   * 从缓存的开始位置移除数据
   * 0=成功,-1=错误产生
   */
  void buffer_drain(struct buffer *, size_t);


  /**
   * 把数据写入到文件中,成功写入的数据会被擦除
   * 返回被写入的字节数，-1=错误产生
   */
  int buffer_write(struct buffer *, int);


  /**
   * 从一个文件中读取数据存储到缓存中
   * 返回读入的字节数，-1=错误产生
   */
  int buffer_read(struct buffer *, int, int);

  /**
   * 在缓存中查找字符串
   * 返回插座到的字符串的开始位置，或者NULL
   */
  u_char *buffer_find(struct buffer *, const u_char *, size_t);

  /**
   * 设者回调，在缓存被修改时触发
   */
  void buffer_setcb(struct buffer *,
        void (*)(struct buffer *, size_t, size_t, void *), void *);

#ifdef __cplusplus
}
#endif

#endif  // SRC_BUFFER_H_

