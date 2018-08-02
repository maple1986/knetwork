#ifndef __DOWNLOAD_MANAGER_H__
#define __DOWNLOAD_MANAGER_H__

#include <list>
#include <curl/curl.h>
#include <kcl.h>
#include "TaskObject.h"
#include "KNetwork.h"
#include "DownloadManager.h"
//#include <thread>

typedef std::list<TaskObject*> TaskList;

class DownloadManager: public KComponent<IDownloadManager>
{
public:
	DownloadManager();
	virtual ~DownloadManager();

	STDMETHOD(QueryInterface)(REFIID riid, void __RPC_FAR *__RPC_FAR *ppvObject)
	{
		if(riid == IID_IDownloadManager)
		{
			if(ppvObject)
			{
				*ppvObject = (void**)this;
			}			
		}
		return KComponentImpl::QueryInterface(riid,ppvObject);
	}
	//init before use it
	STDMETHOD(Initialize)(bool bResume);
	
	STDMETHOD(Finalize)();
	//add a single download task
	STDMETHOD(Add)(BSTR* remote_path, BSTR* local_path);
	
	//Remove a task from task list
	STDMETHOD(Remove)(const int index);
	
	//Start all!
	STDMETHOD(AllStart)();

	//Pause all!
	STDMETHOD(AllPause)();

 	STDMETHOD(Start)(const int index);
 	STDMETHOD(Pause)(const int index);

	STDMETHOD(getDownloadPercent)(int* percent);
 	// Dispatch
	STDMETHOD(isFinished)(int* bFinished);
public:
	HRESULT _Initialize(DISPPARAMS* pdispparams, VARIANT* pvarResult);
	HRESULT _Add(DISPPARAMS* pdispparams, VARIANT* pvarResult);
	HRESULT _Finalize(DISPPARAMS* pdispparams, VARIANT* pvarResult);
	HRESULT _Remove(DISPPARAMS* pdispparams, VARIANT* pvarResult);
	HRESULT _AllStart(DISPPARAMS* pdispparams, VARIANT* pvarResult);
	HRESULT _AllPause(DISPPARAMS* pdispparams, VARIANT* pvarResult);
	HRESULT _Start(DISPPARAMS* pdispparams, VARIANT* pvarResult);
	HRESULT _Pause(DISPPARAMS* pdispparams, VARIANT* pvarResult);
	HRESULT _getDownloadPercent(DISPPARAMS* pdispparams, VARIANT* pvarResult);
	HRESULT _isFinished(DISPPARAMS* pdispparams, VARIANT* pvarResult);
	DECLARE_DISPATCH_MAP()

private:
	static int DoStartAll(void* p);
	int UpdateList();
	int addList(const char* remote_path, const char* local_path);
	int LoadTask();
	//get a free task node
	int getAFreeNode();
	int isExist(const char* remote_path);
	//get a task node
	int Start(TaskObject& task);
	int Pause(TaskObject& task);
	
	int getTaskObj(const int index, TaskList::iterator& it);
private:
	int task_mode;
	FILE* f_list;
	CURLM* curlm;
	TaskList task_obj_list;
	int finished;
	int64_t xfered_size;
	int64_t total_size;
};

#endif //__DOWNLOAD_MANAGER_H__