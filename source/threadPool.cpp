#include "threadPool.h"






ThreadPool::ThreadPool(int number)
	:	activeTasks(0), stop(false)
{
	for (int i{}; i < number; ++i) threads.emplace_back(&ThreadPool::work, this);
}

ThreadPool::~ThreadPool()
{
	stop.store(true);
	cv.notify_all();

	for (auto& t : threads) t.join();
}





void ThreadPool::push(std::function<void()> task)
{
	std::lock_guard<std::mutex> lg(mtx);
	tasks.push(std::move(task));
	++activeTasks;
	cv.notify_one();
}





void ThreadPool::work()
{
	while (true)
	{
		std::function<void()> task;
		std::unique_lock<std::mutex> ul(mtx);
		cv.wait(ul, [this]() { return stop.load() || !tasks.empty(); });

		if (stop.load() && tasks.empty()) return;

		task = std::move(tasks.front());
		tasks.pop();
		ul.unlock();
		task();
		--activeTasks;
		cv.notify_all();
	}
}





void ThreadPool::wait()
{
	std::unique_lock<std::mutex> ul(mtx);
	cv.wait(ul, [this]() { return isIdle(); });
}





bool ThreadPool::isIdle() const
{
	return tasks.empty() && activeTasks.load() == 0;
}