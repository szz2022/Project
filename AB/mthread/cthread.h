#pragma once

#include "../module/common/proto.h"

class CThread;

//定义了CThread类的Priority属性的可能值，如下所述。根据优先级确定每个线程CPU周期
enum class CThreadPriority
{
    tpIdle = THREAD_PRIORITY_IDLE,                   //只有当系统空闲时该线程执行
    tpLowest = THREAD_PRIORITY_LOWEST,               //线程优先线比正常低2点
    tpLower = THREAD_PRIORITY_BELOW_NORMAL,          //线程优先线比正常低1点
    tpNormal = THREAD_PRIORITY_NORMAL,               //线程优先线比正常低
    tpHigher = THREAD_PRIORITY_ABOVE_NORMAL,         //线程优先线比正常高1点
    tpHighest = THREAD_PRIORITY_HIGHEST,             //线程优先线比正常高2点
    tpTimeCritical = THREAD_PRIORITY_TIME_CRITICAL   //线程优先线最高
};


//线程通知消息类
class CThreadNotifyEvent
{
public:
    //当线程的Execute方法已经返回且在该线程被删除之前发生
    virtual void OnTerminate(CThread* thread) = 0;
    //当线程发生异常时发生
    virtual void OnException(std::exception& e) = 0;
};


class CThread
{
public:
    //创建一个线程对象的实例，并挂起。Execute直到Resume调用后才被调用
    CThread();
    virtual ~CThread();

    bool IsActive();  //线程是否已经激活

    bool IsTerminated();  //如果IsTerminated == true说明进程要求该线程中止。

    bool Resume();  //重新执行一个挂起的线程。成功返回true;

    bool Suspend();  //挂起一个运行中的线程。成功返回true;

    void Terminate(); //终止线程

    void Wait();      //等待线程结果

    HANDLE GetHandle();  //返回线程句柄

    CThreadPriority GetPriority();//返回线程优先级

    void SetPriority(CThreadPriority priority); //设置该线程相对于进程中其他线程的优先级

    void Attach(CThreadNotifyEvent* event); //注册线程通知消息event

    //设置信号量,相当于UnLock
    void SetEvent();

    //释放信号量,相当于Lock
    void ResetEvent();

    void sleep_milliseconds(int count);

protected:
    virtual void Execute() = 0;

private:
    CThreadNotifyEvent* m_threadNotifyEvent;   //观察者
    HANDLE m_hHandle;     //线程句柄
    HANDLE m_hWaitEvent;  //事件对象句柄
    DWORD m_dwThreadID;   //线程ID
    bool m_bActive;       //标识线程是否处于激活状态
    bool m_bTerminated;   //标识是否需要结束线程

    //!所有线程必须从一个指定的函数开始执行，该函数称为线程函数，它必须具有下列原型：
    static DWORD WINAPI ThreadProc(LPVOID lpvThreadParm);
};