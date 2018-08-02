#ifndef __KNETWORK_CLASSFACTORY_H__
#define __KNETWORK_CLASSFACTORY_H__

#include <kcl.h>
#include "KNetwork.h"

#ifndef KKAPI
	#ifdef X_OS_WINDOWS
		#ifdef KNETWORK_EXPORT
			#define KKAPI __declspec(dllexport)
		#else
			#define KKAPI __declspec(dllimport)
		#endif
	#else
		#define KKAPI
	#endif
#endif

class ClassFactory : public KComponent<IClassFactory>
{
public:
	ClassFactory();
	virtual ~ClassFactory();
	
	STDMETHOD(CreateInstance)( 
		/* [unique][in] */ IUnknown __RPC_FAR *pUnkOuter,
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

	STDMETHOD(LockServer)( 
		/* [in] */ BOOL fLock);
};

extern "C" HRESULT KKAPI KCLGetClassObject(/* [in] */ REFCLSID rclsid, /* [in] */ REFIID riid, /*[out] */ LPVOID* ppv);
extern "C" HRESULT KKAPI KCLRegisterServer(IKCLRegister* pIRegister);
extern "C" HRESULT KKAPI KCLUnregisterServer(IKCLRegister* pIRegister);

#endif//__KGKEY_CLASSFACTORY_H__
