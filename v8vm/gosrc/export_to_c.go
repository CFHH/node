package v8vm

/*
#include <stdio.h>

extern int BalanceTransfer(__int64, char*, char*, __int64);
int BalanceTransferStub(__int64 vmid, char* from, char* to, __int64 amount)
{
	return BalanceTransfer(vmid, from, to, amount);
}
*/
import "C"
