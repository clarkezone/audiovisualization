#pragma once
#include<wrl.h>
#include <mfapi.h>
//////////////////////////////////////////////////////////////////////////
//  AsyncCallback [template]
//
//  Description:
//  Helper class that routes IMFAsyncCallback::Invoke calls to a class
//  method on the parent class.
//
//  Usage:
//  Add this class as a member variable. In the parent class constructor,
//  initialize the AsyncCallback class like this:
//      m_cb(this, &CYourClass::OnInvoke)
//  where
//      m_cb       = AsyncCallback object
//      CYourClass = parent class
//      OnInvoke   = Method in the parent class to receive Invoke calls.
//
//  The parent's OnInvoke method (you can name it anything you like) must
//  have a signature that matches the InvokeFn typedef below.
//  From: https://msdn.microsoft.com/en-us/library/windows/desktop/ms703889(v=vs.85).aspx
//////////////////////////////////////////////////////////////////////////

// T: Type of the parent object
template<class T>
class AsyncCallback : public Microsoft::WRL::RuntimeClass<
	Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::RuntimeClassType::ClassicCom>,
	IMFAsyncCallback>
{
public:
	typedef HRESULT(T::*InvokeFn)(IMFAsyncResult *pAsyncResult);

	AsyncCallback(T *pParent, InvokeFn fn) : m_pParent(pParent), m_pInvokeFn(fn)
	{
	}

	STDMETHODIMP_(ULONG) AddRef() {
		// Delegate to parent class.
		return m_pParent->AddRef();
	}
	STDMETHODIMP_(ULONG) Release() {
		// Delegate to parent class.
		return m_pParent->Release();
	}

	// IMFAsyncCallback methods
	STDMETHODIMP GetParameters(DWORD*, DWORD*)
	{
		// Implementation of this method is optional.
		return E_NOTIMPL;
	}

	STDMETHODIMP Invoke(IMFAsyncResult* pAsyncResult)
	{
		return (m_pParent->*m_pInvokeFn)(pAsyncResult);
	}

	T *m_pParent;
	InvokeFn m_pInvokeFn;
};
