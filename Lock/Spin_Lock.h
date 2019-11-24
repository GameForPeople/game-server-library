#pragma once

//#include "../GameServerLibrary/GameServerLibrary/stdafx.h"

#define WONSY_TEST
#define WONSY_SPIN_LOCK

#ifndef WONSY_PCH
// C++11
#include <atomic>

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
	class SpinLock_TTAS
	{
	public:
		_INLINE void lock() const noexcept
		{
			while (_flag)
			{
				TIME_TO_SLEEP:
				std::this_thread::yield();
			}

			if (_flag.exchange(true, std::memory_order::memory_order_acquire))
			{
				goto TIME_TO_SLEEP;
			}
		}

		_INLINE void unlock() const noexcept
		{
			_flag.store(false, std::memory_order::memory_order_release);
		}

		SpinLock_TTAS() noexcept : _flag(false) {};
		~SpinLock_TTAS() { unlock(); };

	private:
		mutable atomic<bool> _flag;
	};

	class SpinLock_TAS
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

		SpinLock_TAS() noexcept : _flag(false) {};
		~SpinLock_TAS() { unlock(); };

	private:
		mutable atomic<bool> _flag;
	};

	class SpinLock_BackOff
	{
	public:
		_INLINE void lock() const noexcept
		{
			if (_flag.exchange(true, std::memory_order::memory_order_acquire))
			{
				std::this_thread::sleep_for(std::chrono::milliseconds((rand() % _maxRandomValue) + _minSleepTime));
			}
		}

		_INLINE void unlock() const noexcept
		{
			_flag.store(false, std::memory_order::memory_order_release);
		}

		SpinLock_BackOff(const int _minSleepTime, const int _maxRandomValue) noexcept
			: _flag(false) 
			, _minSleepTime(_minSleepTime)
			, _maxRandomValue(_maxRandomValue)
		{
		};
		~SpinLock_BackOff() { unlock(); };

	private:
		mutable atomic<bool> _flag;
		const int _minSleepTime;
		const int _maxRandomValue;
	};

	template <const int _maxRandomValue /*std::thread::hardware_concurrency()*/, const int _minSleepTime >
	class SpinLock_TTAS_BackOff
	{
	public:
		_INLINE void lock() const noexcept
		{
			while (flag)
			{
			TIME_TO_SLEEP:
				if constexpr (_maxRandomValue != 0)
				{
					if constexpr (_minSleepTime != 0)
					{
						// auto localSleepTime{ (rand() % _maxRandomValue) + _minSleepTime };
						std::this_thread::sleep_for(std::chrono::milliseconds((rand() % _maxRandomValue) + _minSleepTime));
					}
					else
					{
						//auto localSleepTime{ (rand() % _maxRandomValue)};
						std::this_thread::sleep_for(std::chrono::milliseconds((rand() % _maxRandomValue)));
					}
				}
				else
				{
					if constexpr (_minSleepTime != 0)
					{
						std::this_thread::sleep_for(std::chrono::milliseconds(_minSleepTime));
					}
					else
					{
						std::this_thread::yield();
					}
				}
			}

			if (flag.exchange(true, std::memory_order::memory_order_acquire))
			{
				goto TIME_TO_SLEEP;
			}
		}

		_INLINE void unlock() const noexcept
		{
			flag.store(false, std::memory_order::memory_order_release);
		}

		SpinLock_TTAS_BackOff() noexcept
			: flag(false) 
		{};

		~SpinLock_TTAS_BackOff() { unlock(); };

	private:
		mutable atomic<bool> flag;
	};

	template <const int _maxRandomValue = 0 /*std::thread::hardware_concurrency()*/, const int _minSleepTime = 0>
	using SpinLock = SpinLock_TTAS_BackOff<_maxRandomValue, _minSleepTime>;

#ifdef WONSY_TEST
	class RWSpinLock_0
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

		RWSpinLock_0() noexcept : _flag(UNSIGNED_CHAR_ZERO) {};
		~RWSpinLock_0() { unlock(); };

	private:
		mutable atomic<unsigned char> _flag;
	};

	using Shared_SpinLock = RWSpinLock_0;
#endif
}

#ifdef WONSY_TEST
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
	const auto NUM_TEST = 10000000;
	const auto THREAD_COUNT = 4;

	long long volatile buffer{ 0 };
	long long volatile buffer1[100]{ 0,  };

	void DoFunc()
	{
		for (int i = 0; i < 3; ++i)
		{
			buffer += 1;
		}
	}

	void DoInit()
	{
		buffer = 0;
	}

	void TestFunc_SpinLocks()
	{
		{
			std::vector<std::thread> threadCont;
			threadCont.reserve(THREAD_COUNT);

			DoInit();
			SpinLock_TAS spinLock;

			auto startTime = high_resolution_clock::now();

			for (int i = 0; i < THREAD_COUNT; ++i)
			{
				threadCont.emplace_back([&]() {
					for (int j = 0
						; j < NUM_TEST
						; j++)
					{
						spinLock.lock();
						DoFunc();
						spinLock.unlock();
					}
				});
			}

			for (auto& thread : threadCont) { thread.join(); }

			auto endTime = high_resolution_clock::now() - startTime;
			std::cout << typeid(spinLock).name() << "의 성능은? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
			std::cout << "값은? " << buffer << "\n";
			std::cout << "\n";
		}

		{
			std::vector<std::thread> threadCont;
			threadCont.reserve(THREAD_COUNT);

			DoInit();
			SpinLock_TTAS spinLock;

			auto startTime = high_resolution_clock::now();

			for (int i = 0; i < THREAD_COUNT; ++i)
			{
				threadCont.emplace_back([&]() {
					for (int j = 0
						; j < NUM_TEST
						; j++)
					{
						spinLock.lock();
						DoFunc();
						spinLock.unlock();
					}
				});
			}

			for (auto& thread : threadCont) { thread.join(); }

			auto endTime = high_resolution_clock::now() - startTime;
			std::cout << typeid(spinLock).name() << "의 성능은? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
			std::cout << "값은? " << buffer << "\n";
			std::cout << "\n";
		}

		{
			std::vector<std::thread> threadCont;
			threadCont.reserve(THREAD_COUNT);

			DoInit();
			SpinLock_BackOff spinLock(0,5);

			auto startTime = high_resolution_clock::now();

			for (int i = 0; i < THREAD_COUNT; ++i)
			{
				threadCont.emplace_back([&]() {
					for (int j = 0
						; j < NUM_TEST
						; j++)
					{
						spinLock.lock();
						DoFunc();
						spinLock.unlock();
					}
				});
			}

			for (auto& thread : threadCont) { thread.join(); }

			auto endTime = high_resolution_clock::now() - startTime;
			std::cout << typeid(spinLock).name() << "의 성능은? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
			std::cout << "값은? " << buffer << "\n";
			std::cout << "\n";
		}

		{
			std::vector<std::thread> threadCont;
			threadCont.reserve(THREAD_COUNT);
			
			DoInit();
			//SpinLock_TTAS_BackOff<5,0> spinLock;
			SpinLock<> spinLock;


			auto startTime = high_resolution_clock::now();

			for (int i = 0; i < THREAD_COUNT; ++i)
			{
				threadCont.emplace_back([&]() {
					for (int j = 0
						; j < NUM_TEST
						; j++)
					{
						spinLock.lock();
						DoFunc();
						spinLock.unlock();
					}
				});
			}

			for (auto& thread : threadCont) { thread.join(); }

			auto endTime = high_resolution_clock::now() - startTime;
			std::cout << typeid(spinLock).name() << "의 성능은? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
			std::cout << "값은? " << buffer << "\n";
			std::cout << "\n";
		}

		{
			std::vector<std::thread> threadCont;
			threadCont.reserve(THREAD_COUNT);

			DoInit();
			//SpinLock_TTAS_BackOff<5,0> spinLock;
			SpinLock<0, 1> spinLock;


			auto startTime = high_resolution_clock::now();

			for (int i = 0; i < THREAD_COUNT; ++i)
			{
				threadCont.emplace_back([&]() {
					for (int j = 0
						; j < NUM_TEST
						; j++)
					{
						spinLock.lock();
						DoFunc();
						spinLock.unlock();
					}
				});
			}

			for (auto& thread : threadCont) { thread.join(); }

			auto endTime = high_resolution_clock::now() - startTime;
			std::cout << typeid(spinLock).name() << "의 성능은? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
			std::cout << "값은? " << buffer << "\n";
			std::cout << "\n";
		}

		{
			std::vector<std::thread> threadCont;
			threadCont.reserve(THREAD_COUNT);

			DoInit();
			//SpinLock_TTAS_BackOff<5,0> spinLock;
			SpinLock<1, 0> spinLock;


			auto startTime = high_resolution_clock::now();

			for (int i = 0; i < THREAD_COUNT; ++i)
			{
				threadCont.emplace_back([&]() {
					for (int j = 0
						; j < NUM_TEST
						; j++)
					{
						spinLock.lock();
						DoFunc();
						spinLock.unlock();
					}
				});
			}

			for (auto& thread : threadCont) { thread.join(); }

			auto endTime = high_resolution_clock::now() - startTime;
			std::cout << typeid(spinLock).name() << "의 성능은? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
			std::cout << "값은? " << buffer << "\n";
			std::cout << "\n";
		}

		{
			std::vector<std::thread> threadCont;
			threadCont.reserve(THREAD_COUNT);

			DoInit();
			//SpinLock_TTAS_BackOff<5,0> spinLock;
			SpinLock<1, 1> spinLock;


			auto startTime = high_resolution_clock::now();

			for (int i = 0; i < THREAD_COUNT; ++i)
			{
				threadCont.emplace_back([&]() {
					for (int j = 0
						; j < NUM_TEST
						; j++)
					{
						spinLock.lock();
						DoFunc();
						spinLock.unlock();
					}
				});
			}

			for (auto& thread : threadCont) { thread.join(); }

			auto endTime = high_resolution_clock::now() - startTime;
			std::cout << typeid(spinLock).name() << "의 성능은? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
			std::cout << "값은? " << buffer << "\n";
			std::cout << "\n";
		}

		{
			std::vector<std::thread> threadCont;
			threadCont.reserve(THREAD_COUNT);

			long long buffer{ 0 };
			RWSpinLock_0 spinLock;

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
			std::cout << typeid(spinLock).name() << "의 성능은? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
			std::cout << "값은? " << buffer << "\n";
			std::cout << "\n";
		}
	}

	void TestFunc_Comp()
	{
		std::cout << " 0. WonSY::SpinLock vs mutex vs shared_mutex vs SRWLock vs atomic_int " << std::endl;

		{
			{
				std::vector<std::thread> threadCont;
				threadCont.reserve(THREAD_COUNT);

				DoInit();
				SpinLock<> lock;

				auto startTime = high_resolution_clock::now();

				for (int i = 0; i < THREAD_COUNT; ++i)
				{
					threadCont.emplace_back([&]() {
						for (int j = 0
							; j < NUM_TEST
							; j++)
						{
							lock.lock();
							DoFunc();
							lock.unlock();
						}
					});
				}

				for (auto& thread : threadCont) { thread.join(); }

				auto endTime = high_resolution_clock::now() - startTime;
				std::cout << typeid(lock).name() << "의 성능은? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
				std::cout << "값은? " << buffer << "\n";
				std::cout << "\n";
			}

			{
				std::vector<std::thread> threadCont;
				threadCont.reserve(THREAD_COUNT);

				DoInit();
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
							DoFunc();
							lock.unlock();
						}
					});
				}

				for (auto& thread : threadCont) { thread.join(); }

				auto endTime = high_resolution_clock::now() - startTime;
				std::cout << typeid(lock).name() << "의 성능은? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
				std::cout << "값은? " << buffer << "\n";
				std::cout << "\n";
			}

			{
				std::vector<std::thread> threadCont;
				threadCont.reserve(THREAD_COUNT);

				DoInit();
				shared_mutex lock;

				auto startTime = high_resolution_clock::now();

				for (int i = 0; i < THREAD_COUNT; ++i)
				{
					threadCont.emplace_back([&]() {
						for (int j = 0
							; j < NUM_TEST
							; j++)
						{
							lock.lock();
							DoFunc();
							lock.unlock();
						}
					});
				}

				for (auto& thread : threadCont) { thread.join(); }

				auto endTime = high_resolution_clock::now() - startTime;
				std::cout << typeid(lock).name() << "의 성능은? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
				std::cout << "값은? " << buffer << "\n";
				std::cout << "\n";
			}

			{
				std::vector<std::thread> threadCont;
				threadCont.reserve(THREAD_COUNT);

				std::atomic<long long> buffer{ 0 };

				auto startTime = high_resolution_clock::now();

				for (int i = 0; i < THREAD_COUNT; ++i)
				{
					threadCont.emplace_back([&]() {
						for (int j = 0
							; j < NUM_TEST
							; j++)
						{
							++buffer;
						}
					});
				}

				for (auto& thread : threadCont) { thread.join(); }

				auto endTime = high_resolution_clock::now() - startTime;
				std::cout << typeid(buffer).name() << "의 성능은? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
				std::cout << "값은? " << buffer << "\n";
				std::cout << "\n";
			}
		}

		std::cout << " 1. WonSY::SpinLock vs WonSY::Shared_SpinLock vs shared_mutex " << std::endl;

		{
			{
				std::vector<std::thread> threadCont;
				threadCont.reserve(THREAD_COUNT);

				long long buffer{ 0 };
				SpinLock_TTAS lock;

				auto startTime = high_resolution_clock::now();

				for (int i = 0; i < THREAD_COUNT; ++i)
				{
					threadCont.emplace_back([&]() {
						for (int j = 0
							; j < NUM_TEST
							; j++)
						{
							if (j % 5 == 0)
							{
								lock.lock();
								++buffer;
								lock.unlock();
							}
							else
							{
								lock.lock();
								long long temp = buffer;
								lock.unlock();
							}
						}
					});
				}

				for (auto& thread : threadCont) { thread.join(); }

				auto endTime = high_resolution_clock::now() - startTime;
				std::cout << "SpinLock의 성능은? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
				std::cout << "값은? " << buffer << "\n";
				std::cout << "\n";
			}

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
							if (j % 5 == 0)
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
				std::cout << "Shared_SpinLock의 성능은? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
				std::cout << "값은? " << buffer << "\n";
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
							if (j % 5 == 0)
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
				std::cout << "Shared_mutex의 성능은? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
				std::cout << "값은? " << buffer << "\n";
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
							if (j % 5 == 0)
							{
								lock.lock();
								++buffer;
								lock.unlock();
							}
							else
							{
								lock.lock();
								long long temp = buffer;
								lock.unlock();
							}
						}
					});
				}

				for (auto& thread : threadCont) { thread.join(); }

				auto endTime = high_resolution_clock::now() - startTime;
				std::cout << "mutex의 성능은? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
				std::cout << "값은? " << buffer << "\n";
				std::cout << "\n";
			}
		}
	}

	void TestFunc_Functions()
	{
		// With no
		{
			std::vector<std::thread> threadCont;
			threadCont.reserve(THREAD_COUNT);

			long long buffer{ 0 };
			SpinLock_TTAS spinLock;

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
			std::cout << "   스핀락 날것의 성능은? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
			std::cout << "   값은? " << buffer << "\n";
			std::cout << "\n";
		}

		// With lock_guard
		{
			std::vector<std::thread> threadCont;
			threadCont.reserve(THREAD_COUNT);

			long long buffer{ 0 };
			SpinLock_TTAS spinLock;

			auto startTime = high_resolution_clock::now();

			for (int i = 0; i < THREAD_COUNT; ++i)
			{
				threadCont.emplace_back([&]() {
					for (int j = 0
						; j < NUM_TEST
						; j++)
					{
						std::lock_guard<SpinLock_TTAS> localLock(spinLock);
						++buffer;
					}
				});
			}

			for (auto& thread : threadCont) { thread.join(); }

			auto endTime = high_resolution_clock::now() - startTime;
			std::cout << "   With lock_guard 성능은? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
			std::cout << "   값은? " << buffer << "\n";
			std::cout << "\n";
		}

		// With Unique Lock
		{
			std::vector<std::thread> threadCont;
			threadCont.reserve(THREAD_COUNT);

			long long buffer{ 0 };
			SpinLock_TTAS spinLock;

			auto startTime = high_resolution_clock::now();

			for (int i = 0; i < THREAD_COUNT; ++i)
			{
				threadCont.emplace_back([&]() {
					for (int j = 0
						; j < NUM_TEST
						; j++)
					{
						std::unique_lock<SpinLock_TTAS> localLock(spinLock);
						++buffer;
					}
				});
			}

			for (auto& thread : threadCont) { thread.join(); }

			auto endTime = high_resolution_clock::now() - startTime;
			std::cout << "   With Unique Lock 성능은? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
			std::cout << "   값은? " << buffer << "\n";
			std::cout << "\n";
		}
	}
}
#endif