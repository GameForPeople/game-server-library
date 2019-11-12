#include "stdafx.h"

#include "../../Container/Lockfree_Hash_Set/LockFree_Hash_Set.h"
#include "../../Container/Lockfree_Set_Linkedlist/Lockfree_Set_Linkedlist.h"
#include "../../Lock/Spin_Lock.h"
#include "../../Container/Lockfree_Stack/Lockfree_Stack.h"

int main()
{
//	WonSY::LOCKFREE_HASH_SET::TEST::TestFunc();
//	WonSY::LOCKFREE_SET_LINKEDLIST::TEST::TestFunc();
	LOCK_STACK::TestFunc();// testFunc();
	std::system("PAUSE");
	return 0;
}