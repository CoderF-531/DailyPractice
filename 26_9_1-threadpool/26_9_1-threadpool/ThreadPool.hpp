#pragma once
#define _CRT_SECURE_NO_WARNINGS


#include <thread>
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <list>

namespace ThreadPoolModel {
	using Task = std::packaged_task<void()>;

	/*
		首先我不希望线程池被拷贝，我希望它能以单例的形式在需要的地方调用, 
		那么单例模式就需要删除拷贝构造和拷贝赋值，所以我设计一个基类
	*/
	class NoneCopy {
	public:
		~NoneCopy() {}
	protected:
		NoneCopy(){}
	private:
		NoneCopy(const NoneCopy&) = delete;
		NoneCopy& operator=(const NoneCopy&) = delete;
	
	};
	/*
	然后让线程池ThreadPool类继承NoneCopy, 这样ThreadPool也就不支持拷贝构造和拷贝赋值了，
	拷贝构造和拷贝赋值的前提是其基类可以拷贝构造和赋值。
	*/
	class ThreadPool  : public NoneCopy{
	public:
			~ThreadPool() {
					stop();
				}
		/*
			局部静态变量只会在第一次调用这个函数时初始化一次。故可以作为单例模式。
			这种模式在C++ 11之前是不安全的，
			因为各平台编译器实现规则可能不统一导致多线程会生成多个实例。
		*/
		static ThreadPool& instance() {
			static ThreadPool ins;
			return ins;
		}
		//获取空闲线程个数
		int idleThreadCount() {
			return _thread_num;
		}

		template <class F, class... Args>
		auto commit(F&& f, Args&&... args) ->
			std::future<decltype(std::forward<F>(f)(std::forward<Args>(args)...))> {
			using RetType = decltype(std::forward<F>(f)(std::forward<Args>(args)...));
			if (_stop.load())
				return std::future<RetType>{};

			auto task = std::make_shared<std::packaged_task<RetType()>>(
				std::bind(std::forward<F>(f), std::forward<Args>(args)...));

			std::future<RetType> ret = task->get_future();
			{
				std::lock_guard<std::mutex> cv_mt(_cv_mt);
				_task.emplace([task] { (*task)(); });
			}
			_cv_lock.notify_one();
			return ret;
		}

	private:
		ThreadPool(unsigned int num = std::thread::hardware_concurrency())
			:_stop(false)
		{
			if (num <= 1)
				_thread_num = 2;
			else
				_thread_num = num;
			start();
		}

		void start()
		{
			//1、根据 _thread_num创建线程
			for (int i = 0; i < _thread_num; i++)
			{
				//	插入对应的线程对象 以及他要执行的任务
				_threads.emplace_back([this]() {
					while (!this->_stop.load())
					{
						Task task;
						{
							//加锁 因为上层while确定了没有stop 
							std::unique_lock<std::mutex> cv_mt(_cv_mt);
							this->_cv_lock.wait(cv_mt, [this] {
								return this->_stop.load() || !this->_task.empty();
								});
							// 生产者没有新任务直接退出
							if (this->_task.empty())
								return;
							//插入任务队列
							task = std::move(this->_task.front());
							this->_task.pop();
						}
						//PV 操作  让空闲线程执行对应 task 再 加回来
						this->_thread_num--;
						task();
						this->_thread_num++;
					}

					});
			}
		}
	

		void stop()
		{
			//把当前状态设为 停止
			_stop.store(true);
			//判断处理剩下任务

			_cv_lock.notify_all();
			//回收所有线程
			for (auto& td : _threads)
			{	
				if (td.joinable())
				{
					std::cout << "join thread" << td.get_id() << std::endl;
					td.join();
				}
			}
		}
		std::atomic_int _thread_num; //空闲线程个数
		std::queue<Task> _task; //任务队列
		std::vector<std::thread> _threads; //线程队列
		std::atomic_bool _stop;  //线程池退出状态
		std::mutex _cv_mt;    //互斥
		std::condition_variable _cv_lock; //条件变量

	};


	template<typename T>
	std::list<T>pool_thread_quick_sort(std::list<T> input) {
		if (input.empty())
		{
			return input;
		}
		std::list<T> result;
		result.splice(result.begin(), input, input.begin());
		T const& partition_val = *result.begin();
		typename std::list<T>::iterator divide_point =
			std::partition(input.begin(), input.end(),
				[&](T const& val) {return val < partition_val; });
		std::list<T> new_lower_chunk;
		new_lower_chunk.splice(new_lower_chunk.end(),
			input, input.begin(),
			divide_point);

		std::future<std::list<T> > new_lower = ThreadPool::instance().commit(pool_thread_quick_sort<T>, new_lower_chunk);

		std::list<T> new_higher(pool_thread_quick_sort(input));
		result.splice(result.end(), new_higher);
		result.splice(result.begin(), new_lower.get());
		return result;
	}
}