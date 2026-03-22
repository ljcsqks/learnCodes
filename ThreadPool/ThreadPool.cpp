#include "ThreadPool.h"

ThreadPool::ThreadPool(int minThreads, int maxThreads) : m_minThreads(minThreads),
    m_maxThreads(maxThreads), m_shutdown(false), m_curThreads(minThreads), m_idleThreads(minThreads), m_exitThreads(0)
{
    cout << "====创建线程池，最小线程数：" << minThreads << "，最大线程数：" << maxThreads << endl;
    //创建管理者线程
    m_manager = new thread(&ThreadPool::managerThread, this);
    //创建初始工作线程
    for (int i = 0; i < minThreads; ++i)
    {
        // m_workers.emplace_back(thread(&ThreadPool::workerThread, this));//emplace_back 直接在容器末尾构造对象，避免了不必要的拷贝或移动,push_back需要先创建对象再拷贝或移动到容器末尾
        thread t(&ThreadPool::workerThread, this);
        m_workers.insert(make_pair(t.get_id(), move(t)));
    }
}

ThreadPool::~ThreadPool()
{
    //销毁线程池
    m_shutdown.store(true); //设置关闭标志
    m_condition.notify_all(); //通知所有线程退出

    //等待管理者线程退出
    if (m_manager->joinable())
    {
        cout << "主线程等待管理者线程退出" << endl;
        m_manager->join();
    }
    delete m_manager;

    //等待所有工作线程退出
    for (auto& pair : m_workers)
    {
        if (pair.second.joinable())
        {
            pair.second.join();
            cout << "====工作线程 " << pair.first << " 已销毁" << endl;
        }
    }
}

void ThreadPool::managerThread()
{
    //管理者线程函数实现
    while (!m_shutdown.load())
    {
        this_thread::sleep_for(chrono::seconds(1)); //每隔3秒检查一次
        int cur = m_curThreads;
        int idle = m_idleThreads;
        cout << "----管理者线程检查：当前线程数 " << cur << "，空闲线程数 " << idle << endl;
        if (idle > cur/2 && cur > m_minThreads)
        {
            //空闲线程过多，每次减少2个线程
            m_exitThreads.store(2);
            m_condition.notify_all();
            cout << "--------------------------------" << endl;
            lock_guard<mutex> locker(m_idsMutex);
            for (auto id : m_ids)
            {
                auto it = m_workers.find(id);
                if (it != m_workers.end())
                {
                    if (it->second.joinable())
                        it->second.join(); //等待线程退出
                    m_workers.erase(it); //从工作线程列表中移除
                    cout << "====管理者线程销毁工作线程 " << id << endl;
                }
            }
            m_ids.clear();
        }
        //空闲线程过少，且当前线程数未达最大线程数，则增加线程
        else if (idle == 0 && cur < m_maxThreads)
        {
            int addCount = 2;
            if (cur + addCount > m_maxThreads)
                addCount = m_maxThreads - cur;
            for (int i = 0; i < addCount; ++i)
            {
                thread t(&ThreadPool::workerThread, this);
                {
                    lock_guard<mutex> locker(m_idsMutex);
                    m_workers.insert(make_pair(t.get_id(), move(t)));
                }
                m_curThreads++;
                m_idleThreads++;
            }
        }
    }
}

void ThreadPool::workerThread()
{
    //工作线程函数实现
    while (!m_shutdown.load()) //线程池关闭则退出
    {
        function<void()> task;
        {
            unique_lock<mutex> locker(m_taskMutex);
            //等待任务到来或线程池关闭
            m_condition.wait(locker, [this]() { return !m_taskQueue.empty() || m_shutdown.load() || m_exitThreads.load() > 0; });//捕获this = 把当前ThreadPool对象的指针传入 lambda 内部
            //检查线程是否需要退出
            cout <<"退出线程数：" << m_exitThreads.load() << endl;
            if (m_exitThreads.load() > 0)
            {
                m_exitThreads--;//减少退出线程数
                m_curThreads--;//当前线程数减1
                m_idleThreads--;//空闲线程数减1
                {
                    lock_guard<mutex> idLocker(m_idsMutex);
                    m_ids.push_back(this_thread::get_id());//将当前线程id存入要退出的线程id列表
                }
                cout << "----线程 " << this_thread::get_id() << " 退出" << endl;
                return; //退出线程
            }

            if (m_shutdown.load() && m_taskQueue.empty())
                return; //线程池关闭且任务队列为空，退出线程

            //获取任务
            cout << "线程 " << this_thread::get_id() << " 获取任务" << endl;
            task = move(m_taskQueue.front());
            m_taskQueue.pop();
        }
        /*
        {
            unique_lock<mutex> locker(m_taskMutex);
            while (m_taskQueue.empty() && !m_shutdown)
            {
                m_condition.wait(locker);//等待任务到来或线程池关闭,wait会自动释放锁，唤醒时重新加锁
            }
            if (!m_taskQueue.empty())
            {
                //获取任务
                task = move(m_taskQueue.front());//move将左值转换为右值引用，避免不必要的拷贝，提高性能
                m_taskQueue.pop();//弹出任务
            }
        }
        */
        if (task)
        {
            m_idleThreads--;//空闲线程数减1
            task();         //执行任务
            m_idleThreads++;//空闲线程数加1
        }
    }
}

// void ThreadPool::addTask(function<void()> task)
// {
//     //添加任务到任务队列实现
//     {
//         lock_guard<mutex> locker(m_taskMutex);//对共享资源加锁，locker在作用域结束时自动释放锁
//         m_taskQueue.emplace(task); //将任务添加到队列
//     }//通过添加作用域提前释放锁，相当于只锁了上面那行代码
//     //只要保证 “修改共享资源的操作是原子的（不可被打断）”，就不会出现线程安全问题，这也是为什么只需要锁这一行。
//     m_condition.notify_one();   //通知一个等待的工作线程
// }

void calc(int a, int b)
{
    // cout << "计算 " << a << " + " << b << " = " << a + b << " 在线程 " << this_thread::get_id() << " 中执行" << endl;
    
    this_thread::sleep_for(chrono::seconds(3)); //模拟计算时间
    cout << "线程执行结果：" << a+b << endl;
}

int calc1(int a, int b)
{
    // cout << "计算 " << a << " + " << b << " = " << a + b << " 在线程 " << this_thread::get_id() << " 中执行" << endl;
    
    this_thread::sleep_for(chrono::seconds(2)); //模拟计算时间
    return a+b;
}

int main()
{
    ThreadPool pool;
    vector<future<int>> result;
    // for (int i =0; i < 10; ++i)
    // {
    //     auto obj = bind(calc, i , i*10);
    //     pool.addTask(obj);
    // }
    for (int i=0; i < 10; i++)
    {
        result.emplace_back(pool.addTask(calc1, i, i+100));
    }
    for (auto& item : result)
    {
        cout << "thread result: " << item.get() << endl;
    }

    // getchar(); //等待用户输入，防止主线程过早退出
    return 0;
}