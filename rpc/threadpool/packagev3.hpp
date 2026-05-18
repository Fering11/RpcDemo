#ifndef _FR_PACKAGEv3_HEADER
#define _FR_PACKAGEv3_HEADER
#include <exception>
#include <functional>
#include <future>
#include <new> // for std::align_val_t

namespace packagev3 {
namespace detail {

constexpr size_t Storage_Size = 7; // vtab* = 8 ; 8 + 56 = 64

//是否为过度对齐
template<class _Fx>
constexpr bool is_over_aligned = alignof(_Fx) > alignof(std::max_align_t);
//是否需要走堆分配路线
template<class _Fx>
constexpr bool require_on_heap = is_over_aligned<_Fx>
|| sizeof(_Fx) > sizeof(void*) * Storage_Size;

//TODO add noexcept
template<class _Rx, class... _Ax>
struct _MFPack {
	union alignas(std::max_align_t) _sbo_storage {
		void* ptr[Storage_Size];
		char data;

		template<class _Dx>
		void set_heap(_Dx* const _func)noexcept {
			ptr[0] = _func;
		}

		template<class _Dx>
		_Dx* stack_ptr()const noexcept {
			return reinterpret_cast<_Dx*>(&(const_cast<_sbo_storage*>(this)->data));
		}

		template<class _Dx>
		_Dx* heap_ptr()const noexcept {
			return static_cast<_Dx*>(ptr[0]);
		}

	}storage;

	struct _virtual_table {
		_Rx(*_Invoke)(const _sbo_storage&, _Ax&&...);
		void(*_Move)(_sbo_storage&, _sbo_storage&);
		void(*_Destory)(_sbo_storage&) noexcept; //析构是noexcept
	}*vtab = nullptr;
	/*
	* 判空时，不能只通过storage里面的内容进行判断，里面的内容是不确定的
	* 需要结合vtab的属性进行判断。如果vtab为空，则storage内容无意义
	*/
};

template<class _Fx, class _Dx>
void* _Alloca_Heap(_Fx&& _Fn) {
	struct _Guard {
		void* ptr = nullptr;

		~_Guard() {
			if (ptr) {
				if constexpr (is_over_aligned<_Dx>) {
					::operator delete(ptr, static_cast<std::align_val_t>(alignof(_Dx)));
				} else {
					::operator delete(ptr);
				}
			}
		}
	};

	_Guard guard;
	void* palloc;
	if constexpr (is_over_aligned<_Dx>) {
		palloc = ::operator new(sizeof(_Dx), static_cast<std::align_val_t>(alignof(_Dx)));
	} else {
		palloc = ::operator new(sizeof(_Dx));
	}
	guard.ptr = palloc;
	//构造
	::new(palloc) _Dx(std::forward<_Fx>(_Fn));
	guard.ptr = nullptr;
	return palloc;
}

template<class _Rx, class... _Ax>
_Rx _Invalid_Function(const typename _MFPack<_Rx, _Ax...>::_sbo_storage&,
	_Ax&&...) {
	//看情况再设置不同的异常效果
	throw std::exception("Invaild function.");
}

template<class _Dx, class _OrDx, class _Rx, class... _Ax>
_Rx _Invoke_Stack(const typename _MFPack<_Rx, _Ax...>::_sbo_storage& _storage,
	_Ax&&... _args) {
	if constexpr (std::is_void_v<_Rx>) {
		std::invoke(static_cast<_OrDx>(*_storage.stack_ptr<_Dx>()),
			std::forward<_Ax>(_args)...);
	} else {
		return std::invoke(static_cast<_OrDx>(*_storage.stack_ptr<_Dx>()),
			std::forward<_Ax>(_args)...);
	}
}
template<class _Dx, class _OrDx, class _Rx, class... _Ax>
_Rx _Invoke_Heap(const typename _MFPack<_Rx, _Ax...>::_sbo_storage& _storage,
	_Ax&&... _args) {
	if constexpr (std::is_void_v<_Rx>) {
		std::invoke(static_cast<_OrDx>(*_storage.heap_ptr<_Dx>()),
			std::forward<_Ax>(_args)...);
	} else {
		return std::invoke(static_cast<_OrDx>(*_storage.heap_ptr<_Dx>()),
			std::forward<_Ax>(_args)...);
	}
}
//void(*_Move)(_sbo_storage&, _sbo_storage&);

template<class _Rx, class... _Ax>
void _Move_heap(typename _MFPack<_Rx, _Ax...>::_sbo_storage& _dest,
	typename _MFPack<_Rx, _Ax...>::_sbo_storage& _src) {
	//使用memcpy复制
	memcpy(&_dest.data, &_src.data, sizeof(typename _MFPack<_Rx, _Ax...>::_sbo_storage));
}
//调用对象的移动构造函数，再析构src
template<class _Dx, class _Rx, class... _Ax>
void _Move_stack(typename _MFPack<_Rx, _Ax...>::_sbo_storage& _dest,
	typename _MFPack<_Rx, _Ax...>::_sbo_storage& _src) {
	auto src_ptr = _src.stack_ptr<_Dx>();
	::new(_dest.stack_ptr<_Dx>()) _Dx(std::move(*src_ptr));
	src_ptr->~_Dx();
}
template<class _Dx, class _Rx, class... _Ax>
void _Destory_heap(typename _MFPack<_Rx, _Ax...>::_sbo_storage& _dest) noexcept{
	auto obj = _dest.heap_ptr<_Dx>();
	obj->~_Dx();
	if constexpr (is_over_aligned<_Dx>) {
		::operator delete (obj, std::align_val_t{ alignof(_Dx) });
	} else {
		::operator delete (obj);
	}
}

template<class _Dx, class _Rx, class... _Ax>
void _Destory_stack(typename _MFPack<_Rx, _Ax...>::_sbo_storage& _dest) noexcept{
	auto obj = _dest.stack_ptr<_Dx>();
	obj->~_Dx();
}

template<class _Rx,class... _Ax>
class _MFBase {
	using _MyPack = _MFPack<_Rx, _Ax...>;
	using _MyTable = typename _MyPack::_virtual_table;
	using _MyStorage = typename _MyPack::_sbo_storage;

	//构建虚表
	template<class _Dx,class _OrDx>
	static constexpr _MyTable _create_vtable() {
		_MyTable table{ nullptr,nullptr,nullptr };

		if constexpr (require_on_heap<_Dx>) {
			table._Invoke = _Invoke_Heap<_Dx, _OrDx, _Rx, _Ax...>;
			//堆移动，我们移动一个整个storage,memcpy
			table._Move = _Move_heap<_Rx, _Ax...>;
			//堆销毁，调用析构函数，并且delete
			table._Destory = _Destory_heap<_Dx, _Rx, _Ax...>;
		} else {
			table._Invoke = _Invoke_Stack<_Dx, _OrDx, _Rx, _Ax...>;
			table._Move = _Move_stack<_Dx, _Rx, _Ax...>;
			table._Destory = _Destory_stack<_Dx, _Rx, _Ax...>;
		}
		return table;
	}

	template<class _Dx, class _OrDx>
	static constexpr const _MyTable* _get_vtable_ptr() {
		static constexpr _MyTable tab = _create_vtable<_Dx, _OrDx>();
		return &tab;
	}
	
public:
	const _MyTable* vtable()const noexcept {
		return pack.vtab;
	}
	
	_MyStorage& storage() noexcept {
		return pack.storage;
	}
	//销毁自身数据,转换为 empty状态
	void destory() noexcept{
		if (!empty()) {
			pack.vtab->_Destory(pack.storage);
			pack.vtab = nullptr;
		}
	}
	//将_other移动到自身，默认自身为空
	void move_from(_MyPack&& _other) {
		if (_other.vtab != nullptr) {
			_other.vtab->_Move(pack.storage, _other.storage);
			pack.vtab = _other.vtab; 
			_other.vtab = nullptr;//这里我们不可以使用destory 会导致双重释放
		}
	}
	void assign(_MFBase&& _other) {
		if (!empty()) {
			this->destory();
		}
		move_from(std::move(_other.pack));
	}
	//isNoexcept
	template<class _Fx,class _Dx, class _OrDx>
	void construct(_Fx&& _Fn) {
		//设置缓存
		if constexpr (require_on_heap<_Dx>) {
			//堆分配道路
			pack.storage.set_heap(_Alloca_Heap<_Fx, _Dx>(std::forward<_Fx>(_Fn)));
		} else {
			//栈内联
			std::construct_at(pack.storage.stack_ptr<_Dx>(), std::forward<_Fx>(_Fn));
		}
		//创建虚表
		//这里去除了const修饰，但是保证vtab里面的成员不会被修改，只会有vtab = *这种操作。
		pack.vtab = const_cast<_MyTable*>(_get_vtable_ptr<_Dx, _OrDx>());
	}

	bool empty() const noexcept{
		return pack.vtab == nullptr;
	}
	//安全获得invoke
	auto get_invoke()const -> _Rx(*)(const _MyStorage&, _Ax&&...) {
		if (pack.vtab == nullptr) {
			//返回一个抛出异常的函数
			return &_Invalid_Function<_Rx, _Ax...>;
		}
		return pack.vtab->_Invoke;
	}

	_MyPack pack;
};

template<class... _Sign>
class _MFCall {
	static_assert(sizeof...(_Sign) == 0&&false, "Wrong function type");
};

//萃取 const noexcept &
template<class _Rx,class..._Ax>
class _MFCall<_Rx(_Ax...)>:public _MFBase<_Rx,_Ax...> {
public:
	template<class _Ty>
	using _Origial = _Ty&;

	template<class _Dx>
	static constexpr bool _Invocable = std::_Is_invocable_r<_Rx, _Dx, _Ax...>::value;
	//可调用对象
	_Rx operator()(_Ax... args) {
		return this->get_invoke()(this->storage(), std::forward<_Ax>(args)...);
	}
};

template<class... _Sign>
class mfunction :private _MFCall<_Sign...> {
	using _Parent = _MFCall<_Sign...>;

	//从_Fn构造
	template<class _Fx>
	void _Assign(_Fx&& _Fn) {
		using _Dx = std::decay_t<_Fx>;
		using _OrDx = _Parent::template _Origial<_Dx>;
		static_assert(std::is_constructible_v<_Dx, _Fx>, "Function type wrong, try std::move?");
		//先销毁自身
		this->destory();

		this->template construct<_Fx, _Dx, _OrDx>(std::forward<_Fx>(_Fn));
	}

public:
	//接入的函数需要进行检查，首先是需要能够从_Fx构造到_Dx
	template<class _Fx>
	mfunction(_Fx&& _Fn) {
		using _Dx = std::decay_t<_Fx>;
		using _OrDx = _Parent::template _Origial<_Dx>;
		static_assert(std::is_constructible_v<_Dx, _Fx>, "Function type wrong, try std::move?");
		static_assert(_Parent::template _Invocable<_OrDx>, "Callable signature does't match _Rx(_Ax...)");
		this->template construct<_Fx, _Dx, _OrDx>(std::forward<_Fx>(_Fn));
	}

	mfunction() {
	}
	
	mfunction& operator=(std::nullptr_t) noexcept{
		this->destory();
		return *this;
	}
	//禁止复制
	mfunction(const mfunction&) = delete;
	mfunction& operator=(const mfunction&) = delete;

	//_Move函数不保证move noexcept
	mfunction(mfunction&& _Other) {
		_Parent::move_from(std::move(_Other.pack));
	}

	mfunction& operator=(mfunction&& _Other) {
		if (this != std::addressof(_Other)) {
			_Parent::assign(std::move(_Other));
		}
		return *this;
	}

	//会检查是否处于empty状态
	~mfunction() {
		this->destory();
	}

	using _Parent::operator();
	using _Parent::empty;
	using _Parent::assign;
};

}

class package {
	using _MyFunc = detail::mfunction<void()>;
public:
	//无返回值函数
	template<class _Call , class... _Ax >
	void make_detach(_Call&& _Fn,_Ax&&... _Args) {
		callable_ = std::move(_MyFunc([inv = std::forward<_Call>(_Fn)
			, ...pack = std::forward<_Ax>(_Args)]()mutable {
				//这里我们不处理异常，也不吞掉异常，全靠用户设计
				inv(pack...);
			}));
	}
	//class _Rx = std::invoke_result_t<_Call,_Ax...>
	template<class _Call, class... _Ax, class _Rx = std::invoke_result_t<_Call, _Ax...>>
	auto make(_Call&& _Fn, _Ax&&... _Args) {
		std::promise<_Rx> pro;
		auto future = pro.get_future();
		
		callable_ = std::move(_MyFunc([inv = std::forward<_Call>(_Fn)
			, ...pack = std::forward<_Ax>(_Args)
			, mypro = std::move(pro)]()mutable {
				try {
					if constexpr (std::is_void_v<_Rx>) {
						inv(pack...);
						mypro.set_value();
					} else {
						mypro.set_value(inv(pack...));
					}
				} catch (...) {
					mypro.set_exception(std::current_exception());
				}
			}));

		return future;
	}
	
	void operator()() {
		callable_();
	}
	bool empty() {
		return callable_.empty();
	}
private:
	_MyFunc callable_; //lambda
};

}

#endif