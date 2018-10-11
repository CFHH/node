package v8vm

/*
#include <stdio.h>

extern int BalanceTransfer(int, char*);
int BalanceTransferStub(int value1, char* value2)
{
	return BalanceTransfer(value1, value2);
}
*/
import "C"
