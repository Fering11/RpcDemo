#ifndef _FUTURE_V5_HEADER
#define _FUTURE_V5_HEADER

#include <future>
#include <memory>
#include <type_traits>
#include "shared_state.hpp"

namespace poolv5 {

// Forward declaration
namespace detail {
    class pool;
}

// Compatibility handling for MSVC
#ifdef _MSC_VER
    template<class Func, class... Args>
    using invoke_result_t = std::_Invoke_result_t<Func, Args...>;
#else
    template<class Func, class... Args>
    using invoke_result_t = std::invoke_result_t<Func, Args...>;
#endif

template<class T>
class Future {
public:
    Future() = default;
    
    explicit Future(std::shared_ptr<detail::SharedState<T>> state)
        : state_(std::move(state)) {}
    
    T get() {
        if (!state_) {
            throw std::future_error(std::future_errc::no_state);
        }
        return state_->get_future().get();
    }
    
    void wait() {
        if (!state_) {
            throw std::future_error(std::future_errc::no_state);
        }
        state_->get_future().wait();
    }
    
    bool valid() const {
        return state_ != nullptr;
    }
    
    // Execute continuation in thread pool
    template<class Func>
    auto then(Func&& func) -> Future<invoke_result_t<Func, T>> {
        using ReturnType = invoke_result_t<Func, T>;
        return then_impl<ReturnType>(std::forward<Func>(func), false);
    }
    
    // Execute continuation inline in current thread
    template<class Func>
    auto then_inline(Func&& func) -> Future<invoke_result_t<Func, T>> {
        using ReturnType = invoke_result_t<Func, T>;
        return then_impl<ReturnType>(std::forward<Func>(func), true);
    }
    
private:
    template<class ReturnType, class Func>
    Future<ReturnType> then_impl(Func&& func, bool inline_exec);
    
    std::shared_ptr<detail::SharedState<T>> state_;
};

// void specialization
template<>
class Future<void> {
public:
    Future() = default;
    
    explicit Future(std::shared_ptr<detail::SharedState<void>> state)
        : state_(std::move(state)) {}
    
    void get() {
        if (!state_) {
            throw std::future_error(std::future_errc::no_state);
        }
        state_->get_future().get();
    }
    
    void wait() {
        if (!state_) {
            throw std::future_error(std::future_errc::no_state);
        }
        state_->get_future().wait();
    }
    
    bool valid() const {
        return state_ != nullptr;
    }
    
    // For void type, callback takes no parameters
    template<class Func>
    auto then(Func&& func) -> Future<invoke_result_t<Func>> {
        using ReturnType = invoke_result_t<Func>;
        return then_impl<ReturnType>(std::forward<Func>(func), false);
    }
    
    template<class Func>
    auto then_inline(Func&& func) -> Future<invoke_result_t<Func>> {
        using ReturnType = invoke_result_t<Func>;
        return then_impl<ReturnType>(std::forward<Func>(func), true);
    }
    
private:
    template<class ReturnType, class Func>
    Future<ReturnType> then_impl(Func&& func, bool inline_exec);
    
    std::shared_ptr<detail::SharedState<void>> state_;
};

} // namespace poolv5

#endif
