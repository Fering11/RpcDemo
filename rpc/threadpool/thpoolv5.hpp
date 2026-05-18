#ifndef _HEAD_POOLV5
#define _HEAD_POOLV5

#include <functional>
#include <mutex>
#include <deque>
#include <condition_variable>
#include <atomic>
#include <memory>

#include "packagev3.hpp"
#include "futurev5.hpp"

namespace poolv5 {

namespace detail {

class pool {
	using package = packagev3::package;

	struct work_thread {
		std::deque<package> task_queue{};
		std::mutex mtx{};
		std::thread th{};
		size_t id{};
	};

public:
	pool(size_t th_size){
		for (size_t i = 0; i != th_size; ++i) {
			work_thread* worker = new work_thread{};
			worker->id = i;
			worker->th = std::thread(&pool::workThread, this, worker);

			workers_.push_back(std::unique_ptr<work_thread>(worker));
		}
	}

	~pool() {
		if (!shutdown_.load(std::memory_order_acquire)) {
			shutdown_.store(true, std::memory_order_release);
			cond_th_.notify_all();
			for (auto& wk : workers_) {
				if (wk->th.joinable()) {
					wk->th.join();
				}
			}
		}
	}

	template<class _Fx,class... _Ax>
	void post_detach(_Fx&& _Fn,_Ax&&... _Args) {
		package pack;
		pack.make_detach(std::forward<_Fx>(_Fn), std::forward<_Ax>(_Args)...);

		const auto& victim = workers_[round_index_.fetch_add(1, std::memory_order_relaxed) % workers_.size()].get();
		
		unfinished_.fetch_add(1, std::memory_order_release);

		std::unique_lock<std::mutex> lock(victim->mtx);
		victim->task_queue.push_front(std::move(pack));
		lock.unlock();

		cond_th_.notify_one();
	}
	// Noticed:Never call get in the work thread like this:
	/*
	auto fut = thpool.post([i, &thpool]() {
    auto inner = thpool.post([i]() { return i; })
        .then([](int v) { return v + 10; });
    
    return inner.get();  //
	})
	*/
	// Post method that returns Future<T>
	template<class _Fx, class... _Ax>
	auto post(_Fx&& _Fn, _Ax&&... _Args) -> Future<typename std::_Invoke_result_t<_Fx, _Ax...>> {
		using ReturnType = typename std::_Invoke_result_t<_Fx, _Ax...>;
		
		// Create shared state
		auto state = std::make_shared<SharedState<ReturnType>>();
		state->set_pool(this);
		
		// Create Future
		Future<ReturnType> future(state);
		
		// Wrap task
		package pack;
		if constexpr (std::is_void_v<ReturnType>) {
			pack.make_detach([state, fn = std::forward<_Fx>(_Fn), 
			                  ...args = std::forward<_Ax>(_Args)]() mutable {
				try {
					fn(args...);
					state->set_value();
				} catch (...) {
					state->set_exception(std::current_exception());
				}
			});
		} else {
			pack.make_detach([state, fn = std::forward<_Fx>(_Fn), 
			                  ...args = std::forward<_Ax>(_Args)]() mutable {
				try {
					ReturnType result = fn(args...);
					state->set_value(std::move(result));
				} catch (...) {
					state->set_exception(std::current_exception());
				}
			});
		}

		const auto& victim = workers_[round_index_.fetch_add(1, std::memory_order_relaxed) % workers_.size()].get();

		unfinished_.fetch_add(1, std::memory_order_release);

		std::unique_lock<std::mutex> lock(victim->mtx);
		victim->task_queue.push_front(std::move(pack));
		lock.unlock();
		cond_th_.notify_one();
		
		return future;
	}

	void wait() {
		std::unique_lock<std::mutex> lock(mtx_wait_);
		cond_wait_.wait(lock, [this]() {
			return unfinished_.load(std::memory_order_acquire) == 0;
		});
	}

	void shutdown() {
		shutdown_.store(true, std::memory_order_release);
		cond_th_.notify_all();
		
		for (auto& wk : workers_) {
			if (wk->th.joinable()) {
				wk->th.join();
			}
		}
	}

private:
	void workThread(work_thread* mine) {
		for (;;) {
			if (shutdown_.load(std::memory_order_acquire)) {
				break;
			}

			// 1. Process own tasks first
			{
				std::unique_lock<std::mutex> local_lock(mine->mtx);

				if (!mine->task_queue.empty()) {
					package task = std::move(mine->task_queue.front());
					mine->task_queue.pop_front();
					local_lock.unlock();

					task();
					unfinished_.fetch_sub(1, std::memory_order_release);
					cond_wait_.notify_all();
					continue;
				}
			}
			
			// 2. Try to steal tasks
			if (workers_.size() > 1) {
				size_t victim_idx = round_index_.fetch_add(1, std::memory_order_relaxed) % workers_.size();

				if (victim_idx == mine->id) {
					victim_idx = (victim_idx + 1) % workers_.size();
				}

				auto& victim = workers_[victim_idx];
				std::unique_lock<std::mutex> victim_lock(victim->mtx);
				if (!victim->task_queue.empty()) {
					package task = std::move(victim->task_queue.back());
					victim->task_queue.pop_back();
					victim_lock.unlock();

					task();

					unfinished_.fetch_sub(1, std::memory_order_release);
					cond_wait_.notify_all();
					continue;
				}
			}

			// 3. Wait for new tasks
			std::unique_lock<std::mutex> global_mtx(mtx_th_);
			cond_th_.wait(global_mtx, [this]() {
				return shutdown_.load(std::memory_order_acquire) ||
					unfinished_.load(std::memory_order_acquire) > 0;
			});
		}
	}

	std::vector<std::unique_ptr<work_thread>> workers_{};
	std::atomic<size_t> round_index_{};
	std::atomic<size_t> unfinished_{};

	std::atomic<bool> shutdown_{};
	
	std::mutex mtx_wait_{};
	std::condition_variable cond_wait_{};

	std::mutex mtx_th_{};
	std::condition_variable cond_th_{};
};

} // namespace detail

// Export pool type
using pool = detail::pool;

} // namespace poolv5

// Implementation of Future<T>::then_impl
namespace poolv5 {

template<class T>
template<class ReturnType, class Func>
Future<ReturnType> Future<T>::then_impl(Func&& func, bool inline_exec) {
    if (!state_) {
        throw std::future_error(std::future_errc::no_state);
    }
    
    // Create new shared state
    auto next_state = std::make_shared<detail::SharedState<ReturnType>>();
    next_state->set_pool(state_->get_pool());
    
    // Create returned Future
    Future<ReturnType> next_future(next_state);
    
    // IMPORTANT: Get the shared_future BEFORE setting continuation
    // This ensures the future is retrieved before the task might complete
    auto current_future = state_->get_future();
    
    // Define continuation task
    auto continuation = [current_future, func = std::forward<Func>(func), next_state]() mutable {
        try {
            // Wait for previous task to complete and get result
            T value = current_future.get();
            
            // Execute user callback
            if constexpr (std::is_void_v<ReturnType>) {
                func(std::move(value));
                next_state->set_value();
            } else {
                ReturnType result = func(std::move(value));
                next_state->set_value(std::move(result));
            }
        } catch (...) {
            // Exception propagation
            next_state->set_exception(std::current_exception());
        }
    };
    
    if (inline_exec) {
        // Inline execution: execute directly in the thread that completes the previous task
        state_->set_continuation(std::move(continuation));
    } else {
        // Thread pool execution: submit continuation to thread pool
        auto pool_ptr = state_->get_pool();
        if (!pool_ptr) {
            throw std::runtime_error("No thread pool associated with this future");
        }
        
        state_->set_continuation([pool_ptr, cont = std::move(continuation)]() mutable {
            pool_ptr->post_detach(std::move(cont));
        });
    }
    
    return next_future;
}

// Implementation of Future<void>::then_impl
template<class ReturnType, class Func>
Future<ReturnType> Future<void>::then_impl(Func&& func, bool inline_exec) {
    if (!state_) {
        throw std::future_error(std::future_errc::no_state);
    }
    
    // Create new shared state
    auto next_state = std::make_shared<detail::SharedState<ReturnType>>();
    next_state->set_pool(state_->get_pool());
    
    // Create returned Future
    Future<ReturnType> next_future(next_state);
    
    // IMPORTANT: Get the shared_future BEFORE setting continuation
    auto current_future = state_->get_future();
    
    // Define continuation task
    auto continuation = [current_future, func = std::forward<Func>(func), next_state]() mutable {
        try {
            // Wait for previous task to complete
            current_future.get();
            
            // Execute user callback (void type takes no parameters)
            if constexpr (std::is_void_v<ReturnType>) {
                func();
                next_state->set_value();
            } else {
                ReturnType result = func();
                next_state->set_value(std::move(result));
            }
        } catch (...) {
            // Exception propagation
            next_state->set_exception(std::current_exception());
        }
    };
    
    if (inline_exec) {
        // Inline execution
        state_->set_continuation(std::move(continuation));
    } else {
        // Thread pool execution
        auto pool_ptr = state_->get_pool();
        if (!pool_ptr) {
            throw std::runtime_error("No thread pool associated with this future");
        }
        
        state_->set_continuation([pool_ptr, cont = std::move(continuation)]() mutable {
            pool_ptr->post_detach(std::move(cont));
        });
    }
    
    return next_future;
}

} // namespace poolv5

#endif
