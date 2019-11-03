#pragma once

#include "../../GameServerLibrary/GameServerLibrary/stdafx.h"

#define WONSY_LOCKFREE_SET_LINKEDLIST

#ifndef WONSY_PCH
#include <iostream>
#include <chrono>

#include <atomic>

#include <vector>
#include <concurrent_queue.h>

#define NDEBUG
#include <cassert>

#define	_DEPRECATED		[[deprecated]]
#define _INLINE inline
#define _DO_NOT_DELETE 
#define _NOT_NULLPTR

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
#endif

/*
	WonSY::LOCKFREE_SET_LINKEDLIST
		- Thread-Safe�� Linked List ����� LockFree_Set �Դϴ�.

	#0. �⺻ _KeyType�� int�Դϴ�.
	
	!0. LockfreeSet�� �޸�Ǯ ����� �˳����� ���� ���, ������������ ������ ���ɼ��� �ſ� �����ϴ�.
	!1. template�� _Data�� _KeyType�� �����ϴ� �Լ� "_KeyType GetKey()"�� �ʿ��մϴ�.
	!2. template�� _Data�� _KeyType�� �����ϴ� �Լ� "void SetKey(_KeyType)"�� �ʿ��մϴ�.
	!3. _KeyType�� �� �񱳸� ����, �� �����ڸ� �����ؾ��մϴ�.
	!4. _KeyType�� ��� �����ڸ� �����ؾ��մϴ�.
*/

namespace WonSY::LOCKFREE_SET_LINKEDLIST
{
	namespace USING
	{
		using _KeyType = int;

#ifdef _WIN64
		using _PointerType = long long;	// x64 64bit
#else
		using _PointerType = int;	// x32 32bit
#endif

	}using namespace USING;

	namespace Define
	{
		constexpr _PointerType REMOVED_MASK = 0x01;

#ifdef _WIN64
		constexpr _PointerType POINTER_MASK = 0xFFFFFFFFFFFFFFFE;
#else 
		constexpr _PointerType POINTER_MASK = 0xFFFFFFFE;
#endif
	}using namespace Define;

	template<typename _Data>
	class Node;

	template<typename _Data>
	class MarkedPointer
	{
		_PointerType value;	// next Node Pointer(nBit) with removed Mark(1bit).
	public:
		MarkedPointer() noexcept;
		~MarkedPointer() = default;

		MarkedPointer(const MarkedPointer&) = default;
		MarkedPointer& operator=(const MarkedPointer&) = default;

		MarkedPointer(MarkedPointer&&) = delete;
		MarkedPointer& operator=(MarkedPointer&&) = delete;

	public:

		void Set(const Node<_Data>* const node, const bool removed) noexcept;

		_DO_NOT_DELETE Node<_Data>* GetPtr() const;

		_DO_NOT_DELETE Node<_Data>* GetPtrWithRemoved(bool& removed) const;

		bool CAS(const Node<_Data>* const oldNode, const Node<_Data>* const newNode, const bool oldRemoved, const bool newRemoved);

		bool TryMark(const Node<_Data>* const oldNode, const bool newMark);
	};

	template<typename _Data>
	class Node
	{
	public:
		_Data data;
		MarkedPointer<_Data> markedPointer;

	public:
		Node() noexcept;
		Node(const _Data& data) noexcept;

		~Node() = default;
		Node(const Node&) = delete;
		Node& operator=(const Node&) = delete;

		Node(Node&&) = delete;
		Node& operator=(Node&&) = delete;

		_KeyType key() noexcept;
	};

	template<typename _Data>
	class LockfreeSet {
		Node<_Data> head, tail;
		concurrent_queue<Node<_Data>*> memoryPool;

	public:
		LockfreeSet(const int memoryPoolSize /* = GLOBAL::KEY_RANGE * 2 */) ;
		~LockfreeSet();

		LockfreeSet(const LockfreeSet&) = delete;
		LockfreeSet& operator=(const LockfreeSet&) = delete;

		LockfreeSet(LockfreeSet&&) = delete;
		LockfreeSet& operator=(LockfreeSet&&) = delete;

	public:
		void Init();

		void Find(const _KeyType key, Node<_Data>* (&pred), Node<_Data>* (&curr));

		pair<bool, Node<_Data>*> Add(const _KeyType key);
		bool Remove(const _KeyType key);
		bool Contains(const _KeyType key);

		/*_DEPRECATED */ void Display(const int dlsplayNum)
		{
			auto ptr = head.markedPointer.GetPtr();

			for (int i = 0; i < dlsplayNum; ++i)
			{
				if (ptr == &tail) break;

				std::cout << " " << ptr->key() << ",";
				ptr = ptr->markedPointer.GetPtr();
			}

			std::cout << "\n";
		}
	};
}

namespace WonSY::LOCKFREE_SET_LINKEDLIST
{
	template<typename _Data>
	MarkedPointer<_Data>::MarkedPointer() noexcept
		: value()
	{
		Set(nullptr, false);
	};

	template<typename _Data>
	void MarkedPointer<_Data>::Set(const Node<_Data>* const node, const bool removed) noexcept
	{
		value = reinterpret_cast<_PointerType>(node);

		value = removed
			? value | REMOVED_MASK
			: value & POINTER_MASK;
	}

	template<typename _Data>
	_DO_NOT_DELETE Node<_Data>* MarkedPointer<_Data>::GetPtr() const
	{
		return reinterpret_cast<Node<_Data>*>(value & POINTER_MASK);
	}

	template<typename _Data>
	_DO_NOT_DELETE Node<_Data>* MarkedPointer<_Data>::GetPtrWithRemoved(bool& removed) const
	{
		const auto temp = value;

		0 == (temp & REMOVED_MASK)
			? removed = false
			: removed = true;

		return reinterpret_cast<Node<_Data>*>(value & POINTER_MASK);
	}

	template<typename _Data>
	bool MarkedPointer< _Data>::CAS(const Node<_Data>* const oldNode, const Node<_Data>* const newNode, const bool oldRemoved, const bool newRemoved)
	{
		_PointerType oldValue = reinterpret_cast<_PointerType>(oldNode);

		if (oldRemoved) oldValue = oldValue | REMOVED_MASK;
		else oldValue = oldValue & POINTER_MASK;

		_PointerType newValue = reinterpret_cast<_PointerType>(newNode);
		if (newRemoved) newValue = newValue | REMOVED_MASK;
		else newValue = newValue & POINTER_MASK;

		return ATOMIC_UTIL::T_CAS(&value, oldValue, newValue);
	}

	template<typename _Data>
	bool MarkedPointer< _Data>::TryMark(const Node<_Data>* const oldNode, const bool newMark)
	{
		auto oldValue = reinterpret_cast<_PointerType>(oldNode);
		auto newValue = oldValue;

		if (newMark) newValue = newValue | REMOVED_MASK;
		else newValue = newValue & POINTER_MASK;

		return ATOMIC_UTIL::T_CAS(&value, oldValue, newValue);
	}

	template<typename _Data>
	Node<_Data>::Node() noexcept
		: data()
		, markedPointer()
	{
	}

	template<typename _Data>
	Node<_Data>::Node(const _Data& dataValue) noexcept
		: data(dataValue)
		, markedPointer()
	{
	}

	template<typename _Data>
	_KeyType Node<_Data>::key() noexcept
	{
		return data.GetKey();
	}

	template<typename _Data>
	LockfreeSet<_Data>::LockfreeSet(const int memoryPoolSize) /*noexcept*/
		: head(/*INT_MIN*/)
		, tail(/*INT_MAX*/)
	{
		if /*constexpr*/ (typeid(_KeyType) == typeid(int))
		{
			head.data.SetKey(INT_MIN);
			tail.data.SetKey(INT_MAX);
		}
		else if /*constexpr*/ (typeid(_KeyType) == typeid(long long))
		{
			head.data.SetKey(LLONG_MIN);
			tail.data.SetKey(LLONG_MAX);
		}
		else if /*constexpr*/ (typeid(_KeyType) == typeid(short))
		{
			head.data.SetKey(SHRT_MIN);
			tail.data.SetKey(SHRT_MAX);
		}
		else
		{
			std::cout << "[ERROR] Need! Key Min Max Value!" << "\n";  
			throw -1;
		}

		head.markedPointer.Set(&tail, false);

		Node<_Data>* pushedNode{ nullptr };
		for (int i = 0; i < memoryPoolSize; ++i)
		{
			pushedNode = new Node<_Data>();
			memoryPool.push(pushedNode);
		}
	}

	template<typename _Data>
	LockfreeSet<_Data>::~LockfreeSet()
	{
		Init();

		Node<_Data>* deletedNode{ nullptr };
		bool retValue{ true };

		while (retValue == false)
		{
			retValue = memoryPool.try_pop(deletedNode);
			delete deletedNode;
		}
	}

	template<typename _Data>
	void LockfreeSet<_Data>::Init()
	{
		Node<_Data>* ptr;

		while (head.markedPointer.GetPtr() != &tail)
		{
			ptr = head.markedPointer.GetPtr();
			head.markedPointer = head.markedPointer.GetPtr()->markedPointer;

			//delete ptr;
			memoryPool.push(ptr);
		}
	}

	template<typename _Data>
	void LockfreeSet<_Data>::Find(const _KeyType key, Node<_Data>* (&pred), Node<_Data>* (&curr))
	{
	retry:
		while (true)
		{
			pred = &head;
			curr = pred->markedPointer.GetPtr();

			while (true)
			{
				bool removed{};
				auto succ = curr->markedPointer.GetPtrWithRemoved(removed);

				while (removed)
				{
					if (!(pred->markedPointer.CAS(curr, succ, false, false))) { goto retry; }

					memoryPool.push(curr);

					curr = succ;
					succ = curr->markedPointer.GetPtrWithRemoved(removed);
				}

				if (curr->key() >= key) return;

				pred = curr;
				curr = succ;
			}
		}
	}

	template<typename _Data>
	pair<bool, Node<_Data>*> LockfreeSet<_Data>::Add(const _KeyType key)
	{
		// assert(addedNode == nullptr && "[ERROR] addedNode is nullptr!");

		Node<_Data>* pred{ nullptr };
		Node<_Data>* curr{ nullptr };

		while (7)
		{
			Find(key, pred, curr);

			if (curr->key() == key) { return make_pair(false, nullptr); }
			else
			{
				Node<_Data>* addedNode{ nullptr };

				if (!memoryPool.try_pop(addedNode))
				{
					addedNode = new Node<_Data>();

					assert(false && "[Warning] Please set the memoryPoolSize!");
					// �޸�Ǯ ����� �˳����� ���� ���, ������������ ������ ���ɼ��� �����ϴ�.
					// ����� �Ҵ��ؾ� ���������� �����մϴ�.
				}
				addedNode->data.SetKey(key);

				addedNode->markedPointer.Set(curr, false);

				if (pred->markedPointer.CAS(curr, addedNode, false, false)) 
				{ 
					return make_pair(true, addedNode);
				}
			}
		}
	}

	template<typename _Data>
	bool LockfreeSet<_Data>::Remove(const _KeyType key)
	{
		Node<_Data>* pred{ nullptr };
		Node<_Data>* curr{ nullptr };

		while (7)
		{
			Find(key, pred, curr);

			if (curr->key() != key) { return false; }
			else
			{
				Node<_Data>* nextNodeOfDeletedNode = curr->markedPointer.GetPtr();

				if (!curr->markedPointer.TryMark(nextNodeOfDeletedNode, true)) { continue; }
				if (pred->markedPointer.CAS(curr, nextNodeOfDeletedNode, false, false)) { memoryPool.push(curr); }

				return true;
			}
		}
	}

	template<typename _Data>
	bool LockfreeSet<_Data>::Contains(const _KeyType key)
	{
		Node<_Data>* curr = &head;
		bool removed{};

		while (curr->key() < key)
		{
			curr = curr->markedPointer.GetPtr();
			Node<_Data>* succ = curr->markedPointer.GetPtrWithRemoved(removed);
		}

		return curr->key() == key && !removed;
	}
}


// for TestFunc
#ifndef WONSY_PCH
#include <thread>
#endif

namespace WonSY::LOCKFREE_SET_LINKEDLIST
{
	struct ExampleStruct
	{
		WonSY::LOCKFREE_SET_LINKEDLIST::_KeyType key;
		char data[100];

		_INLINE WonSY::LOCKFREE_SET_LINKEDLIST::_KeyType GetKey() { return key; }
		_INLINE void SetKey(WonSY::LOCKFREE_SET_LINKEDLIST::_KeyType key) { this->key = key; }
	};

	void TestFunc()
	{
		const auto NUM_TEST = 1000000;
		const auto KEY_RANGE = 1000;

		for (int i = 1; i <= 8; i = i * 2)
		{
			WonSY::LOCKFREE_SET_LINKEDLIST::LockfreeSet<ExampleStruct> cont(1000);

			std::vector<std::thread> threadCont;
			threadCont.reserve(i);

			auto startTime = high_resolution_clock::now();

			for (int j = 0; j < i; ++j)
			{
				threadCont.emplace_back([&]() {
					for (int k = 0, size = (NUM_TEST / i)
						; k < size
						; k++)
					{
						switch (const int key = rand() % KEY_RANGE
							; rand() % 3)
						{
						case 0: cont.Add(key);	break;
						case 1: cont.Remove(key); break;
						case 2: cont.Contains(key); break;
						default: cout << "Error\n"; exit(-1);
						}
					}
				});
			}

			for (auto& thread : threadCont) { thread.join(); }

			auto endTime = high_resolution_clock::now() - startTime;
			std::cout << i << "���� ������? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";

			cont.Display(20);
		}
	}
}