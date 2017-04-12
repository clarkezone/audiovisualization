#pragma once
#include <mfapi.h>
#include <wrl.h>
#include <windows.foundation.diagnostics.h>

// Helper class to log stop event automatically when object goes out of scope
class CLogActivityHelper
{
	Microsoft::WRL::ComPtr<ABI::Windows::Foundation::Diagnostics::ILoggingActivity> m_spActivity;
public:
	CLogActivityHelper(ABI::Windows::Foundation::Diagnostics::ILoggingActivity *pActivity)
	{
		m_spActivity = pActivity;
	}
	// If destructed when going out of scope log stop event with same name
	~CLogActivityHelper()
	{
		using namespace ABI::Windows::Foundation::Diagnostics;
		using namespace Microsoft::WRL;
		ComPtr<ILoggingActivity2> m_spActivity2;
		m_spActivity.As(&m_spActivity2);
		HSTRING hEventName = nullptr;
		m_spActivity->get_Name(&hEventName);
		m_spActivity2->StopActivity(hEventName);
	}
};

class Trace
{
	static HRESULT GetMediaProperties(Microsoft::WRL::ComPtr<ABI::Windows::Foundation::Diagnostics::ILoggingFields> *ppFields,IMFMediaType *);
	static HRESULT CreateLoggingFields(Microsoft::WRL::ComPtr<ABI::Windows::Foundation::Diagnostics::ILoggingFields> *ppFields);
public:
	static HRESULT Initialize();
	static HRESULT Log_ProcessInput(ABI::Windows::Foundation::Diagnostics::ILoggingActivity **pActivity, DWORD dwInputStreamID, IMFSample *pSample, DWORD dwFlags,REFERENCE_TIME presentationTime);
	static HRESULT Log_ProcessOutput(ABI::Windows::Foundation::Diagnostics::ILoggingActivity **pActivity,DWORD dwFlags,DWORD cOutputBufferCount,REFERENCE_TIME presentationTime);
	static HRESULT Log_SetInputType(DWORD dwStreamID, IMFMediaType *);
	static HRESULT Log_SetOutputType(DWORD dwStreamID, IMFMediaType *);
};

