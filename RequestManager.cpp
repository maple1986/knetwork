#include "RequestManager.h"
#include "RequestObject.h"
#include "ConfigManager.h"
//#include "FileManager.h"

RequestManager::RequestManager()
{

}

RequestManager::~RequestManager()
{
}


int RequestManager::Initialize(const int request_size)
{
	return 0;
// 	pFileManager = new FileManager;
// 	pConfigManager = new ConfigManager;
// 
// 	size_t content_length = getContentSize(remote_path);
// 
// 	pConfigManager->Initialize( remote_path, local_path, content_length);
// 	pFileManager->Initialize( local_path, content_length);
// 
// 	int block_total = pConfigManager->getBlockTotal();
// 	int block_each  = pConfigManager->getBlockEach();
// 	if ( block_total == block_each)
// 	{
// 	 	pRequests = new RequestObject[1];
//  		request_count = 1;
//  	}
//  	else
//  	{
// 		pRequests = new RequestObject[THREAD_COUNT];
//  		request_count = THREAD_COUNT;
//  	}
// 	int length = block_each * BLOCK_SIZE;
// 	for (int i = 0; i < request_count; ++i)
// 	{
// 		if ( request_count - 1 == i)
// 		{
// 			length = content_length - block_each * BLOCK_SIZE * i;
// 		}
// 		pRequests[i].Initialize( pFileManager->getFileHandle(), remote_path, block_each * BLOCK_SIZE * i, length );
// 		//int res = init_download_handle( prog[i], curl_handles[i], f, remote_path, each_block * BLOCK_SIZE * i, length);
// 		pRequests[i].myp.cm = pConfigManager;
// 	}
// 
// 	return 0;
}

int RequestManager::assignDownload( const char* remote_path, const size_t offset, const size_t length, FILE* f )
{
	RequestObject* p_obj = new RequestObject;
	request_object_list.push_back(p_obj);
	request_object_list.back()->DownloadHandle(f, remote_path, offset, length);
	return 0;
}

int RequestManager::assignUpload( const char* remote_path, const char* local_path, const char* file_name)
{
	RequestObject* p_obj = new RequestObject;
	request_object_list.push_back( p_obj );
	request_object_list.back()->UploadHandle(remote_path, local_path, file_name);

	return 0;
}

int RequestManager::assignUpload( const char* remote_path, const char* local_path, const char* file_name, const int offset, const int length)
{
	RequestObject* p_obj = new RequestObject;
	request_object_list.push_back( p_obj );
	request_object_list.back()->UploadHandle(remote_path, local_path, file_name, offset, length);

	return 0;
}

int RequestManager::Finalize()
{
	if (!request_object_list.empty())
	{
		for ( int i = 0; i < request_object_list.size(); ++i)
		{
			delete request_object_list[i];
			request_object_list[i] = NULL;
		}
		request_object_list.clear();
	}
 	return 0;
}

