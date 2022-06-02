#include "cthread.h"

using PTHREAD_START = unsigned(__stdcall*) (void*);

CThread::CThread()
{
    m_threadNotifyEvent = nullptr;
    m_bTerminated = false;
    m_bActive = false;
    m_dwThreadID = 0;

    m_hHandle = (HANDLE)_beginthreadex(NULL, 0, (PTHREAD_START)ThreadProc,
                this, CREATE_SUSPENDED, (unsigned*)&m_dwThreadID);
    if (m_hHandle == nullptr)
    {
        char msg[64];
        sprintf_s(msg, "Create thread failed with error code %d.\n", GetLastError());
        throw std::exception(msg);
    }

    //安全属性：NULL, 复位方式：手动, 初始状态：有信号状态
    m_hWaitEvent = ::CreateEvent(nullptr, TRUE, TRUE, NULL);
    if (m_hWaitEvent == nullptr)
    {
        char msg[64];
        sprintf_s(msg, "Create event failed with error code %d.\n", GetLastError());
        throw std::exception(msg);
    }
}

CThread::~CThread(void)
{
    this->Terminate();
    ::CloseHandle(m_hWaitEvent);
    ::CloseHandle(m_hHandle);
}

bool CThread::IsActive()
{
    return m_bActive;
}

bool CThread::IsTerminated()
{
    return m_bTerminated;
}

bool CThread::Resume(void)
{
    if (m_bActive)
        return true;

    if (::ResumeThread(m_hHandle) == -1)
    {
        char msg[128];
        sprintf_s(msg, "Resume thread failed with error code %d.\n", GetLastError());
        throw std::exception(msg);
        return false;
    }

    m_bActive = true;
    return true;
}

bool CThread::Suspend(void)
{
    if (!m_bActive)
        return true;

    if (SuspendThread(m_hHandle) == -1)
    {
        char msg[128];
        sprintf_s(msg, "Suspend thread failed with error code %d.\n", GetLastError());
        throw std::exception(msg);
        return false;
    }
    m_bActive = false;
    return true;
}

void CThread::Terminate()
{
    //这里加锁，确保调用Terminate函数之后，Execute函数能够立即停止运行
    Wait();
    ResetEvent();  //Lock
    m_bTerminated = true;
    SetEvent();  //Unlock,设置成有信号状态
}

void CThread::Wait()
{
    //函数将直到相应时间事件变成有信号状态才返回，否则就一直等待下去
    ::WaitForSingleObject(m_hWaitEvent, INFINITE);
}

HANDLE CThread::GetHandle()
{
    return m_hHandle;
}

CThreadPriority CThread::GetPriority()
{
    return (CThreadPriority)GetThreadPriority(m_hHandle);
}

void CThread::SetPriority(CThreadPriority priority)
{
    SetThreadPriority(m_hHandle, (int)priority);
}

void CThread::Attach(CThreadNotifyEvent* event)
{
    m_threadNotifyEvent = event;
}

void CThread::SetEvent()  //Unlock
{
    ::SetEvent(m_hWaitEvent);
}

void CThread::ResetEvent()  //Lock
{
    ::ResetEvent(m_hWaitEvent);
}

void CThread::sleep_milliseconds(int count)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(count));
}

DWORD WINAPI CThread::ThreadProc(LPVOID lpvThreadParm)
{
    CThread* thread = (CThread*)lpvThreadParm;
    CThreadNotifyEvent* event = thread->m_threadNotifyEvent;

    try
    {
        thread->Execute();
    }
    catch (std::exception& e)
    {
        if (event)
            event->OnException(e);
    }

    try
    {
        if (event)
            event->OnTerminate(thread);
    }
    catch (std::exception& e)
    {
        if (event)
            event->OnException(e);
    }

    return 0;
}
