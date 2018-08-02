#ifndef __K_NETWORK_H__
#define __K_NETWORK_H__

#ifdef __cplusplus
extern "C" {
#endif

//extern "C++" const IID LIBID_KGKey;

const IID LIBID_KNetwork = 
{ 0x877bc8be, 0x3a70, 0x45d2, { 0x88, 0x4c, 0xa5, 0x83, 0x91, 0x3d, 0x66, 0x61 } };
// {877BC8BE-3A70-45D2-884C-A583913D6661}

#ifndef __IDownloadManager_INTERFACE_DEFINED__
#define __IDownloadManager_INTERFACE_DEFINED__

const IID IID_IDownloadManager = 
{ 0x2939e64f, 0x4617, 0x47b1, { 0xb0, 0x82, 0xf1, 0x12, 0xcb, 0xd0, 0x43, 0xe3 } };

MIDL_INTERFACE("2939E64F-4617-47B1-B082-F112CBD043E3")
IDownloadManager : public IDispatch
{
public:
	virtual HRESULT STDMETHODCALLTYPE Initialize(bool bResume) = 0;
	virtual HRESULT STDMETHODCALLTYPE Finalize() = 0;
	virtual HRESULT STDMETHODCALLTYPE Add(BSTR* remote_path, BSTR* local_path) = 0;
	virtual HRESULT STDMETHODCALLTYPE Start(const int index) = 0;
	virtual HRESULT STDMETHODCALLTYPE Pause(const int index) = 0;
	virtual HRESULT STDMETHODCALLTYPE Remove(const int index) = 0;
	virtual HRESULT STDMETHODCALLTYPE AllStart() = 0;
	virtual HRESULT STDMETHODCALLTYPE AllPause() = 0;
	virtual HRESULT STDMETHODCALLTYPE getDownloadPercent(int* percent)=0;
	virtual HRESULT STDMETHODCALLTYPE isFinished(int* bFinished)=0;
	//virtual HRESULT STDMETHODCALLTYPE UpdateList() = 0;
};

#endif//__IDownloadManager_INTERFACE_DEFINED__

#ifndef __IUploadManager_INTERFACE_DEFINED__
#define __IUploadManager_INTERFACE_DEFINED__

// {860E0EE9-A7BB-49D4-BF1E-CD9F8F74249C}
const IID IID_IUploadManager = 
{ 0x860e0ee9, 0xa7bb, 0x49d4, { 0xbf, 0x1e, 0xcd, 0x9f, 0x8f, 0x74, 0x24, 0x9c } };

MIDL_INTERFACE("860E0EE9-A7BB-49D4-BF1E-CD9F8F74249C")
IUploadManager : public IDispatch
{
public:
	virtual HRESULT STDMETHODCALLTYPE Initialize(bool bResume) = 0;
	virtual HRESULT STDMETHODCALLTYPE Finalize() = 0;
	virtual HRESULT STDMETHODCALLTYPE Add(BSTR* remote_path, BSTR* local_path) = 0;
	virtual HRESULT STDMETHODCALLTYPE Start(const int index) = 0;
	virtual HRESULT STDMETHODCALLTYPE Pause(const int index) = 0;
	virtual HRESULT STDMETHODCALLTYPE Remove(const int index) = 0;
	virtual HRESULT STDMETHODCALLTYPE AllStart() = 0;
	virtual HRESULT STDMETHODCALLTYPE AllPause() = 0;
	virtual HRESULT STDMETHODCALLTYPE getUploadPercent(int* percent)=0;
	virtual HRESULT STDMETHODCALLTYPE isFinished(int* bFinished)=0;
	virtual HRESULT STDMETHODCALLTYPE Delete(BSTR* remote_path, BSTR* local_path)=0;
};

#endif//__IUploadManager_INTERFACE_DEFINED__

//HRESULT KKAPI KCLGetClassObject(/* [in] */ REFCLSID rclsid, /* [in] */ REFIID riid, /*[out] */ LPVOID* ppv);
//extern "C" HRESULT KKAPI KCLRegisterServer(IKCLRegister* pIRegister);
//extern "C" HRESULT KKAPI KCLUnregisterServer(IKCLRegister* pIRegister);


#ifndef __IKCLRegister_INTERFACE_DEFINED__
#define __IKCLRegister_INTERFACE_DEFINED__

// {89FA0249-2B8A-44BC-BDA4-CB60BE03AA2E}
const IID IID_IKCLRegister = 
{ 0x89fa0249, 0x2b8a, 0x44bc, { 0xbd, 0xa4, 0xcb, 0x60, 0xbe, 0x3, 0xaa, 0x2e } };

MIDL_INTERFACE("89FA0249-2B8A-44BC-BDA4-CB60BE03AA2E")
IKCLRegister : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE Register(REFCLSID clsid, REFIID riid, BSTR parameter) = 0;
	virtual HRESULT STDMETHODCALLTYPE Unregister(REFIID riid) = 0;
};

#endif//__IKCLRegister_INTERFACE_DEFINED__

HRESULT KCLGetClassObject(/* [in] */ REFCLSID rclsid, /* [in] */ REFIID riid, /*[out] */ LPVOID* ppv);

#ifdef __cplusplus
}
#endif

#endif//__K_NETWORK_H__