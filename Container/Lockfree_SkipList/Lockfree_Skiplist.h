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

		// ���ʳ�忡 ���� ������
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

		// �Ϲݳ�忡 ���� ������
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
						while (Marked(succ)) { //ǥ�õǾ��ٸ� ����
							snip = pred->CompareAndSet(level, curr, succ, false, false);
							if (!snip) goto retry;
							//	if (level == bottomLevel) freelist.free(curr);
							curr = GetReference(pred->next[level]);
							succ = curr->next[level];
						}

						// ǥ�� ���� ���� ���
						// Ű���� ���� ����� Ű������ �۴ٸ� pred����
						if (curr->key < x) {
							pred = curr;
							curr = GetReference(succ);
							// Ű���� �׷��� ���� ���
							// currŰ�� ���Ű���� ���ų� ū���̹Ƿ� pred�� Ű���� 
							// ��� ����� �ٷ� �� ��尡 �ȴ�.		
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
				// ��� Ű�� ���� ǥ�õ��� ���� ��带 ã���� Ű�� �̹� ���տ� �����Ƿ� false ��ȯ
				if (found) {
					return false;
				}
				else {
					Node* newNode = new Node;
					newNode->InitNode(x, topLevel);

					for (int level = bottomLevel; level <= topLevel; level++) {
						Node* succ = succs[level];
						// ���� ������� next�� ǥ�õ��� ���� ����, find()�� ��ȯ�� ��带 ����
						newNode->next[level] = Set(succ, false);
					}

					//find���� ��ȯ�� pred�� succ�� ���� �������� ���� ����
					Node* pred = preds[bottomLevel];
					Node* succ = succs[bottomLevel];

					newNode->next[bottomLevel] = Set(succ, false);

					//pred->next�� ���� succ�� ����Ű�� �ִ��� �ʾҴ��� Ȯ���ϰ� newNode�� ��������
					if (!pred->CompareAndSet(bottomLevel, succ, newNode, false, false))
						// �����ϰ��� next���� ����Ǿ����Ƿ� �ٽ� ȣ���� ����
						continue;

					for (int level = bottomLevel + 1; level <= topLevel; level++) {
						while (true) {
							pred = GetReference(preds[level]);
							succ = GetReference(succs[level]);
							// ������ ���� ���� ������ ���ʴ�� ����
							// ������ �����Ұ�� �����ܰ�� �Ѿ��
							if (pred->CompareAndSet(level, succ, newNode, false, false))
								break;
							//Findȣ���� ���� ����� preds, succs�� ���� ��´�.
							Find(x, preds, succs);
						}
					}

					//��� ������ ����Ǿ����� true��ȯ
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
					//�������� �����Ϸ��� ��尡 ���ų�, ¦�� �´� Ű�� ���� ��尡 ǥ�� �Ǿ� �ִٸ� false��ȯ
					return false;
				}
				else {
					Node* nodeToRemove = succs[bottomLevel];
					//�������� ������ ��� ����� next�� mark�� �а� AttemptMark�� �̿��Ͽ� ���ῡ ǥ��
					for (int level = nodeToRemove->topLevel; level >= bottomLevel + 1; level--) {
						succ = nodeToRemove->next[level];
						// ���� ������ ǥ�õǾ������� �޼���� ���������� �̵�
						// �׷��� ���� ��� �ٸ� �����尡 ������ �޴ٴ� ���̹Ƿ� ���� ���� ������ �ٽ� �а�
						// ���ῡ �ٽ� ǥ���Ϸ��� �õ��Ѵ�.
						while (!Marked(succ)) {
							nodeToRemove->CompareAndSet(level, succ, succ, false, true);
							succ = nodeToRemove->next[level];
						}
					}
					//�̺κп� �Դٴ� ���� �������� ������ ��� ���� ǥ���ߴٴ� �ǹ�

					bool marked = false;
					succ = nodeToRemove->next[bottomLevel];
					while (true) {
						//�������� next������ ǥ���ϰ� ���������� Remove()�Ϸ�
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