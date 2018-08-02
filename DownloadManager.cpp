#include <stdio.h>
#include "curl/curl.h"

#include "klib.h"

#include "DownloadManager.h"

#include "ConfigManager.h"
//#include "FileManager.h"
#include "RequestManager.h"
#include "RequestObject.h"
#include "TaskObject.h"
//#include <thread>   // std::thread
using namespace kgapi;
#define DOWNLOAD_LIST_FILE "c:/dl.lst"
#define END_OF_LINE "\n"

BEGIN_DISPATCH_MAP(DownloadManager)
	DISP_FUNCTION(DownloadManager, 1, "Add",		enDispatchMethod, DownloadManager::_Add,		VT_BOOL, VTS_BSTR VTS_BSTR)
	DISP_FUNCTION(DownloadManager, 2, "Initialize", enDispatchMethod, DownloadManager::_Initialize, VT_BOOL, VTS_BOOL)
	DISP_FUNCTION(DownloadManager, 3, "Finalize",	enDispatchMethod, DownloadManager::_Finalize,	VT_BOOL, VTS_NONE)
	DISP_FUNCTION(DownloadManager, 4, "Remove",		enDispatchMethod, DownloadManager::_Remove,		VT_BOOL, VTS_I4)
	DISP_FUNCTION(DownloadManager, 5, "AllStart",	enDispatchMethod, DownloadManager::_AllStart,	VT_BOOL, VTS_NONE)
	DISP_FUNCTION(DownloadManager, 6, "AllPause",	enDispatchMethod, DownloadManager::_AllPause,	VT_BOOL, VTS_NONE)
	DISP_FUNCTION(DownloadManager, 7, "Start",		enDispatchMethod, DownloadManager::_Start,		VT_BOOL, VTS_I4)
	DISP_FUNCTION(DownloadManager, 8, "Pause",		enDispatchMethod, DownloadManager::_Pause,		VT_BOOL, VTS_I4)
	DISP_FUNCTION(DownloadManager, 11, "getDownloadPercent", enDispatchMethod, DownloadManager::_getDownloadPercent, VT_BOOL, VTS_I4)
	DISP_FUNCTION(DownloadManager, 12, "isFinished",		enDispatchMethod, DownloadManager::_isFinished,		VT_BOOL, VTS_I4)
END_DISPATCH_MAP(DownloadManager)

DownloadManager::DownloadManager(): curlm(NULL), f_list(NULL), task_mode(0),finished(0), xfered_size(0), total_size(0)
{
	// FILE* fpConsole = NULL;
	// AllocConsole();
	// fpConsole = freopen("CONOUT$", "w+t", stdout);
}

DownloadManager::~DownloadManager()
{
	Finalize();
}

STDMETHODIMP DownloadManager::Initialize(bool bResume)
{
	printf("[DownloadManager::Initialize]  call in ....\n");
	curl_global_init(CURL_GLOBAL_ALL);
	curlm = curl_multi_init();
	if (bResume)
	{
		if( 0 == LoadTask())
		{
			return S_OK;
		}
	}
	f_list = fopen(DOWNLOAD_LIST_FILE, "wb+");
	fclose(f_list);

	return S_OK;
}

STDMETHODIMP DownloadManager::Finalize()
{
	printf("[DownloadManager::Finalize] call in ...\n");
	curl_global_cleanup();
	if(f_list)
	{
		fclose(f_list);
		f_list = NULL;
	}
	if (curlm)
	{
		curl_multi_cleanup(curlm);
		curlm = NULL;
	}
	if ( !task_obj_list.empty() )
	{
		TaskList::iterator it = task_obj_list.begin();
		for(; it != task_obj_list.end(); ++it)
		{
			delete *it;
			*it = NULL;
		}
		task_obj_list.clear();
	}
	printf("[DownloadManager::Finalize] call UpdateList ...\n");

	UpdateList();

	printf("[DownloadManager::Finalize] End...\n");

	return S_OK;
}

int DownloadManager::LoadTask()
{
	f_list = fopen(DOWNLOAD_LIST_FILE, "rb+");
	if ( NULL == f_list)
	{
		return -1;
	}
	while ( !feof(f_list) )
	{
		//Load Exist Tasks
		char pRemote[128];
		memset( pRemote, 0, 128 );
		fread( pRemote, 1, sizeof(DWORD), f_list);
		DWORD url_length = ReadInt(pRemote);
		if ( 0 == url_length )
		{
			if (!task_obj_list.empty())
			{
				return 0;
			}
			return -1;
		}
		memset( pRemote, 0, 128 );
		fread( pRemote, 1, url_length, f_list);
		
		char pLocal[128];
		memset( pLocal, 0, 128 );
		fread( pLocal, 1, sizeof(DWORD), f_list);
		DWORD local_length = ReadInt(pLocal);
		if ( 0 == local_length )
		{
			return -1;
		}
		memset( pLocal, 0, 128 );
		fread( pLocal, 1, local_length, f_list);
		TaskObject* p_obj = new TaskObject;
		task_obj_list.push_back(p_obj);
		if ( TASK_STATUS_ERROR == task_obj_list.back()->initDL( pRemote, pLocal))
		{
			task_obj_list.pop_back();
		}	
// 		fread( pTemp, 1, sizeof(DWORD), f_list);
// 		DWORD temp = atoi(pTemp);
// 		TaskStatus ts; 
// 		switch(temp)
// 		{
// 		case 0: ts = TASK_STATUS_OK; break;
// 		case 1:	ts = TASK_STATUS_RUNNING; break;
// 		case 2: ts = TASK_STATUS_PAUSED; break;
// 		case 3:	ts = TASK_STATUS_FINISHED; break;
// 		default: ts = TASK_STATUS_OK; break;
// 		}
// 		task_obj_list.back().setTaskStatus(ts);
	}
	return 0;
}

int DownloadManager::isExist(const char* remote_path)
{
	int index = 0;
	TaskList::iterator it = task_obj_list.begin();
	for(; it != task_obj_list.end(); ++it)
	{
		if ( 0 == strcmp(remote_path, (*it)->getConfigManager()->getUrl()))
		{
			return index;
		}
		++index;
	}
	return -1;
}

STDMETHODIMP DownloadManager::Add(BSTR* remote_path, BSTR* local_path)
{
	printf("[DownloadManager::Add] call in...\n");

	KComBSTR kbstr(*remote_path);
	const char* pRemote_path = (LPCSTR)kbstr;

	KComBSTR kbstr2(*local_path);
	const char* pLocal_path = (LPCSTR)kbstr2;

	printf("pRemote_path = [%s]\n", pRemote_path);
	printf("pLocal_path = [%s]\n", pLocal_path);

//	int index = isExist(pRemote_path);
//	if ( -1 != index )
//	{
//		printf("isExist index = [%d]\n", index);
//		Start( index );
//		return index;
//	}
	TaskObject* p_obj = new TaskObject;
	TaskStatus ts = p_obj->initDL( pRemote_path, pLocal_path);
	if ( ts == TASK_STATUS_ERROR )
	{
		delete p_obj;
		return E_FAIL;
	}
	else if ( TASK_STATUS_FINISHED == ts )
	{
		delete p_obj;
		return S_FALSE;
	}
	p_obj->setTaskStatus(TASK_STATUS_OK);
	task_obj_list.push_back(p_obj);
	//CURL* p = task_obj_list.back()->getRequestManager()->getRequestObj(0)->getCurlHandle();
	curl_multi_add_handle( curlm, task_obj_list.back()->getRequestManager()->getRequestObj(0)->getCurlHandle());

	printf("[DownloadManager::Add] OK...\n");

	return S_OK;
}

STDMETHODIMP DownloadManager::Remove(const int index)
{
	TaskList::iterator it;
	getTaskObj(index, it);
	if (task_obj_list.end() != it)
	{
		delete *it;
		*it = NULL;
		task_obj_list.erase(it);
	}
	return S_OK;
}

int DownloadManager::Start(TaskObject& task)
{
	CURLMsg* msg;
	TaskStatus ts =task.getTaskStatus();
	if( (task.getTaskStatus() != TASK_STATUS_OK) && (task.getTaskStatus() != TASK_STATUS_PAUSED) )
	{
		return -1;
	}
	task.setTaskStatus(TASK_STATUS_RUNNING);

 	int still_running = -1;
	curl_multi_add_handle( curlm, task.getRequestManager()->getRequestObj(0)->getCurlHandle());
	do {
		CURLMcode mc;
		int numfds;

		mc = curl_multi_perform(curlm, &still_running);
		//int msgq = 0;
		//msg = curl_multi_info_read(curlm, &msgq);
		//TaskList::iterator it = task_obj_list.begin();
		do {
			int msgq = 0;
			msg = curl_multi_info_read(curlm, &msgq);
			if(msg && (msg->msg == CURLMSG_DONE)) 
			{
				CURL *e = msg->easy_handle;
				curl_multi_remove_handle(curlm, e);
				//curl_easy_cleanup(e);
				//set easy handle to null?
				//e = NULL;
			}
		} while(msg);

		if(mc == CURLM_OK ) {
			/* wait for activity, timeout or "nothing" */
			mc = curl_multi_wait(curlm, NULL, 0, 1000, &numfds);
		}

		if(mc != CURLM_OK) {
			fprintf(stderr, "curl_multi failed, code %d.n", mc);
			break;
		}
	} while(still_running);

// 	if ()
// 	{
	task.setTaskStatus(TASK_STATUS_FINISHED);
//	}
 	return 0;
}

STDMETHODIMP DownloadManager::Start(const int index)
{
	TaskList::iterator it;
	getTaskObj(index, it);
	if (task_obj_list.end() != it)
	{
		if ( (*it)->getTaskStatus() == TASK_STATUS_PAUSED || (*it)->getTaskStatus() == TASK_STATUS_OK )
		{
			Start(**it);
			return S_OK;
		}
	}
	return S_OK;
}

int DownloadManager::DoStartAll(void* p)
{
	DownloadManager* dm = (DownloadManager*)p;
	CURLMsg* msg;
 	int still_running = -1;
	do {
		CURLMcode mc;
		int numfds;

		mc = curl_multi_perform(dm->curlm, &still_running);
		//int msgq = 0;
		//msg = curl_multi_info_read(curlm, &msgq);
		//TaskList::iterator it = task_obj_list.begin();
		do {
			int msgq = 0;
			msg = curl_multi_info_read(dm->curlm, &msgq);
			if(msg && (msg->msg == CURLMSG_DONE)) 
			{
				CURL *e = msg->easy_handle;
				curl_multi_remove_handle(dm->curlm, e);
				//curl_easy_cleanup(e);
				//set easy handle to null?
				//e = NULL;
			}
		} while(msg);

		// if(mc == CURLM_OK ) {
		// 	 wait for activity, timeout or "nothing" 
		// 	mc = curl_multi_wait(curlm, NULL, 0, 1000, &numfds);
		// }

		if(mc != CURLM_OK) {
			fprintf(stderr, "curl_multi failed, code %d.n", mc);
			break;
		}
	} while(still_running);
	TaskList::iterator it = dm->task_obj_list.begin();
	for (; it != dm->task_obj_list.end(); ++it)
	{
		(*it)->DeleteConfigFile();
	}
	dm->finished = 1;
 	return 0;
}

STDMETHODIMP DownloadManager::getDownloadPercent(int* percent)
{
	xfered_size = 0, total_size = 0;
	TaskList::iterator it = task_obj_list.begin();
	for (; it != task_obj_list.end(); ++it)
	{
		int64_t now=0, total=0;
		(*it)->getDownloadStatus(now, total);
		xfered_size += (*it)->getXferedSize();
		xfered_size += now;
		total_size += (*it)->getTotalSize();
	}
	if ( total_size != 0)
	{
		*percent = xfered_size*100/total_size;
	}
	else
	{
		(*percent) = 0;
	}

	if (*percent >= 100)
	{
		if (finished)
		{
			*percent = 100;
		}
		else
		{
			*percent = 99;
		}
	}
	return S_OK;
}

STDMETHODIMP DownloadManager::isFinished(int* bFinished)
{
	*bFinished = finished;
	return S_OK;
}


int DownloadManager::Pause(TaskObject& task)
{
 	if ( TASK_STATUS_RUNNING != task.getTaskStatus())
 	{
 		return 0;
 	}

 	CURL* curl = task.getRequestManager()->getRequestObj(0);
// 	curl_easy_pause( curl, CURLPAUSE_RECV);
 	curl_multi_remove_handle( curlm, curl);
	task.setTaskStatus(TASK_STATUS_PAUSED);
	return 0;
}

STDMETHODIMP DownloadManager::Pause(const int index)
{
	TaskList::iterator it;
	getTaskObj(index, it);
	if (task_obj_list.end() != it)
	{
		Pause(**it);
	}
	return S_OK;
}

STDMETHODIMP DownloadManager::AllStart()
{
	// TaskList::iterator it = task_obj_list.begin();
	// for (; it != task_obj_list.end(); ++it)
	// {
	// 	if (TASK_STATUS_PAUSED == (*it)->getTaskStatus() )
	// 	{
	// 		Start(**it);
	// 	}
	// }
	void* p = (void*)this;
	BOOL bSuccess = kgutil_create_thread(DoStartAll, p);
	if (bSuccess)
	{
		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

STDMETHODIMP DownloadManager::AllPause()
{
	TaskList::iterator it = task_obj_list.begin();
	for (; it != task_obj_list.end(); ++it)
	{
		if (TASK_STATUS_RUNNING == (*it)->getTaskStatus() )
		{
			Pause(**it);
		}
	}
	return S_OK;
}

int DownloadManager::getTaskObj(const int index, TaskList::iterator& it)
{
	if ( task_obj_list.size() <= index )
	{
		it = task_obj_list.end();
		return -1;
	}

	it = task_obj_list.begin();
	for(int i = 0; i < index; ++i)
	{
		it++;
	}
	return 0;
}

int DownloadManager::UpdateList()
{
	printf("[DownloadManager::UpdateList] call in ...\n");

	f_list = fopen(DOWNLOAD_LIST_FILE, "wb+");
	if ( NULL == f_list )
	{
		printf("Task List File can't be opened!\n");
		return -1;
	}

	printf("[DownloadManager::UpdateList] open  DOWNLOAD_LIST_FILE = [%s]\n", DOWNLOAD_LIST_FILE);

	TaskList::iterator it = task_obj_list.begin();
	fseek(f_list, 0, SEEK_SET);

	printf("[DownloadManager::UpdateList] for ...\n");
	int iCount = 0;

	for (; it != task_obj_list.end(); ++it )
	{
		printf("for ---- iCount = [%d]\n", iCount++);

		const char* purl = (*it)->getConfigManager()->getUrl(); 

		printf("purl = [%s]\n", purl);

		DWORD length = strlen(purl);

		printf("length = [%d]\n", length);

		fwrite(&length, sizeof(DWORD), 1, f_list);
		fwrite(purl , 1, length, f_list);
		const char* plocal = (*it)->getConfigManager()->getLocal(); 

		printf("plocal = [%s]\n", plocal);

		length = strlen(plocal);

		printf("length = [%d]\n", length);

		fwrite(&length, sizeof(DWORD), 1, f_list);
		fwrite(plocal , 1, length, f_list);
		// 		switch(it->getTaskStatus())
		// 		{
		// 		case TASK_STATUS_OK: length = 0; break;
		// 		case TASK_STATUS_RUNNING:	length = 1; break;
		// 		case TASK_STATUS_PAUSED: length = 2; break;
		// 		case TASK_STATUS_FINISHED:	length = 3; break;
		// 		default: length = 0; break;
		// 		}
		// 		fwrite(&length, sizeof(DWORD), 1, f_list);
	}
	fclose(f_list);

	printf("[DownloadManager::UpdateList] for end...\n");

	return 0;
}

int DownloadManager::addList(const char* remote_path, const char* local_path)
{
	f_list = fopen( DOWNLOAD_LIST_FILE, "ab+");
	if ( NULL == f_list)
	{
		return -1;
	}
	fseek( f_list, 0, SEEK_END);
	DWORD length = strlen(remote_path);
	fwrite(&length, sizeof(DWORD), 1, f_list);
	fwrite(remote_path, 1, length, f_list);

	length = strlen(local_path);
	fwrite(&length, sizeof(DWORD), 1, f_list);
	fwrite(local_path , 1, length, f_list);

	fclose(f_list);
	return 0;
}

// STDMETHODIMP DownloadManager::getDownloadStatus(const int index, int& now, int& total )
// {
// 	TaskList::iterator it;
// 	getTaskObj(index, it);
// 	if (task_obj_list.end() != it)
// 	{
// 		(*it)->getDownloadStatus(now, total);
// 	}
// 	return 0;
// }

HRESULT DownloadManager::_Initialize(DISPPARAMS* pdispparams, VARIANT* pvarResult)
{
	printf("[DownloadManager::_Initialize]  call in ....\n");

	if (pdispparams == NULL || pvarResult == NULL || pdispparams->cArgs != 1)
	{
		return E_INVALIDARG;
	}

	if((V_VT(&pdispparams->rgvarg[0]) != VT_BOOL))
	{
		return E_INVALIDARG;
	}

	bool bResume = (V_BOOL(&pdispparams->rgvarg[0]) == VARIANT_TRUE) ? true : false;

	HRESULT hr = Initialize(bResume);

	if(pvarResult)
	{		
		bool bRet = SUCCEEDED(hr) ? true : false;
		KComVariant(bRet).Detach(pvarResult);
	}

	return hr;
}

HRESULT DownloadManager::_Add(DISPPARAMS* pdispparams, VARIANT* pvarResult)
{		
	printf("[DownloadManager::_Add] call in...\n");
	if (pdispparams == NULL || pvarResult == NULL || pdispparams->cArgs != 2)
	{
		return E_INVALIDARG;
	}
	
	if((V_VT(&pdispparams->rgvarg[0]) != VT_BSTR) || (V_VT(&pdispparams->rgvarg[1]) != VT_BSTR))
	{
		return E_INVALIDARG;
	}

	HRESULT hr = Add(&(V_BSTR(&pdispparams->rgvarg[1])), &(V_BSTR(&pdispparams->rgvarg[0])));

	if(pvarResult)
	{
		V_VT(pvarResult) = VT_I4;
		if(hr == S_OK)
		{
			V_I4(pvarResult) = 0;
		}
		else if ( hr == S_FALSE)
		{
			V_I4(pvarResult) = 1;
		}
		else if ( hr == E_FAIL)
		{
			V_I4(pvarResult) = -1;
		}
	}

	return hr;
}

HRESULT DownloadManager::_Finalize(DISPPARAMS* pdispparams, VARIANT* pvarResult)
{
	printf("[DownloadManager::_Finalize] call in...\n");

	if (pdispparams == NULL || pvarResult == NULL || pdispparams->cArgs != 0)
	{
		return E_INVALIDARG;
	}

	HRESULT hr = Finalize();

	if(pvarResult)
	{		
		bool bRet = SUCCEEDED(hr) ? true : false;
		KComVariant(bRet).Detach(pvarResult);
	}

	printf("[DownloadManager::_Finalize] End...\n");

	return hr;
}

HRESULT DownloadManager::_Remove(DISPPARAMS* pdispparams, VARIANT* pvarResult)
{
	printf("[DownloadManager::_Remove] call in...\n");

	if (pdispparams == NULL || pvarResult == NULL || pdispparams->cArgs != 1)
	{
		return E_INVALIDARG;
	}

	if((V_VT(&pdispparams->rgvarg[0]) != VT_I4))
	{
		return E_INVALIDARG;
	}

	HRESULT hr = Remove(V_I4(&pdispparams->rgvarg[0]));

	if(pvarResult)
	{		
		bool bRet = SUCCEEDED(hr) ? true : false;
		KComVariant(bRet).Detach(pvarResult);
	}

	printf("[DownloadManager::_Remove] End...\n");

	return hr;
}

HRESULT DownloadManager::_AllStart(DISPPARAMS* pdispparams, VARIANT* pvarResult)
{
	printf("[DownloadManager::_AllStart] call in...\n");

	if (pdispparams == NULL || pvarResult == NULL || pdispparams->cArgs != 0)
	{
		return E_INVALIDARG;
	}

	HRESULT hr = AllStart();

	if(pvarResult)
	{		
		bool bRet = SUCCEEDED(hr) ? true : false;
		KComVariant(bRet).Detach(pvarResult);
	}

	printf("[DownloadManager::_AllStart] End...\n");

	return hr;
}

HRESULT DownloadManager::_AllPause(DISPPARAMS* pdispparams, VARIANT* pvarResult)
{
	printf("[DownloadManager::_AllPause] call in...\n");

	if (pdispparams == NULL || pvarResult == NULL || pdispparams->cArgs != 0)
	{
		return E_INVALIDARG;
	}

	HRESULT hr = AllPause();

	if(pvarResult)
	{		
		bool bRet = SUCCEEDED(hr) ? true : false;
		KComVariant(bRet).Detach(pvarResult);
	}

	printf("[DownloadManager::_AllPause] End...\n");

	return hr;
}

HRESULT DownloadManager::_Start(DISPPARAMS* pdispparams, VARIANT* pvarResult)
{
	printf("[DownloadManager::_Start] call in...\n");

	if (pdispparams == NULL || pvarResult == NULL || pdispparams->cArgs != 1)
	{
		return E_INVALIDARG;
	}

	if((V_VT(&pdispparams->rgvarg[0]) != VT_I4))
	{
		return E_INVALIDARG;
	}

	HRESULT hr = Start(V_I4(&pdispparams->rgvarg[0]));

	if(pvarResult)
	{		
		bool bRet = SUCCEEDED(hr) ? true : false;
		KComVariant(bRet).Detach(pvarResult);
	}

	printf("[DownloadManager::_Start] End...\n");

	return hr;
}

HRESULT DownloadManager::_Pause(DISPPARAMS* pdispparams, VARIANT* pvarResult)
{
	printf("[DownloadManager::_Pause] call in...\n");

	if (pdispparams == NULL || pvarResult == NULL || pdispparams->cArgs != 1)
	{
		return E_INVALIDARG;
	}

	if((V_VT(&pdispparams->rgvarg[0]) != VT_I4))
	{
		return E_INVALIDARG;
	}

	HRESULT hr = Pause(V_I4(&pdispparams->rgvarg[0]));

	if(pvarResult)
	{		
		bool bRet = SUCCEEDED(hr) ? true : false;
		KComVariant(bRet).Detach(pvarResult);
	}

	printf("[DownloadManager::_Pause] End...\n");

	return hr;
}

HRESULT DownloadManager::_getDownloadPercent(DISPPARAMS* pdispparams, VARIANT* pvarResult)
{
	if (pdispparams == NULL || pvarResult == NULL || pdispparams->cArgs != 0)
	{
		return E_INVALIDARG;
	}

	int nPercent = 0;
	HRESULT hr = getDownloadPercent(&nPercent);
	if (FAILED(hr))
	{
		return hr;
	}

	if(pvarResult)
	{
		V_VT(pvarResult) = VT_I4;
		V_I4(pvarResult) = nPercent;
	}

	return S_OK;
}

HRESULT DownloadManager::_isFinished(DISPPARAMS* pdispparams, VARIANT* pvarResult)
{
	if (pdispparams == NULL || pvarResult == NULL || pdispparams->cArgs != 0)
	{
		return E_INVALIDARG;
	}

	int nFinished = 0;
	HRESULT hr = isFinished(&nFinished);
	if (FAILED(hr))
	{
		return hr;
	}

	if(pvarResult)
	{
		V_VT(pvarResult) = VT_I4;
		V_I4(pvarResult) = nFinished;
	}

	return S_OK;
}