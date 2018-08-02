#ifndef __TASK_MANAGER_H__
#define __TASK_MANAGER_H__

#include "RequestObject.h"
#include "ConfigManager.h"

#include <vector>

#define MULTI_THREAD 5
class RequestManager
{
public:
	RequestManager();
	~RequestManager();

	int Initialize(const int request_size);
	int assignDownload( const char* remote_path, const size_t offset, const size_t length, FILE* f );
	int assignUpload( const char* remote_path, const char* local_path, const char* file_name);
	int assignUpload( const char* remote_path, const char* local_path, const char* file_name, const int offset, const int length);
	int Finalize();
	
	//get abbr
	int getRequestObjSize() { return request_object_list.size(); }
	RequestObject* getRequestObj(const int index) { return request_object_list[index];}
private:
	std::vector<RequestObject*> request_object_list; 
// 	FileManager* pFileManager;
// 	ConfigManager* pConfigManager;
// 	RequestObject* pRequests;
//	int request_count;
//	TaskStatus task_status;
};



#endif //__TASK_MANAGER_H__