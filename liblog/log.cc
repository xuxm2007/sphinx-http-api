/****************************************
 * log
 * log.h log.cpp
 * Copyright 2012 zhaigy hontlong@gmail.com
 ****************************************/
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "./log.h"

#define WRITE_LOG(level, head, fmt) \
  do { \
    if (_log_level < level) {\
      return 0;\
    }\
    if (_fp == NULL) {\
      return -1;\
    }\
    struct tm tm;\
    time_t tnow = time(NULL);\
    localtime_r(&tnow, &tm);\
    \
    int res = snprintf(_log_buf, sizeof(_log_buf), \
          "%04d-%02d-%02d %02d:%02d:%02d [%s]:", \
          tm.tm_year + 1900, tm.tm_mon + 1, \
          tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, head);\
    if (res < 0) return res;\
    int write_byte = res;\
    \
    va_list ap;\
    va_start(ap, fmt);\
    res = vsnprintf(_log_buf + res, sizeof(_log_buf) - res - 2, fmt, ap);\
    if (res < 0) return res;\
    write_byte += res;\
    _log_buf[write_byte] = '\0';\
    va_end(ap);\
    \
    res = _write(_log_buf);\
    if (res <= 0) return -1;\
    return res; \
  } while (0)

CLog::CLog(const char *log_path_name, int max_log_size, int max_log_num,
      int log_level) {
  _max_log_size = max_log_size;
  _log_size = 0;
  _max_log_num = max_log_num;
  _log_num = 0;
  _log_level = log_level;
  _log_file_idx = 0;
  _fp = NULL;

  if (strlen(log_path_name) >= (sizeof(_log_path_name) - 15)) {  // 15 for .idx
    ::puts("special log pathname is too long!");
    return;
  }

  snprintf(_log_path_name, sizeof(_log_path_name), "%s", log_path_name);
  open();
}

int CLog::debug(const char *fmt, ...) {
  WRITE_LOG(LOG_DEBUG, "DEBUG", fmt);
}

int CLog::info(const char *fmt, ...) {
  WRITE_LOG(LOG_INFO, "INFO", fmt);
}

int CLog::error(const char *fmt, ...) {
  WRITE_LOG(LOG_ERROR, "ERROR", fmt);
}

int CLog::warn(const char *fmt, ...) {
  WRITE_LOG(LOG_WARN, "WARN", fmt);
}

int CLog::_write(const char *str) {
  int ret = fprintf(_fp, "%s\n", str);
  if (ret < 0) {
    this->close();
    fprintf(stderr, "ERROR: fputs %s: %s\n", _log_path_name, strerror(errno));
    return ret;
  }
  int write_size = strlen(str);
  printf("%s\n", str);

  _log_size+=write_size;

  if (_max_log_size > _log_size) {
    return 0;
  }
  // 达到转移条件
  ret = shift_files();
  return ret;
}

// 日志满，转名
int CLog::shift_files() {
  close();

  char sNewLogFileName[256];

  for (; true; ++_log_file_idx) {
    snprintf(sNewLogFileName, sizeof(sNewLogFileName), "%s.%d",
          _log_path_name, _log_file_idx);
    if (access(sNewLogFileName, F_OK) == 0) {  // 目标文件存在
      _log_num += 1;
      if (_log_num >= _max_log_num) {  // 日志文件数目达到最大值，删除旧的日志
        snprintf(sNewLogFileName, sizeof(sNewLogFileName), "%s.%d",
              _log_path_name, (_log_file_idx - _max_log_num));
        remove(sNewLogFileName);
        _log_num--;
      }
      continue;
    }
    break;
  }

  if (rename(_log_path_name, sNewLogFileName) < 0) {
    snprintf(_error_text, sizeof(_error_text), "rename %s %s: %s",
          _log_path_name, sNewLogFileName, strerror(errno));
    return -1;
  }

  // 重新打开
  open();
  return 0;
}

int CLog::open() {
  // 以追加方式打开日志文件
  if ((_fp = fopen(_log_path_name, "a")) == NULL) {
    snprintf(_error_text, sizeof(_error_text), "fopen %s:%s",
          _log_path_name, strerror(errno));
    ::puts(_error_text);
    ::puts("WARN:open log file error,don't log anything!");
  }
  return 0;
}
