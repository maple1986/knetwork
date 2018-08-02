#ifndef __FILE_MANAGER_H__
#define __FILE_MANAGER_H__

#include <stdio.h>

class FileManager
{
public:
	FileManager();
	virtual ~FileManager();
	int Initialize(const char* local_name);
	FILE* getFileHandle() {return f;}
	int SaveFile(const char* pbuf, size_t offset, size_t length);
	int OpenFile(const char* local_name);
	int CloseFile();

private:
	FILE* f;
};


#endif //__FILE_MANAGER_H__