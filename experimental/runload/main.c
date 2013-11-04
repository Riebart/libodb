#include <dlfcn.h>
#include <stdio.h>

#include "comm.h"

int mul(int a, int b)
{
return a * b;
}

typedef int (*add_func)(int, int);
typedef int (*fmad_func)(int, int, int);

int main(int argc, char** argv)
{
void* lib = dlopen("/home/mike/Documents/runload/libload.so", RTLD_LAZY);
printf("%p\n", lib); fflush(stdout);

void* func;
int r;

func= dlsym(lib, "add");
printf("%p\n", func); fflush(stdout);
add_func add = (add_func)func;
r = add(10, 23);
printf("%d\n", r);

func= dlsym(lib, "fmad");
printf("%p\n", func); fflush(stdout);
fmad_func fmad = (fmad_func)func;
r = fmad(10, 23, 44);
printf("%d\n", r);

return r;

}
