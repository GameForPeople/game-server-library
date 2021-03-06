#pragma once

//-----
#define WONSY_PCH
//-----

// C++ Base
#include <iostream>
#include <chrono>

#define NDEBUG
#include <cassert>

// C++11
#include <mutex>
#include <atomic>
#include <thread>
#include <future>

// C++ 17
#include <shared_mutex>
#include <string_view>
#include <optional>

// C++ STL
#include <unordered_set>
#include <array>
#include <vector>
#include <list>

// C++ PPL
#include <concurrent_vector.h>
#include <concurrent_queue.h>
#include <concurrent_priority_queue.h>
#include <concurrent_unordered_map.h>
#include <concurrent_unordered_set.h>

// Attributes
#define	_NORETURN		[[noreturn]]
#define	_DEPRECATED		[[deprecated]]
#define	_MAYBE_UNUSED	[[maybe_unused]]
#define	_FALLTHROUGH	[[fallthrough]]
#define	_NODISCARD		[[nodiscard]]

#define	_INLINE			inline
#define	_DO_NOT_DELETE	/* copyed pointer */	
#define	_NOT_NULLPTR

using namespace std;
using namespace std::chrono;
using namespace concurrency;

namespace ATOMIC_UTIL
{
	template <class TYPE> bool T_CAS(std::atomic<TYPE>* addr, TYPE oldValue, TYPE newValue) noexcept
	{
		return atomic_compare_exchange_strong(addr, &oldValue, newValue);
	};

	template <class TYPE> bool T_CAS(volatile TYPE* addr, TYPE oldValue, TYPE newValue) noexcept
	{
		return atomic_compare_exchange_strong(reinterpret_cast<volatile std::atomic<TYPE>*>(addr), &oldValue, newValue);
	};
}

namespace ERROR_UTIL
{
	_NORETURN void Error(const std::string_view errorString);

	_NORETURN void Error(const std::wstring_view errorString);
}