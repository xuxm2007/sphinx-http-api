// Copyright 2012 zhaigy hontlong@gmail.com
#include <stdio.h>
#include <pthread.h>
#include <time.h>

#include <string>

#include "./CThread.h"

CThreadPool::CThreadPool(int threadNum) {
  init_(threadNum, threadNum);
}

CThreadPool::CThreadPool(int min, int max) {
  init_(min, max);
}

void CThreadPool::init_(int min, int max) {
  pthread_mutex_init(&(this->m_pthreadMutex), NULL);
  pthread_cond_init(&(this->m_pthreadCond), NULL);
  this->m_max          = max;
  this->m_min          = min;
  m_thread_num         = 0;
  m_max_task           = 100;
  m_before_run_task_cb = NULL;
  m_after_run_task_cb  = NULL;
  m_thread_exit_cb     = NULL;
  Create(this->m_min);
}

int CThreadPool::MoveOutIdleThread(pthread_t tid) {
  vector<pthread_t>::iterator idleIter = m_vecIdleThread.begin();
  while (idleIter != m_vecIdleThread.end()) {
    if (tid == *idleIter) {
      break;
    }
    idleIter++;
  }
  if (idleIter != m_vecIdleThread.end()) {
    m_vecIdleThread.erase(idleIter);
    m_thread_num--;
    if (m_thread_exit_cb != NULL) {
      m_thread_exit_cb(tid);
    }
  }
  return 0;
}

int CThreadPool::MoveToIdle(pthread_t tid) {
  vector<pthread_t>::iterator busyIter = m_vecBusyThread.begin();
  while (busyIter != m_vecBusyThread.end()) {
    if (tid == *busyIter) {
      break;
    }
    busyIter++;
  }
  m_vecBusyThread.erase(busyIter);
  m_vecIdleThread.push_back(tid);
  return 0;
}

int CThreadPool::MoveToBusy(pthread_t tid) {
  vector<pthread_t>::iterator idleIter = m_vecIdleThread.begin();
  while (idleIter != m_vecIdleThread.end()) {
    if (tid == *idleIter) {
      break;
    }
    idleIter++;
  }
  m_vecIdleThread.erase(idleIter);
  m_vecBusyThread.push_back(tid);
  return 0;
}

int CThreadPool::Create(int thread_num) {
  // static int total_thread_num = 0;
  for (int i = 0; i < thread_num; i++) {
    pthread_t tid = 0;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid, &attr, CThreadPool::ThreadFunc, this);
    m_vecIdleThread.push_back(tid);
    m_thread_num++;
  }
  // total_thread_num+=thread_num;
  // printf("THREAD:thread_num = %d,m_thread_num=%d, total_thread_num = %d\n",
  // thread_num,m_thread_num,total_thread_num);
  return 0;
}

/**
 * 添加任务并触发等待线程工作，如果不能加入，返回-1.
 */
int CThreadPool::AddTask(CTask *task) {
  pthread_mutex_lock(&m_pthreadMutex);
  if (m_vecTaskList.size() >= (unsigned int)m_max_task) {
    pthread_mutex_unlock(&m_pthreadMutex);
    return -1;
  }
  this->m_vecTaskList.push_back(task);
  int need_thread_num = this->m_vecTaskList.size() - m_thread_num;
  // 为了避免频繁的创建后又销毁线程，不能需要就创建
  if (need_thread_num > m_thread_num) {
    if (need_thread_num + this->m_thread_num > this->m_max) {
      need_thread_num = this->m_max - this->m_thread_num;
    }
    if (need_thread_num>0) {
      Create(need_thread_num);
    }
  }
  pthread_cond_signal(&m_pthreadCond);
  pthread_mutex_unlock(&m_pthreadMutex);
  return 0;
}

// 线程主调函数
void* CThreadPool::ThreadFunc(void * p_thread_pool) {
  pthread_t tid = pthread_self();
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  CThreadPool * pool = reinterpret_cast<CThreadPool *>(p_thread_pool);

  int WAIT=100 + (int)(((unsigned int)(tid))%100); //s
  struct timespec tp;
  tp.tv_sec = time(NULL) + WAIT;
  tp.tv_nsec = 0;

  bool doing = false;  // 正在循环执行任务,doing时，线程在忙盒子里
  while (1) {
    pthread_mutex_lock(&(pool->m_pthreadMutex));
    vector<CTask*> & taskList = pool->m_vecTaskList;
    if (taskList.empty()) {  // 当前没有任务
      if (doing) {  // 当前线程状态是忙
        pool->MoveToIdle(tid);  // 转成空闲
        doing = false;
      }
      // 在给定时间内前惊醒
      pthread_cond_timedwait(&pool->m_pthreadCond, &pool->m_pthreadMutex, &tp);
    }

    int res = 0;  // 调用任务运行检查函数，看资源是否准备好
    CTask* task = NULL;
    if (pool->m_before_run_task_cb != NULL) {
      if (pool->m_before_run_task_cb(tid) != 0) {
        res = 1;
      }
    }
    if (res == 0) {
      // 资源准备好了
      vector<CTask*>::iterator iter = taskList.begin();
      if (iter != taskList.end()) {
        if (!doing) {
          pool->MoveToBusy(tid);  // 转成忙状态
          doing = true;
        }
        task = *iter;
        taskList.erase(iter);
      }
    }
    pthread_mutex_unlock(&(pool->m_pthreadMutex));
    if (task != NULL) {
      if (doing != true) {
        // 安全性检测，不是必要的
        // 应该打印一下警告信息
      }
      try {
        task->Run();
      } catch (...) {
        // FIXME: 打印
      }
      // FIXME: 这行的代码位置不是很合适，现在还没有找到合适的释放任务的地方
      delete task;
      if (pool->m_after_run_task_cb != NULL) {
        pool->m_after_run_task_cb(tid);
      }
      tp.tv_sec = time(0) + 60;
    } else {  // task == NULL
      if (time(0) < tp.tv_sec) {
        // printf("THREAD:%u:tid=%u,time in,go on\n",time(0),tid);
        continue;
      }
      // 被唤醒，但是没有任务，判断是否需要退出
      if (!doing) {
        // 仅空闲状态下退出
        if (pool->m_thread_num > pool->m_min) {
          pthread_mutex_lock(&(pool->m_pthreadMutex));
          pool->MoveOutIdleThread(tid);
          pthread_mutex_unlock(&(pool->m_pthreadMutex));
          break;  // exit thread
        } else {
          // 对保留线程续时
          tp.tv_sec = time(0) + 60;
        }
      }
    }
  }
  // 一定从此处退出
  // pthread_detach(tid);
  return NULL;
}

// 测试不通过，不要调用,可能是互斥锁的原因
int CThreadPool::StopAll() {
  vector<pthread_t>::iterator iter = m_vecIdleThread.begin();
  while (iter != m_vecIdleThread.end()) {
    pthread_cancel(*iter);
    iter++;
  }
  iter = m_vecIdleThread.begin();
  while (iter != m_vecIdleThread.end()) {
    pthread_join(*iter, NULL);
    iter++;
  }
  m_vecIdleThread.clear();

  iter = m_vecBusyThread.begin();
  while (iter != m_vecBusyThread.end()) {
    pthread_cancel(*iter);
    iter++;
  }
  iter = m_vecBusyThread.begin();
  while (iter != m_vecBusyThread.end()) {
    pthread_join(*iter, NULL);
    iter++;
  }
  m_vecBusyThread.clear();

  pthread_mutex_destroy(&this->m_pthreadMutex);
  pthread_cond_destroy(&this->m_pthreadCond);

  return 0;
}

