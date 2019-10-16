#define LOAD_NO 0 
#define LOAD_LOCAL 1
#define LOAD_GLOBAL 2

MODULE_ID loadModule(int fd, int symFlag);
MODULE_ID loadModuleAt(int fd, int symFlag, char * *ppText, char * *ppData, char * *ppBss);
