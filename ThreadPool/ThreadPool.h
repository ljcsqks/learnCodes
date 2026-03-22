/*
构成：
1，管理者线程--子线程，1个
    -控制工作线程的数量，增加或减少
2，工作线程--子线程，多个
    -从任务队列中获取任务并执行
    -任务队列为空，被阻塞（被条件变量阻塞）
    -线程同步（互斥锁）
    -当前数量，空闲的线程数量
    -最大，最小线程数量
3，任务队列--存放任务的队列（stl->queue）
    -线程安全的队列（互斥锁+条件变量）
4,线程池开关--bool变量
    -线程池是否开启
*/
#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <map>
#include <future>
#include <memory>
using namespace std;

class ThreadPool
{
public:
    ThreadPool(int minThreads = 2, int maxThreads = thread::hardware_concurrency());
    ~ThreadPool();

    //添加任务到任务队列
    // void addTask(function<void()> task);
    
    //异步实现
    template <typename F, typename... Args>
    auto addTask(F&& f, Args&&... args ) 
    {
        //1, 构造packaged_task
        using fReturnType = invoke_result_t<F,Args...>;
        auto mytask = make_shared< packaged_task<fReturnType()>> ( bind(forward<F>(f), forward<Args>(args)...));

        //2,得到future对象
        auto res = mytask -> get_future();

        //3,添加到任务队列
        lock_guard<mutex> locker(m_taskMutex);
        m_taskQueue.emplace([mytask](){
            (*mytask)();
        });
        return res;
    }

private:
    void managerThread();                   //管理者线程函数
    void workerThread();                    //工作线程函数
private:
    thread* m_manager;                       //管理者线程
    map<thread::id, thread> m_workers;     //工作线程
    vector<thread::id> m_ids;          //存储要退出的线程id
    atomic<int> m_minThreads;               //最小线程数 ,不清楚是不是共享资源会被读写，所以用atomic  
    atomic<int> m_maxThreads;               //最大线程数
    atomic<int> m_curThreads;               //当前线程数
    atomic<int> m_idleThreads;              //空闲线程数
    atomic<int> m_exitThreads;              //退出线程数
    atomic<bool> m_shutdown;                //线程池开关
    queue<function<void(void)>> m_taskQueue; //任务队列
    mutex m_taskMutex;                      //任务队列互斥锁
    mutex m_idsMutex;                       //工作线程id锁
    condition_variable m_condition;          //条件变量，用于通知工作线程取任务
};


#endif // THREADPOOL_H