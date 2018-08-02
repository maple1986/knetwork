#ifndef __UPLOAD_MANAGER_H__
#define __UPLOAD_MANAGER_H__

#include <list>
#include <vector>
#include <kcl.h>
#include <curl/curl.h>
#include "TaskObject.h"
#include "KNetwork.h"

typedef std::list<TaskObject*> TaskList;

class UploadManager : public KComponent<IUploadManager>
{
public:
	UploadManager();
	virtual ~UploadManager();

	STDMETHOD(QueryInterface)(REFIID riid, void __RPC_FAR *__RPC_FAR *ppvObject)
	{
		if(riid == IID_IUploadManager)
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

	STDMETHOD(OpenFileDlg)(HWND hwdParent, VARIANT* pvarResult);

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
	//int Resume(const int index);

	STDMETHOD(UpdateList)();

	STDMETHOD(getUploadPercent)(int* percent);
	STDMETHOD(isFinished)(int* bFinished);
	STDMETHOD(Delete)(BSTR* remote_path, BSTR* file_name);
//	STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid);
public:
	HRESULT _Initialize(DISPPARAMS* pdispparams, VARIANT* pvarResult);
	HRESULT _Finalize(DISPPARAMS* pdispparams, VARIANT* pvarResult);
	HRESULT _Add(DISPPARAMS* pdispparams, VARIANT* pvarResult);
	HRESULT _Remove(DISPPARAMS* pdispparams, VARIANT* pvarResult);
	HRESULT _AllStart(DISPPARAMS* pdispparams, VARIANT* pvarResult);
	HRESULT _AllPause(DISPPARAMS* pdispparams, VARIANT* pvarResult);
	HRESULT _Start(DISPPARAMS* pdispparams, VARIANT* pvarResult);
	HRESULT _Pause(DISPPARAMS* pdispparams, VARIANT* pvarResult);
	HRESULT _UpdateList(DISPPARAMS* pdispparams, VARIANT* pvarResult);
	HRESULT _getUploadPercent(DISPPARAMS* pdispparams, VARIANT* pvarResult);
	HRESULT _isFinished(DISPPARAMS* pdispparams, VARIANT* pvarResult);
	HRESULT _Delete(DISPPARAMS* pdispparams, VARIANT* pvarResult);
	HRESULT _OpenFileDlg(DISPPARAMS* pdispparams, VARIANT* pvarResult);
	DECLARE_DISPATCH_MAP()

private:
	int DoStartAll();
	int addList(const char* remote_path, const char* local_path);
//	int LoadTask();
	//get a free task node
	//get a task node
	int (getTaskObjCount)();
	int (getUploadStatus)(const int index, int64_t& now, int64_t& total);
	static int DoStartAll(void* p);
	int CheckStatusAll_v2();
	int CheckStatusAll();
	int Start(TaskObject& task);
	int Pause(TaskObject& task);
	int getTaskObj(const int index, TaskList::iterator& it);
	BOOL tryConnection(const char* remote_path);
	int LoadTask();
	//int Merge();
	//get a free task node
	//DWORD getHashCode(const char* local_path, unsigned char *digest);
	//int getContentSize(const char* local_path);
	//int splitTask(const char* local_path, std::vector<char*>& vFile);

private:
	int task_mode;
	FILE* f_list;
	CURLM* curlm;
	TaskList task_obj_list;
	int finished;
	int64_t xfered_size;
	int64_t total_size;
};

#endif //__UPLOAD_MANAGER_H__