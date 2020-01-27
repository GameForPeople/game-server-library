#pragma once

#include "../../GameServerLibrary/GameServerLibrary/stdafx.h"

#define WONSY_LOCKFREE_SKIPLIST

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

namespace WonSY::LOCKFREE_SKIPLIST
{
	class Node
	{
	public:
		int key;
		Node* next[MAX_LEVEL];
		int topLevel;

		// 보초노드에 관한 생성자
		Node() {
			for (int i = 0; i < MAX_LEVEL; i++) {
				next[i] = AtomicMarkableReference(NULL, false);
			}
			topLevel = MAX_LEVEL;
		}
		Node(int myKey) {
			key = myKey;
			for (int i = 0; i < MAX_LEVEL; i++) {
				next[i] = AtomicMarkableReference(NULL, false);
			}
			topLevel = MAX_LEVEL;
		}

		// 일반노드에 관한 생성자
		Node(int x, int height) {
			key = x;
			for (int i = 0; i < MAX_LEVEL; i++) {
				next[i] = AtomicMarkableReference(NULL, false);
			}
			topLevel = height;
		}

		void InitNode() {
			key = 0;
			for (int i = 0; i < MAX_LEVEL; i++) {
				next[i] = AtomicMarkableReference(NULL, false);
			}
			topLevel = MAX_LEVEL;
		}

		void InitNode(int x, int top) {
			key = x;
			for (int i = 0; i < MAX_LEVEL; i++) {
				next[i] = AtomicMarkableReference(NULL, false);
			}
			topLevel = top;
		}

		bool CompareAndSet(int level, Node* old_node, Node* next_node, bool old_mark, bool next_mark) {
			int old_addr = reinterpret_cast<int>(old_node);
			if (old_mark) old_addr = old_addr | 0x1;
			else old_addr = old_addr & 0xFFFFFFFE;
			int next_addr = reinterpret_cast<int>(next_node);
			if (next_mark) next_addr = next_addr | 0x1;
			else next_addr = next_addr & 0xFFFFFFFE;
			return atomic_compare_exchange_strong(reinterpret_cast<atomic_int*>(&next[level]), &old_addr, next_addr);
			//int prev_addr = InterlockedCompareExchange(reinterpret_cast<long *>(&next[level]), next_addr, old_addr);
			//return (prev_addr == old_addr);
		}
	};

	class LockFreeSkipList
	{
	public:

		Node* head;
		Node* tail;

		LockFreeSkipList() {
			head = new Node(0x80000000);
			tail = new Node(0x7FFFFFFF);
			for (int i = 0; i < MAX_LEVEL; i++) {
				head->next[i] = AtomicMarkableReference(tail, false);
			}
		}

		void Init()
		{
			Node* curr = head->next[0];
			while (curr != tail) {
				Node* temp = curr;
				curr = GetReference(curr->next[0]);
				delete temp;
			}
			for (int i = 0; i < MAX_LEVEL; i++) {
				head->next[i] = AtomicMarkableReference(tail, false);
			}
		}

		bool Find(int x, Node* preds[], Node* succs[])
		{
			int bottomLevel = 0;
			bool marked = false;
			bool snip;
			Node* pred = NULL;
			Node* curr = NULL;
			Node* succ = NULL;

		retry:
			while (true) {
				pred = head;
				for (int level = MAX_LEVEL - 1; level >= bottomLevel; level--) {
					curr = GetReference(pred->next[level]);
					while (true) {
						succ = curr->next[level];
						while (Marked(succ)) { //표시되었다면 제거
							snip = pred->CompareAndSet(level, curr, succ, false, false);
							if (!snip) goto retry;
							//	if (level == bottomLevel) freelist.free(curr);
							curr = GetReference(pred->next[level]);
							succ = curr->next[level];
						}

						// 표시 되지 않은 경우
						// 키값이 현재 노드의 키값보다 작다면 pred전진
						if (curr->key < x) {
							pred = curr;
							curr = GetReference(succ);
							// 키값이 그렇지 않은 경우
							// curr키는 대상키보다 같거나 큰것이므로 pred의 키값이 
							// 대상 노드의 바로 앞 노드가 된다.		
						}
						else {
							break;
						}
					}
					preds[level] = pred;
					succs[level] = curr;
				}
				return (curr->key == x);
			}
		}

		bool Add(int x)
		{
			int topLevel = 0;
			while ((rand() % 2) == 1)
			{
				topLevel++;
				if (topLevel >= MAX_LEVEL - 1) break;
			}

			int bottomLevel = 0;

			Node* preds[MAX_LEVEL];
			Node* succs[MAX_LEVEL];
			while (true) {
				bool found = Find(x, preds, succs);
				// 대상 키를 갖는 표시되지 않은 노드를 찾으면 키가 이미 집합에 있으므로 false 반환
				if (found) {
					return false;
				}
				else {
					Node* newNode = new Node;
					newNode->InitNode(x, topLevel);

					for (int level = bottomLevel; level <= topLevel; level++) {
						Node* succ = succs[level];
						// 현재 새노드의 next는 표시되지 않은 상태, find()가 반환반 노드를 참조
						newNode->next[level] = Set(succ, false);
					}

					//find에서 반환한 pred와 succ의 가장 최하층을 먼저 연결
					Node* pred = preds[bottomLevel];
					Node* succ = succs[bottomLevel];

					newNode->next[bottomLevel] = Set(succ, false);

					//pred->next가 현재 succ를 가리키고 있는지 않았는지 확인하고 newNode와 참조설정
					if (!pred->CompareAndSet(bottomLevel, succ, newNode, false, false))
						// 실패일경우는 next값이 변경되었으므로 다시 호출을 시작
						continue;

					for (int level = bottomLevel + 1; level <= topLevel; level++) {
						while (true) {
							pred = GetReference(preds[level]);
							succ = GetReference(succs[level]);
							// 최하층 보다 높은 층들을 차례대로 연결
							// 연결을 성공할경우 다음단계로 넘어간다
							if (pred->CompareAndSet(level, succ, newNode, false, false))
								break;
							//Find호출을 통해 변경된 preds, succs를 새로 얻는다.
							Find(x, preds, succs);
						}
					}

					//모든 층에서 연결되었으면 true반환
					return true;
				}
			}
		}

		bool Remove(int x)
		{
			int bottomLevel = 0;
			Node* preds[MAX_LEVEL];
			Node* succs[MAX_LEVEL];
			Node* succ;

			while (true) {
				bool found = Find(x, preds, succs);
				if (!found) {
					//최하층에 제거하려는 노드가 없거나, 짝이 맞는 키를 갖는 노드가 표시 되어 있다면 false반환
					return false;
				}
				else {
					Node* nodeToRemove = succs[bottomLevel];
					//최하층을 제외한 모든 노드의 next와 mark를 읽고 AttemptMark를 이용하여 연결에 표시
					for (int level = nodeToRemove->topLevel; level >= bottomLevel + 1; level--) {
						succ = nodeToRemove->next[level];
						// 만약 연결이 표시되어있으면 메서드는 다음층으로 이동
						// 그렇지 않은 경우 다른 스레드가 병행을 햇다는 뜻이므로 현재 층의 연결을 다시 읽고
						// 연결에 다시 표시하려고 시도한다.
						while (!Marked(succ)) {
							nodeToRemove->CompareAndSet(level, succ, succ, false, true);
							succ = nodeToRemove->next[level];
						}
					}
					//이부분에 왔다는 것은 최하층을 제외한 모든 층에 표시했다는 의미

					bool marked = false;
					succ = nodeToRemove->next[bottomLevel];
					while (true) {
						//최하층의 next참조에 표시하고 성공했으면 Remove()완료
						bool iMarkedIt = nodeToRemove->CompareAndSet(bottomLevel, succ, succ, false, true);
						succ = succs[bottomLevel]->next[bottomLevel];

						if (iMarkedIt) {
							Find(x, preds, succs);
							return true;
						}
						else if (Marked(succ)) return false;
					}
				}
			}
		}

		bool Contains(int x)
		{
			int bottomLevel = 0;
			bool marked = false;
			Node* pred = head;
			Node* curr = NULL;
			Node* succ = NULL;

			for (int level = MAX_LEVEL - 1; level >= bottomLevel; level--) {
				curr = GetReference(pred->next[level]);
				while (true) {
					succ = curr->next[level];
					while (Marked(succ)) {
						curr = GetReference(curr->next[level]);
						succ = curr->next[level];
					}
					if (curr->key < x) {
						pred = curr;
						curr = GetReference(succ);
					}
					else {
						break;
					}
				}
			}
			return (curr->key == x);
		}
		void Dump()
		{
			Node* curr = head;
			printf("First 20 entries are : ");
			for (int i = 0; i < 20; ++i) {
				curr = curr->next[0];
				if (NULL == curr) break;
				printf("%d(%d), ", curr->key, curr->topLevel);
			}
			printf("\n");
		}
	};
}