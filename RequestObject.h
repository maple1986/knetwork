#ifndef __REQUEST_OBJECT_H__
#define __REQUEST_OBJECT_H__
#include <stdio.h>
#include <curl/curl.h>
//for test only
class ConfigManager;

struct myprogress {
	myprogress(): lastruntime(0), dltotal(0), dlnow(0), ulnow(0), ultotal(0), curl(NULL)
	{

	}
	CURL *curl;
	double lastruntime;
	int dltotal; 
	int dlnow;
	int ultotal;
	int ulnow;

};

class RequestObject
{
public:
	RequestObject();
	~RequestObject();

	int Initialize();
	int Finalize();
	
	inline CURL* getCurlHandle(){return single_curl;}
//	int Initialize();
	int DownloadHandle(FILE* f, const char * remotepath, const int offset, const int length);
	int UploadHandle(const char* remote_path, const char* local_path, const char* file_name);
	int UploadHandle(const char* remote_path, const char* local_path, const char* file_name, const int offset, const int length );
	
	int test();

	myprogress* getProgress();
	const char* getFilepath() {return m_file_path; }
	const char* getFilename() {return m_file_name; }
private:
	char* m_file_path;
	char* m_file_name;
	myprogress* myp;
	CURL* single_curl;
};

#endif //__REQUEST_OBJECT_H__