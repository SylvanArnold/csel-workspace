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
 * Purpose:	Memory leak demo program 
 *		call this demo program with 
 *              $ valgrind  --leak-check=full ./app_a
 *
 * Autĥor:	Daniel Gachet
 * Date:	28.08.2015
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#ifdef DEBUG
#define TRACE(x) printf x
#else
#define TRACE(x)
#endif

struct alpha
{ 
	int attr;
	struct alpha* next;
};


void alloc1 (struct alpha** root)
{
	static int cnt=0;

	struct alpha* ele = malloc (sizeof(*ele));
	ele->next = 0;
	ele->attr = cnt++;
	if (*root == 0) {
		*root = ele;
	} else {
		struct alpha* n = *root;
		while (n->next != 0) n = n->next;
		n->next = ele;
	}
}

void alloc2(struct alpha** root)
{
	static int cnt=0;

	struct alpha* ele = malloc (sizeof(*ele));
	ele->next = 0;
	ele->attr = cnt++;
	if (*root == 0) {
		*root = ele;
	} else {
		struct alpha* n = *root;
		while (n->next != 0) n = n->next;
		n->next = ele;
	}
}

void alloc(struct alpha** root)
{
	static int x=0;

	if (x == 0) alloc1(root); 
	else alloc2 (root);

	x = (x + 1) % 2;
}

void dealloc(struct alpha** root)
{
	struct alpha* ele = *root;
	if (*root != 0) *root = ele->next;
	free (ele);
}

int main ()
{
	struct alpha* root = 0;

	for (int i=0; i<4000; i++) alloc (&root);

	struct alpha* p = root;
	int i = 0;
	while (p != 0) {
		TRACE (("ele %2d, attr=%2d, ptr=%p\n", i, p->attr, p));
		p = p->next;
		i++;
	}

	for (int i=0; i<15; i++) dealloc (&root);

	return 0;
}


