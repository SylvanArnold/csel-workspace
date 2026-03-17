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
 * Purpose:	Fibonacci numbers: a simple demo program to exercice debugging
 * 		facilities on embedded Linux system.
 *
 *		-->> tracing with printf
 *
 * Autĥor:	Daniel Gachet
 * Date:	28.08.2015
 */

#include <stdio.h>
#include <stdlib.h>

/**
 * The Fibonacci numbers form a sequence of integers, mathematically defined by
 *   F(0)=0; F(1)=1; F(n) = F(n - 1) + F(n - 2) for n > 1.
 *
 * This results in the following sequence of numbers: 0, 1, 1, 2, 3, 5, 8, 13,
 *    21, 34, 55, 89, 144, 233, 377, 610, 987, ...
 * so that each new number is the sum of the previous two, after seeding the
 * sequence with the starting pair 0, 1.
 *
 * @param n value for which the Fibonacci number should be computed
 * @return Fibonacci number
 */
long fibonacci_r(long n)
{
    if (n <= 1)
        return n;
    return fibonacci_r(n - 1) + fibonacci_r(n - 2);
}

long fibonacci_i(long n)
{
    long f0 = 0;
    long f1 = 1;
    while (n > 0)
    {
        f1 = f1 + f0;
        f0 = f1 - f0; // WAT !!!
        n--;
    }
    return f0;
}

/**
 * Main program computing the Fibonacci numbers for a given sequence starting
 * from 0 to a number specified at the command line.
 */
int main(int argc, char **argv)
{
    long n = 0;
    long i = 0;

    if (argc == 2)
    {
        n = atol(argv[1]);
    }

    printf("%s, %d, %s, %s\n", __FILE__, __LINE__, __DATE__, __TIME__);
    printf("The first %ld Fibonacci numbers are:\n", n);

    for (i = 0; i < n; i++)
    {
        printf("%ld%s", fibonacci_i(i), i < n-1 ? ", " : "");
    }
    printf("\n");

    return 0;
}
