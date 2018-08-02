#include <memory.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <assert.h>

#include "KNetworkUtil.h"
#include "ConfigManager.h"
//#include "FileManager.h"

#define CONFIG_DL_FILE_SUFFIX ".dl.cfg"
#define CONFIG_UL_FILE_SUFFIX ".ul.cfg"

ConfigManager::ConfigManager(): total_size(0), xfered_size(0), url_length(0), pURL(NULL), pConfig(NULL), pLocal(NULL),pFileName(NULL), f(NULL)
{

}

ConfigManager::~ConfigManager()
{
	Finalize();
}

void ConfigManager::Finalize()
{
	if ( NULL != pURL )
	{
		delete[] pURL;
		pURL = NULL;
	}
	if ( NULL != pLocal )
	{
		delete[] pLocal;
		pLocal = NULL;
	}
	if ( NULL != pConfig )
	{
		delete[] pConfig;
		pConfig = NULL;
	}
//pFileName does not allocate new memory space, it is pointer to the pLocal string.
// 	if ( NULL != pFileName )
// 	{
// 		delete[] pFileName;
// 		pFileName = NULL;
// 	}
	if ( NULL != f )
	{
		fclose(f);
		f = NULL;
	}
}
/*
int ConfigManager::CheckDLStatus(const char* remote_path, const char* local_path)
{
	if ( NULL == remote_path || NULL == local_path )
	{
		return -1;
	}
	size_t temp = strlen(local_path) + strlen(CONFIG_DL_FILE_SUFFIX);
	char* pbuff = new char[temp+1];
	memset( pConfig, 0, temp+1);
//	sprintf( pConfig, "%s%s", local_path, CONFIG_DL_FILE_SUFFIX);
	if ( pbuff )
	{
		strcat( pbuff, pConfig);
		strcat( pbuff, CONFIG_DL_FILE_SUFFIX);
	}

	struct stat file_info;
	size_t local_file_len;
	if(stat(pbuff, &file_info) == 0) 
	{
		local_file_len =  file_info.st_size;
		if ( 0 == local_file_len )
		{
			return -1;
		}
	}

	FILE* f_temp = fopen(pbuff, "rb+");
	if ( NULL == f_temp )
	{
		return -1;
	}
	char* pfbuff = new char[local_file_len];
	fread( pfbuff, 1, local_file_len, f_temp);
	int rt = 0;
	if ( 0 != strcmp(pbuff, remote_path))
	{
		rt = -1;
	}
	fclose(f_temp);
	delete[] pbuff;
	pbuff = NULL;
	return rt;
}

int ConfigManager::CheckULStatus(const char* remote_path, const char* local_path)
{
	if ( NULL == remote_path || NULL ==local_path )
	{
		return -1;
	}
	struct stat file_info;
	size_t local_file_len;
	if(stat(local_path, &file_info) == 0) 
	{
		return 0;
	}
	return -1;
}
*/
int ConfigManager::SaveULConfig()
{
	return 0;
}

int ConfigManager::LoadULConfig()
{
	return 0;
}

TaskStatus ConfigManager::initDL(const char* url, const char* local_name, const int size)
{
	if (NULL == url || NULL == local_name || 0 == size)
	{
		return TASK_STATUS_ERROR;
	}
	// set config properties, and check if the task exist.
	total_size = size;

	size_t temp = strlen(local_name) + strlen(CONFIG_DL_FILE_SUFFIX);
	pConfig = new char[temp + 1];
	memset(pConfig, 0, temp + 1);
	if (pConfig)
	{
		strcat(pConfig, local_name);
		strcat(pConfig, CONFIG_DL_FILE_SUFFIX);
	}

	temp =strlen(local_name);
	pLocal = new char[temp+ 1];
	memset(pLocal, 0, temp+1);
	memcpy(pLocal, local_name, temp);

	url_length = strlen(url);
	pURL = new char[url_length+1];
	memset(pURL, 0, url_length+1);
	memcpy(pURL, url, url_length);

	xfered_size = isExist();
	if ( xfered_size )
	{
		if (xfered_size == total_size)
		{
			return TASK_STATUS_FINISHED;
		}
		return TASK_STATUS_PAUSED;
	}
	SaveDLConfig();
	return TASK_STATUS_OK;
}

TaskStatus ConfigManager::initUL(const char* url, const char* local_name)
{
	if (NULL == url || NULL == local_name)
	{
		return TASK_STATUS_ERROR;
	}
	// set config name, local name and url
	size_t temp = strlen(local_name) + strlen(CONFIG_UL_FILE_SUFFIX);
	pConfig = new char[temp+1];
	memset( pConfig, 0, temp+1);
	if (pConfig)
	{
		strcat(pConfig, local_name);
		strcat(pConfig, CONFIG_UL_FILE_SUFFIX);
	}

	temp =strlen(local_name);
	pLocal = new char[temp+1];
	memset( pLocal, 0, temp+1);
	memcpy( pLocal, local_name, temp);

	int pos = find_last_of(pLocal, '\\');
	if ( -1 != pos )
	{
		pFileName = pLocal + pos + 1;
	}
	url_length = strlen(url);
	pURL = new char[url_length+1];
	memset( pURL, 0, url_length+1);
	memcpy( pURL, url, url_length);

	struct stat file_info;

	if(stat(pLocal, &file_info) == 0) 
	{
		total_size =  file_info.st_size;
	}
	//SaveDLConfig();
	return TASK_STATUS_OK;
}

void ConfigManager::test(const int n)
{

}

int ConfigManager::isExist()
{
	if ( NULL == pConfig ||  NULL == pLocal )
	{
		printf("Invalid Task or has not initialized yet!\n");
		return 0;
	}

	if( (_access(pConfig, 0 )) != -1 && (_access(pLocal, 0 )) != -1 )
	{
		if( -1 == OpenConfig())
		{
			return 0;
		}

		struct stat file_info;
		size_t local_file_len;
		if(stat(pConfig, &file_info) == 0) 
		{
			local_file_len =  file_info.st_size;
		}
		if ( local_file_len == url_length)
		{
			char* ptemp = new char[local_file_len+1];
			memset( ptemp, 0, local_file_len+1);
			fread(ptemp, 1, local_file_len, f);
			if(0 == strncmp( pURL, ptemp, url_length)) 
			{
				if(stat(pLocal, &file_info) == 0) 
				{
					return file_info.st_size;
				}
			}
		}
	}
	
	return 0;
}

int ConfigManager::LoadDLConfig()
{
// 	struct stat file_info;
// 	size_t local_file_len;
// 	if(stat(pConfig, &file_info) == 0) 
// 	{
// 		local_file_len =  file_info.st_size;
// 		if ( 0 == local_file_len )
// 		{
// 			return -1;
// 		}
// 	}
	return 0;
}

int ConfigManager::SaveDLConfig()
{	
	OpenConfig();
	fwrite(pURL, 1, url_length, f);
	CloseConfig();
	return 0;
}

int ConfigManager::OpenConfig()
{
	if ( NULL == f)
	{
		if (_access(pConfig, 0 ) != -1)
		{
			f = fopen(pConfig, "rb+");
		}
		else
		{
			f = fopen(pConfig, "wb+");
		}
		if ( NULL == f )
		{
			return -1;
		}
	}
	//if the config has been opened, do nothing.
	fseek( f, 0, SEEK_SET);
	return 0;
}

int ConfigManager::CloseConfig()
{
	if ( NULL != f )
	{
		fclose(f);
		f = NULL;
	}
	return 0;
}
