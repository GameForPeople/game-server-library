#include "stdafx.h"
#include "Lockfree_Set_Linkedlist.h"

namespace LIST_4_LOCKFREE
{
	MarkedPointer::MarkedPointer() noexcept
		: value()
	{
		Set(nullptr, false);
	};

	void MarkedPointer::Set(const Node* const node, const bool removed) noexcept
	{
		value = reinterpret_cast<_PointerType>(node);

		value = removed
			? value | REMOVED_MASK
			: value & POINTER_MASK;
	}

	_NODISCARD Node* MarkedPointer::GetPtr() const
	{
		return reinterpret_cast<Node*>(value & POINTER_MASK);
	}

	_NODISCARD Node* MarkedPointer::GetPtrWithRemoved(bool& removed) const
	{
		const auto temp = value;

		0 == (temp & REMOVED_MASK)
			? removed = false
			: removed = true;

		return reinterpret_cast<Node*>(value & POINTER_MASK);
	}

	bool MarkedPointer::CAS(const Node* const oldNode, const Node* const newNode, const bool oldRemoved, const bool newRemoved)
	{
		_PointerType oldValue = reinterpret_cast<_PointerType>(oldNode);

		if (oldRemoved) oldValue = oldValue | REMOVED_MASK;
		else oldValue = oldValue & POINTER_MASK;

		_PointerType newValue = reinterpret_cast<_PointerType>(newNode);
		if (newRemoved) newValue = newValue | REMOVED_MASK;
		else newValue = newValue & POINTER_MASK;

		return ATOMIC_UTIL::T_CAS(&value, oldValue, newValue);
	}

	bool MarkedPointer::TryMark(const Node* const oldNode, const bool newMark)
	{
		auto oldValue = reinterpret_cast<_PointerType>(oldNode);
		auto newValue = oldValue;

		if (newMark) newValue = newValue | REMOVED_MASK;
		else newValue = newValue & POINTER_MASK;

		return ATOMIC_UTIL::T_CAS(&value, oldValue, newValue);
	}

	Node::Node() noexcept
		: key()
		, next()
	{
	}

	Node::Node(const _KeyType keyValue) noexcept
		: key(keyValue)
		, next()
	{
	}

	List::List(const int memoryPoolSize = 2) noexcept
		: head(INT_MIN)
		, tail(INT_MAX)
	{
		head.next.Set(&tail, false);

		Node* pushedNode{ nullptr };
		for (int i = 0; i < memoryPoolSize; ++i)
		{
			pushedNode = new Node();
			memoryPool.push(pushedNode);
		}
	}

	List::~List()
	{
		Init();

		Node* deletedNode{ nullptr };
		bool retValue{ true };

		while (retValue == false)
		{
			retValue = memoryPool.try_pop(deletedNode);
			delete deletedNode;
		}
	}

	void List::Init()
	{
		Node* ptr;

		while (head.next.GetPtr() != &tail)
		{
			ptr = head.next.GetPtr();
			head.next = head.next.GetPtr()->next;

			//delete ptr;
			memoryPool.push(ptr);
		}
	}

	void List::Find(const _KeyType key, Node* (&pred), Node* (&curr))
	{
	retry:
		while (true)
		{
			pred = &head;
			curr = pred->next.GetPtr();

			while (true)
			{
				bool removed{};
				auto succ = curr->next.GetPtrWithRemoved(removed);

				while (removed)
				{
					if (!(pred->next.CAS(curr, succ, false, false))) { goto retry; }

					memoryPool.push(curr);

					curr = succ;
					succ = curr->next.GetPtrWithRemoved(removed);
				}

				if (curr->key >= key) return;

				pred = curr;
				curr = succ;
			}
		}
	}

	bool List::Add(const _KeyType key)
	{
		Node* pred{ nullptr };
		Node* curr{ nullptr };

		while (7)
		{
			Find(key, pred, curr);

			if (curr->key == key) { return false; }
			else
			{
				// Node* addedNode = new Node(key);
				Node* addedNode{ nullptr };

				if (!memoryPool.try_pop(addedNode))
				{
					addedNode = new Node(/*key*/);
				}

				addedNode->key = key;
				addedNode->next.Set(curr, false);

				if (pred->next.CAS(curr, addedNode, false, false)) { return true; }
			}
		}
	}

	bool List::Remove(const _KeyType key)
	{
		Node* pred{ nullptr };
		Node* curr{ nullptr };

		while (7)
		{
			Find(key, pred, curr);

			if (curr->key != key) { return false; }
			else
			{
				Node* nextNodeOfDeletedNode = curr->next.GetPtr();

				if (!curr->next.TryMark(nextNodeOfDeletedNode, true)) { continue; }
				if (pred->next.CAS(curr, nextNodeOfDeletedNode, false, false)) { memoryPool.push(curr); }

				return true;
			}
		}
	}

	bool List::Contains(const _KeyType key)
	{
		Node* curr = &head;
		bool removed{};

		while (curr->key < key) {
			curr = curr->next.GetPtr();
			Node* succ = curr->next.GetPtrWithRemoved(removed);
		}
		return curr->key == key && !removed;
	}
}