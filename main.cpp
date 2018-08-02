#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <curl/curl.h>
#include <sys/types.h>
#include "UploadManager.h"
#include "DownloadManager.h"
#include "ConfigManager.h"
#include <thread>
#include <klib.h>
#include <kcl.h>
//#include "FileManager.h"
//#include "RequestObject.h"
//ignore
//#define STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES         6000
#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     3


//Use buffer
struct MemoryStruct {
	char *memory;
	size_t size;
	size_t offset;
	FILE* f;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	void *new_ptr = realloc(mem->memory, mem->size + realsize + 1);
	if(new_ptr == NULL) {
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}
	mem->memory = (char *)new_ptr;

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}


long get_contentsize(const char* remotepath)
{
	CURL* curl_local = curl_easy_init();
	curl_easy_setopt(curl_local, CURLOPT_URL, remotepath);
	curl_easy_setopt(curl_local, CURLOPT_HEADER, 1);
	curl_easy_setopt(curl_local, CURLOPT_NOBODY, 1);
	CURLcode r = curl_easy_perform(curl_local);
	if ( r != CURLE_OK)
	{
		return 0;
	}
	double len = 0;
	curl_easy_getinfo(curl_local, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &len);
	curl_easy_cleanup(curl_local);

	return len;
}

size_t caculate_size(const size_t data_size, int& block_count_single_thread)
{
	if ( data_size == 0 || data_size > MAX_FILE_SIZE)
	{
		printf("Can't support the data size = %d!\n", data_size);
		return 0;
	}
	if (data_size <= (THREAD_COUNT * BLOCK_SIZE))
	{
		block_count_single_thread = data_size;
		return 1;
	}
	else
	{
		block_count_single_thread = data_size / (THREAD_COUNT * BLOCK_SIZE);
		return THREAD_COUNT;
	}
}
/*
bool init_download_handle( myprogress& prog, CURL* curl_local, FILE* f, const char * remotepath,
	const int offset, const int length)
{
	if (f == NULL) {
		perror(NULL);
		return 0;
	}
	CURLcode r = curl_easy_setopt(curl_local, CURLOPT_URL, remotepath);
	//set the download range, e.g 100-200, if to the end of file use xxx-,e.g 200-
	char range[128];
	memset( range, 0, 128);
	if ( length > 0 )
	{
		sprintf(range, "%d-%d", offset, offset+length-1);
	}
	else
	{
		sprintf(range, "%d-", offset);
	}
	fseek(f, offset, SEEK_SET);
	curl_easy_setopt(curl_local, CURLOPT_RANGE, range);
	curl_easy_setopt(curl_local, CURLOPT_WRITEDATA, f);
	//curl_easy_setopt(curl_local, CURLOPT_WRITEFUNCTION, wirtefunc);

	curl_easy_setopt(curl_local, CURLOPT_XFERINFODATA, &prog);
	curl_easy_setopt(curl_local, CURLOPT_XFERINFOFUNCTION, xferinfo);
	curl_easy_setopt(curl_local, CURLOPT_HEADER, 0L);
	curl_easy_setopt(curl_local, CURLOPT_NOBODY, 0L);
	curl_easy_setopt(curl_local, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl_local, CURLOPT_VERBOSE, 1L);

	return true;
}
*/
int test()
{
	const char p[] = "111111111";
	const char q[] = "111111111";
	ConfigManager cfg;
	cfg.initDL( p, q, 128);
// 	cfg.setStatus(8);
// 
// 	int rs = cfg.CheckStatus(8);
	cfg.test(128);
	return 0;
}


int main(int argc, char **argv)
{
	//hard coded, test purpose.
	//const char* remote_path = "http://192.168.0.141:8000/server/boost_1_60_0.zip";
	//const char* local_path = "f:\\local\\123";

	BSTR remote_path = KSysAllocUTF8String("http://192.168.0.141:8000/server/");
	BSTR local_path = KSysAllocUTF8String("f:/local/123");
	
	//KComBSTR kbstr(remote_path);
	//char szBuf[128] = {0};
	//strcpy(szBuf, (LPCSTR)kbstr);
	//printf("szBuf=[%s]\n", szBuf);

	


	//Curl initialize
	curl_global_init(CURL_GLOBAL_ALL);
	UploadManager um;
	um.Initialize(0);
	printf("um.Initialize finished!\n");
	um.Add(&remote_path, &local_path);
	
  	DownloadManager dm;
  	dm.Initialize(0);
  	dm.Add(&remote_path, &local_path);
// 	dm.Pause(0);
// 	dm.Start(0);
	curl_global_cleanup();
	return 0;
// 	//size_t content_length = get_contentsize(remote_path);
// 	//ConfigManager init, split the task. 
// 	//ConfigManager cm;
// 	cm.Initialize(remote_path, local_path, content_length);
// 	int total_block = cm.getBlockTotal();
// 	int each_block = cm.getBlockEach();
// 	cm.PrintConfig();
// 	//FileManager init
// 	FileManager  fm;
// 	fm.Initialize(local_path, content_length);
// 	FILE* f = fm.getFileHandle();
// 	
// 	//DownloadManager init
// 	DownloadManager dm;
// 	dm.Initialize();
// 	CURLM * curl_multi = dm.getTaskHandle();
// 	
// 	int Request_num = 0;
// 	RequestManager* requests;
// 	if ( total_block == each_block)
// 	{
// 		requests = new RequestManager[1];
// 		Request_num = 1;
// 	}
// 	else
// 	{
// 		requests = new RequestManager[THREAD_COUNT];
// 		Request_num = THREAD_COUNT;
// 	}
// 	for( int i = 0; i < Request_num; ++i)
// 	{
// 		//requests[i] = curl_easy_init();
// 		
// 		int length = each_block * BLOCK_SIZE;
// 		if ( Request_num - 1 == i)
// 		{
// 			length = content_length - each_block * BLOCK_SIZE * i;
// 		}
// 		requests[i].Initialize( f, remote_path, each_block * BLOCK_SIZE * i, length );
// 		//int res = init_download_handle( prog[i], curl_handles[i], f, remote_path, each_block * BLOCK_SIZE * i, length);
// 		requests[i].myp.cm = &cm;
// 		curl_multi_add_handle( curl_multi, requests[i].getCurlHandle());
// 	}
// 	
// 	dm.AllStart();
// 	dm.AllPause();
// 	dm.AllResume();
// 
// 	cm.SaveConfig();
// 	cm.CloseConfig();
// 	fm.CloseFile();
// 
// 	curl_multi_cleanup(curl_multi);
// 	for( int i = 0; i < Request_num; ++i)
// 	{
// 		requests[i].EndRequest();
// 	}

}