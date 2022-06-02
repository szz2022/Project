#pragma once

#include "../module/common/proto.h"

class CThread;

//������CThread���Priority���ԵĿ���ֵ�������������������ȼ�ȷ��ÿ���߳�CPU����
enum class CThreadPriority
{
    tpIdle = THREAD_PRIORITY_IDLE,                   //ֻ�е�ϵͳ����ʱ���߳�ִ��
    tpLowest = THREAD_PRIORITY_LOWEST,               //�߳������߱�������2��
    tpLower = THREAD_PRIORITY_BELOW_NORMAL,          //�߳������߱�������1��
    tpNormal = THREAD_PRIORITY_NORMAL,               //�߳������߱�������
    tpHigher = THREAD_PRIORITY_ABOVE_NORMAL,         //�߳������߱�������1��
    tpHighest = THREAD_PRIORITY_HIGHEST,             //�߳������߱�������2��
    tpTimeCritical = THREAD_PRIORITY_TIME_CRITICAL   //�߳����������
};


//�߳�֪ͨ��Ϣ��
class CThreadNotifyEvent
{
public:
    //���̵߳�Execute�����Ѿ��������ڸ��̱߳�ɾ��֮ǰ����
    virtual void OnTerminate(CThread* thread) = 0;
    //���̷߳����쳣ʱ����
    virtual void OnException(std::exception& e) = 0;
};


class CThread
{
public:
    //����һ���̶߳����ʵ����������Executeֱ��Resume���ú�ű�����
    CThread();
    virtual ~CThread();

    bool IsActive();  //�߳��Ƿ��Ѿ�����

    bool IsTerminated();  //���IsTerminated == true˵������Ҫ����߳���ֹ��

    bool Resume();  //����ִ��һ��������̡߳��ɹ�����true;

    bool Suspend();  //����һ�������е��̡߳��ɹ�����true;

    void Terminate(); //��ֹ�߳�

    void Wait();      //�ȴ��߳̽��

    HANDLE GetHandle();  //�����߳̾��

    CThreadPriority GetPriority();//�����߳����ȼ�

    void SetPriority(CThreadPriority priority); //���ø��߳�����ڽ����������̵߳����ȼ�

    void Attach(CThreadNotifyEvent* event); //ע���߳�֪ͨ��Ϣevent

    //�����ź���,�൱��UnLock
    void SetEvent();

    //�ͷ��ź���,�൱��Lock
    void ResetEvent();

    void sleep_milliseconds(int count);

protected:
    virtual void Execute() = 0;

private:
    CThreadNotifyEvent* m_threadNotifyEvent;   //�۲���
    HANDLE m_hHandle;     //�߳̾��
    HANDLE m_hWaitEvent;  //�¼�������
    DWORD m_dwThreadID;   //�߳�ID
    bool m_bActive;       //��ʶ�߳��Ƿ��ڼ���״̬
    bool m_bTerminated;   //��ʶ�Ƿ���Ҫ�����߳�

    //!�����̱߳����һ��ָ���ĺ�����ʼִ�У��ú�����Ϊ�̺߳������������������ԭ�ͣ�
    static DWORD WINAPI ThreadProc(LPVOID lpvThreadParm);
};