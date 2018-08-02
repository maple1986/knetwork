#include "ClassFactory.h"
#include <stdio.h>
#include <kcl.h>

#include "KNetwork.h"
#include <windows.h>
#include "UploadManager.h"
#include "DownloadManager.h"

// {735F882E-9E36-4392-A2D9-BE7808CC1D97}
//const IID IID_IClassFactory = 
//{ 0x735f882e, 0x9e36, 0x4392, { 0xa2, 0xd9, 0xbe, 0x78, 0x8, 0xcc, 0x1d, 0x97 } };

//{00000001-0000-0000-C000-000000000046}
const IID IID_IClassFactory = 
{ 0x00000001, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };

HRESULT KCLGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	printf("KCLGetClassObject call...\n");

	if(ppv == NULL)
	{
		return E_INVALIDARG;
	}

	if(rclsid == LIBID_KNetwork)
	{
		if(riid == IID_IClassFactory)
		{
			ClassFactory* pCF = new ClassFactory;		
			if(pCF == NULL)
			{
				return E_OUTOFMEMORY;
			}
			*ppv = (void*)pCF;
			pCF->AddRef();
			return S_OK;
		}
		else
		{
			return E_NOINTERFACE;
		}
	}
	else
	{
		printf("%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n", rclsid.Data1, rclsid.Data2, rclsid.Data3, rclsid.Data4[0], 
			rclsid.Data4[1], rclsid.Data4[2], rclsid.Data4[3], rclsid.Data4[4], rclsid.Data4[5], 
			rclsid.Data4[6], rclsid.Data4[7]);

		printf("%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n", LIBID_KNetwork.Data1, 
			LIBID_KNetwork.Data2, LIBID_KNetwork.Data3, LIBID_KNetwork.Data4[0], 
			LIBID_KNetwork.Data4[1], LIBID_KNetwork.Data4[2], LIBID_KNetwork.Data4[3], 
			LIBID_KNetwork.Data4[4], LIBID_KNetwork.Data4[5], 
			LIBID_KNetwork.Data4[6], LIBID_KNetwork.Data4[7]);
	}

	return E_NOTIMPL;
}

ClassFactory::ClassFactory()
{
	// FILE* fpConsole = NULL;
	// AllocConsole();
	// fpConsole = freopen("CONOUT$", "w+t", stdout);
}

ClassFactory::~ClassFactory()
{
}

STDMETHODIMP ClassFactory::CreateInstance(IUnknown __RPC_FAR *pUnkOuter, REFIID riid, void __RPC_FAR *__RPC_FAR *ppvObject)
{
	if(riid == IID_IClassFactory)
	{
		*ppvObject = (void**)static_cast<IClassFactory*>(this);
		return S_OK;
	}	
	else if(riid == IID_IDownloadManager)
	{
		printf("[ClassFactory::CreateInstance] IID_IDownloadManager...\n");

		DownloadManager* pDownloadManager = new DownloadManager;
		if(pDownloadManager == NULL)
		{
			return E_OUTOFMEMORY;
		}

		pDownloadManager->AddRef();
		*ppvObject = (void*)pDownloadManager;

		printf("[ClassFactory::CreateInstance] IID_IDownloadManager return OK...\n");
		return S_OK;
	}
	else if (riid == IID_IUploadManager)
	{
		printf("[ClassFactory::CreateInstance] IID_IUploadManager...\n");

		UploadManager* pUploadManager = new UploadManager;
		if(pUploadManager == NULL)
		{
			return E_OUTOFMEMORY;
		}

		pUploadManager->AddRef();
		*ppvObject = (void*)pUploadManager;

		printf("[ClassFactory::CreateInstance] IID_IUploadManager return OK...\n");

		return S_OK;
	}	
	
	return E_NOINTERFACE;
}

STDMETHODIMP ClassFactory::LockServer(BOOL fLock)
{
	return S_OK;
}

HRESULT KCLRegisterServer(IKCLRegister* pIRegister)
{
	if(pIRegister == NULL)
	{
		return E_INVALIDARG;
	}
	HRESULT hr;

	hr = pIRegister->Register(LIBID_KNetwork, IID_IDownloadManager, NULL);	
	if(FAILED(hr))
		return hr;

	hr = pIRegister->Register(LIBID_KNetwork, IID_IUploadManager, NULL);	
	if(FAILED(hr))
		return hr;

	return S_OK;
}

HRESULT KCLUnregisterServer(IKCLRegister* pIRegister)
{
	if(pIRegister == NULL)
	{
		return E_INVALIDARG;
	}
	HRESULT hr;
	hr = pIRegister->Unregister(IID_IDownloadManager);
	if(FAILED(hr))
		return hr;

	hr = pIRegister->Unregister(IID_IUploadManager);
	if(FAILED(hr))
		return hr;

	return S_OK;
}