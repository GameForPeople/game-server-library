#pragma once

#define WONSY_CONNECT_MANAGER
#define WONSY_CONNECT_MANAGER_SINGLETON false
#define WONSY_CONNECT_MANAGER_TEST false

#ifndef WONSY_PCH
#include <atomic>
#include <optional>
#include <concurrent_queue.h>
#endif

namespace WonSY::CONNECT_MANAGER
{
	class ConnectManager /* Singleton */
	{
		using _KeyType = int;
		static constexpr int MAX_CONNECT = 10000;

#if WONSY_CONNECT_MANAGER_SINGLETON == true
		enum class SINGLETON_INSTANCE { DEFAULT = 0 };

	public:
		ConnectManager() = delete;
		ConnectManager(SINGLETON_INSTANCE);

		inline static ConnectManager& GetInstance() noexcept { return ConnectManager::instance; }
	private:
		/*inline*/ static ConnectManager instance;
		inline static std::atomic<bool> instanceFlag{ false };
#else 
	public:
		ConnectManager();
#endif
	public:
		~ConnectManager();

		ConnectManager(const ConnectManager& other) = delete;
		ConnectManager& operator=(const ConnectManager&) = delete;

		ConnectManager(ConnectManager&& other) = delete;
		ConnectManager& operator=(ConnectManager&&) = delete;

	public:
		std::optional<_KeyType> DemandKey();
		void ReturnKey(const _KeyType oldKey);

	private:
		void InitManager();

		concurrency::concurrent_queue<_KeyType> connectMemoryPool;
	};

#if WONSY_CONNECT_MANAGER_TEST == true
	void TestFunc();
#endif
}