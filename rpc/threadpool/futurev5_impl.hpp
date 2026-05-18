#ifndef _FUTURE_V5_IMPL_HEADER
#define _FUTURE_V5_IMPL_HEADER

namespace poolv5 {

namespace detail {
    class pool;
}

// Future<T> 的 then_impl 实现
template<class T>
template<class ReturnType, class Func>
Future<ReturnType> Future<T>::then_impl(Func&& func, bool inline_exec) {
    if (!state_) {
        throw std::future_error(std::future_errc::no_state);
    }
    
    // 创建新的共享状态
    auto next_state = std::make_shared<detail::SharedState<ReturnType>>();
    next_state->set_pool(state_->get_pool());
    
    // 创建返回的 Future
    Future<ReturnType> next_future(next_state);
    
    // 获取当前 future 的副本用于延续任务
    auto current_future = state_->get_future();
    
    // 定义延续任务
    auto continuation = [current_future = std::move(current_future),
                         func = std::forward<Func>(func),
                         next_state]() mutable {
        try {
            // 等待前置任务完成并获取结果
            T value = current_future.get();
            
            // 执行用户回调
            if constexpr (std::is_void_v<ReturnType>) {
                func(std::move(value));
                next_state->set_value();
            } else {
                ReturnType result = func(std::move(value));
                next_state->set_value(std::move(result));
            }
        } catch (...) {
            // 异常传播
            next_state->set_exception(std::current_exception());
        }
    };
    
    if (inline_exec) {
        // 内联执行：直接在完成前置任务的线程中执行
        state_->set_continuation(std::move(continuation));
    } else {
        // 线程池执行：将延续任务提交到线程池
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

// Future<void> 的 then_impl 实现
template<>
template<class ReturnType, class Func>
Future<ReturnType> Future<void>::then_impl(Func&& func, bool inline_exec) {
    if (!state_) {
        throw std::future_error(std::future_errc::no_state);
    }
    
    // 创建新的共享状态
    auto next_state = std::make_shared<detail::SharedState<ReturnType>>();
    next_state->set_pool(state_->get_pool());
    
    // 创建返回的 Future
    Future<ReturnType> next_future(next_state);
    
    // 获取当前 future 的副本
    auto current_future = state_->get_future();
    
    // 定义延续任务
    auto continuation = [current_future = std::move(current_future),
                         func = std::forward<Func>(func),
                         next_state]() mutable {
        try {
            // 等待前置任务完成
            current_future.get();
            
            // 执行用户回调（void 类型无参数）
            if constexpr (std::is_void_v<ReturnType>) {
                func();
                next_state->set_value();
            } else {
                ReturnType result = func();
                next_state->set_value(std::move(result));
            }
        } catch (...) {
            // 异常传播
            next_state->set_exception(std::current_exception());
        }
    };
    
    if (inline_exec) {
        // 内联执行
        state_->set_continuation(std::move(continuation));
    } else {
        // 线程池执行
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
