#ifndef __TASK_OBJECT_H__
#define __TASK_OBJECT_H__

#include <curl/curl.h>
#include <vector>
#include <list>
#include "KNetworkUtil.h"
//#include "FileManager.h"
class FileManager;
class ConfigManager;
class RequestManager;

struct RequestItem
{
	int start;
	int length;
};

struct pFileObj
{
	char* whole_path;
	char* name;
};

struct pFileObj_v2
{
	char* name;
	uint32_t crc32;
	uint32_t offset;
	uint32_t length;
};

class TaskObject
{
public:
	TaskObject();
	~TaskObject();
	TaskStatus initDL(const char* remote_path, const char* local_path);
	TaskStatus initUL(const char* remote_path, const char* local_path);
	TaskStatus initUL_v2(const char* remote_path, const char* local_path);
	
	void Finalize();
	//set abbr
	void setTaskStatus(TaskStatus ts) { task_status = ts; }
	CURLcode Merge();
	bool CheckSum(const char* local_path, const char* file_name);
	bool CheckSum_v2(pFileObj_v2& pfile);
	int getDownloadStatus(int64_t& now, int64_t& total);
	int getUploadStatus(int64_t& now, int64_t& total);
	//get abbr
	RequestManager* getRequestManager() { return prequest_manager; }
	ConfigManager* getConfigManager() { return pconfig_manager; }
	FileManager* getFileManager(){ return pfile_manager; }
	TaskStatus getTaskStatus() { return task_status; }
	const int getUploadFileSize() { return vUploadFiles.size(); }
	const char* getUploadFiles(const int index) { return vUploadFiles[index].whole_path; }
	const char* getFileNames(const int index) { return vUploadFiles[index].name; }
	int64_t getTotalSize(){ return total_size;}
	int64_t getXferedSize() { return xfered_size; }
	int DeleteTempFile(const int index);
	int DeleteAllTempFile();
	int DeleteConfigFile();
	const int hasSplitted() { return nSplit; }
	//int RunTask();
private:
	static int RunThread(void* argv);
	//calc the content size
	int getContentSize(const char* pRequest);
	unsigned int getHashCode(const char* local_path);
	//split upload file into small pieces
	int splitTask(const char* local_path);
	int splitTask_v2(const char* local_path);
public:
	//CURLM* curlm;
private:
	//split the task into request items, and record the unfinished item.
//	std::list<RequestItem> free_request_list;
	RequestManager* prequest_manager;
	FileManager* pfile_manager;
	ConfigManager* pconfig_manager;
	TaskStatus task_status;
	int nSplit;
	std::vector<pFileObj> vUploadFiles;
	int64_t xfered_size;
	int64_t total_size;
	std::vector<pFileObj_v2> vUploadFiles_v2;
};

#endif //__TASK_OBJECT_H__