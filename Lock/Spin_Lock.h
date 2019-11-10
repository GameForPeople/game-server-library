#pragma once

//#include "../GameServerLibrary/GameServerLibrary/stdafx.h"

#define WONSY_SPIN_LOCK

#ifndef WONSY_PCH
// C++11
#include <atomic>
#include <thread>

// Attributes or Others
#define _INLINE inline

// Using namespace
using namespace std;

// UTILs
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
#endif

namespace WonSY::SPIN_LOCK
{
	class SpinLock_0
	{
		/*
			atomic ���̺귯������ �����Խ��ϴ�.
		*/

	public:
		_INLINE void lock() const noexcept
		{
			while (_InterlockedExchange(&_lock, 1)) {
				_YIELD_PROCESSOR();
			}
		}

		_INLINE void unlock() const noexcept
		{
#if defined(_M_ARM) || defined(_M_ARM64)
			ERROR! Do Not Use WonSY::Spin_Lock!
#else // ^^^ ARM32/ARM64 hardware / x86/x64 hardware vvv
			_InterlockedExchange(&_lock, 0);
#endif // hardware
		}

		SpinLock_0() : _lock(0) {};
		~SpinLock_0() { _lock = 0; };

	private:
		mutable long _lock;
	};

	class SpinLock_1
	{
	public:
		_INLINE void lock() const noexcept
		{
			while (_flag.test_and_set(std::memory_order::memory_order_seq_cst))
			{
			}
		}

		_INLINE void unlock() const noexcept
		{
			_flag.clear(std::memory_order::memory_order_seq_cst);
		}

		SpinLock_1() : _flag() {};
		~SpinLock_1() { unlock(); };

	private:
		mutable atomic_flag _flag;
	};

	class SpinLock_2
	{
	public:
		_INLINE void lock() const noexcept
		{
			while (_flag.test_and_set(std::memory_order::memory_order_acquire))
			{
			}
		}

		_INLINE void unlock() const noexcept
		{
			_flag.clear(std::memory_order::memory_order_release);
		}

		SpinLock_2() : _flag() {};
		~SpinLock_2() { unlock(); };

	private:
		mutable atomic_flag _flag;
	};

	class SpinLock_3
	{
	public:
		_INLINE void lock() const noexcept
		{
			while (_flag.test_and_set(std::memory_order::memory_order_acquire))
			{
				std::this_thread::yield();
			}
		}

		_INLINE void unlock() const noexcept
		{
			_flag.clear(std::memory_order::memory_order_release);
		}

		SpinLock_3() noexcept : _flag() {};
		~SpinLock_3() { unlock(); };

	private:
		mutable atomic_flag _flag;
	};

	class SpinLock_4
	{
	public:
		_INLINE void lock() const noexcept
		{
			while (_flag.exchange(true, std::memory_order::memory_order_acquire))
			{
				std::this_thread::yield();
			}
		}

		_INLINE void unlock() const noexcept
		{
			_flag.store(false, std::memory_order::memory_order_release);
		}

		SpinLock_4() noexcept : _flag(false) {};
		~SpinLock_4() { unlock(); };

	private:
		mutable atomic<bool> _flag;
	};

	class SpinLock_5
	{
	public:
		_INLINE void lock() const noexcept
		{
			while (_flag)
			{
				TIME_TO_SLEEP:
				std::this_thread::yield();
			}

			while (_flag.exchange(true, std::memory_order::memory_order_acquire))
			{
				goto TIME_TO_SLEEP;
			}
		}

		_INLINE void unlock() const noexcept
		{
			_flag.store(false, std::memory_order::memory_order_release);
		}

		SpinLock_5() noexcept : _flag(false) {};
		~SpinLock_5() { unlock(); };

	private:
		mutable atomic<bool> _flag;
	};

	class SpinLock_6
	{
	public:
		_INLINE void lock() const noexcept
		{
			while (_flag.exchange(true, std::memory_order::memory_order_acquire))
			{
				std::this_thread::yield();
			}
		}

		_INLINE void unlock() const noexcept
		{
			_flag.store(false, std::memory_order::memory_order_release);
		}

		SpinLock_6() noexcept : _flag(false) {};
		~SpinLock_6() { unlock(); };

	private:
		mutable atomic<bool> _flag;
	};

	class SpinLock_7
	{
		static constexpr unsigned char WRITE_FLAG = 0x7F; // 1000 0000
		static constexpr unsigned char READ_VALUE = 0x01; // 0000 0001
		static constexpr unsigned char UNSIGNED_CHAR_ZERO = static_cast<unsigned char>(0); // 0000 0000
	public:

		_INLINE void lock() const noexcept
		{
			while (_flag != UNSIGNED_CHAR_ZERO)
			{
			TIME_TO_SLEEP:
				std::this_thread::yield();
			}

			while (!ATOMIC_UTIL::T_CAS(&_flag, UNSIGNED_CHAR_ZERO, WRITE_FLAG))
			{
				goto TIME_TO_SLEEP;
			}
		}

		_INLINE void unlock() const noexcept
		{
			_flag.store(UNSIGNED_CHAR_ZERO);
			//ATOMIC_UTIL::T_CAS(&_flag, WRITE_FLAG, UNSIGNED_CHAR_ZERO);
		}

		_INLINE void lock_shared() const noexcept
		{
			while (_flag == WRITE_FLAG)
			{
			TIME_TO_SLEEP_WRITE:
				std::this_thread::yield();
			}

			TIME_TO_SLEEP_READ:
			unsigned char oldValue = _flag;
			unsigned char newValue = oldValue + 1;
			
			if (!(newValue < WRITE_FLAG))
			{
				goto TIME_TO_SLEEP_WRITE;
			}

			while (!ATOMIC_UTIL::T_CAS(&_flag, oldValue, newValue))
			{
				goto TIME_TO_SLEEP_READ;
			}
		}

		_INLINE void unlock_shared() const noexcept
		{
			--(_flag);
		}

		SpinLock_7() noexcept : _flag(UNSIGNED_CHAR_ZERO) {};
		~SpinLock_7() { unlock(); };

	private:
		mutable atomic<unsigned char> _flag;
	};

	using SpinLock = SpinLock_5;
	using Shared_SpinLock = SpinLock_7;
}

#ifndef WONSY_PCH
// C++
#include <iostream>
#include <chrono>

// C++ 11
#include <mutex>

// C++ 17
#include <shared_mutex>

// C++ STL
#include <vector>

// Using namespace
using namespace std::chrono;
#endif

namespace WonSY::SPIN_LOCK::TEST
{
	const auto NUM_TEST = 1000000;
	const auto THREAD_COUNT = 8;

	void TestFunc1()
	{
		{
			std::vector<std::thread> threadCont;
			threadCont.reserve(THREAD_COUNT);

			long long buffer{ 0 };
			SpinLock_0 spinLock;

			auto startTime = high_resolution_clock::now();

			for (int i = 0; i < THREAD_COUNT; ++i)
			{
				threadCont.emplace_back([&]() {
					for (int j = 0
						; j < NUM_TEST
						; j++)
					{
						spinLock.lock();
						++buffer;
						spinLock.unlock();
					}
				});
			}

			for (auto& thread : threadCont) { thread.join(); }

			auto endTime = high_resolution_clock::now() - startTime;
			std::cout << "������? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
			std::cout << "����? " << buffer << "\n";
			std::cout << "\n";
		}

		{
			std::vector<std::thread> threadCont;
			threadCont.reserve(THREAD_COUNT);

			long long buffer{ 0 };
			SpinLock_1 spinLock;

			auto startTime = high_resolution_clock::now();

			for (int i = 0; i < THREAD_COUNT; ++i)
			{
				threadCont.emplace_back([&]() {
					for (int j = 0
						; j < NUM_TEST
						; j++)
					{
						spinLock.lock();
						++buffer;
						spinLock.unlock();
					}
				});
			}

			for (auto& thread : threadCont) { thread.join(); }

			auto endTime = high_resolution_clock::now() - startTime;
			std::cout << "������? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
			std::cout << "����? " << buffer << "\n";
			std::cout << "\n";
		}

		{
			std::vector<std::thread> threadCont;
			threadCont.reserve(THREAD_COUNT);

			long long buffer{ 0 };
			SpinLock_2 spinLock;

			auto startTime = high_resolution_clock::now();

			for (int i = 0; i < THREAD_COUNT; ++i)
			{
				threadCont.emplace_back([&]() {
					for (int j = 0
						; j < NUM_TEST
						; j++)
					{
						spinLock.lock();
						++buffer;
						spinLock.unlock();
					}
				});
			}

			for (auto& thread : threadCont) { thread.join(); }

			auto endTime = high_resolution_clock::now() - startTime;
			std::cout << "������? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
			std::cout << "����? " << buffer << "\n";
			std::cout << "\n";
		}

		{
			std::vector<std::thread> threadCont;
			threadCont.reserve(THREAD_COUNT);

			long long buffer{ 0 };
			SpinLock_3 spinLock;

			auto startTime = high_resolution_clock::now();

			for (int i = 0; i < THREAD_COUNT; ++i)
			{
				threadCont.emplace_back([&]() {
					for (int j = 0
						; j < NUM_TEST
						; j++)
					{
						spinLock.lock();
						++buffer;
						spinLock.unlock();
					}
				});
			}

			for (auto& thread : threadCont) { thread.join(); }

			auto endTime = high_resolution_clock::now() - startTime;
			std::cout << "������? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
			std::cout << "����? " << buffer << "\n";
			std::cout << "\n";
		}

		{
			std::vector<std::thread> threadCont;
			threadCont.reserve(THREAD_COUNT);

			long long buffer{ 0 };
			SpinLock_4 spinLock;

			auto startTime = high_resolution_clock::now();

			for (int i = 0; i < THREAD_COUNT; ++i)
			{
				threadCont.emplace_back([&]() {
					for (int j = 0
						; j < NUM_TEST
						; j++)
					{
						spinLock.lock();
						++buffer;
						spinLock.unlock();
					}
				});
			}

			for (auto& thread : threadCont) { thread.join(); }

			auto endTime = high_resolution_clock::now() - startTime;
			std::cout << "������? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
			std::cout << "����? " << buffer << "\n";
			std::cout << "\n";
		}

		{
			std::vector<std::thread> threadCont;
			threadCont.reserve(THREAD_COUNT);

			long long buffer{ 0 };
			SpinLock_5 spinLock;

			auto startTime = high_resolution_clock::now();

			for (int i = 0; i < THREAD_COUNT; ++i)
			{
				threadCont.emplace_back([&]() {
					for (int j = 0
						; j < NUM_TEST
						; j++)
					{
						spinLock.lock();
						++buffer;
						spinLock.unlock();
					}
				});
			}

			for (auto& thread : threadCont) { thread.join(); }

			auto endTime = high_resolution_clock::now() - startTime;
			std::cout << "������? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
			std::cout << "����? " << buffer << "\n";
			std::cout << "\n";
		}

		{
			std::vector<std::thread> threadCont;
			threadCont.reserve(THREAD_COUNT);

			long long buffer{ 0 };
			SpinLock_6 spinLock;

			auto startTime = high_resolution_clock::now();

			for (int i = 0; i < THREAD_COUNT; ++i)
			{
				threadCont.emplace_back([&]() {
					for (int j = 0
						; j < NUM_TEST
						; j++)
					{
						spinLock.lock();
						++buffer;
						spinLock.unlock();
					}
				});
			}

			for (auto& thread : threadCont) { thread.join(); }

			auto endTime = high_resolution_clock::now() - startTime;
			std::cout << "������? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
			std::cout << "����? " << buffer << "\n";
			std::cout << "\n";
		}

		{
			std::vector<std::thread> threadCont;
			threadCont.reserve(THREAD_COUNT);

			long long buffer{ 0 };
			SpinLock_7 spinLock;

			auto startTime = high_resolution_clock::now();

			for (int i = 0; i < THREAD_COUNT; ++i)
			{
				threadCont.emplace_back([&]() {
					for (int j = 0
						; j < NUM_TEST
						; j++)
					{
						if (j % 2 == 0)
						{
							spinLock.lock();
							++buffer;
							spinLock.unlock();
						}
						else
						{
							spinLock.lock_shared();
							long long temp = buffer;
							spinLock.unlock_shared();
						}
					}
				});
			}

			for (auto& thread : threadCont) { thread.join(); }

			auto endTime = high_resolution_clock::now() - startTime;
			std::cout << "������? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
			std::cout << "����? " << buffer << "\n";
			std::cout << "\n";
		}
	}

	void TestFunc()
	{
		std::cout << " 0. SpinLock vs mutex " << std::endl;

		{
			std::vector<std::thread> threadCont;
			threadCont.reserve(THREAD_COUNT);

			long long buffer{ 0 };
			SpinLock lock;

			auto startTime = high_resolution_clock::now();

			for (int i = 0; i < THREAD_COUNT; ++i)
			{
				threadCont.emplace_back([&]() {
					for (int j = 0
						; j < NUM_TEST
						; j++)
					{
						lock.lock();
						++buffer;
						lock.unlock();
					}
				});
			}

			for (auto& thread : threadCont) { thread.join(); }

			auto endTime = high_resolution_clock::now() - startTime;
			std::cout << "���ɶ��� ������? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
			std::cout << "����? " << buffer << "\n";
			std::cout << "\n";
		}

		{
			std::vector<std::thread> threadCont;
			threadCont.reserve(THREAD_COUNT);

			long long buffer{ 0 };
			mutex lock;

			auto startTime = high_resolution_clock::now();

			for (int i = 0; i < THREAD_COUNT; ++i)
			{
				threadCont.emplace_back([&]() {
					for (int j = 0
						; j < NUM_TEST
						; j++)
					{
						lock.lock();
						++buffer;
						lock.unlock();
					}
				});
			}

			for (auto& thread : threadCont) { thread.join(); }

			auto endTime = high_resolution_clock::now() - startTime;
			std::cout << "���ؽ��� ������? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
			std::cout << "����? " << buffer << "\n";
			std::cout << "\n";
		}

		std::cout << " 1. Shared_SpinLock vs shared_mutex " << std::endl;

		{
			std::vector<std::thread> threadCont;
			threadCont.reserve(THREAD_COUNT);

			long long buffer{ 0 };
			Shared_SpinLock lock;

			auto startTime = high_resolution_clock::now();

			for (int i = 0; i < THREAD_COUNT; ++i)
			{
				threadCont.emplace_back([&]() {
					for (int j = 0
						; j < NUM_TEST
						; j++)
					{
						if (j % 2 == 0)
						{
							lock.lock();
							++buffer;
							lock.unlock();
						}
						else
						{
							lock.lock_shared();
							long long temp = buffer;
							lock.unlock_shared();
						}
					}
				});
			}

			for (auto& thread : threadCont) { thread.join(); }

			auto endTime = high_resolution_clock::now() - startTime;
			std::cout << "Shared_SpinLock�� ������? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
			std::cout << "����? " << buffer << "\n";
			std::cout << "\n";
		}
		{
			std::vector<std::thread> threadCont;
			threadCont.reserve(THREAD_COUNT);

			long long buffer{ 0 };
			shared_mutex lock;

			auto startTime = high_resolution_clock::now();

			for (int i = 0; i < THREAD_COUNT; ++i)
			{
				threadCont.emplace_back([&]() {
					for (int j = 0
						; j < NUM_TEST
						; j++)
					{
						if (j % 2 == 0)
						{
							lock.lock();
							++buffer;
							lock.unlock();
						}
						else
						{
							lock.lock_shared();
							long long temp = buffer;
							lock.unlock_shared();
						}
					}
				});
			}

			for (auto& thread : threadCont) { thread.join(); }

			auto endTime = high_resolution_clock::now() - startTime;
			std::cout << "Shared_mutex�� ������? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
			std::cout << "����? " << buffer << "\n";
			std::cout << "\n";
		}
	}
}