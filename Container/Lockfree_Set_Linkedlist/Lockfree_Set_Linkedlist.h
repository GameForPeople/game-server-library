#pragma once

namespace LIST_4_LOCKFREE // ¶ô ÇÁ¸®.
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

	class Node;

	class MarkedPointer
	{
		_PointerType value;	// next Node Pointer with removed Mark(1bit).
	public:
		MarkedPointer() noexcept;

		void Set(const Node* const node, const bool removed) noexcept;

		_NODISCARD Node* GetPtr() const;

		_NODISCARD Node* GetPtrWithRemoved(bool& removed) const;

		bool CAS(const Node* const oldNode, const Node* const newNode, const bool oldRemoved, const bool newRemoved);

		bool TryMark(const Node* const oldNode, const bool newMark);
	};

	class Node
	{
	public:
		_KeyType key;
		MarkedPointer next;

		Node() noexcept;
		Node(const _KeyType keyValue) noexcept;
		~Node() = default;
	};

	class List {
	public:
		Node head, tail;
		concurrent_queue<Node*> memoryPool;

		List(const int memoryPoolSize /* = GLOBAL::KEY_RANGE * 2 */) noexcept;

		~List();

		void Init();

		void Find(const _KeyType key, Node* (&pred), Node* (&curr));

		bool Add(const _KeyType key);

		bool Remove(const _KeyType key);

		bool Contains(const _KeyType key);
	};
}