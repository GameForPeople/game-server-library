#include "stdafx.h"

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
		return reinterpret_cast<Node*>(value & POINTER_MASK);
	}

	template<typename _Data>
	_DO_NOT_DELETE Node<_Data>* MarkedPointer<_Data>::GetPtrWithRemoved(bool& removed) const
	{
		const auto temp = value;

		0 == (temp & REMOVED_MASK)
			? removed = false
			: removed = true;

		return reinterpret_cast<Node*>(value & POINTER_MASK);
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
	Node<_Data>::Node(const _Data dataValue) noexcept
		: data(dataValue)
		, markedPointer()
	{
	}

	template<typename _Data>
	_KeyType Node<_Data>::key() noexcept
	{
		return data->key();
	}

	template<typename _Data>
	List<_Data>::List(const int memoryPoolSize) noexcept
		: head(INT_MIN)
		, tail(INT_MAX)
	{
		head.markedPointer.Set(&tail, false);

		Node* pushedNode{ nullptr };
		for (int i = 0; i < memoryPoolSize; ++i)
		{
			pushedNode = new Node();
			memoryPool.push(pushedNode);
		}
	}

	template<typename _Data>
	List<_Data>::~List()
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

	template<typename _Data>
	void List<_Data>::Init()
	{
		Node* ptr;

		while (head.markedPointer.GetPtr() != &tail)
		{
			ptr = head.markedPointer.GetPtr();
			head.markedPointer = head.markedPointer.GetPtr()->markedPointer;

			//delete ptr;
			memoryPool.push(ptr);
		}
	}

	template<typename _Data>
	void List<_Data>::Find(const _KeyType key, Node<_Data>* (&pred), Node<_Data>* (&curr))
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
	bool List<_Data>::Add(const _NOT_NULLPTR Node<_Data>* addedNode)
	{
		assert(addedNode == nullptr, "[ERROR] addedNode is nullptr!");

		Node<_Data>* pred{ nullptr };
		Node<_Data>* curr{ nullptr };

		auto key{ addedNode->key() };

		while (7)
		{
			Find(key, pred, curr);

			if (curr->key() == key) { return false; }
			else
			{
				// Node* addedNode = new Node(key);
				// Node<_Data>* addedNode{ nullptr };

				//if (!memoryPool.try_pop(addedNode))
				//{
				//	addedNode = new Node(/*key*/);
				//}

				//addedNode->key = key;

				addedNode->markedPointer.Set(curr, false);

				if (pred->markedPointer.CAS(curr, addedNode, false, false)) { return true; }
			}
		}
	}

	template<typename _Data>
	bool List<_Data>::Remove(const _KeyType key)
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
	bool List<_Data>::Contains(const _KeyType key)
	{
		Node<_Data>* curr = &head;
		bool removed{};

		while (curr->key() < key)
		{
			curr = curr->markedPointer.GetPtr();
			Node* succ = curr->markedPointer.GetPtrWithRemoved(removed);
		}

		return curr->key() == key && !removed;
	}
}