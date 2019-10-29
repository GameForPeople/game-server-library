#pragma once

#define WONSY_PCH

// C++ Base
#include <iostream>
#include <chrono>

// C++11
#include <mutex>
#include <atomic>
#include <thread>
#include <future>

// C++ 17
#include <shared_mutex>

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
#define		_NORETURN		[[noreturn]]
#define		_DEPRECATED		[[deprecated]]
#define		_MAYBE_UNUSED	[[maybe_unused]]
#define		_FALLTHROUGH	[[fallthrough]]
#define		_NODISCARD		[[nodiscard]]

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