/**
 * Copyright 2015 University of Applied Sciences Western Switzerland / Fribourg
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * Module:	MA-CSEL1 - Building Embedded Systems under Linux
 *
 * Abstract: 	Introduction to the Embedded Linux Development Environment
 *
 * Purpose:	Backtracing demo program.
 *
 * Autĥor:	Daniel Gachet
 * Date:	28.08.2015
 */

#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include <unistd.h>
#include <signal.h>

#define SIZE 1000

void signal_handler(int __attribute__((unused)) sig)
{
	void* buffer[SIZE];
	int nptrs = backtrace(buffer, SIZE);
	printf("backtrace() returned %d addresses\n", nptrs);
	backtrace_symbols_fd (buffer, nptrs, STDOUT_FILENO);
	exit(EXIT_FAILURE);
}

void access_data() 
{
	int* p=0;
	*p=10;
}

void call (int n)
{
	if (n>0) call (n-1);
	access_data();
}


int main ()
{
	signal(SIGSEGV, signal_handler); // Invalid memory segment access 

	call(10);

	return 0;
}
