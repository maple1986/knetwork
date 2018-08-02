#ifndef __CONFIG_MANAGER_H__
#define __CONFIG_MANAGER_H__

#include <stdio.h>
#include "KNetworkUtil.h"
#define BLOCK_SIZE  1024
#define THREAD_COUNT 5
#define MAX_FILE_SIZE (1*1024*1024*1024)

class ConfigManager
{
public:
	ConfigManager();
	~ConfigManager();
	//inline bool isSameTask(const char*& url){ return strUrl == url;}
//	int CheckDLStatus(const char* remote_path, const char* local_path);
//	int CheckULStatus(const char* remote_path, const char* local_path);
	
	TaskStatus initDL(const char* url, const char* local_name, const int size);
	TaskStatus initUL(const char* url, const char* local_name);
	
	int SaveDLConfig();
	int LoadDLConfig();

	int SaveULConfig();
	int LoadULConfig();
	
	inline void setUrl(char* ch) { pURL = ch; }
	inline void setLocal(char* ch) { pLocal = ch; }
	inline const char* getUrl() { return pURL;}
	inline const char* getLocal() { return pLocal; }
	inline const char* getConfig() { return pConfig; }
	inline const char* getFileName() { return pFileName; }
	inline const size_t getXferedLength() {return xfered_size;}
	inline const size_t getTotalLength() {return total_size;}
	//test bit set
	void test(const int n);
	void Finalize();
private:
	int OpenConfig();
	int CloseConfig();
	//return  = 0, means not exist or xferred length = 0
	// > 0, means the xferred length.
	int isExist();
private:
	size_t total_size;
	size_t xfered_size;
	size_t url_length;
	
	char* pURL;
	char* pConfig;
	char* pLocal;
	char* pFileName;

	FILE* f;
};

#endif //__CONFIG_MANAGER_H__