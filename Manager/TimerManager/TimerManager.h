#pragma once

#include "../../GameServerLibrary/GameServerLibrary/stdafx.h"

#define WONSY_TIMER_MANAGER
#define WONSY_TIMER_MANAGER_SINGLETON false
#define WONSY_TIMER_MANAGER_TEST false

#ifndef WONSY_PCH
#include <atomic>
#include <thread>
#include <concurrent_queue.h>
#include <concurrent_priority_queue.h>
#define _NODISCARD [[nodiscard]]
#endif

namespace WonSY
{
	template <typename _TimerUnit>
	struct TimerUnitCompareFunction
	{
		bool operator()(_TimerUnit* left, _TimerUnit* right) noexcept
		{
			return (left->eventTime) > (right->eventTime);
		}
	};

	template <typename _TimerUnit, typename _KeyType, typename TIMER_TYPE, typename TIME>
	class TimerManager /* Singleton */
	{
#if WONSY_TIMER_MANAGER_SINGLETON == true
		enum class SINGLETON_INSTANCE { DEFAULT = 0 };
	public:
		TimerManager() = delete;

		TimerManager(SINGLETON_INSTANCE)
			: timerThread()
			, timerCont()
			, timerMemoryPool()
		{
			if (ATOMIC_UTIL::T_CAS(&instanceFlag, false, true) == false)
			{
				std::cout << "ERROR TimerManager�� �����ڰ� �ι� ȣ��Ǿ����ϴ�." << std::endl;
				throw ERROR;
			}

			for (int i = 0; i < DEFINE::MAX_TIMER_COUNT; ++i) { timerMemoryPool.push(new TimerUnit()); }

			timerThread = std::thread([this]() {this->TimerThread(THREAD_INSTANCE::DEFAULT); });
		}
		
		inline static TimerManager& GetInstance() noexcept { return TimerManager::instance; }
#else
	public:
		TimerManager();
#endif
	public:
		~TimerManager()
		{
			_TimerUnit* retTimerUnit = nullptr;
			while (timerMemoryPool.try_pop(retTimerUnit)) { delete retTimerUnit; }
			while (timerCont.try_pop(retTimerUnit)) { delete retTimerUnit; }
		}

		TimerManager(const TimerManager& other) = delete;
		TimerManager& operator=(const TimerManager&) = delete;

		TimerManager(TimerManager&& other) = delete;
		TimerManager& operator=(TimerManager&&) = delete;

	public:
		void AddTimerEvent(const TIMER_TYPE, const _KeyType ownerKey, const _KeyType targetKey, const TIME waitTime)
		{
			_TimerUnit* timerUnit = PopTimerUnit();

			timerUnit->timerType = inTimerType;
			timerUnit->ownerKey = ownerKey;
			timerUnit->targetKey = targetKey;
			timerUnit->eventTime = GetTickCount64() + static_cast<_TimeType>(waitTime);

			timerCont.push(timerUnit);
		}
		
		void AddTimerEvent(_TimerUnit timerUnit*, const TIME waitTime)
		{
			timerUnit->eventTime = GetTickCount64() + static_cast<_TimeType>(waitTime);
			timerCont.push(timerUnit);
		}

		void TimerThread()
		{
			while (7)
			{
				std::this_thread::sleep_for(0ns);
				const auto tempNowTime = GetTickCount64();

				while (timerCont.size())
				{
					_TimerUnit* retTimerUnit{ nullptr };

					// Timer Cont�� ��ϵ� Ÿ�̸Ӱ� ������ ���� �� ����, ���Ŀ��� �ּ� Ÿ�̸ӷ� Loop!
					while (!timerCont.try_pop(retTimerUnit)) { std::this_thread::sleep_for(100ms); }

					if (tempNowTime < retTimerUnit->eventTime)
					{
						// ����
						timerCont.push(retTimerUnit);
						break;
					}

					if (ProcessTimerUnit(retTimerUnit)) { PushTimerUnit(retTimerUnit); }
				}
			}
		}

	private:

		bool ProcessTimerUnit(_TimerUnit* retTimerUnit)	// return true - ��ȯ �ʿ�, false - ������
		{
			switch (retTimerUnit->timerType)
			{
			case TIMER_TYPE::PUSH_OLD_KEY:
				//ConnectManager::GetInstance().PushOldKey(retTimerUnit->ownerKey);
				return true; break;
			default:
				//LogManager::GetInstance().AddLog(LOG_TYPE::WARNING_LOG, L"TimerUnit�� ó�������ʴ� Ÿ���� �����մϴ�.", SourceLocation(SOURCE_LOCATION));
				return true; break;
			}
		}

		_NODISCARD TimerUnit* PopTimerUnit()
		{
			_TimerUnit* retTimerUnit{ nullptr };

			return timerMemoryPool.try_pop(retTimerUnit)
				? retTimerUnit
				: []()->_TimerUnit*
			{
				//LogManager::GetInstance().AddLog(LOG_TYPE::WARNING_LOG, L"Timer�� �����Ͽ� �߰� �Ҵ��Ͽ����ϴ�.", SourceLocation(SOURCE_LOCATION));
				return new TimerUnit();
			}();
		}

		void PushTimerUnit(TimerUnit* timerUnit)
		{
			timerMemoryPool.push(timerUnit);
		}

	private:
		/*inline*/ static TimerManager instance;
		inline static std::atomic<bool> instanceFlag{ false };

		std::thread timerThread;

		concurrency::concurrent_priority_queue<_TimerUnit*, TimerUnitCompareFunction<_TimerUnit>> timerCont;
		concurrency::concurrent_queue<_TimerUnit*> timerMemoryPool;
	};

	// TimerManager TimerManager::instance(SINGLETON_INSTANCE::DEFAULT);
}