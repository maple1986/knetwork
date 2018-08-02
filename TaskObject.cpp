#include <sys/stat.h>
#include <stdlib.h>
#include "TaskObject.h"
#include "ConfigManager.h"
#include "FileManager.h"
#include "RequestManager.h"
#include "KNetworkUtil.h"
#define UPLOAD_BLOCK_SIZE (1*1024*1024)
#define UPLOAD_LARGE_BLOCKS_SIZE (10*1024*1024)

#define MERGE_STR "/merge?"
#define CHECKSUM_STR "/checksum?"


TaskObject::TaskObject(): prequest_manager(NULL), pfile_manager(NULL), pconfig_manager(NULL), task_status(TASK_STATUS_ERROR), xfered_size(0), total_size(0), nSplit(0)
{
//	curlm = curl_multi_init();
}

TaskObject::~TaskObject()
{
	Finalize();
}

int TaskObject::splitTask(const char* local_path)
{
	if ( NULL == local_path )
	{
		return 0;
	}
	struct stat file_info;
	if(stat(local_path, &file_info) != 0 || file_info.st_size == 0) 
	{
		return 0;
	}
	total_size = file_info.st_size;
	int nblocks = 1;
	if (file_info.st_size <= UPLOAD_BLOCK_SIZE)
	{
		int len = strlen(local_path);
		char* pPath = new char[len+1];
		memset(pPath, 0, len+1);
		memcpy(pPath, local_path, len);
		
		int pos = find_last_of( local_path, '\\');
		len = strlen(pPath) - pos;
		char* pName = new char[len+1];
		memset( pName, 0, len+1);
		memcpy( pName, local_path + pos + 1, len);
		vUploadFiles.push_back(pFileObj());
		vUploadFiles.back().whole_path = pPath;
		vUploadFiles.back().name = pName;
	}
	if (file_info.st_size > UPLOAD_LARGE_BLOCKS_SIZE)
	{
		nblocks = file_info.st_size/UPLOAD_LARGE_BLOCKS_SIZE;
		if (file_info.st_size%UPLOAD_LARGE_BLOCKS_SIZE)
		{
			nblocks+=1;
		}
		FILE* f_origin = fopen(local_path, "rb+");
		if ( f_origin == NULL )
		{
			return 0;
		}
		char* pbuf =new char[UPLOAD_LARGE_BLOCKS_SIZE];
		if ( !pbuf )
		{
			return 0;
		}
		for ( int i = 0; i < nblocks; ++i)
		{
			int length = UPLOAD_LARGE_BLOCKS_SIZE;
			if ( i == nblocks - 1)
			{
				length = file_info.st_size - UPLOAD_LARGE_BLOCKS_SIZE * i;
			}
			memset( pbuf, 0, length);
			fread( pbuf, 1, length, f_origin);
			char* pTemp = new char [256];
			memset( pTemp, 0, 256);
			sprintf( pTemp, "%s.%d", local_path, i);
			FILE* f_split = fopen(pTemp, "wb+");
			if ( f_split == NULL)
			{
				delete pbuf;
				pbuf = NULL;
				fclose(f_origin);
				return 0;
			}
			fwrite( pbuf, 1, length, f_split );
			fflush(f_split);
			fclose(f_split);

			int pos = find_last_of( pTemp, '\\');
			int len = strlen(pTemp) - pos;
			char* pName = new char[len+1];
			memset( pName, 0, len+1);
			memcpy( pName, pTemp + pos + 1, len);
			vUploadFiles.push_back(pFileObj());
			vUploadFiles.back().whole_path = pTemp;
			vUploadFiles.back().name = pName;
		}
		delete pbuf;
		pbuf = NULL;
		fclose(f_origin);
	}
	else if(file_info.st_size > UPLOAD_BLOCK_SIZE)
	{
		nblocks = file_info.st_size/UPLOAD_BLOCK_SIZE;
		if (file_info.st_size%UPLOAD_BLOCK_SIZE)
		{
			nblocks+=1;
		}
		FILE* f_origin = fopen(local_path, "rb+");
		if ( f_origin == NULL )
		{
			return 0;
		}
		char* pbuf =new char[UPLOAD_BLOCK_SIZE];
		for ( int i = 0; i < nblocks; ++i)
		{
			int length = UPLOAD_BLOCK_SIZE;
			if ( i == nblocks - 1)
			{
				length = file_info.st_size - UPLOAD_BLOCK_SIZE * i;
			}
			memset( pbuf, 0, length);
			fread( pbuf, 1, length, f_origin);
			char* pTemp = new char[MAX_PATH];
			memset(pTemp, 0, MAX_PATH);
			sprintf( pTemp, "%s.%d", local_path, i);
			FILE* f_split = fopen(pTemp, "wb+");
			fwrite( pbuf, 1, length, f_split );
			fflush(f_split);
			fclose(f_split);
			
			int pos = find_last_of( pTemp, '\\');
			int len = strlen(pTemp) - pos;
			char* pName = new char[len+1];
			memset( pName, 0, len+1);
			memcpy( pName, pTemp + pos + 1, len);
			vUploadFiles.push_back(pFileObj());
			vUploadFiles.back().whole_path = pTemp;
			vUploadFiles.back().name = pName;
		}
		delete pbuf;
		pbuf = NULL;
		fclose(f_origin);
	}
	return nblocks;
}

int TaskObject::splitTask_v2(const char* local_path)
{
	if ( NULL == local_path )
	{
		return 0;
	}
	struct stat file_info;
	if(stat(local_path, &file_info) != 0 || file_info.st_size == 0) 
	{
		return 0;
	}
	total_size = file_info.st_size;
	int nblocks = 1;
	if (file_info.st_size <= UPLOAD_BLOCK_SIZE)
	{
		int len = strlen(local_path);
		char* pPath = new char[len+1];
		memset(pPath, 0, len+1);
		memcpy(pPath, local_path, len);
		
		int pos = find_last_of( local_path, '\\');
		len = strlen(pPath) - pos;
		char* pName = new char[len+1];
		memset( pName, 0, len+1);
		memcpy( pName, local_path + pos + 1, len);
		pFileObj_v2 pfile;
		pfile.name = pName;
		pfile.offset = 0;
		pfile.length = file_info.st_size;
		pfile.crc32 = calc_crc(local_path, pfile.offset, pfile.length);
		vUploadFiles_v2.push_back(pfile);
	}
	if (file_info.st_size > UPLOAD_LARGE_BLOCKS_SIZE)
	{
		nblocks = file_info.st_size/UPLOAD_LARGE_BLOCKS_SIZE;
		if (file_info.st_size%UPLOAD_LARGE_BLOCKS_SIZE)
		{
			nblocks+=1;
		}
		for ( int i = 0; i < nblocks; ++i)
		{
			int length = UPLOAD_LARGE_BLOCKS_SIZE;
			if ( i == nblocks - 1)
			{
				length = file_info.st_size - UPLOAD_LARGE_BLOCKS_SIZE * i;
			}
			char* pTemp = new char [256];
			memset( pTemp, 0, 256);
			sprintf( pTemp, "%s.%d", local_path, i);
			int pos = find_last_of( pTemp, '\\');
			int len = strlen(pTemp) - pos;
			char* pName = new char[len+1];
			memset( pName, 0, len+1);
			memcpy( pName, pTemp + pos + 1, len);
			delete[] pTemp;
			pFileObj_v2 pfile;
			pfile.name = pName;
			pfile.offset = i*UPLOAD_LARGE_BLOCKS_SIZE;
			pfile.length = file_info.st_size;
			pfile.crc32 = calc_crc(local_path, pfile.offset, pfile.length);
			vUploadFiles_v2.push_back(pfile);
		}
	}
	else if(file_info.st_size > UPLOAD_BLOCK_SIZE)
	{
		nblocks = file_info.st_size/UPLOAD_BLOCK_SIZE;
		if (file_info.st_size%UPLOAD_BLOCK_SIZE)
		{
			nblocks+=1;
		}
		for ( int i = 0; i < nblocks; ++i)
		{
			int length = UPLOAD_BLOCK_SIZE;
			if ( i == nblocks - 1)
			{
				length = file_info.st_size - UPLOAD_BLOCK_SIZE * i;
			}
			char* pTemp = new char [256];
			memset( pTemp, 0, 256);
			sprintf( pTemp, "%s.%d", local_path, i);
			int pos = find_last_of( pTemp, '\\');
			int len = strlen(pTemp) - pos;
			char* pName = new char[len+1];
			memset( pName, 0, len+1);
			memcpy( pName, pTemp + pos + 1, len);
			delete[] pTemp;
			pFileObj_v2 pfile;
			pfile.name = pName;
			pfile.offset = i*UPLOAD_BLOCK_SIZE;
			pfile.length = file_info.st_size;
			pfile.crc32 = calc_crc(local_path, pfile.offset, pfile.length);
			vUploadFiles_v2.push_back(pfile);
		}
	}
	return nblocks;
}

int TaskObject::getContentSize(const char* pRequest)
{
	CURL* curl_local = curl_easy_init();
	curl_easy_setopt(curl_local, CURLOPT_URL, pRequest);
	curl_easy_setopt(curl_local, CURLOPT_HEADER, 1);
	curl_easy_setopt(curl_local, CURLOPT_NOBODY, 1);
	curl_easy_setopt(curl_local, CURLOPT_TIMEOUT, 5L);
	CURLcode r = curl_easy_perform(curl_local);
	int http_code = 0;
	curl_easy_getinfo (curl_local, CURLINFO_RESPONSE_CODE, &http_code);
	if ( r != CURLE_OK || http_code != 200 )
	{
		return 0;
	}
	double len = 0;
	curl_easy_getinfo(curl_local, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &len);
	curl_easy_cleanup(curl_local);

	return len;
};

TaskStatus TaskObject::initDL(const char* remote_path, const char* local_path)
{
	pfile_manager = new FileManager;
	pconfig_manager = new ConfigManager;
	prequest_manager = new RequestManager;

	total_size = getContentSize(remote_path);
	if (0 == total_size)
	{
		return TASK_STATUS_ERROR;
	}
	task_status = pconfig_manager->initDL( remote_path, local_path, total_size);
	if ( TASK_STATUS_ERROR == task_status )
	{
		return task_status;
	}
	else if ( TASK_STATUS_FINISHED == task_status )
	{
		return task_status;
	}
	size_t offset = 0; 
	if ( TASK_STATUS_PAUSED == task_status )
	{
		offset = pconfig_manager->getXferedLength();
		xfered_size = offset;
	}
	pfile_manager->Initialize(local_path);

	prequest_manager->assignDownload(remote_path, offset,  0,  pfile_manager->getFileHandle());

	return TASK_STATUS_OK;
}

void TaskObject::Finalize()
{
	printf("TaskObject::Finalize In\n");
	if (pfile_manager)
	{
		delete pfile_manager;
		pfile_manager = NULL;
	}

	if (pconfig_manager)
	{
		delete pconfig_manager;
		pconfig_manager = NULL;
	}

	if (prequest_manager)
	{
		delete prequest_manager;
		prequest_manager = NULL;
	}

	if ( !vUploadFiles.empty() )
	{
		for ( int i = 0; i < vUploadFiles.size(); ++i)
		{
			delete[] vUploadFiles[i].whole_path;
			delete[] vUploadFiles[i].name;
		}
		vUploadFiles.clear();
	}
	return;
}

TaskStatus TaskObject::initUL(const char* remote_path, const char* local_path)
{
	pconfig_manager = new ConfigManager;
	prequest_manager = new RequestManager;
	pfile_manager = new FileManager;
	task_status = pconfig_manager->initUL( remote_path, local_path);
	total_size = pconfig_manager->getTotalLength();
	if ( CheckSum(pconfig_manager->getLocal(), pconfig_manager->getFileName()))
	{
		xfered_size = total_size;
		task_status = TASK_STATUS_FINISHED;
		return task_status;
	}
	int nblocks = splitTask(local_path);
	if( 0 == nblocks )
	{
		task_status = TASK_STATUS_ERROR;
		return task_status;
	}
	if ( nblocks > 1)
	{
		nSplit = nblocks;
	}
	task_status = TASK_STATUS_FINISHED;
	for ( int i = 0; i < nblocks; ++i )
	{
		if (!CheckSum(vUploadFiles[i].whole_path, vUploadFiles[i].name))
		{
			prequest_manager->assignUpload(remote_path, vUploadFiles[i].whole_path, vUploadFiles[i].name);
			task_status = TASK_STATUS_OK;
		}
		else
		{
			struct stat file_info;
			if(stat(vUploadFiles[i].whole_path, &file_info) == 0) 
			{
				xfered_size +=  file_info.st_size;
			}
			DeleteTempFile(i);
		}
	}
	if (xfered_size == total_size)
	{
		Merge();
		task_status =TASK_STATUS_FINISHED;
	}
	return task_status;
}

TaskStatus TaskObject::initUL_v2(const char* remote_path, const char* local_path)
{
	pconfig_manager = new ConfigManager;
	prequest_manager = new RequestManager;
	pfile_manager = new FileManager;
	task_status = pconfig_manager->initUL( remote_path, local_path);
	total_size = pconfig_manager->getTotalLength();
	if ( CheckSum(pconfig_manager->getLocal(), pconfig_manager->getFileName()))
	{
		xfered_size = total_size;
		task_status = TASK_STATUS_FINISHED;
		return task_status;
	}
	int nblocks = splitTask_v2(local_path);
	if( 0 == nblocks )
	{
		task_status = TASK_STATUS_ERROR;
		return task_status;
	}
	if ( nblocks > 1)
	{
		nSplit = nblocks;
	}
	task_status = TASK_STATUS_FINISHED;
	for ( int i = 0; i < nblocks; ++i )
	{
		if (!CheckSum_v2(vUploadFiles_v2[i]))
		{
			prequest_manager->assignUpload(remote_path, local_path, vUploadFiles_v2[i].name, vUploadFiles_v2[i].offset, vUploadFiles_v2[i].length);
			task_status = TASK_STATUS_OK;
		}
		else
		{
			xfered_size += vUploadFiles_v2[i].length;
		}
	}
	if (xfered_size == total_size)
	{
		Merge();
		task_status =TASK_STATUS_FINISHED;
	}
	return task_status;
}

//check sum
uint32_t TaskObject::getHashCode(const char* local_path)
{
	return calc_crc(local_path);
}

bool TaskObject::CheckSum(const char* local_path, const char* file_name)
{
	//RequestObject* pR_obj = prequest_manager->getRequestObj(index);
	unsigned int crc = getHashCode( local_path );
	CURL* curl = curl_easy_init();
	char pCheckSum[4098] = {0};
	//memset( pCheckSum, 0, 4098);
	struct stat file_info;
	//size_t file_len;
	if(stat(local_path, &file_info) != 0 ) 
	{
		return false;
	}
	struct MemoryStruct chunk;

	chunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */ 
	chunk.size = 0;    /* no data at this point */ 

	char* pEncode = curl_easy_escape( curl, file_name, strlen(file_name));
	sprintf( pCheckSum, "%s%sfilename=%s&size=%u&crc32=%u", pconfig_manager->getUrl(), CHECKSUM_STR, pEncode, file_info.st_size , crc );
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L); 
	curl_easy_setopt(curl, CURLOPT_URL, pCheckSum);
	/* send all data to this function  */ 
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

	/* we pass our 'chunk' struct to the callback function */ 
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
	CURLcode rc = curl_easy_perform(curl);
	bool rt = false;
	if ( 0 == strcmp(chunk.memory, "true") )
	{
		rt = true;
	}
	curl_free(pEncode);
	curl_easy_cleanup(curl);
	return rt;
}

bool TaskObject::CheckSum_v2(pFileObj_v2& pfile)
{
	//RequestObject* pR_obj = prequest_manager->getRequestObj(index);
	CURL* curl = curl_easy_init();
	char pCheckSum[4098] = {0};
	//memset( pCheckSum, 0, 4098);
	struct MemoryStruct chunk;

	chunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */ 
	chunk.size = 0;    /* no data at this point */ 

	char* pEncode = curl_easy_escape( curl, pfile.name, strlen(pfile.name));
	sprintf( pCheckSum, "%s%sfilename=%s&size=%u&crc32=%u", pconfig_manager->getUrl(), CHECKSUM_STR, pEncode, pfile.length, pfile.crc32 );
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L); 
	curl_easy_setopt(curl, CURLOPT_URL, pCheckSum);
	/* send all data to this function  */ 
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

	/* we pass our 'chunk' struct to the callback function */ 
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
	CURLcode rc = curl_easy_perform(curl);
	bool rt = false;
	if ( 0 == strcmp(chunk.memory, "true") )
	{
		rt = true;
	}
	curl_free(pEncode);
	curl_easy_cleanup(curl);
	return rt;
}

CURLcode TaskObject::Merge()
{
	CURL* curl = curl_easy_init();
	unsigned int crc = getHashCode(pconfig_manager->getLocal());
	char pMerge[4098] = {0};
	struct stat file_info;
	//size_t file_len;
	if(stat(pconfig_manager->getLocal(), &file_info) != 0 ) 
	{
		return CURLE_AGAIN;
	}
	char* pEncode = curl_easy_escape( curl, pconfig_manager->getFileName(), strlen(pconfig_manager->getFileName()));
	sprintf( pMerge, "%s%sfilename=%s&size=%u&blocks=%u&crc32=%u", pconfig_manager->getUrl(), MERGE_STR, pEncode, file_info.st_size, nSplit, crc );
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
	curl_easy_setopt(curl, CURLOPT_URL, pMerge);
	CURLcode rc = curl_easy_perform(curl);
	curl_free(pEncode);
	curl_easy_cleanup(curl);
	return rc;
}

// int TaskObject::RunTask()
// {
// 	if ((task_status != TASK_STATUS_OK) && (task_status != TASK_STATUS_PAUSED))
// 	{
// 		return -1;
// 	}
// 	task_status = TASK_STATUS_RUNNING;
// 	//thread pth(RunThread, this);
// 	//pth.join();
// 	thread_param* tp = (thread_param*)malloc(sizeof(thread_param));
// 	if(!tp)
// 	{
// 		return FALSE;
// 	}
// 	memset(tp, 0, sizeof(thread_param));
// 	tp->callback = &RunThread;
// 	tp->param = this;
// 	printf("kgutil_create_thread win32.....call\n");
// 	HANDLE Handle = CreateThread(NULL, 0, Win32Thread, (LPVOID)tp, 0, NULL);
// 	if (!Handle)
// 	{
// 		return -1;
// 	}
// 	return 0;
// }

int TaskObject::RunThread(void* argv)
{
	// TaskObject* p = (TaskObject*)argv;
	// CURLMsg* msg;

	// int still_running = -1;
	// for (int i = 0; i < p->getRequestManager()->getRequestObjSize(); ++i)
	// {
	// 	curl_multi_add_handle(p->curlm, p->getRequestManager()->getRequestObj(i)->getCurlHandle());
	// }
	// do {
	// 	CURLMcode mc;
	// 	int numfds;

	// 	mc = curl_multi_perform(p->curlm, &still_running);
	// 	//int msgq = 0;
	// 	//msg = curl_multi_info_read(curlm, &msgq);
	// 	//TaskList::iterator it = task_obj_list.begin();
	// 	do {
	// 		int msgq = 0;
	// 		msg = curl_multi_info_read(p->curlm, &msgq);
	// 		if (msg && (msg->msg == CURLMSG_DONE)) {
	// 			CURL *e = msg->easy_handle;
	// 			//transfers--;

	// 			curl_multi_remove_handle(p->curlm, e);
	// 			//curl_easy_cleanup(e);

	// 		}
	// 	} while (msg);

	// 	if (mc == CURLM_OK) {
	// 		/* wait for activity, timeout or "nothing" */
	// 		mc = curl_multi_wait(p->curlm, NULL, 0, 1000, &numfds);
	// 	}

	// 	if (mc != CURLM_OK) {
	// 		fprintf(stderr, "curl_multi failed, code %d.n", mc);
	// 		break;
	// 	}
	// } while (still_running);
	
	// for (int i = 0; i < p->getRequestManager()->getRequestObjSize(); ++i)
	// {
	// 	if (CURLE_OK != p->CheckSum(i))
	//  	{
	// 		curl_easy_perform(p->getRequestManager()->getRequestObj(i)->getCurlHandle());
	//  	}
	// }
	// p->Merge();
	// printf("Thread out!\n");
	return 0;
}

int TaskObject::getDownloadStatus(int64_t& now, int64_t& total)
{
	now = 0;
	total = 0;
	for ( int i = 0; i < getRequestManager()->getRequestObjSize(); ++i )
	{
		myprogress* myp = getRequestManager()->getRequestObj(i)->getProgress();
		now += myp->dlnow;
		total += myp->dltotal;
	}
	return 0;
}

int TaskObject::getUploadStatus(int64_t& now, int64_t& total)
{
	for ( int i = 0; i < getRequestManager()->getRequestObjSize(); ++i )
	{
		myprogress* myp = getRequestManager()->getRequestObj(i)->getProgress();
		now += myp->ulnow;
		total += myp->ultotal;
	}
	return 0;
}

int TaskObject::DeleteTempFile(const int index)
{
	if (vUploadFiles.size() > 1)
	{
		remove(vUploadFiles[index].whole_path);
	}
	return 0;
}

int TaskObject::DeleteAllTempFile()
{
	if (vUploadFiles.size() > 1)
	{
		for ( int i = 0; i < vUploadFiles.size(); ++i)
		{
			remove(vUploadFiles[i].whole_path);
		}
	}
	return 0;
}

int TaskObject::DeleteConfigFile()
{
	if (pconfig_manager->getConfig())
	{
		remove(pconfig_manager->getConfig());
	}
	return 0;
}
