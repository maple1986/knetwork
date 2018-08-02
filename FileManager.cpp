#include "FileManager.h"
#include <sys/stat.h>
#include  <io.h>

FileManager::FileManager():f(NULL)
{
}

FileManager::~FileManager()
{
	printf("FileManager closeFile!!!\n");
	CloseFile();
}

int FileManager::Initialize(const char* local_name)
{
	if ( -1 == OpenFile(local_name))
	{
		return -1;
	}

	return 0;
}

int FileManager::SaveFile(const char* pbuf, size_t offset, size_t length)
{
	if ( NULL == f)
	{
		printf("Initialize FileManager first, please\n");
		return -1;
	}
	fseek(f, offset, SEEK_SET);
	fwrite( pbuf, 1, length, f);
	CloseFile();
	return 0;
}

int FileManager::OpenFile(const char* local_name)
{
	if ( NULL == f)
	{
		if (_access(local_name, 0 ) != -1)
		{
			f = fopen(local_name, "rb+");
		}
		else
		{
			f = fopen(local_name, "wb+");
		}
		if ( NULL == f)
		{
			return -1;
		}
	}
	fseek( f, 0 , SEEK_SET);
	return 0;
}

int FileManager::CloseFile()
{
	if ( NULL != f)
	{
		fflush(f);	
		fclose(f);
		f = NULL;
	}
	return 0;
}