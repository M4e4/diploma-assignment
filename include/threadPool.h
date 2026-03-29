#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <atomic>
#include <functional>

class ThreadPool
{
public:
	ThreadPool(int number);
	~ThreadPool();

public:
	void push(std::function<void()> task);
	void work();
	void wait();
	bool isIdle() const;

private:
	std::vector<std::thread> threads;
	std::queue<std::function<void()>> tasks;
	std::mutex mtx;
	std::condition_variable cv;
	std::atomic<int> activeTasks = 0;
	std::atomic<bool> stop;
};

#endif // !THREAD_POOL_H
