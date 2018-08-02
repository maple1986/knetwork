#include <shlobj.h>
#include <winsock2.h>
#include <windows.h>
#include <sys/stat.h>
#include "UploadManager.h"
#include "RequestManager.h"
//#include <string>
#include "klib.h"


#define UPLOAD_LIST_FILE "c:/ul.lst"

#define DELETE_STR "KGDeleteFile?"

using namespace kgapi;


BEGIN_DISPATCH_MAP(UploadManager)
	DISP_FUNCTION(UploadManager, 1, "Add",			enDispatchMethod, UploadManager::_Add,			VT_BOOL, VTS_BSTR VTS_BSTR)
	DISP_FUNCTION(UploadManager, 2, "Initialize",	enDispatchMethod, UploadManager::_Initialize,	VT_BOOL, VTS_BOOL)
	DISP_FUNCTION(UploadManager, 3, "Finalize",		enDispatchMethod, UploadManager::_Finalize,		VT_BOOL, VTS_NONE)
	DISP_FUNCTION(UploadManager, 4, "Remove",		enDispatchMethod, UploadManager::_Remove,		VT_BOOL, VTS_I4)
	DISP_FUNCTION(UploadManager, 5, "AllStart",		enDispatchMethod, UploadManager::_AllStart,		VT_BOOL, VTS_NONE)
	DISP_FUNCTION(UploadManager, 6, "AllPause",		enDispatchMethod, UploadManager::_AllPause,		VT_BOOL, VTS_NONE)
	DISP_FUNCTION(UploadManager, 7, "Start",		enDispatchMethod, UploadManager::_Start,		VT_BOOL, VTS_I4)
	DISP_FUNCTION(UploadManager, 8, "Pause",		enDispatchMethod, UploadManager::_Pause,		VT_BOOL, VTS_I4)
	DISP_FUNCTION(UploadManager, 9, "UpdateList",		enDispatchMethod, UploadManager::_UpdateList,		VT_BOOL, VTS_I4)
	DISP_FUNCTION(UploadManager, 10, "Delete",		enDispatchMethod, UploadManager::_Delete,		VT_BOOL, VTS_BSTR VTS_BSTR)
	DISP_FUNCTION(UploadManager, 11, "getUploadPercent", enDispatchMethod, UploadManager::_getUploadPercent, VT_BOOL, VTS_I4)
	DISP_FUNCTION(UploadManager, 12, "isFinished",		enDispatchMethod, UploadManager::_isFinished,		VT_BOOL, VTS_I4)
	DISP_FUNCTION(UploadManager, 13, "OpenFileDlg", enDispatchMethod, UploadManager::_OpenFileDlg,  VT_BOOL, VTS_BSTR)
END_DISPATCH_MAP(UploadManager)

UploadManager::UploadManager():finished(0),f_list(0),curlm(NULL), task_mode(0), xfered_size(0), total_size(0)
{
	// FILE* fpConsole = NULL;
	// AllocConsole();
	// fpConsole = freopen("CONOUT$", "w+t", stdout);
}
UploadManager::~UploadManager()
{
	Finalize();
}

	//init before use it
STDMETHODIMP UploadManager::Initialize(bool bResume)
{
	if ( bResume)
	{
		task_mode = 1;
	}
	else
	{
		task_mode = 0;
	}
	curlm = curl_multi_init();
	return S_OK;
}

STDMETHODIMP  UploadManager::Finalize()
{
	if(f_list)
	{
		fclose(f_list);
		f_list = NULL;
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
	if (curlm)
	{
		curl_multi_cleanup(curlm);
		curlm = NULL;
	}
	return S_OK;
}

int UploadManager::LoadTask()
{
	f_list = fopen(UPLOAD_LIST_FILE, "rb+");
	if ( NULL == f_list)
	{
		return -1;
	}
	while ( !feof(f_list) )
	{
		//Load Exist Tasks
		char pRemote[MAX_PATH];
		memset( pRemote, 0, MAX_PATH);
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
		memset( pRemote, 0, MAX_PATH);
		fread( pRemote, 1, url_length, f_list);

		char pLocal[MAX_PATH];
		memset( pLocal, 0, MAX_PATH);
		fread( pLocal, 1, sizeof(DWORD), f_list);
		DWORD local_length = ReadInt(pLocal);
		if ( 0 == local_length )
		{
			return -1;
		}
		memset( pLocal, 0, MAX_PATH);
		fread( pLocal, 1, local_length, f_list);
		TaskObject* p_obj = new TaskObject;
		task_obj_list.push_back(p_obj);
		if ( TASK_STATUS_ERROR == task_obj_list.back()->initDL( pRemote, pLocal))
		{
			task_obj_list.pop_back();
		}	
	}
	fclose(f_list);
	return 0;
}

STDMETHODIMP UploadManager::Add(BSTR* remote_path, BSTR* local_path)
{
	printf("[UploadManager::Add] call in...\n");

	KComBSTR kbstr(*remote_path);
	const char* pRemote_path = (LPCSTR)kbstr;

	KComBSTR kbstr2(*local_path);
	const char* pLocal_path = (LPCSTR)kbstr2;
	
	printf("pRemote_path = [%s]\n", pRemote_path);
	printf("pLocal_path = [%s]\n", pLocal_path);
	//check if the file is valid
	struct stat file_info;
	//size_t file_len;
	if(stat(pLocal_path, &file_info) != 0 || file_info.st_size == 0) 
	{
		return E_INVALIDARG;
	}
	if (!tryConnection(pRemote_path))
	{
		return E_INVALIDARG;
	}
	HRESULT hr = S_OK;
	TaskObject* p_obj = new TaskObject;
	TaskStatus ts = p_obj->initUL( pRemote_path, pLocal_path );
	if ( ts == TASK_STATUS_ERROR )
	{
		delete p_obj;
		return E_FAIL;
	}
	if ( ts == TASK_STATUS_FINISHED )
	{
		delete p_obj;
		return S_FALSE;
	}
	task_obj_list.push_back(p_obj);

	//task_obj_list.back()->setTaskStatus(TASK_STATUS_OK);
	for (int i = 0; i < task_obj_list.back()->getRequestManager()->getRequestObjSize(); ++i)
	{
		curl_multi_add_handle(curlm, task_obj_list.back()->getRequestManager()->getRequestObj(i)->getCurlHandle());
	}
	return S_OK;
}

STDMETHODIMP UploadManager::OpenFileDlg(HWND hwdParent, VARIANT* pvarResult)
{
	if(pvarResult == NULL)
	{
		return E_INVALIDARG;
	}

	// WCHAR szBuffer[MAX_PATH] = {0};   
	// OPENFILENAMEW ofn= {0};   
	// ofn.lStructSize = sizeof(ofn);   
	// //ofn.hwndOwner = m_hWnd;   
	// ofn.lpstrFilter = L"所有文件(*.*)\0*.*\0";//要选择的文件后缀   
	// ofn.lpstrInitialDir = L"c:\\";//默认的文件路径   
	// ofn.lpstrFile = szBuffer;//存放文件的缓冲区   
	// ofn.nMaxFile = sizeof(szBuffer)/sizeof(*szBuffer);   
	// ofn.nFilterIndex = 0;   
	// ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER ;//标志如果是多选要加上OFN_ALLOWMULTISELECT  
	// BOOL bSel = GetOpenFileNameW(&ofn);
	
	// BSTR bstrPath = KSysAllocString(szBuffer);

	// V_VT(pvarResult) = VT_BSTR;
	// V_BSTR(pvarResult) = bstrPath;

	OPENFILENAMEW ofn = {0};
	WCHAR szOpenFileNames[80*MAX_PATH];
	ZeroMemory(szOpenFileNames, sizeof(szOpenFileNames));
	WCHAR szPath[MAX_PATH];
	ZeroMemory(szPath, sizeof(szPath));
	WCHAR szFileName[80*MAX_PATH];
	WCHAR* p;
	int nLen = 0;
	//ZeroMemory( &ofn, sizeof(ofn) );
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_ALLOWMULTISELECT;
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = szOpenFileNames;
	ofn.nMaxFile = sizeof(szOpenFileNames);
	ofn.lpstrFile[0] = '\0';
	ofn.lpstrFilter = L"所有文件(*.*)\0*.*\0";
	ofn.lpstrInitialDir = L"c:\\";
	ofn.hwndOwner = hwdParent;
	if( GetOpenFileNameW( &ofn ) )
	{  
		//把第一个文件名前的复制到szPath,即:
		//如果只选了一个文件,就复制到最后一个'/'
		//如果选了多个文件,就复制到第一个NULL字符
		lstrcpynW(szPath, szOpenFileNames, ofn.nFileOffset );
		//当只选了一个文件时,下面这个NULL字符是必需的.
		//这里不区别对待选了一个和多个文件的情况
		szPath[ ofn.nFileOffset ] = L'\0';
		nLen = lstrlenW(szPath);

		if( szPath[nLen-1] != '\\' )   //如果选了多个文件,则必须加上'//'
		{
			lstrcatW(szPath, L"\\");
		}

		p = szOpenFileNames + ofn.nFileOffset; //把指针移到第一个文件

		ZeroMemory(szFileName, sizeof(szFileName));
		lstrcatW(szFileName, L"[");
		while( *p )
		{   
			lstrcatW(szFileName, L"\"");
			lstrcatW(szFileName, szPath);  //给文件名加上路径  
			lstrcatW(szFileName, p);    //加上文件名  
			lstrcatW(szFileName, L"\","); //换行   
			p += lstrlenW(p) +1;     //移至下一个文件
		}
		nLen = lstrlenW(szFileName);

		if( szFileName[nLen-1] == ',' )   //如果选了多个文件,则必须加上'//'
		{
			szFileName[nLen-1] = L']';
		}
		else
		{
			lstrcatW(szFileName, L"]");
		}
		printf("###szFileName=%s\n", szFileName);
		BSTR bstrPath = KSysAllocString(szFileName);

		V_VT(pvarResult) = VT_BSTR;
		V_BSTR(pvarResult) = bstrPath;
	}
	return S_OK;
}

// STDMETHODIMP UploadManager::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid)
// {
// 	printf("[UploadManager::GetIDsOfNames] call in ... \n");
// 	return KComponentImpl::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);	
// }

STDMETHODIMP UploadManager::Remove(const int index)
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

STDMETHODIMP UploadManager::AllStart()
{
// 	TaskList::iterator it = task_obj_list.begin();
// 	for (; it != task_obj_list.end(); ++it)
// 	{
// 		if (TASK_STATUS_PAUSED == (*it)->getTaskStatus() || TASK_STATUS_OK == (*it)->getTaskStatus())
// 		{
// 			Start(**it);
// 		}
// 	}
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
	//Pause all!
STDMETHODIMP UploadManager::AllPause()
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

STDMETHODIMP UploadManager::isFinished(int* bFinished)
{
	*bFinished = finished; 
	return S_OK;
} 

STDMETHODIMP UploadManager::Start(const int index)
{
	TaskList::iterator it;
	getTaskObj(index, it);
	if (task_obj_list.end() != it)
	{
		Start(**it);
	}
	return S_OK;
}

STDMETHODIMP UploadManager::Pause(const int index)
{
	TaskList::iterator it;
	getTaskObj(index, it);
	if (task_obj_list.end() != it)
	{
		Pause(**it);
	}
	return S_OK;
}

STDMETHODIMP UploadManager::UpdateList()
{
	return S_OK;
}

int UploadManager::Start(TaskObject& task)
{
	return 0;
}
int UploadManager::Pause(TaskObject& task)
{
	return 0;
}

int UploadManager::getTaskObj(const int index, TaskList::iterator& it)
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

STDMETHODIMP UploadManager::Delete(BSTR* remote_path, BSTR* file_name)
{
	printf("[UploadManager::Delete] call in...\n");
	KComBSTR kbstr(*remote_path);
	const char* pRemote_path = (LPCSTR)kbstr;

	KComBSTR kbstr2(*file_name);
	const char* pFile_name = (LPCSTR)kbstr2;

	CURL* curl = curl_easy_init();
	char pDelete[4098];
	memset(pDelete, 0, 4098);
	//struct stat file_info;
	//size_t file_len;
	
	char* pEncode = curl_easy_escape( curl, pFile_name, strlen(pFile_name));
	sprintf(pDelete, "%s/%sfilename=%s", pRemote_path, DELETE_STR, pEncode);
	printf("[UploadManager::Delete] delete %s...\n", pDelete);
	curl_easy_setopt(curl, CURLOPT_URL, pDelete);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
	struct MemoryStruct chunk;
	chunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */ 
	chunk.size = 0;    /* no data at this point */ 
	/* we pass our 'chunk' struct to the callback function */ 
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
	CURLcode rc = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	if ( 0 == strcmp(chunk.memory, "true") )
	{
		printf("Delete return true\n!");
		free(chunk.memory);
		return S_OK;
	}
	printf("Delete return false\n!");
	free(chunk.memory);
	return E_FAIL;
}

BOOL UploadManager::tryConnection(const char* remote_path)
{
	CURL* curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, remote_path);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
	CURLcode rc = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	if ( CURLE_OK != rc )
	{
		return FALSE;
	}
	return TRUE;
}

int UploadManager::getUploadStatus(const int index, int64_t& now, int64_t& total )
{
	TaskList::iterator it;
	getTaskObj(index, it);
	if (task_obj_list.end() != it)
	{
		(*it)->getUploadStatus(now, total);
	}
	return 0;
}

STDMETHODIMP UploadManager::getUploadPercent(int* percent)
{
	xfered_size = 0, total_size = 0;
	TaskList::iterator it = task_obj_list.begin();
	for (; it != task_obj_list.end(); ++it)
	{
		int64_t now=0, total=0;
		(*it)->getUploadStatus(now, total);
		xfered_size += (*it)->getXferedSize();
		xfered_size += now;
		total_size	+= (*it)->getTotalSize();
	}
	if ( total_size != 0)
	{
		(*percent) = xfered_size*100/total_size;
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

int UploadManager::DoStartAll(void* p)
{
	UploadManager* q = (UploadManager*)p;
	CURLMsg* msg;

	int still_running = -1;

	do {
		CURLMcode mc;
		int numfds;

		mc = curl_multi_perform(q->curlm, &still_running);
		//int msgq = 0;
		//msg = curl_multi_info_read(curlm, &msgq);
		//TaskList::iterator it = task_obj_list.begin();
		do {
			int msgq = 0;
			msg = curl_multi_info_read(q->curlm, &msgq);
			if (msg && (msg->msg == CURLMSG_DONE)) {
				CURL *e = msg->easy_handle;
				//transfers--;

				curl_multi_remove_handle(q->curlm, e);
				//curl_easy_cleanup(e);

			}
		} while (msg);

		if (mc == CURLM_OK) {
			/* wait for activity, timeout or "nothing" */
			mc = curl_multi_wait(q->curlm, NULL, 0, 1000, &numfds);
		}

		if (mc != CURLM_OK) {
			fprintf(stderr, "curl_multi failed, code %d.n", mc);
			break;
		}
	} while (still_running);

	q->CheckStatusAll();
	printf("Upload Done!\n");
	q->finished = 1;
	return 0;
}

int UploadManager::CheckStatusAll()
{
	printf("UploadManager::CheckStatusAll call in...\n");
	TaskList::iterator it = task_obj_list.begin();
	for (; it != task_obj_list.end(); ++it)
	{
		if((*it)->hasSplitted())
		{
			for (int i = 0; i < (*it)->getRequestManager()->getRequestObjSize(); ++i)
			{
				if ( true != (*it)->CheckSum((*it)->getRequestManager()->getRequestObj(i)->getFilepath(), (*it)->getRequestManager()->getRequestObj(i)->getFilename()))
				{
					curl_easy_perform((*it)->getRequestManager()->getRequestObj(i)->getCurlHandle());
				}
			}
			(*it)->Merge();
			(*it)->DeleteAllTempFile();
		}
		(*it)->DeleteConfigFile();
		(*it)->setTaskStatus(TASK_STATUS_FINISHED);
	}

	return S_OK;
}

int UploadManager::CheckStatusAll_v2()
{
	printf("UploadManager::CheckStatusAll_v2 call in...\n");
	TaskList::iterator it = task_obj_list.begin();
	for (; it != task_obj_list.end(); ++it)
	{
		if((*it)->hasSplitted())
		{
			for (int i = 0; i < (*it)->getRequestManager()->getRequestObjSize(); ++i)
			{
				// if ( true != (*it)->CheckSum_v2((*it)->getRequestManager()->getRequestObj(i)->getFilepath(), (*it)->getRequestManager()->getRequestObj(i)->getFilename()))
				// {
				// 	curl_easy_perform((*it)->getRequestManager()->getRequestObj(i)->getCurlHandle());
				// }
			}
			(*it)->Merge();
			//(*it)->DeleteAllTempFile();
		}
		(*it)->DeleteConfigFile();
		(*it)->setTaskStatus(TASK_STATUS_FINISHED);
	}

	return S_OK;
}

int UploadManager::getTaskObjCount()
{
	return task_obj_list.size();
}

// dispatch api
HRESULT UploadManager::_Initialize(DISPPARAMS* pdispparams, VARIANT* pvarResult)
{
	printf("[UploadManager::_Initialize]  call in ....\n");

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

HRESULT UploadManager::_Add(DISPPARAMS* pdispparams, VARIANT* pvarResult)
{
	printf("[UploadManager::_Add] call in...\n");

	if(pvarResult)
	{
		V_VT(pvarResult) = VT_I4;
		V_I4(pvarResult) = -1;
	}

	if (pdispparams == NULL || pvarResult == NULL || pdispparams->cArgs != 2)
	{
		return E_INVALIDARG;
	}

	if((V_VT(&pdispparams->rgvarg[0]) != VT_BSTR) || (V_VT(&pdispparams->rgvarg[1]) != VT_BSTR))
	{
		return E_INVALIDARG;
	}

	HRESULT hr = Add(&(V_BSTR(&pdispparams->rgvarg[1])), &(V_BSTR(&pdispparams->rgvarg[0])));

	//if(pvarResult)
	//{		
	//	bool bRet = SUCCEEDED(hr) ? true : false;
	//	KComVariant(bRet).Detach(pvarResult);
	//}
	if(hr == S_OK)
	{
		V_I4(pvarResult) = 0;
	}
	else if ( hr == S_FALSE)
	{
		V_I4(pvarResult) = 1;
	}
	else
	{
		V_I4(pvarResult) = -1;
	}
	
	return S_OK;
}

HRESULT UploadManager::_OpenFileDlg(DISPPARAMS* pdispparams, VARIANT* pvarResult)
{
	if (pdispparams == NULL || pvarResult == NULL || pdispparams->cArgs != 1)
	{
		printf("opps!error!\n");
		return E_INVALIDARG;
	}
	HWND pParent = (HWND)(V_I4(&pdispparams->rgvarg[0]));
	printf("%08x\n", pParent);
	HRESULT hr = OpenFileDlg(pParent, pvarResult);
	
	return S_OK;
}

HRESULT UploadManager::_Finalize(DISPPARAMS* pdispparams, VARIANT* pvarResult)
{
	printf("[UploadManager::_Finalize] call in...\n");
	printf("[UploadManager::_Finalize] %d, %d,%d\n",pdispparams, pvarResult, pdispparams->cArgs );
	if (pdispparams == NULL || pvarResult == NULL || pdispparams->cArgs != 0)
	{
		printf("opps!error!\n");
		return E_INVALIDARG;
	}
	printf("[UploadManager::_Finalize] call Finalize...\n");
	HRESULT hr = Finalize();
	printf("[UploadManager::_Finalize] End Finalize...\n");
	if(pvarResult)
	{		
		bool bRet = SUCCEEDED(hr) ? true : false;
		KComVariant(bRet).Detach(pvarResult);
	}

	printf("[UploadManager::_Finalize] End...\n");

	return hr;
}

HRESULT UploadManager::_Remove(DISPPARAMS* pdispparams, VARIANT* pvarResult)
{
	printf("[UploadManager::_Remove] call in...\n");

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

	printf("[UploadManager::_Remove] End...\n");

	return hr;
}

HRESULT UploadManager::_AllStart(DISPPARAMS* pdispparams, VARIANT* pvarResult)
{
	printf("[UploadManager::_AllStart] call in...\n");

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

	printf("[UploadManager::_AllStart] End...\n");

	return hr;
}

HRESULT UploadManager::_AllPause(DISPPARAMS* pdispparams, VARIANT* pvarResult)
{
	printf("[UploadManager::_AllPause] call in...\n");

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

	printf("[UploadManager::_AllPause] End...\n");

	return hr;
}

HRESULT UploadManager::_Start(DISPPARAMS* pdispparams, VARIANT* pvarResult)
{
	printf("[UploadManager::_Start] call in...\n");

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

	printf("[UploadManager::_Start] End...\n");

	return hr;
}

HRESULT UploadManager::_Pause(DISPPARAMS* pdispparams, VARIANT* pvarResult)
{
	printf("[UploadManager::_Pause] call in...\n");

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

	printf("[UploadManager::_Pause] End...\n");

	return hr;
}

HRESULT UploadManager::_UpdateList(DISPPARAMS* pdispparams, VARIANT* pvarResult)
{
	printf("[UploadManager::_Pause] call in...\n");

	if (pdispparams == NULL || pvarResult == NULL)
	{
		return E_INVALIDARG;
	}

	HRESULT hr = UpdateList();

	if(pvarResult)
	{		
		bool bRet = SUCCEEDED(hr) ? true : false;
		KComVariant(bRet).Detach(pvarResult);
	}

	printf("[UploadManager::_Pause] End...\n");

	return hr;
}

HRESULT UploadManager::_Delete(DISPPARAMS* pdispparams, VARIANT* pvarResult)
{
	printf("[UploadManager::_Delete] call in...\n");

	if (pdispparams == NULL || pvarResult == NULL || pdispparams->cArgs != 2)
	{
		return E_INVALIDARG;
	}

	if((V_VT(&pdispparams->rgvarg[0]) != VT_BSTR) || (V_VT(&pdispparams->rgvarg[1]) != VT_BSTR))
	{
		return E_INVALIDARG;
	}

	HRESULT hr = Delete(&(V_BSTR(&pdispparams->rgvarg[1])), &(V_BSTR(&pdispparams->rgvarg[0])));
	printf("Delete has been done!\n");
	if(pvarResult)
	{		
		bool bRet = SUCCEEDED(hr) ? true : false;
		KComVariant(bRet).Detach(pvarResult);
	}
	printf("Delete Return \n");
	return hr;
}


HRESULT UploadManager::_getUploadPercent(DISPPARAMS* pdispparams, VARIANT* pvarResult)
{
	if (pdispparams == NULL || pvarResult == NULL || pdispparams->cArgs != 0)
	{
		return E_INVALIDARG;
	}

	int nPercent = 0;
	HRESULT hr = getUploadPercent(&nPercent);
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

HRESULT UploadManager::_isFinished(DISPPARAMS* pdispparams, VARIANT* pvarResult)
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
