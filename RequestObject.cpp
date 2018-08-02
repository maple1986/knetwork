#include <sys/stat.h>
#include "RequestObject.h"
#include "curl/curl.h"
#include "KNetworkUtil.h"
//#include "ConfigManager.h"
#define UPLOAD_STR "/upload?d=upload"

RequestObject::RequestObject(): single_curl(NULL), m_file_name(NULL), m_file_path(NULL)
{
	single_curl = curl_easy_init();
	myp = new myprogress;
	myp->curl = single_curl;
}

RequestObject::~RequestObject()
{
	Finalize();
}


static int xferinfo(void *p,
                    int64_t dltotal, int64_t dlnow,
                    int64_t ultotal, int64_t ulnow)
{
  myprogress *myp = (struct myprogress *)p;
  CURL *curl = myp->curl;
  double curtime = 0;
 
  curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &curtime);


  fprintf(stderr, "UP: %" CURL_FORMAT_CURL_OFF_T " of %" CURL_FORMAT_CURL_OFF_T
          "  DOWN: %" CURL_FORMAT_CURL_OFF_T " of %" CURL_FORMAT_CURL_OFF_T
          "\r\n",
          ulnow, ultotal, dlnow, dltotal);
  myp->dlnow = dlnow;
  myp->dltotal = dltotal;
  myp->ulnow = ulnow;
  myp->ultotal = ultotal;
  /*download delta size over single block size or download thread done. record it!!!*/

  return 0;
}
 
myprogress* RequestObject::getProgress()
{
	return myp; 
}

static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
  curl_off_t nread;
  /* in real-world cases, this would probably get this data differently
     as this fread() stuff is exactly what the library already would do
     by default internally */ 
  size_t retcode = fread(ptr, size, nmemb, (FILE*)stream);
 
  nread = (curl_off_t)retcode;
 
  fprintf(stderr, "*** We read %" CURL_FORMAT_CURL_OFF_T
          " bytes from file\n", nread);
  return retcode;
}

int RequestObject::UploadHandle(const char* remote_path, const char* local_path, const char* file_name, const int offset, const int length )
{
	struct curl_httppost *formpost=NULL;
	struct curl_httppost *lastptr=NULL;
	struct curl_slist *headerlist=NULL;

	struct stat file_info;
	if(stat(local_path, &file_info) != 0 || file_info.st_size == 0) 
	{
		return -1;
	}
	/* Fill in the file upload field. This makes libcurl load data from
	the given file name when curl_easy_perform() is called. */
	FILE* f = fopen(local_path, "rb");
	//char buf[256];
	//fread(buf, 1, 256, f);
	fseek(f, offset, SEEK_SET);
	curl_formadd((curl_httppost**)&formpost,
		(curl_httppost**)&lastptr,
		CURLFORM_COPYNAME, "1",
		//CURLFORM_FILE, local_path,
		CURLFORM_STREAM, f, 
		CURLFORM_CONTENTSLENGTH, length,
		CURLFORM_FILENAME, file_name,
		CURLFORM_END);
	/* Set the dest path */
	size_t temp = strlen(remote_path) + strlen(UPLOAD_STR);
	char* pbuff = new char[temp+1];
	memset( pbuff, 0, temp+1);
	//	sprintf( pConfig, "%s%s", local_path, CONFIG_DL_FILE_SUFFIX);
	if ( pbuff )
	{
		strcat( pbuff, remote_path);
		strcat( pbuff, UPLOAD_STR);
	}
	curl_easy_setopt(single_curl, CURLOPT_URL, pbuff);
    /* Now specify the POST data */ 
	curl_easy_setopt(single_curl, CURLOPT_READFUNCTION, read_callback); 
	curl_easy_setopt(single_curl, CURLOPT_POST, 1L);
	curl_easy_setopt(single_curl, CURLOPT_HEADER, 1L);
	curl_easy_setopt(single_curl, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(single_curl, CURLOPT_VERBOSE, 0L);
	curl_easy_setopt(single_curl, CURLOPT_HTTPPOST, formpost);
	curl_easy_setopt(single_curl, CURLOPT_XFERINFODATA, myp);
	curl_easy_setopt(single_curl, CURLOPT_XFERINFOFUNCTION, xferinfo);
	delete[] pbuff;
	pbuff = NULL;
	return 0;
}

int RequestObject::UploadHandle(const char* remote_path, const char* local_path, const char* file_name)
{
	//single_curl = curl_easy_init();
	struct curl_httppost *formpost=NULL;
	struct curl_httppost *lastptr=NULL;
	struct curl_slist *headerlist=NULL;

	struct stat file_info;
	if(stat(local_path, &file_info) != 0 || file_info.st_size == 0) 
	{
		return 0;
	}
	/* Fill in the file upload field. This makes libcurl load data from
	the given file name when curl_easy_perform() is called. */
	int len = strlen(local_path);
	m_file_path = new char[len+1];
	memset( m_file_path, 0, len+1);
	memcpy( m_file_path, local_path, len);

	len = strlen(file_name);
	m_file_name = new char[len+1];
	memset( m_file_name, 0, len+1);
	memcpy( m_file_name, file_name, len);

	curl_formadd((curl_httppost**)&formpost,
		(curl_httppost**)&lastptr,
		CURLFORM_COPYNAME, "1",
		CURLFORM_FILE, local_path,
		CURLFORM_FILENAME, file_name,
		CURLFORM_END);
	/* Set the dest path */
	size_t temp = strlen(remote_path) + strlen(UPLOAD_STR);
	char* pbuff = new char[temp+1];
	memset( pbuff, 0, temp+1);
	//	sprintf( pConfig, "%s%s", local_path, CONFIG_DL_FILE_SUFFIX);
	if ( pbuff )
	{
		strcat( pbuff, remote_path);
		strcat( pbuff, UPLOAD_STR);
	}
	curl_easy_setopt(single_curl, CURLOPT_URL, pbuff);
    /* Now specify the POST data */ 
	curl_easy_setopt(single_curl, CURLOPT_POST, 1L);
	curl_easy_setopt(single_curl, CURLOPT_HEADER, 1L);
	curl_easy_setopt(single_curl, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(single_curl, CURLOPT_VERBOSE, 0L);
	curl_easy_setopt(single_curl, CURLOPT_HTTPPOST, formpost);
	curl_easy_setopt(single_curl, CURLOPT_XFERINFODATA, myp);
	curl_easy_setopt(single_curl, CURLOPT_XFERINFOFUNCTION, xferinfo);
	delete[] pbuff;
	pbuff = NULL;
	return 0;
}

size_t wirtefunc(void *ptr, size_t size, size_t nmemb, void *stream)
{
	return fwrite(ptr, size, nmemb, (FILE*)stream);
}

int RequestObject::DownloadHandle(FILE* f, const char * remotepath,const int offset, const int length)
{
	if (f == NULL) {
		perror(NULL);
		return 0;
	}
	//single_curl = curl_easy_init();
	CURLcode r = curl_easy_setopt(single_curl, CURLOPT_URL, remotepath);
	//set the download range, e.g 100-200, if to the end of file use xxx-,e.g 200-
	printf("%s\n",remotepath);
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
//	printf("%s, %d, %d, %u", range, offset, length, f);
	fseek(f, offset, SEEK_SET);
//	fwrite("1111111", 1, 5, f);
//	printf("fwrite done!\n");
//	fflush(f);
//	fclose(f);
	curl_easy_setopt(single_curl, CURLOPT_RANGE, range);
	curl_easy_setopt(single_curl, CURLOPT_WRITEDATA, f);
	curl_easy_setopt(single_curl, CURLOPT_WRITEFUNCTION, wirtefunc);
	curl_easy_setopt(single_curl, CURLOPT_HEADER, 0L);
	curl_easy_setopt(single_curl, CURLOPT_NOBODY, 0L);
	curl_easy_setopt(single_curl, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(single_curl, CURLOPT_VERBOSE, 0L);
	curl_easy_setopt(single_curl, CURLOPT_XFERINFODATA, myp);
	curl_easy_setopt(single_curl, CURLOPT_XFERINFOFUNCTION, xferinfo);
	//curl_easy_perform(single_curl);
	//printf("download is done!\n");
	return 0;
}

int RequestObject::Finalize()
{
	if ( NULL != single_curl )
	{
		curl_easy_cleanup(single_curl);
		single_curl = NULL;
	}
	if ( NULL != myp )
	{
		delete myp;
		myp = NULL;
	}
	if ( NULL != m_file_path )
	{
		delete[] m_file_path;
		m_file_path = NULL;
	}
	if ( NULL != m_file_name)
	{
		delete[] m_file_name;
		m_file_name = NULL;
	}
	return 0;
}