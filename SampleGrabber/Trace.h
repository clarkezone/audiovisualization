#pragma once
#include <mfapi.h>

// Helper class to log stop event automatically when object goes out of scope
class CLogActivityHelper
{
	Windows::Foundation::Diagnostics::LoggingActivity ^m_Activity;
public:
	CLogActivityHelper(Windows::Foundation::Diagnostics::LoggingActivity ^activity)
	{
		m_Activity = activity;
	}
	// If destructed when going out of scope log stop event with same name
	~CLogActivityHelper()
	{
		m_Activity->StopActivity(m_Activity->Name);
	}
};

class Trace
{
	static Windows::Foundation::Diagnostics::LoggingFields ^GetMediaProperties(IMFMediaType *);
public:
	static void Initialize();
	static Windows::Foundation::Diagnostics::LoggingActivity ^Log_ProcessInput(DWORD dwInputStreamID, IMFSample *pSample, DWORD dwFlags,REFERENCE_TIME presentationTime);
	static Windows::Foundation::Diagnostics::LoggingActivity ^Log_ProcessOutput(DWORD dwFlags,DWORD cOutputBufferCount,REFERENCE_TIME presentationTime);
	static void Log_SetInputType(DWORD dwStreamID, IMFMediaType *);
	static void Log_SetOutputType(DWORD dwStreamID, IMFMediaType *);
};

