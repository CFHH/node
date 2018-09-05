// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "v8vm.h"

int TransactionCallbackStub(int value1, char* value2)
{
	return 0;
}

int TransactionResultStub(int value1, char* value2)
{
	return 0;
}


int main(int argc, char* argv[])
{
	SetTransactionCallback(TransactionCallbackStub);
	SetTransactionResult(TransactionResultStub);
	InitializeVM();
	int retstr = ExecuteTransaction("TransactionCallback(123, 'fromjs')");
	//int retstr = ExecuteTransaction("print1(123, 'fromjs')");
	DisposeVM();
	return retstr;
}
