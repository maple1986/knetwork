#ifndef __KNETWORK_UTIL_H__
#define __KNETWORK_UTIL_H__
#include <windows.h>

typedef enum {
	TASK_STATUS_ERROR = -1,
	TASK_STATUS_OK = 0,
	TASK_STATUS_RUNNING,
	TASK_STATUS_PAUSED,
	TASK_STATUS_FINISHED,
	TASK_STATUS_LAST
} TaskStatus;

typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;

static int ReadInt(const char* pch) 
{ 
	return *pch | (DWORD)*(pch+1) << 8 & 0xff00 | (DWORD)*(pch+2) << 16 & 0xff0000 | (DWORD)*(pch+3) << 24 & 0xff000000;
}

static int find_last_of(const char* p, const char ch)
{
	int i;
	for( i = strlen(p)-1; i >= 0; --i) 
	{
		if(ch == *(p+i))
		{
			return i;
		}
	}
	return -1;
}
typedef int (*thread_fun)(void*);

typedef struct _thread_param
{
	thread_fun	callback;
	void*		param;
}thread_param;

DWORD WINAPI Win32Thread(LPVOID pParam);

BOOL kgutil_create_thread(thread_fun funcAddr, void* param);

uint32_t calc_crc(const char* local_path);
uint32_t calc_crc(const char* local_path, uint32_t offset, uint32_t length);
struct MemoryStruct {
	char *memory;
	size_t size;
};

size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
int SplitFiles(const char* file_name, uint32_t block_size, char* files);
#endif //__KNETWORK_UTIL_H__