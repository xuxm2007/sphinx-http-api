/*****************************************************************************************
 * File Name        : lock.h
 * Description      : 线程锁,守护者和条件变量的封装
 * Copyright 2012 zhaigy hontlong@gmail.com
 ****************************************************************************************/

#ifndef LIBLOG_LOCK_H_
#define LIBLOG_LOCK_H_

#include <pthread.h>

/**
 * 线程锁,利用构造和析构函数自动init和destroy
 */
class CLock {
  public:
    explicit CLock(pthread_mutexattr_t *attrs = NULL) {
      pthread_mutex_init(&m_mutex, attrs);
    }

    ~CLock() {
      pthread_mutex_destroy(&m_mutex);
    }

    // 把m_mutex的地址拿出来给其他底层类使用,应用层一般不要调用这个函数
    pthread_mutex_t* GetKernel() {
      return &m_mutex;
    }

    int Lock() {
      return pthread_mutex_lock(&m_mutex);
    }

    int UnLock() {
      return pthread_mutex_unlock(&m_mutex);
    }

    int Lock(const struct timespec* time) {
      return pthread_mutex_timedlock(&m_mutex, time);
    }

    int TryLock() {
      return pthread_mutex_trylock(&m_mutex);
    }

  private:
    pthread_mutex_t m_mutex;
};


class CGuard {
  private:
    CLock* m_pLock;
    bool is_locked;

  public:
    explicit CGuard(CLock & lock):m_pLock(&lock) {
      m_pLock->Lock();
      is_locked = true;  // 防止自己锁死自己
    }

    bool Lock() {
      if (is_locked) {
        return false;
      }

      m_pLock->Lock();
      is_locked = true;
      return true;
    }

    void UnLock() {
      m_pLock->UnLock();
    }

    ~CGuard() {
      UnLock();
    }
};

#endif  // LIBLOG_LOCK_H_

