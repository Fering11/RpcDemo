#ifndef _SHARED_STATE_V5_HEADER
#define _SHARED_STATE_V5_HEADER

#include <future>
#include <mutex>
#include <exception>
#include <memory>
#include "packagev3.hpp"

namespace poolv5 {
namespace detail {

// Forward declaration
class pool;

// Shared state base class
template<class T>
class SharedState {
public:
    SharedState() = default;
    
    void set_pool(pool* p) {
        pool_ = p;
    }
    
    pool* get_pool() const {
        return pool_;
    }
    
    std::shared_future<T> get_future() {
        std::lock_guard<std::mutex> lock(mtx_);
        if (!future_retrieved_) {
            shared_future_ = promise_.get_future().share();
            future_retrieved_ = true;
        }
        return shared_future_;
    }
    
    void set_value(T&& value) {
        packagev3::detail::mfunction<void()> cont_to_execute;
        {
            std::lock_guard<std::mutex> lock(mtx_);
            promise_.set_value(std::forward<T>(value));
            completed_ = true;
            cont_to_execute = std::move(continuation_);
        }
        // Execute continuation outside the lock
        if (!cont_to_execute.empty()) {
            cont_to_execute();
        }
    }
    
    void set_value(const T& value) {
        packagev3::detail::mfunction<void()> cont_to_execute;
        {
            std::lock_guard<std::mutex> lock(mtx_);
            promise_.set_value(value);
            completed_ = true;
            cont_to_execute = std::move(continuation_);
        }
        // Execute continuation outside the lock
        if (!cont_to_execute.empty()) {
            cont_to_execute();
        }
    }
    
    void set_exception(std::exception_ptr ex) {
        packagev3::detail::mfunction<void()> cont_to_execute;
        {
            std::lock_guard<std::mutex> lock(mtx_);
            promise_.set_exception(ex);
            completed_ = true;
            cont_to_execute = std::move(continuation_);
        }
        // Execute continuation outside the lock
        if (!cont_to_execute.empty()) {
            cont_to_execute();
        }
    }
    
    template<class F>
    void set_continuation(F&& cont) {
        bool should_execute = false;
        packagev3::detail::mfunction<void()> cont_to_execute;
        
        {
            std::lock_guard<std::mutex> lock(mtx_);
            
            if (continuation_set_) {
                // Continuation already set, ignore
                return;
            }
            
            if (completed_) {
                // Task already completed, execute immediately
                should_execute = true;
                cont_to_execute = packagev3::detail::mfunction<void()>(std::forward<F>(cont));
            } else {
                // Task not completed, save continuation
                continuation_ = packagev3::detail::mfunction<void()>(std::forward<F>(cont));
                continuation_set_ = true;
            }
        }
        
        // Execute outside the lock if needed
        if (should_execute && !cont_to_execute.empty()) {
            cont_to_execute();
        }
    }
    
private:
    std::promise<T> promise_;
    std::shared_future<T> shared_future_;
    std::mutex mtx_;
    packagev3::detail::mfunction<void()> continuation_;
    pool* pool_ = nullptr;
    bool future_retrieved_ = false;
    bool completed_ = false;
    bool continuation_set_ = false;
};

// void specialization
template<>
class SharedState<void> {
public:
    SharedState() = default;
    
    void set_pool(pool* p) {
        pool_ = p;
    }
    
    pool* get_pool() const {
        return pool_;
    }
    
    std::shared_future<void> get_future() {
        std::lock_guard<std::mutex> lock(mtx_);
        if (!future_retrieved_) {
            shared_future_ = promise_.get_future().share();
            future_retrieved_ = true;
        }
        return shared_future_;
    }
    
    void set_value() {
        packagev3::detail::mfunction<void()> cont_to_execute;
        {
            std::lock_guard<std::mutex> lock(mtx_);
            promise_.set_value();
            completed_ = true;
            cont_to_execute = std::move(continuation_);
        }
        // Execute continuation outside the lock
        if (!cont_to_execute.empty()) {
            cont_to_execute();
        }
    }
    
    void set_exception(std::exception_ptr ex) {
        packagev3::detail::mfunction<void()> cont_to_execute;
        {
            std::lock_guard<std::mutex> lock(mtx_);
            promise_.set_exception(ex);
            completed_ = true;
            cont_to_execute = std::move(continuation_);
        }
        // Execute continuation outside the lock
        if (!cont_to_execute.empty()) {
            cont_to_execute();
        }
    }
    
    template<class F>
    void set_continuation(F&& cont) {
        bool should_execute = false;
        packagev3::detail::mfunction<void()> cont_to_execute;
        
        {
            std::lock_guard<std::mutex> lock(mtx_);
            
            if (continuation_set_) {
                // Continuation already set, ignore
                return;
            }
            
            if (completed_) {
                // Task already completed, execute immediately
                should_execute = true;
                cont_to_execute = packagev3::detail::mfunction<void()>(std::forward<F>(cont));
            } else {
                // Task not completed, save continuation
                continuation_ = packagev3::detail::mfunction<void()>(std::forward<F>(cont));
                continuation_set_ = true;
            }
        }
        
        // Execute outside the lock if needed
        if (should_execute && !cont_to_execute.empty()) {
            cont_to_execute();
        }
    }
    
private:
    std::promise<void> promise_;
    std::shared_future<void> shared_future_;
    std::mutex mtx_;
    packagev3::detail::mfunction<void()> continuation_;
    pool* pool_ = nullptr;
    bool future_retrieved_ = false;
    bool completed_ = false;
    bool continuation_set_ = false;
};

} // namespace detail
} // namespace poolv5

#endif
