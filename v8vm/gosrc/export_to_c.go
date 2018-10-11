package v8vm

/*
#include <stdio.h>

extern int BalanceTransfer(__int64, char*, char*, int);
int BalanceTransferStub(__int64 vmid, char* from, char* to, int amount)
{
	return BalanceTransfer(vmid, from, to, amount);
}
*/
import "C"
