#include "pch.h"
#include "Trace.h"
#include <wrl.h>
#include <mfidl.h>
#include <windows.foundation.diagnostics.h>
#include <windows.media.mediaproperties.h>

using namespace ABI::Windows::Foundation::Diagnostics;
using namespace Microsoft::WRL;

#define LOG_CHANNEL_NAME L"AudioVisualizer-SampleGrabber"

ComPtr<ILoggingTarget> g_spLogChannel;

HRESULT Trace::CreateLoggingFields(ComPtr<ILoggingFields> *ppFields)
{
	using namespace Wrappers;
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
	using namespace Wrappers;

	HRESULT hr = CreateLoggingFields(pFields);

	if (pMediaType != nullptr)
	{
		Microsoft::WRL::ComPtr<IMediaEncodingProperties> spMediaProps;

		HRESULT hr = MFCreatePropertiesFromMediaType(pMediaType, IID_PPV_ARGS(&spMediaProps));
		if (FAILED(hr))
			throw ref new Platform::COMException(hr);
		
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
	using namespace Wrappers;
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
	using namespace Wrappers;
	ComPtr<ILoggingFields> fields;
	HRESULT hr = CreateLoggingFields(&fields);
	if (FAILED(hr))
		return hr;
	fields->AddUInt32(HStringReference(L"InputStreamId").Get(), dwInputStreamID);

	if (pSample != nullptr)
	{
		REFERENCE_TIME sampleTime = 0,duration = 0;
		DWORD sampleFlags = 0;
		pSample->GetSampleTime(&sampleTime);
		pSample->GetSampleDuration(&duration);
		pSample->GetSampleFlags(&sampleFlags);
		fields->AddTimeSpan(HStringReference(L"Time").Get(), ABI::Windows::Foundation::TimeSpan() = { sampleTime });
		fields->AddTimeSpan(HStringReference(L"Duration").Get(), ABI::Windows::Foundation::TimeSpan() = { duration });
		fields->AddUInt32(HStringReference(L"SampleFlags").Get(), sampleFlags);
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

	hr = g_spLogChannel->StartActivityWithFields(HStringReference(L"ProcessInput").Get(), fields.Get(), ppActivity);
	return hr;
}

HRESULT Trace::Log_ProcessOutput(ABI::Windows::Foundation::Diagnostics::ILoggingActivity **ppActivity,DWORD dwFlags, DWORD cOutputBufferCount,REFERENCE_TIME presentationTime)
{
	using namespace Wrappers;
	ComPtr<ILoggingFields> fields;
	HRESULT hr = CreateLoggingFields(&fields);
	if (FAILED(hr))
		return hr;
	fields->AddUInt32(HStringReference(L"OutputBufferCount").Get(), cOutputBufferCount);
	fields->AddTimeSpan(HStringReference(L"PresentationTime").Get(), ABI::Windows::Foundation::TimeSpan() = { presentationTime });
	fields->AddUInt32(HStringReference(L"Flags").Get(), dwFlags);
	hr = g_spLogChannel->StartActivityWithFields(HStringReference(L"ProcessOutput").Get(), fields.Get(), ppActivity);
	return hr;
}

HRESULT Trace::Log_SetInputType(DWORD dwStreamID, IMFMediaType *pMediaType)
{
	using namespace Wrappers;
	ComPtr<ILoggingFields> fields;
	HRESULT hr = GetMediaProperties(&fields,pMediaType);
	if (FAILED(hr))
		return hr;
	fields->AddUInt32(HStringReference(L"StreamId").Get(), dwStreamID);
	hr = g_spLogChannel->LogEventWithFields(HStringReference(L"SetInputType").Get(), fields.Get());
	return hr;
}

HRESULT Trace::Log_SetOutputType(DWORD dwStreamID, IMFMediaType *pMediaType)
{
	using namespace Wrappers;
	ComPtr<ILoggingFields> fields;
	HRESULT hr = GetMediaProperties(&fields, pMediaType);
	if (FAILED(hr))
		return hr;
	fields->AddUInt32(HStringReference(L"StreamId").Get(), dwStreamID);
	hr = g_spLogChannel->LogEventWithFields(HStringReference(L"SetInputType").Get(), fields.Get());
	return hr;
}
