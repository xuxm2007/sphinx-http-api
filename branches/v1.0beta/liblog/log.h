// Copyright 2012 zhaigy hontlong@gmail.com

/****************************************
 * log
 * 简单日志功能
 * 不能多进程
 * 支持多线程
 *
 * 日志总是以给的路径名存在，达到记录的最
 * 大值后，进行轮转，旧日志文件名后添加递
 * 增序号。如果已经存在同名文件，序号自动
 * 增长后再尝试保存。
 ****************************************/
#ifndef LIBLOG_LOG_H_
#define LIBLOG_LOG_H_

#include <stdarg.h>
#include <cstdio>
#include "./lock.h"

class CLog {
  public:
    CLog(const char *log_base_path, int max_log_size = 64*1024*1024,
          int max_log_num = 10, int log_level = CLog::LOG_DEBUG);
    virtual ~CLog() { close(); }

  public:
    enum CLogLevel {
      LOG_NO    = 0,
      LOG_ERROR = 1,
      LOG_WARN  = 2,
      LOG_INFO  = 3,
      LOG_DEBUG = 4
    };

    int debug(const char *fmt, ...);
    int info(const char *fmt, ...);
    int warn(const char *fmt, ...);
    int error(const char *fmt, ...);

    void set_log_level(int log_level) {
      _log_level = log_level;
    }

    const char * get_error_msg() const {
      return _error_text;
    }

    inline void close() {
      if (_fp != NULL) {
        fclose(_fp);
        _fp = NULL;
      }
    }

  private:
    int _write(const char * str);
    int shift_files();
    int open();

    FILE * _fp;
    char _log_path_name[256];

    CLock _log_lock;

    int _max_log_size;
    int _log_size;
    int _max_log_num;
    int _log_num;
    int _log_level;
    int _log_file_idx;  // 记录日志文件的后缀序号

    char _log_buf[16*1024];  // cache log content
    char _error_text[256];
};

#endif  // LIBLOG_LOG_H_
