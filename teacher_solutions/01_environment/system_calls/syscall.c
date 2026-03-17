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
 * Purpose:	System call demo program 
 *		call this demo program with 
 *              # strace ./app_a
 *
 * Autĥor:	Daniel Gachet
 * Date:	28.08.2015
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main() 
{	
	int temp = 99000;
	int f = open("/sys/class/thermal/thermal_zone0/temp", O_RDONLY);
	if (f >= 0) {
		char val[50] = "";
		ssize_t r = read (f, val, sizeof(val));
		close (f);
		if (r > 0) {
			temp = atoi(val);
		}
	}
	printf ("current temperature: %d.%02d degree Celcius\n", 
		temp/1000, (temp/10)%100);

	return 0;
}


