package v8vm

/*

#include <stdio.h>

extern int TransactionCallback(int, char*);
int TransactionCallbackStub(int value1, char* value2)
{
	return TransactionCallback(value1, value2);
}

extern int TransactionResult(int, char*);
int TransactionResultStub(int value1, char* value2)
{
	return TransactionResult(value1, value2);
}

extern void TransactionResultVoid(int, char*);
void TransactionResultVoidStub(int value1, char* value2)
{
	TransactionResultVoid(value1, value2);
}
*/
import "C"
