#pragma once

#include "../../GameServerLibrary/GameServerLibrary/stdafx.h"

#define WONSY_LOCKFREE_SET_LINKEDLIST
#define WONSY_LOCKFREE_SET_LINKEDLIST__USE_SIZE		// LockfreeSet이 Size를 지원 여부입니다. 비활성화를 원할 시, 주석처리해주세요.
#define WONSY_LOCKFREE_SET_LINKEDLIST__USE_ITERATOR  // LockfreeSet이 Iterator를 지원할 것인지.  비활성화를 원할 시, 주석처리해주세요.

#ifndef WONSY_PCH
// C++
#include <iostream>
#include <chrono>

#define NDEBUG
#include <cassert>

// C++11
#include <atomic>

// C++ STL
#include <vector>

// C++ PPL
#include <concurrent_queue.h>

// Attributes or Others
#define	_DEPRECATED		[[deprecated]]
#define _INLINE inline
#define _DO_NOT_DELETE 
#define _NOT_NULLPTR

// Using namespace
using namespace std;
using namespace std::chrono;
using namespace concurrency;

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

/*
	WonSY::LOCKFREE_SET_LINKEDLIST
		- Thread-Safe한 Linked List 기반의 LockFree_Set 입니다.

	#0. 기본 _KeyType은 int입니다.
	
	!0. LockfreeSet의 메모리풀 사이즈가 넉넉하지 않을 경우, 비정상적으로 동작할 가능성이 매우 높습니다.
	!1. template의 _Data는 _KeyType을 리턴하는 함수 "_KeyType GetKey()"가 필요합니다.
	!2. template의 _Data는 _KeyType을 설정하는 함수 "void SetKey(_KeyType)"가 필요합니다.
	!3. _KeyType은 값 비교를 위해, 비교 연산자를 제공해야합니다.
	!4. _KeyType은 출력 연산자를 제공해야합니다.
*/

// .h
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

	template<typename _Data>
	class Node;

#ifdef WONSY_LOCKFREE_SET_LINKEDLIST__USE_ITERATOR
	template<typename _Data>
	struct LockfreeSetLinkedlistIterator;
#endif

	template<typename _Data>
	class MarkedPointer
	{
		constexpr static _PointerType REMOVED_MASK = 0x01;
#ifdef _WIN64
		constexpr static _PointerType POINTER_MASK = 0xFFFFFFFFFFFFFFFE;
#else 
		constexpr static _PointerType POINTER_MASK = 0xFFFFFFFE;
#endif

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

#ifdef WONSY_LOCKFREE_SET_LINKEDLIST__USE_ITERATOR
		template<typename _Data> friend struct LockfreeSetLinkedlistIterator;
		
		LockfreeSetLinkedlistIterator<_Data> begin();
		
		LockfreeSetLinkedlistIterator<_Data> end();
#endif

#ifdef WONSY_LOCKFREE_SET_LINKEDLIST__USE_SIZE
	public:
		_INLINE const int GetSize() const noexcept { return size; };

	private:
		std::atomic<int> size;
#endif
	};

#ifdef WONSY_LOCKFREE_SET_LINKEDLIST__USE_ITERATOR
	template<typename _Data>
	struct LockfreeSetLinkedlistIterator
	{
		Node<_Data>* node;
		LockfreeSet<_Data>& pCont;

		LockfreeSetLinkedlistIterator(LockfreeSet<_Data>& pCont, const bool isEnd);

		void operator++();
		bool operator!=(const LockfreeSetLinkedlistIterator& other);
		_Data& operator*();
	};
#endif
}

// .hpp
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
		: head()
		, tail()
#ifdef WONSY_LOCKFREE_SET_LINKEDLIST__USE_SIZE
		, size()
#endif
	{
		if constexpr (is_pointer<_Data>::value)
		{
			head.data->SetKey(std::numeric_limits<_KeyType>::min());
			tail.data->SetKey(std::numeric_limits<_KeyType>::max());
		}
		else
		{
			head.data.SetKey(std::numeric_limits<_KeyType>::min());
			tail.data.SetKey(std::numeric_limits<_KeyType>::max());
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
#ifdef WONSY_LOCKFREE_SET_LINKEDLIST__USE_SIZE
					--size;
#endif
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

		Node<_Data>* addedNode{ nullptr };

		while (7)
		{
			Find(key, pred, curr);

			if (curr->key() == key) 
			{ 
				if (addedNode != nullptr) { memoryPool.push(addedNode); }
				return make_pair(false, nullptr); 
			}
			else
			{
				if (addedNode == nullptr)
				{
					if (!memoryPool.try_pop(addedNode))
					{
						addedNode = new Node<_Data>();

						assert(false && "[Warning] Please set the memoryPoolSize!");
						// 메모리풀 사이즈가 넉넉하지 않을 경우, 비정상적으로 동작할 가능성이 높습니다.
						// 충분히 할당해야 정상적으로 동작합니다.
					}
				}

				addedNode->data.SetKey(key);
				addedNode->markedPointer.Set(curr, false);
				if (pred->markedPointer.CAS(curr, addedNode, false, false)) 
				{ 
#ifdef WONSY_LOCKFREE_SET_LINKEDLIST__USE_SIZE
					++size;
#endif
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
				if (pred->markedPointer.CAS(curr, nextNodeOfDeletedNode, false, false)) 
				{ 
					memoryPool.push(curr); 
#ifdef WONSY_LOCKFREE_SET_LINKEDLIST__USE_SIZE
					--size;
#endif
				}

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

#ifdef WONSY_LOCKFREE_SET_LINKEDLIST__USE_ITERATOR
	template<typename _Data>
	LockfreeSetLinkedlistIterator<_Data> LockfreeSet<_Data>::begin()
	{
		return LockfreeSetLinkedlistIterator<_Data>(*this, false);
	}

	template<typename _Data>
	LockfreeSetLinkedlistIterator<_Data> LockfreeSet<_Data>::end()
	{
		return LockfreeSetLinkedlistIterator<_Data>(*this, true);
	}
	
	template<typename _Data>
	LockfreeSetLinkedlistIterator<_Data>::LockfreeSetLinkedlistIterator(LockfreeSet<_Data>& pCont, const bool isEnd)
		: node(nullptr)
		, pCont(pCont)
	{
		if (isEnd) { node = &(pCont.tail); }
		else
		{
			node = &(pCont.head);
			++(*this);
		}
	}

	template<typename _Data>
	void LockfreeSetLinkedlistIterator<_Data>::operator++()
	{
		bool removed;
		_KeyType tailKey = pCont.tail.key();

		while (true)
		{
			node = node->markedPointer.GetPtrWithRemoved(removed);
			
			if (!removed) { return; }
		}
	}

	template<typename _Data>
	bool LockfreeSetLinkedlistIterator<_Data>::operator!=(const LockfreeSetLinkedlistIterator& other)
	{
		return node->key() != other.node->key();
	}

	template<typename _Data>
	_Data& LockfreeSetLinkedlistIterator<_Data>::operator*()
	{
		return node->data;
	}
#endif
}


// for TestFunc
#ifndef WONSY_PCH
#include <thread>
#endif

namespace WonSY::LOCKFREE_SET_LINKEDLIST::TEST
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
			std::cout << i << "개의 성능은? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";

			// cont.Display(20);

			int iterSize = 0;
			for (auto iter = cont.begin(), endIter = cont.end(); iter != endIter; ++iter)
			{
				std::cout << (*iter).GetKey() << " ";
				++iterSize;
			}
			std::cout << "iterSize : " << iterSize << std::endl;
			std::cout << "Size : " << cont.GetSize() << std::endl;
		}
	}
}