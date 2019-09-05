//https://linux.die.net/man/2/execve
//https://en.wikipedia.org/wiki/Exec_(system_call)
//execve("/usr/bin/uname", ["uname"], 0x7fffc120c130 /* 21 vars */) = 0
//    int execve(char const *path, char const *argv[], char const *envp[]);
//int execve(const char *filename, char *const argv[],
//char *const envp[]); 

//http://man7.org/linux/man-pages/man2/brk.2.html
//extern int brk (void *__addr) __THROW __wur;
// int brk(void *addr);

//       void *sbrk(intptr_t increment);
//brk(0x10032c00000)                      = 0x10032c00000

//brk(NULL)
