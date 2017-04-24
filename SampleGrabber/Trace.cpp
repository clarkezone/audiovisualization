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

// Analyzer events
#define ANALYZER_CONFIGURE L"SA_Configure"
#define ANALYZER_STEP L"SA_Step"
#define ANALYZER_SCHEDULE L"SA_Schedule"
#define ANALYZER_ALREADYRUNNING L"SA_AlreadyRunning"
#define ANALYZER_COPY_DATA L"SA_CopyInput"
#define ANALYZER_LOCK_INPUT_BUFFER L"SA_LockBuffer"
#define ANALYZER_SET_FRAME_TIME L"SA_SetTime"
#define ANALYZER_COPY_NEXT_SAMPLE L"SA_NextSample"

// Input queue events
#define QI_PUSH L"QI_Push"
#define QI_DISCONTINUITY L"QI_Discontinuity"

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

HRESULT Trace::Log_QueueInput(IMFSample * pSample, HRESULT hResult)
{
	ComPtr<ILoggingFields> fields;
	HRESULT hr = CreateLoggingFields(&fields);
	if (FAILED(hr))
		return hr;

	REFERENCE_TIME sampleTime = 0, duration = 0;
	DWORD sampleFlags = 0;
	pSample->GetSampleTime(&sampleTime);
	pSample->GetSampleDuration(&duration);
	pSample->GetSampleFlags(&sampleFlags);
	UINT32 discontinuity = 0;
	pSample->GetUINT32(MFSampleExtension_Discontinuity, &discontinuity);
	fields->AddTimeSpan(HStringReference(L"Time").Get(), ABI::Windows::Foundation::TimeSpan() = { sampleTime });
	fields->AddTimeSpan(HStringReference(L"Duration").Get(), ABI::Windows::Foundation::TimeSpan() = { duration });
	fields->AddUInt32(HStringReference(L"SampleFlags").Get(), sampleFlags);
	fields->AddBoolean(HStringReference(L"Discontinuity").Get(), discontinuity != 0);
	fields->AddUInt32WithFormat(HStringReference(L"Result").Get(), hResult, LoggingFieldFormat::LoggingFieldFormat_HResult);
	
	hr = g_spLogChannel->LogEventWithFields(HStringReference(QI_PUSH).Get(), fields.Get());

	return hr;
}

HRESULT Trace::Log_InputDiscontinuity()
{
	return g_spLogChannel->LogEvent(HStringReference(QI_DISCONTINUITY).Get());
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

HRESULT Trace::Log_TestFrame(REFERENCE_TIME currentTime, REFERENCE_TIME start, REFERENCE_TIME duration)
{
	ComPtr<ILoggingFields> fields;
	HRESULT hr = CreateLoggingFields(&fields);
	if (FAILED(hr))
		return hr;
	fields->AddTimeSpan(HStringReference(L"Time").Get(), ABI::Windows::Foundation::TimeSpan() = { currentTime });
	fields->AddTimeSpan(HStringReference(L"Start").Get(), ABI::Windows::Foundation::TimeSpan() = { start });
	fields->AddTimeSpan(HStringReference(L"End").Get(), ABI::Windows::Foundation::TimeSpan() = { start + duration });

	return g_spLogChannel->LogEventWithFields(HStringReference(APP_GF_TESTFRAME).Get(), fields.Get());
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
HRESULT Trace::Log_SetLinearScale()
{
	return g_spLogChannel->LogEvent(HStringReference(APP_SETLINEARSCALE).Get());
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

HRESULT Trace::Log_OutputQueuePop(REFERENCE_TIME time, REFERENCE_TIME duration, size_t queueSize)
{
	ComPtr<ILoggingFields> fields;
	HRESULT hr = CreateLoggingFields(&fields);
	if (FAILED(hr))
		return hr;

	fields->AddTimeSpan(HStringReference(L"Time").Get(), ABI::Windows::Foundation::TimeSpan() = { time });
	fields->AddTimeSpan(HStringReference(L"Duration").Get(), ABI::Windows::Foundation::TimeSpan() = { duration });
	fields->AddUInt32(HStringReference(L"QueueSize").Get(), queueSize);

	return g_spLogChannel->LogEventWithFields(HStringReference(QO_POP).Get(), fields.Get());
}

HRESULT Trace::Log_OutputQueueDiscontinuity(REFERENCE_TIME time, REFERENCE_TIME expected)
{
	ComPtr<ILoggingFields> fields;
	HRESULT hr = CreateLoggingFields(&fields);
	if (FAILED(hr))
		return hr;

	fields->AddTimeSpan(HStringReference(L"Time").Get(), ABI::Windows::Foundation::TimeSpan() = { time });
	fields->AddTimeSpan(HStringReference(L"Expected").Get(), ABI::Windows::Foundation::TimeSpan() = { expected });

	return g_spLogChannel->LogEventWithFields(HStringReference(QO_DISCONTINUITY).Get(), fields.Get());
}

HRESULT Trace::Log_StartCopyDataFromInput(ABI::Windows::Foundation::Diagnostics::ILoggingActivity ** ppActivity, 
	size_t copiedFrameCount, size_t stepFrameCount, size_t currentBufferIndex)
{
	ComPtr<ILoggingFields> fields;
	HRESULT hr = CreateLoggingFields(&fields);
	if (FAILED(hr))
		return hr;

	fields->AddUInt32(HStringReference(L"CopiedFrames").Get(), copiedFrameCount);
	fields->AddUInt32(HStringReference(L"Step").Get(), stepFrameCount);
	fields->AddUInt32(HStringReference(L"BufferIndex").Get(), currentBufferIndex);
	return g_spLogChannel->StartActivityWithFields(HStringReference(ANALYZER_COPY_DATA).Get(),fields.Get(),ppActivity);
}

HRESULT Trace::Log_StopCopyDataFromInput(ABI::Windows::Foundation::Diagnostics::ILoggingActivity * pActivity, size_t copiedFrameCount, size_t currentBufferIndex, unsigned line, HRESULT result)
{
	ComPtr<ILoggingFields> fields;
	HRESULT hr = CreateLoggingFields(&fields);
	if (FAILED(hr))
		return hr;

	fields->AddUInt32(HStringReference(L"CopiedFrames").Get(), copiedFrameCount);
	fields->AddUInt32(HStringReference(L"BufferIndex").Get(), currentBufferIndex);
	fields->AddUInt32WithFormat(HStringReference(L"HResult").Get(), result, LoggingFieldFormat::LoggingFieldFormat_HResult);
	fields->AddUInt32(HStringReference(L"Line").Get(), line);

	ComPtr<ILoggingActivity> spActivity = pActivity;
	ComPtr<ILoggingActivity2> spActivity2;
	spActivity.As(&spActivity2);
	HSTRING hName;
	spActivity->get_Name(&hName);
	return spActivity2->StopActivityWithFields(hName, fields.Get());
}

HRESULT Trace::Log_LockInputBuffer(IMFMediaBuffer * pBuffer)
{
	ComPtr<ILoggingFields> fields;
	HRESULT hr = CreateLoggingFields(&fields);
	if (FAILED(hr))
		return hr;

	if (pBuffer != nullptr)
	{
		DWORD dwLength = 0, dwMaxLength = 0;
		pBuffer->GetCurrentLength(&dwLength);
		pBuffer->GetMaxLength(&dwMaxLength);
		fields->AddUInt32(HStringReference(L"Samples").Get(), dwLength / sizeof(float));
		fields->AddUInt32(HStringReference(L"MaxSamples").Get(), dwMaxLength / sizeof(float));
	}
	else
	{
		fields->AddEmpty(HStringReference(L"Samples").Get());
		fields->AddEmpty(HStringReference(L"MaxSamples").Get());
	}

	return g_spLogChannel->LogEventWithFields(HStringReference(ANALYZER_LOCK_INPUT_BUFFER).Get(), fields.Get());
}

HRESULT Trace::Log_SetCopiedFrameTime(REFERENCE_TIME time)
{
	ComPtr<ILoggingFields> fields;
	HRESULT hr = CreateLoggingFields(&fields);
	if (FAILED(hr))
		return hr;

	fields->AddTimeSpan(HStringReference(L"Time").Get(), ABI::Windows::Foundation::TimeSpan() = { time });

	return g_spLogChannel->LogEventWithFields(HStringReference(ANALYZER_SET_FRAME_TIME).Get(), fields.Get());
}

HRESULT Trace::Log_CopyNextSample()
{
	return g_spLogChannel->LogEvent(HStringReference(ANALYZER_COPY_NEXT_SAMPLE).Get());
}
