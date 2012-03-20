/**
 * 简单线程池,基于[http://www.iteye.com/topic/345688]修改
 * Copyright 2012 zhaigy hontlong@gmail.com
 */

#ifndef LIBTHREADPOOL_CTHREAD_H_
#define LIBTHREADPOOL_CTHREAD_H_

#include <pthread.h>
#include <vector>
#include <string>

using std::string;
using std::vector;

/**
 * 执行任务的类，设置任务数据并执行
 */
class CTask {
  protected:
    string m_strTaskName;  // 任务的名称
    void* m_ptrData;       // 要执行的任务的具体数据
  public:
    CTask() {}
    explicit CTask(string taskName) {
      this->m_strTaskName = taskName;
      m_ptrData           = NULL;
    }
    virtual ~CTask() {}
    virtual int Run()= 0;
    void SetData(void* data) { m_ptrData = data; }  // 设置任务数据
};

/**
 * 线程池
 */
class CThreadPool {
  private:
    vector<CTask*>    m_vecTaskList;      // 任务列表
    int               m_max_task;         // 任务列表中等待运行的任务的最大个数
    int               m_max;              // 线程池中启动的线程数
    int               m_min;              // 线程池中启动的线程数
    int               m_thread_num;       // 线程池中启动的线程数
    vector<pthread_t> m_vecIdleThread;    // 当前空闲的线程集合
    vector<pthread_t> m_vecBusyThread;    // 当前正在执行的线程集合
    pthread_mutex_t   m_pthreadMutex;     // 线程同步锁
    pthread_cond_t    m_pthreadCond;      // 线程同步的条件变量
    int (* m_thread_exit_cb)(pthread_t);  // 线程退出的回调函数
    // return 0 is ok, else while not run task
    int (* m_before_run_task_cb)(pthread_t);   // 线程运行任务前的回调函数
    int (* m_after_run_task_cb)(pthread_t);    // 线程运行任务前的回调函数
    void init_(int min, int max);              // 构建函数共用的初始化构建
    int MoveOutIdleThread(pthread_t tid);

  protected:
    static void * ThreadFunc(void * p_thread_pool);  // 新线程的线程函数
    int MoveToIdle(pthread_t tid);     // 线程执行结束后，把自己放入到空闲线程中
    int MoveToBusy(pthread_t tid);     // 移入到忙碌线程中去
    int Create(int thread_num);        // 创建所有的线程

  public:
    explicit CThreadPool(int threadNum);
    CThreadPool(int min, int max);
    int AddTask(CTask *task);          // 把任务添加到线程池中
    int StopAll();
    void set_thread_exit_cb(int (* thread_exit_cb)(pthread_t)) {
      this->m_thread_exit_cb = thread_exit_cb;
    };
    void set_before_run_task_cb(int (* before_run_task_cb)(pthread_t)) {
      this->m_before_run_task_cb = before_run_task_cb;
    };
    void set_after_run_task_cb(int (* after_run_task_cb)(pthread_t)) {
      this->m_after_run_task_cb = after_run_task_cb;
    };
    void set_max_task(int max_task) {
      this->m_max_task = max_task;
    };
};

#endif  // LIBTHREADPOOL_CTHREAD_H_

