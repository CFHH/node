package v8vm

/*
#include <stdio.h>

extern int BalanceTransfer(char*, char*, int);
int BalanceTransferStub(char* from, char* to, int amount)
{
	return BalanceTransfer(from, to, amount);
}
*/
import "C"
