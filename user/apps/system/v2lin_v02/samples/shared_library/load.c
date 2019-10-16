/*
 * Copyright (c) 2005 Mike Kemelmakher <mike@ubxess.com>
 * Contributed by Constantine Shulyupin, conan.sh@gmail.com, 2006.
 *
 * This code may be redistributed under the terms of either the GNU General Public
 * License (GPLv2) or (LGPL) or BSD License
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdlib.h>
#include <dlfcn.h>

#include <v2ldebug.h>
#include "sample_lib.h"

sample_struct_t * sample_struct_p;

const char lname[] = "libsample_lib.so";
// define LD_LIBRARY_PATH environment variable with library location
int main()
{
	void *handle = NULL;
	char *error;
	void (*sample_function2)();
	
	handle = dlopen("libsample_lib.so", RTLD_LAZY);
	if (!handle) {
		fprintf(stderr,"dlopen: %s\n",dlerror());
		return -1;
	}

	TRACEF("library loaded successfuly at %p\n", handle);

	sample_struct_p = dlsym(handle, "sample_struct");

	if ((error = dlerror()) != NULL) {
		TRACEF("%s\n", error);
		return -1;
	}

	sample_function2 = dlsym(handle, "sample_function ");

	if ((error = dlerror()) != NULL) {
		TRACEF("%s\n", error);
		return -1;
	}
	printf("loaded: %i\n",sample_struct_p->i);
	sample_function2();

	dlclose(handle);
	exit(0);
}
