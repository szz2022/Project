#pragma once

#include <string>
#include <deque>
#include <mutex>
#include <condition_variable>

template<typename T>
class MessageQueue
{
public:
	MessageQueue() = default;
	~MessageQueue() = default;

	void push_b(T const& msg);
	void push_f(T const& msg);
	void pop(T& msg);
	bool Empty();
	int Size();


private:
	std::deque<T> mQue;
	std::mutex mMutex;
	std::condition_variable mCondition;
};

template<typename T>
void MessageQueue<T>::push_b(T const& msg)
{
	std::unique_lock<std::mutex> lock(mMutex);
	mQue.push_back(msg);
	mCondition.notify_one();
}

template<typename T>
void MessageQueue<T>::push_f(T const& msg)
{
	std::unique_lock<std::mutex> lock(mMutex);
	mQue.push_front(msg);
	mCondition.notify_one();
}


template<typename T>
void MessageQueue<T>::pop(T& msg)
{
	std::unique_lock<std::mutex> lock(mMutex);
	while (mQue.empty())
	{
		mCondition.wait(lock);
	}
	msg = mQue.front();
	mQue.pop_front();
}

template<typename T>
bool MessageQueue<T>::Empty()
{
	std::unique_lock<std::mutex> lock(mMutex);
	return mQue.empty();
}

template<typename T>
int MessageQueue<T>::Size()
{
	std::unique_lock<std::mutex> lock(mMutex);
	return mQue.size();
}



template <class T>
class CQueue
{
protected:
    std::queue<T> _queue;
private:
    typename std::queue<T>::size_type _size_max;
    // Thread gubbins
    std::mutex _mutex;
    std::condition_variable _fullQue;
    std::condition_variable _empty;

    // Exit
    // 原子操作
    std::atomic_bool _quit;
    std::atomic_bool _finished;

public:
    CQueue(const size_t size_max) :_size_max(size_max) {
        _quit = ATOMIC_VAR_INIT(false);
        _finished = ATOMIC_VAR_INIT(false);
    }

    CQueue(CONST CQueue&) = delete;

    ~CQueue()
    {
        Quit();
        while (_queue.size())
            ;
    }

    bool Push(T data)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        while (!_quit && !_finished)
        {
            if (_queue.size() < _size_max)
            {
                _queue.push(std::move(data));
                _empty.notify_all();
                return true;
            }
            else
            {
                // wait的时候自动释放锁，如果wait到了会获取锁
                // _fullQue.wait(lock);
                return false;
            }
        }

        return false;
    }

    bool Pop(T data)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        while (!_quit)
        {
            if (!_queue.empty())                // 队列非空
            {
                data = _queue.front();
                _queue.pop();

                _fullQue.notify_all();       // 通知所有 由于满队无法加入的线程
                return true;
            }
            else if (_queue.empty() && _finished)   // 队列为空 且不再加入
            {
                return false;
            }
            else
            {
                // _empty.wait(lock);          // 等待队列加入元素
                return false;
            }
        }
        return false;
    }

    std::shared_ptr<T> Pop(void)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        std::shared_ptr<T> res = nullptr;
        while (!_quit)
        {
            if (!_queue.empty())                // 队列非空
            {
                //data = std::move(_queue.front());
                res = std::make_shared<T>(_queue.front());
                _queue.pop();

                _fullQue.notify_all();       // 通知所有 由于满队无法加入的线程

                return res;
            }
            else if (_queue.empty() && _finished)   // 队列为空 且不再加入
            {
                return res;     // 无数据进入 智能返回一个空指针 (可能出错)
            }
            else
            {
                _empty.wait(lock);          // 等待队列加入元素
            }
        }
        return false;
    }

 
    void Finished()
    {
        _finished = true;
        _empty.notify_all();
    }

    void Quit()
    {
        _quit = true;
        _empty.notify_all();
        _fullQue.notify_all();
    }

    int Size()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        return static_cast<int>(_queue.size());
    }


    bool Empty(void)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        return (0 == _queue.size());
    }

    bool Clear(void)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        while (!_queue.empty())
        {
            Pop();  // 依次弹出数据
        }
        return true;
    }
};


// 无界多线程队列的实现
