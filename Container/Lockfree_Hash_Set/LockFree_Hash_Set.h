#pragma once

#include "../../GameServerLibrary/GameServerLibrary/stdafx.h"
/*
	LOCKFREE_HASH_SET
		- 
*/

#define WONSY_LOCKFREE_HASH_SET
#define WONSY_LOCKFREE_HASH_SET__USE_SIZE		// LockfreHashSet이 Size를 지원 여부입니다. 비활성화를 원할 시, 주석처리해주세요.
#define WONSY_LOCKFREE_HASH_SET__USE_ITERATOR  // LockfreHashSet이 Iterator를 지원할 것인지.  비활성화를 원할 시, 주석처리해주세요.

#ifndef WONSY_PCH
// C++
#include <iostream>
#include <chrono>
#include <climits>

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

namespace WonSY::LOCKFREE_HASH_SET
{
#ifdef WONSY_LOCKFREE_HASH_SET__USE_ITERATOR 
	template<typename _Data>
	struct LockfreeHashSetIterator;
#endif
	
	namespace USING
	{
		using _KeyType = int;

#ifdef _WIN64
		using _PointerType = long long;	// x64 64bit
#else
		using _PointerType = int;	// x32 32bit
#endif
	}using namespace USING;

	namespace LOCKFREE_SET_LINKEDLIST
	{
		template<typename _Data>
		class Node;

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
			MarkedPointer() noexcept
				: value()
			{
				Set(nullptr, false);
			};

			~MarkedPointer() = default;

			MarkedPointer(const MarkedPointer&) = default;
			MarkedPointer& operator=(const MarkedPointer&) = default;

			MarkedPointer(MarkedPointer&&) = delete;
			MarkedPointer& operator=(MarkedPointer&&) = delete;

		public:

			void Set(const Node<_Data>* const node, const bool removed) noexcept
			{
				value = reinterpret_cast<_PointerType>(node);

				value = removed
					? value | REMOVED_MASK
					: value & POINTER_MASK;
			}

			_DO_NOT_DELETE Node<_Data>* GetPtr() const
			{
				return reinterpret_cast<Node<_Data>*>(value & POINTER_MASK);
			}

			_DO_NOT_DELETE Node<_Data>* GetPtrWithRemoved(bool& removed) const
			{
				const auto temp = value;

				0 == (temp & REMOVED_MASK)
					? removed = false
					: removed = true;

				return reinterpret_cast<Node<_Data>*>(value & POINTER_MASK);
			}

			bool CAS(const Node<_Data>* const oldNode, const Node<_Data>* const newNode, const bool oldRemoved, const bool newRemoved)
			{
				_PointerType oldValue = reinterpret_cast<_PointerType>(oldNode);

				if (oldRemoved) oldValue = oldValue | REMOVED_MASK;
				else oldValue = oldValue & POINTER_MASK;

				_PointerType newValue = reinterpret_cast<_PointerType>(newNode);
				if (newRemoved) newValue = newValue | REMOVED_MASK;
				else newValue = newValue & POINTER_MASK;

				return ATOMIC_UTIL::T_CAS(&value, oldValue, newValue);
			}

			bool TryMark(const Node<_Data>* const oldNode, const bool newMark)
			{
				auto oldValue = reinterpret_cast<_PointerType>(oldNode);
				auto newValue = oldValue;

				if (newMark) newValue = newValue | REMOVED_MASK;
				else newValue = newValue & POINTER_MASK;

				return ATOMIC_UTIL::T_CAS(&value, oldValue, newValue);
			}
		};

		template<typename _Data>
		class Node
		{
		public:
			_Data data;
			MarkedPointer<_Data> markedPointer;

		public:
			Node() noexcept
				: data()
				, markedPointer()
			{
				if constexpr (is_pointer<_Data>::value)
				{
					data = new typename std::remove_pointer<_Data>::type();
				}
			}

			Node(const _Data& dataValue) noexcept
				: data(dataValue)
				, markedPointer()
			{
			}

			~Node() = default;
			Node(const Node&) = default;
			Node& operator=(const Node&) = default;

			Node(Node&&) = delete;
			Node& operator=(Node&&) = delete;

			_KeyType key() noexcept
			{
				if constexpr (is_pointer<_Data>::value) { return data->GetKey(); }
				else { return data.GetKey(); }
			}
		};

		template<typename _Data>
		class LockfreeSet {
			Node<_Data> head, tail;
			concurrent_queue<Node<_Data>*> memoryPool;

		public:
			LockfreeSet(const int memoryPoolSize) /*noexcept*/
				: head(/*INT_MIN*/)
				, tail(/*INT_MAX*/)
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
				// tail.markedPointer.Set(nullptr, false);

				Node<_Data>* pushedNode{ nullptr };
				for (int i = 0; i < memoryPoolSize; ++i)
				{
					pushedNode = new Node<_Data>();
					memoryPool.push(pushedNode);
				}
			}
			~LockfreeSet()
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

			LockfreeSet(const LockfreeSet&) = default;
			LockfreeSet& operator=(const LockfreeSet&) = default;

		public:
			void Init()
			{
				Node<_Data>* ptr;

				while (head.markedPointer.GetPtr() != &tail)
				{
					ptr = head.markedPointer.GetPtr();
					head.markedPointer = head.markedPointer.GetPtr()->markedPointer;

					//delete ptr;
					memoryPool.push(ptr);
				}
			};

			void Find(const _KeyType key, Node<_Data>* (&pred), Node<_Data>* (&curr))
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

			pair<bool, Node<_Data>*> Add(const _KeyType key)
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
							// 메모리풀 사이즈가 넉넉하지 않을 경우
							// 메모리 할당과 관련없이 비정상적으로 동작할 가능성이 매우 높습니다.
							// assert문에 걸리기 전에, 이미 비정상적으로 동작할 가능성이 높으니 충분히 할당해주세요.
						}

						if constexpr (is_pointer<_Data>::value) { addedNode->data->SetKey(key); }
						else { addedNode->data.SetKey(key); }

						addedNode->markedPointer.Set(curr, false);

						if (pred->markedPointer.CAS(curr, addedNode, false, false))
						{
							return make_pair(true, addedNode);
						}
					}
				}
			}
			bool Remove(const _KeyType key)
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
			bool Contains(const _KeyType key)
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
			/* ~testing */ 
			template<typename _Function, typename... Arguments>
			void DoFunc(const _Function& function, const Arguments&... parameter)
			{
				Node<_Data>* curr{ (&head)->markedPointer.GetPtr() };
				Node<_Data>* succ{ nullptr };
				bool removed{};

				while (curr->key() < tail.key())
				{
					if (succ = curr->markedPointer.GetPtrWithRemoved(removed)
						; !removed) 
					{ 
						function(succ->data, parameter...); 
					}
					
					curr = succ;
				}
			}

#ifdef WONSY_LOCKFREE_HASH_SET__USE_ITERATOR 
			template<typename _Data> friend struct  WonSY::LOCKFREE_HASH_SET::LockfreeHashSetIterator;

			Node<_Data>* GetHead() { return &head; };
#endif
		};
	}

	template<typename _Data>
	class LockfreeHashSet
	{
	public:
		pair<bool, LOCKFREE_SET_LINKEDLIST::Node<_Data>*> Add(const _KeyType key)
		{
#ifdef WONSY_LOCKFREE_HASH_SET__USE_SIZE
			auto retValue = setCont[hashFunction(key)].Add(key);
			if (retValue.first) ++size;
			return retValue;
#else
			return setCont[hashFunction(key)].Add(key);
#endif
		}
		bool Remove(const _KeyType key)
		{
#ifdef WONSY_LOCKFREE_HASH_SET__USE_SIZE
			if (setCont[hashFunction(key)].Remove(key))
			{
				--size;
				return true;
			}
			return false;
#else			
			return setCont[hashFunction(key)].Remove(key);
#endif		
		}
		bool Contains(const _KeyType key)
		{
			return setCont[hashFunction(key)].Contains(key);
		}

	public:
		LockfreeHashSet(const int memoryPoolSize, const int bucketCount) 
			: hashFunction()
			, setCont()
#ifdef WONSY_LOCKFREE_HASH_SET__USE_SIZE
			, size()
#endif
		{
			setCont.reserve(bucketCount);

			for (int i = 0; i < bucketCount; ++i) { setCont.emplace_back(memoryPoolSize); }

			hashFunction = [bucketCount](_KeyType key)->_KeyType {return key % bucketCount; };
		}
		LockfreeHashSet(const int memoryPoolSize, const int bucketCount, const std::function<_KeyType(_KeyType)> func)
			: hashFunction(func)
			, setCont()
#ifdef WONSY_LOCKFREE_HASH_SET__USE_SIZE
			, size()
#endif
		{
			setCont.reserve(bucketCount);
			for (int i = 0; i < bucketCount; ++i) { setCont.emplace_back(memoryPoolSize); }
		}

	public:
		//Testing functions 
		template<typename _Function, typename... Arguments>
		void DoFunc(const _Function& function, const Arguments&... parameter)
		{
			for (auto& set : setCont) { set.DoFunc(function, parameter...); };
		}

		void Display()
		{
			for (auto& set : setCont)
			{
				set.Display(20);
				std::cout << "\n";
			}
			std::cout << "\n";
			std::cout << "\n";
		}

#ifdef WONSY_LOCKFREE_HASH_SET__USE_ITERATOR 
		template<typename _Data> friend struct LockfreeHashSetIterator;

		LockfreeHashSetIterator<_Data> begin()
		{
			return LockfreeHashSetIterator<_Data>(*this, false);
		}

		LockfreeHashSetIterator<_Data> end()
		{
			return LockfreeHashSetIterator<_Data>(*this, true);
		}
#endif
	private:
		std::function<_KeyType(_KeyType)> hashFunction;
		std::vector<LOCKFREE_SET_LINKEDLIST::LockfreeSet<_Data>> setCont;
		
#ifdef WONSY_LOCKFREE_HASH_SET__USE_SIZE
	public:
		_INLINE const int GetSize() const noexcept { return size; };
		
	private:
		std::atomic<int> size;
#endif
	};

#ifdef WONSY_LOCKFREE_HASH_SET__USE_ITERATOR
	template<typename _Data>
	struct LockfreeHashSetIterator
	{
		LOCKFREE_SET_LINKEDLIST::Node<_Data>* node;
		LockfreeHashSet<_Data>& pCont;
		const int totalContSize;
		int nowContIndex;

		LockfreeHashSetIterator(LockfreeHashSet<_Data>& pCont, const bool isEnd)
			: node(nullptr)
			, pCont(pCont)
			, totalContSize(pCont.setCont.size())
			, nowContIndex(0)
		{
			node = pCont.setCont[0].GetHead();

			if(isEnd) nowContIndex = totalContSize;
			if(!isEnd) ++(*this);
		}

		// Iterator 한칸 전진
		void operator++()
		{
			bool removed;
			_KeyType tailKey = pCont.setCont[0 /*nowContIndex*/ ].tail.key();

			while (true)
			{
				if (node->key() == tailKey)
				{
					if (++nowContIndex == totalContSize) { return; }	// == end()
					node = pCont.setCont[nowContIndex].GetHead();	// => 컨테이너 인덱스 변경함.
				}

				// 적합한 다음 노드를 받음.
				node = node->markedPointer.GetPtrWithRemoved(removed);
				
				// 제거 안된거일 때는 다음 노드 여부만 처리
				if (!removed)
				{
					if (node->key() != tailKey) { return; } // 다음 노드
				}
				// 제거 되인 것일 때는, end이면 end 처리, 컨테이너 인덱스 변경함.
			}
		}

		// Iter와 End의 비교 용도로만 사용됩니다.
		bool operator!=(const LockfreeHashSetIterator& other)
		{
			return nowContIndex != other.nowContIndex;
		}

		// Iterator 데이터 접근
		_Data& operator*()
		{
			return node->data;
		}
	};
#endif
}

// for TestFunc
#ifndef WONSY_PCH
#include <thread>

#include <concurrent_unordered_set.h>
#endif

namespace WonSY::LOCKFREE_HASH_SET::TEST
{
	struct ExampleStruct
	{
		WonSY::LOCKFREE_HASH_SET::_KeyType key;
		char data[100];

		_INLINE WonSY::LOCKFREE_HASH_SET::_KeyType GetKey() { return key; }
		_INLINE void SetKey(WonSY::LOCKFREE_HASH_SET::_KeyType key) { this->key = key; }
	};

	void TestFunc()
	{
		const auto NUM_TEST = 1000000;
		const auto KEY_RANGE = 10000;

		std::cout << " \n NUM_TEST : "<< NUM_TEST << " , KEY_RANGE " << KEY_RANGE << "\n";

		auto Func = [](ExampleStruct& data, int index, char char1) noexcept ->void
		{
			std::memcpy(data.data, &index, 4);
			data.data[0] = char1;
		};

		//auto Func = [](ExampleStruct*& data, int index, char char1) noexcept ->void
		//{
		//	std::memcpy(data->data, &index, 4);
		//	data->data[0] = char1;
		//};

		// LockFree HASH Set
		std::cout << "\n\n LockFree HASH Set의 add, delete, contains 성능은? \n";
		for (int i = 1; i <= 8; i = i * 2)
		{
			WonSY::LOCKFREE_HASH_SET::LockfreeHashSet<ExampleStruct> cont(1000, 20);

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
						case 1: cont.Contains(key); break;
						case 2: cont.Remove(key); break;
						// case 3: cont.DoFunc(Func, k, 'A'); break;
						default: cout << "Error\n"; exit(-1);
						}
					}
				});
			}

			for (auto& thread : threadCont) { thread.join(); }

			auto endTime = high_resolution_clock::now() - startTime;
			std::cout << "쓰레드 "<< i << "개의 성능은? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
			
			// iterator
			int iterCount{ 0 };
			for (auto iter = cont.begin(); iter != cont.end(); ++iter)
			{
				++iterCount;
			}
			std::cout << " iterCount : " << iterCount << "\n";
			std::cout << " size : " << cont.GetSize() << "\n";

			// Function VS
			{
				auto startTime = high_resolution_clock::now();
				for (auto iter = cont.begin(); iter != cont.end(); ++iter)
				{
					Func(*iter, 1, 'A');
				}
				auto endTime = high_resolution_clock::now() - startTime;
				std::cout << " 이터레이터 의 성능은? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
			}
			{
				auto startTime = high_resolution_clock::now();
				cont.DoFunc(Func, 1, 'A');
				auto endTime = high_resolution_clock::now() - startTime;
				std::cout << " 함수 인자의 성능은? " << duration_cast<milliseconds>(endTime).count() << " msecs\n\n\n";
			}
		}

		// Set - Linkedlist
		std::cout << "\n\n LockFree Set(Linkedlist)의 add, delete, contains 성능은? \n";
		for (int i = 1; i <= 8; i = i * 2)
		{
			WonSY::LOCKFREE_HASH_SET::LOCKFREE_SET_LINKEDLIST::LockfreeSet<ExampleStruct> cont(1000);

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
						case 1: cont.Contains(key); break;
						case 2: cont.Remove(key); break;
						// case 3: cont.DoFunc(Func, k, 'A'); break;
						default: cout << "Error\n"; exit(-1);
						}
					}
				});
			}

			for (auto& thread : threadCont) { thread.join(); }

			auto endTime = high_resolution_clock::now() - startTime;
			std::cout << i << "개의 성능은? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
			// cont.Display(20);
		}

		// concurrent_unorederedSet
		std::cout << "\n\n concurrent_unorederedSet의 add, contains 성능은? (delete - unsafe) \n";
		for (int i = 1; i <= 8; i = i * 2)
		{
#pragma warning(disable : 4996)
			concurrency::concurrent_unordered_set<_KeyType> cont;

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
							; rand() % 2)
						{
						case 0: cont.insert(key);	break;
						case 1: cont.find(key); break;
						default: cout << "Error\n"; exit(-1);
						}
					}
				});
			}

			for (auto& thread : threadCont) { thread.join(); }

			auto endTime = high_resolution_clock::now() - startTime;
			std::cout << i << "개의 성능은? " << duration_cast<milliseconds>(endTime).count() << " msecs\n";
		}
	}
}