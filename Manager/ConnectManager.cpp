#include "stdafx.h"

#include "ConnectManager.h"

namespace WonSY::CONNECT_MANAGER
{
#if WONSY_CONNECT_MANAGER_SINGLETON == true
	ConnectManager ConnectManager::instance(SINGLETON_INSTANCE::DEFAULT);

	ConnectManager::ConnectManager(SINGLETON_INSTANCE)
		: connectMemoryPool()
	{
		if (ATOMIC_UTIL::T_CAS(&instanceFlag, false, true) == false)
		{
			ERROR_UTIL::Error("ERROR ConnectManager의 생성자가 두번 호출되었습니다.");
		}

		InitManager();
	}
#else
	ConnectManager::ConnectManager()
		: connectMemoryPool()
	{
		InitManager();
	}

#endif
	
	ConnectManager::~ConnectManager()
	{
		connectMemoryPool.clear();
	}
	
	std::optional<ConnectManager::_KeyType> ConnectManager::DemandKey()
	{
		_KeyType key{ -1 };
		return connectMemoryPool.try_pop(key)
			? std::optional<ConnectManager::_KeyType>{key}
			: std::nullopt;
	}

	void ConnectManager::ReturnKey(const ConnectManager::_KeyType key)
	{
		connectMemoryPool.push(key);
	}

	void ConnectManager::InitManager()
	{
		for (int inKey = 0; inKey < MAX_CONNECT; ++inKey) { connectMemoryPool.push(inKey); }
	}
}

#if WONSY_CONNECT_MANAGER_TEST == true
void WonSY::CONNECT_MANAGER::TestFunc()
{
	ConnectManager connectManager;

	for (int i = 0; i < 100; ++i)
	{
		if (auto retValue = connectManager.DemandKey()
			; retValue.has_value())
		{
			int key = retValue.value();

			std::cout << key << "\n";
		}
		else
		{
			std::cout << "error \n";
		}
	}
}
#endif