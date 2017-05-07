#include "pch.h"
#include "Trace.h"
#include <wrl.h>
#include <mfidl.h>
#include <windows.foundation.diagnostics.h>
#include <windows.media.mediaproperties.h>

using namespace ABI::Windows::Foundation::Diagnostics;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

#define LOG_CHANNEL_NAME L"AudioVisualizer-SampleGrabber"

// MFT events
#define MFT_PROCESS_INPUT L"MFT_ProcessInput"
#define MFT_PROCESS_OUTPUT L"MFT_ProcessOutput"
#define MFT_SET_INPUT_TYPE L"MFT_SetInputType"
#define MFT_SET_OUTPUT_TYPE L"MFT_SetOutputType"
#define MFT_PROCESS_EVENT L"MFT_ProcessEvent"
#define MFT_PROCESS_MESSAGE L"MFT_ProcessMessage"

// Analyzer events
#define ANALYZER_CONFIGURE L"SA_Configure"
#define ANALYZER_STEP L"SA_Step"
#define ANALYZER_SCHEDULE L"SA_Schedule"
#define ANALYZER_ALREADYRUNNING L"SA_AlreadyRunning"
#define ANALYZER_COPY_DATA L"SA_CopyInput"
#define ANALYZER_LOCK_INPUT_BUFFER L"SA_LockBuffer"
#define ANALYZER_SET_FRAME_TIME L"SA_SetTime"
#define ANALYZER_COPY_NEXT_SAMPLE L"SA_NextSample"
#define ANALYZER_COPY_LOOP L"SA_CopyLoop"
#define ANALYZER_APPEND_INPUT L"SA_AppendInput"
#define ANALYZER_CLEAR_INPUT L"SA_ClearInput"
#define ANALYZER_GET_RB_DATA L"SA_GetRB"
// Output queue events
#define QO_PUSH L"QO_Push"
#define QO_DISCONTINUITY L"QO_Discontinuity"
#define QO_POP L"QO_Pop"

// Application interface events
#define APP_GETFRAME L"App_GF"
#define APP_GF_TESTFRAME L"App_GF_Test"
#define APP_GF_FOUND L"App_GF_Found"
#define APP_GF_NOT_FOUND L"App_GF_NotFound"
#define APP_CONFIGURE L"App_Configure"
#define APP_SETLOGFSCALE L"App_SetLogFScale"
#define APP_SETLINEARSCALE L"App_SetLinearScale"

#ifdef _WIN32
#define AddPtrField(fields,name,ptr) fields->AddUInt32WithFormat(HStringReference(name).Get(),(UINT32)ptr,LoggingFieldFormat::LoggingFieldFormat_Hexadecimal);
#endif
#ifdef _WIN64
#define AddPtrField(fields,name,ptr) fields->AddUInt64WithFormat(HStringReference(name).Get(),(UINT64)ptr,LoggingFieldFormat::LoggingFieldFormat_Hexadecimal);
#endif

ComPtr<ILoggingTarget> Trace::g_spLogChannel;

HRESULT Trace::CreateLoggingFields(ComPtr<ILoggingFields> *ppFields)
{
	HRESULT hr = S_OK;
	HStringReference runtimeClassLoggingFields(RuntimeClass_Windows_Foundation_Diagnostics_LoggingFields);
	ComPtr<IActivationFactory> spFactory;
	hr = ABI::Windows::Foundation::GetActivationFactory(runtimeClassLoggingFields.Get(), &spFactory);
	if (FAILED(hr))
		return hr;
	ComPtr<IInspectable> object;
	spFactory->ActivateInstance(&object);
	
	hr = object.As(ppFields);
	return hr;
}

HRESULT Trace::GetMediaProperties(ComPtr<ILoggingFields> *pFields,IMFMediaType *pMediaType)
{
	using namespace ABI::Windows::Media::MediaProperties;

	HRESULT hr = CreateLoggingFields(pFields);

	if (pMediaType != nullptr)
	{
		Microsoft::WRL::ComPtr<IMediaEncodingProperties> spMediaProps;

		HRESULT hr = MFCreatePropertiesFromMediaType(pMediaType, IID_PPV_ARGS(&spMediaProps));
		if (FAILED(hr))
			return hr;
		
		HSTRING hsMediaType = 0;
		spMediaProps->get_Type(&hsMediaType);
		HSTRING hsMediaSubType = 0;
		spMediaProps->get_Subtype(&hsMediaSubType);
		(*pFields)->AddString(HStringReference(L"Type").Get(), hsMediaType);
		(*pFields)->AddString(HStringReference(L"SubType").Get(), hsMediaSubType);
		UINT32 samplesPerSecond = 0;
		hr = pMediaType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &samplesPerSecond);
		if (SUCCEEDED(hr))
			(*pFields)->AddUInt32(HStringReference(L"SampleRate").Get(), samplesPerSecond);
		UINT32 numChannels;
		hr = pMediaType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &numChannels);
		if (SUCCEEDED(hr))
			(*pFields)->AddUInt32(HStringReference(L"Channels").Get(), numChannels);
	}
	else
	{
		(*pFields)->AddEmpty(HStringReference(L"Type").Get());
	}
	return hr;
}

HRESULT Trace::Initialize()
{
	HRESULT hr = S_OK;
	// Only do it once
	if (g_spLogChannel == nullptr)
	{
		ComPtr<ILoggingChannelFactory2> spLogChannelFactory;
		
		hr = ABI::Windows::Foundation::GetActivationFactory(
			HStringReference(RuntimeClass_Windows_Foundation_Diagnostics_LoggingChannel).Get(), 
			&spLogChannelFactory);
		if (FAILED(hr))
			return hr;
		ComPtr<ILoggingChannel> spChannel;
		hr = spLogChannelFactory->CreateWithOptions(HStringReference(LOG_CHANNEL_NAME).Get(), nullptr, &spChannel);
		if (FAILED(hr))
			return hr;
		hr = spChannel.As(&g_spLogChannel);
	}
	return hr;
}

HRESULT Trace::Log_ProcessInput(ABI::Windows::Foundation::Diagnostics::ILoggingActivity **ppActivity,DWORD dwInputStreamID, IMFSample *pSample, DWORD dwFlags,REFERENCE_TIME presentationTime)
{
	ComPtr<ILoggingFields> fields;
	HRESULT hr = CreateLoggingFields(&fields);
	if (FAILED(hr))
		return hr;
	fields->AddUInt32(HStringReference(L"InputStreamId").Get(), dwInputStreamID);

	if (pSample != nullptr)
	{
		REFERENCE_TIME sampleTime = 0,duration = 0;
		DWORD dwSampleFlags = 0, dwBufferCount = 0;
		pSample->GetSampleTime(&sampleTime);
		pSample->GetSampleDuration(&duration);
		pSample->GetSampleFlags(&dwSampleFlags);
		pSample->GetBufferCount(&dwBufferCount);
		fields->AddTimeSpan(HStringReference(L"Time").Get(), ABI::Windows::Foundation::TimeSpan() = { sampleTime });
		fields->AddTimeSpan(HStringReference(L"Duration").Get(), ABI::Windows::Foundation::TimeSpan() = { duration });
		fields->AddUInt32(HStringReference(L"SampleFlags").Get(), dwSampleFlags);
		fields->AddUInt32(HStringReference(L"BufferCount").Get(), dwBufferCount);
	}
	else
	{
		fields->AddEmpty(HStringReference(L"InputStreamId").Get());
		fields->AddEmpty(HStringReference(L"Time").Get());
		fields->AddEmpty(HStringReference(L"Duration").Get());
		fields->AddEmpty(HStringReference(L"SampleFlags").Get());
	}
	fields->AddTimeSpan(HStringReference(L"PresentationTime").Get(), ABI::Windows::Foundation::TimeSpan() = { presentationTime });
	fields->AddUInt32(HStringReference(L"Flags").Get(), dwFlags);

	hr = g_spLogChannel->StartActivityWithFields(HStringReference(MFT_PROCESS_INPUT).Get(), fields.Get(), ppActivity);
	return hr;
}

HRESULT Trace::Log_ProcessOutput(ABI::Windows::Foundation::Diagnostics::ILoggingActivity **ppActivity,DWORD dwFlags, DWORD cOutputBufferCount,MFT_OUTPUT_DATA_BUFFER *pOutBuffer,REFERENCE_TIME presentationTime)
{
	ComPtr<ILoggingFields> fields;
	HRESULT hr = CreateLoggingFields(&fields);
	if (FAILED(hr))
		return hr;
	fields->AddUInt32(HStringReference(L"OutputBufferCount").Get(), cOutputBufferCount);
	fields->AddTimeSpan(HStringReference(L"PresentationTime").Get(), ABI::Windows::Foundation::TimeSpan() = { presentationTime });
	fields->AddUInt32(HStringReference(L"Flags").Get(), dwFlags);
	hr = g_spLogChannel->StartActivityWithFields(HStringReference(MFT_PROCESS_OUTPUT).Get(), fields.Get(), ppActivity);
	return hr;
}

HRESULT Trace::Log_ProcessEvent(DWORD dwStreamID, IMFMediaEvent * pEvent)
{
	ComPtr<ILoggingFields> fields;
	HRESULT hr = CreateLoggingFields(&fields);
	if (FAILED(hr))
		return hr;
	fields->AddUInt32(HStringReference(L"StreamId").Get(), dwStreamID);
	MediaEventType eventType;
	pEvent->GetType(&eventType);
	HRESULT hs;
	pEvent->GetStatus(&hs);
	fields->AddUInt32(HStringReference(L"Type").Get(), eventType);
	fields->AddUInt32WithFormat(HStringReference(L"Status").Get(), hs,LoggingFieldFormat::LoggingFieldFormat_HResult);

	hr = g_spLogChannel->LogEventWithFields(HStringReference(MFT_PROCESS_EVENT).Get(), fields.Get());
	return hr;
}

HRESULT Trace::Log_ProcessMessage(MFT_MESSAGE_TYPE msgType, ULONG_PTR ulParam)
{
	ComPtr<ILoggingFields> fields;
	HRESULT hr = CreateLoggingFields(&fields);
	if (FAILED(hr))
		return hr;
	fields->AddUInt32(HStringReference(L"Type").Get(), msgType);
	fields->AddUInt64(HStringReference(L"Param").Get(), ulParam);

	hr = g_spLogChannel->LogEventWithFields(HStringReference(MFT_PROCESS_MESSAGE).Get(), fields.Get());
	return hr;
}

HRESULT Trace::Log_TestFrame(REFERENCE_TIME presentationTime, REFERENCE_TIME frameStart, REFERENCE_TIME frameEnd)
{
	ComPtr<ILoggingFields> fields;
	HRESULT hr = CreateLoggingFields(&fields);
	if (FAILED(hr))
		return hr;
	fields->AddTimeSpan(HStringReference(L"Time").Get(), ABI::Windows::Foundation::TimeSpan() = { presentationTime });
	fields->AddTimeSpan(HStringReference(L"Start").Get(), ABI::Windows::Foundation::TimeSpan() = { frameStart });
	fields->AddTimeSpan(HStringReference(L"End").Get(), ABI::Windows::Foundation::TimeSpan() = { frameEnd });

	return g_spLogChannel->LogEventWithFields(HStringReference(APP_GF_TESTFRAME).Get(), fields.Get());
}

HRESULT Trace::Log_SetInputType(DWORD dwStreamID, IMFMediaType *pMediaType)
{
	ComPtr<ILoggingFields> fields;
	HRESULT hr = GetMediaProperties(&fields,pMediaType);
	if (FAILED(hr))
		return hr;
	fields->AddUInt32(HStringReference(L"StreamId").Get(), dwStreamID);
	hr = g_spLogChannel->LogEventWithFields(HStringReference(MFT_SET_INPUT_TYPE).Get(), fields.Get());
	return hr;
}

HRESULT Trace::Log_SetOutputType(DWORD dwStreamID, IMFMediaType *pMediaType)
{
	ComPtr<ILoggingFields> fields;
	HRESULT hr = GetMediaProperties(&fields, pMediaType);
	if (FAILED(hr))
		return hr;
	fields->AddUInt32(HStringReference(L"StreamId").Get(), dwStreamID);
	hr = g_spLogChannel->LogEventWithFields(HStringReference(MFT_SET_OUTPUT_TYPE).Get(), fields.Get());
	return hr;
}

HRESULT Trace::Log_ConfigureAnalyzer(UINT32 samplesPerAnalyzerOutputFrame, UINT32 overlap, UINT32 fftLength, HRESULT hConfigureResult)
{
	ComPtr<ILoggingFields> fields;
	HRESULT hr = CreateLoggingFields(&fields);
	if (FAILED(hr))
		return hr;
	fields->AddUInt32(HStringReference(L"SamplesPerFrame").Get(), samplesPerAnalyzerOutputFrame);
	fields->AddUInt32(HStringReference(L"Overlap").Get(), overlap);
	fields->AddUInt32(HStringReference(L"FFTLength").Get(), fftLength);
	fields->AddUInt32WithFormat(HStringReference(L"Result").Get(), hConfigureResult, LoggingFieldFormat::LoggingFieldFormat_HResult);

	hr = g_spLogChannel->LogEventWithFields(HStringReference(ANALYZER_CONFIGURE).Get(), fields.Get());
	return hr;
}


HRESULT Trace::Log_StartGetFrame(ABI::Windows::Foundation::Diagnostics::ILoggingActivity **ppActivity, REFERENCE_TIME presentationTime,size_t queueSize)
{
	ComPtr<ILoggingFields> fields;
	HRESULT hr = CreateLoggingFields(&fields);
	if (FAILED(hr))
		return hr;

	fields->AddTimeSpan(HStringReference(L"PresentationTime").Get(), ABI::Windows::Foundation::TimeSpan() = { presentationTime });
	fields->AddUInt32(HStringReference(L"Queue.size").Get(),queueSize);
	return g_spLogChannel->StartActivityWithFields(HStringReference(APP_GETFRAME).Get(), fields.Get(), ppActivity);
}

HRESULT Trace::Log_FrameFound(REFERENCE_TIME start, REFERENCE_TIME duration)
{
	ComPtr<ILoggingFields> fields;
	HRESULT hr = CreateLoggingFields(&fields);
	if (FAILED(hr))
		return hr;
	fields->AddTimeSpan(HStringReference(L"Time").Get(), ABI::Windows::Foundation::TimeSpan() = { start });
	fields->AddTimeSpan(HStringReference(L"Duration").Get(), ABI::Windows::Foundation::TimeSpan() = { duration });

	return g_spLogChannel->LogEventWithFields(HStringReference(APP_GF_FOUND).Get(), fields.Get());
}

HRESULT Trace::Log_FrameNotFound()
{
	return g_spLogChannel->LogEvent(HStringReference(APP_GF_NOT_FOUND).Get());
}

HRESULT Trace::Log_StartAnalyzerStep(ABI::Windows::Foundation::Diagnostics::ILoggingActivity ** ppActivity)
{
	return g_spLogChannel->StartActivity(HStringReference(ANALYZER_STEP).Get(), ppActivity);
}

HRESULT Trace::Log_StopAnalyzerStep(ABI::Windows::Foundation::Diagnostics::ILoggingActivity * pActivity, REFERENCE_TIME time, HRESULT hResult)
{
	ComPtr<ILoggingFields> fields;
	HRESULT hr = CreateLoggingFields(&fields);
	if (FAILED(hr))
		return hr;

	fields->AddTimeSpan(HStringReference(L"Time").Get(), ABI::Windows::Foundation::TimeSpan() = { time });
	fields->AddUInt32WithFormat(HStringReference(L"Result").Get(), hResult, LoggingFieldFormat::LoggingFieldFormat_HResult);
	ComPtr<ILoggingActivity> spActivity = pActivity;
	ComPtr<ILoggingActivity2> spActivity2;
	spActivity.As(&spActivity2);
	HSTRING hName;
	spActivity->get_Name(&hName);
	return spActivity2->StopActivityWithFields(hName, fields.Get());
}

HRESULT Trace::Log_BeginAnalysis()
{
	return g_spLogChannel->LogEvent(HStringReference(ANALYZER_SCHEDULE).Get());
}

HRESULT Trace::Log_AnalysisAlreadyRunning()
{
	return g_spLogChannel->LogEvent(HStringReference(ANALYZER_ALREADYRUNNING).Get());
}

HRESULT Trace::Log_Configure(float outFrameRate, float overlapPercentage, unsigned fftLength)
{
	ComPtr<ILoggingFields> fields;
	HRESULT hr = CreateLoggingFields(&fields);
	if (FAILED(hr))
		return hr;

	fields->AddSingle(HStringReference(L"OutFrameRate").Get(),outFrameRate);
	fields->AddSingle(HStringReference(L"Overlap").Get(), overlapPercentage);
	fields->AddUInt32(HStringReference(L"FFTLength").Get(), fftLength);
	return g_spLogChannel->LogEventWithFields(HStringReference(APP_CONFIGURE).Get(), fields.Get());
}
HRESULT Trace::Log_SetLogFScale(float lowFrequency, float highFrequency, unsigned outElementCount)
{
	ComPtr<ILoggingFields> fields;
	HRESULT hr = CreateLoggingFields(&fields);
	if (FAILED(hr))
		return hr;

	fields->AddSingle(HStringReference(L"LowFrequency").Get(), lowFrequency);
	fields->AddSingle(HStringReference(L"HighFrequency").Get(), highFrequency);
	fields->AddUInt32(HStringReference(L"OutElementCount").Get(), outElementCount);
	return g_spLogChannel->LogEventWithFields(HStringReference(APP_SETLOGFSCALE).Get(), fields.Get());

}
HRESULT Trace::Log_SetLinearScale(size_t bins)
{
	ComPtr<ILoggingFields> fields;
	HRESULT hr = CreateLoggingFields(&fields);
	if (FAILED(hr))
		return hr;

	fields->AddUInt32(HStringReference(L"NumberOfElements").Get(), bins);
	return g_spLogChannel->LogEventWithFields(HStringReference(APP_SETLINEARSCALE).Get(),fields.Get());
}

HRESULT Trace::Log_StartOutputQueuePush(ABI::Windows::Foundation::Diagnostics::ILoggingActivity ** ppActivity,REFERENCE_TIME time)
{
	ComPtr<ILoggingFields> fields;
	HRESULT hr = CreateLoggingFields(&fields);
	if (FAILED(hr))
		return hr;

	fields->AddTimeSpan(HStringReference(L"Time").Get(), ABI::Windows::Foundation::TimeSpan() = { time });

	return g_spLogChannel->StartActivityWithFields(HStringReference(QO_PUSH).Get(), fields.Get(), ppActivity);
}

HRESULT Trace::Log_SA_Start_AppendInput(ABI::Windows::Foundation::Diagnostics::ILoggingActivity **ppActivity, 
	REFERENCE_TIME sampleTime, size_t sampleCount,size_t samplesInBuffer, void *pWritePtr, void *pReadPtr, long inputSampleOffset, long expectedOffset)
{
	ComPtr<ILoggingFields> fields;
	HRESULT hr = CreateLoggingFields(&fields);
	if (FAILED(hr))
		return hr;

	fields->AddTimeSpan(HStringReference(L"Time").Get(), ABI::Windows::Foundation::TimeSpan() = { sampleTime });
	fields->AddUInt32(HStringReference(L"SampleSize").Get(), sampleCount);
	fields->AddUInt32(HStringReference(L"BufferSize").Get(), samplesInBuffer);
	fields->AddUInt32WithFormat(HStringReference(L"pWrite").Get(), (UINT32)pWritePtr, LoggingFieldFormat::LoggingFieldFormat_Hexadecimal);
	fields->AddUInt32WithFormat(HStringReference(L"pRead").Get(), (UINT32)pReadPtr, LoggingFieldFormat::LoggingFieldFormat_Hexadecimal);
	fields->AddInt64(HStringReference(L"Offset").Get(), inputSampleOffset);
	fields->AddInt64(HStringReference(L"BufferOffset").Get(), expectedOffset);
	return g_spLogChannel->StartActivityWithFields(HStringReference(ANALYZER_APPEND_INPUT).Get(), fields.Get(), ppActivity);

}
HRESULT Trace::Log_SA_Stop_AppendInput(ABI::Windows::Foundation::Diagnostics::ILoggingActivity *pActivity, 
	REFERENCE_TIME sampleTime, size_t sampleSize, size_t samplesInBuffer, void *pWritePtr, void *pReadPtr, long expectedOffset)
{
	ComPtr<ILoggingFields> fields;
	HRESULT hr = CreateLoggingFields(&fields);
	if (FAILED(hr))
		return hr;

	fields->AddTimeSpan(HStringReference(L"Time").Get(), ABI::Windows::Foundation::TimeSpan() = { sampleTime });
	fields->AddUInt32(HStringReference(L"SampleLength").Get(), sampleSize);
	fields->AddUInt32(HStringReference(L"BufferSize").Get(), samplesInBuffer);
	fields->AddUInt32WithFormat(HStringReference(L"pWrite").Get(), (UINT32)pWritePtr, LoggingFieldFormat::LoggingFieldFormat_Hexadecimal);
	fields->AddUInt32WithFormat(HStringReference(L"pRead").Get(), (UINT32)pReadPtr, LoggingFieldFormat::LoggingFieldFormat_Hexadecimal);
	fields->AddInt64(HStringReference(L"BufferOffset").Get(), expectedOffset);

	ComPtr<ILoggingActivity> spActivity = pActivity;
	ComPtr<ILoggingActivity2> spActivity2;
	spActivity.As(&spActivity2);
	HSTRING hName;
	spActivity->get_Name(&hName);
	return spActivity2->StopActivityWithFields(hName, fields.Get());
}
HRESULT Trace::Log_SA_ClearInputBuffer()
{
	return g_spLogChannel->LogEvent(HStringReference(ANALYZER_CLEAR_INPUT).Get());
}

HRESULT Trace::Log_StartCopyRBData(ABI::Windows::Foundation::Diagnostics::ILoggingActivity **ppActivity, 
	size_t bufferSize, const void *pReadPtr, const void *pWritePtr)
{
	ComPtr<ILoggingFields> fields;
	HRESULT hr = CreateLoggingFields(&fields);
	if (FAILED(hr))
		return hr;

	fields->AddUInt32(HStringReference(L"BufferSize").Get(), bufferSize);
	fields->AddUInt32WithFormat(HStringReference(L"pWrite").Get(), (UINT32)pWritePtr, LoggingFieldFormat::LoggingFieldFormat_Hexadecimal);
	fields->AddUInt32WithFormat(HStringReference(L"pRead").Get(), (UINT32)pReadPtr, LoggingFieldFormat::LoggingFieldFormat_Hexadecimal);
	return g_spLogChannel->StartActivityWithFields(HStringReference(ANALYZER_GET_RB_DATA).Get(), fields.Get(), ppActivity);
}
HRESULT Trace::Log_StopCopyRBData(ABI::Windows::Foundation::Diagnostics::ILoggingActivity *pActivity, bool bSuccess, size_t bufferSize, const void *pReadPtr, const void *pWritePtr)
{
	ComPtr<ILoggingFields> fields;
	HRESULT hr = CreateLoggingFields(&fields);
	if (FAILED(hr))
		return hr;

	fields->AddBoolean(HStringReference(L"Success").Get(), bSuccess);
	fields->AddUInt32(HStringReference(L"BufferSize").Get(), bufferSize);
	fields->AddUInt32WithFormat(HStringReference(L"pWrite").Get(), (UINT32)pWritePtr, LoggingFieldFormat::LoggingFieldFormat_Hexadecimal);
	fields->AddUInt32WithFormat(HStringReference(L"pRead").Get(), (UINT32)pReadPtr, LoggingFieldFormat::LoggingFieldFormat_Hexadecimal);

	ComPtr<ILoggingActivity> spActivity = pActivity;
	ComPtr<ILoggingActivity2> spActivity2;
	spActivity.As(&spActivity2);
	HSTRING hName;
	spActivity->get_Name(&hName);
	return spActivity2->StopActivityWithFields(hName, fields.Get());

}
