#include "pch.h"
#include "Trace.h"
#include <wrl.h>
#include <mfidl.h>
#include <windows.foundation.diagnostics.h>
#include <windows.media.mediaproperties.h>

using namespace Windows::Foundation::Diagnostics;

#define LOG_CHANNEL_NAME "AudioVisualizer-SampleGrabber"

Windows::Foundation::Diagnostics::LoggingChannel ^g_LogChannel = nullptr;

Windows::Foundation::Diagnostics::LoggingFields ^ Trace::GetMediaProperties(IMFMediaType *pMediaType)
{
	using namespace Windows::Media::MediaProperties;

	LoggingFields ^fields = ref new LoggingFields();
	if (pMediaType != nullptr)
	{
		Microsoft::WRL::ComPtr<ABI::Windows::Media::MediaProperties::IMediaEncodingProperties> spMediaProps;

		HRESULT hr = MFCreatePropertiesFromMediaType(pMediaType, IID_PPV_ARGS(&spMediaProps));
		if (FAILED(hr))
			throw ref new Platform::COMException(hr);

		IMediaEncodingProperties ^mediaProperties = reinterpret_cast<IMediaEncodingProperties ^>(spMediaProps.Get());

		fields->AddString("Type", mediaProperties->Type);
		fields->AddString("Subtype", mediaProperties->Subtype);

		if (mediaProperties->Type == "Audio")
		{
			// As this should be audio format we can cast to audio encoding properties
			AudioEncodingProperties ^props = reinterpret_cast<AudioEncodingProperties ^>(spMediaProps.Get());
			fields->AddUInt32("SampleRate", props->SampleRate);
			fields->AddUInt32("Channles", props->ChannelCount);
			fields->AddUInt32("BitsPerSample", props->BitsPerSample);
		}
	}
	else
	{
		fields->AddEmpty("Type");
	}
	return fields;
}

void Trace::Initialize()
{
	// Only do it once
	if (g_LogChannel == nullptr)
		g_LogChannel = ref new LoggingChannel(LOG_CHANNEL_NAME, nullptr);

}

LoggingActivity ^Trace::Log_ProcessInput(DWORD dwInputStreamID, IMFSample *pSample, DWORD dwFlags,REFERENCE_TIME presentationTime)
{
	LoggingFields ^fields = ref new LoggingFields();
	fields->AddUInt32("InputStreamId", dwInputStreamID);
	if (pSample != nullptr)
	{
		REFERENCE_TIME sampleTime = 0,duration = 0;
		DWORD sampleFlags = 0;
		pSample->GetSampleTime(&sampleTime);
		pSample->GetSampleDuration(&duration);
		pSample->GetSampleFlags(&sampleFlags);
		fields->AddTimeSpan("Time", Windows::Foundation::TimeSpan() = { sampleTime });
		fields->AddTimeSpan("Duration", Windows::Foundation::TimeSpan() = { duration });
		fields->AddUInt32("SampleFlags", sampleFlags);
	}
	else
	{
		fields->AddEmpty("Time");
		fields->AddEmpty("Duration");
		fields->AddEmpty("SampleFlags");
	}
	fields->AddTimeSpan("PresentationTime", Windows::Foundation::TimeSpan() = { presentationTime });
	fields->AddUInt32("Flags", dwFlags);

	return g_LogChannel->StartActivity("ProcessInput", fields);
}

LoggingActivity ^Trace::Log_ProcessOutput(DWORD dwFlags, DWORD cOutputBufferCount,REFERENCE_TIME presentationTime)
{
	LoggingFields ^fields = ref new LoggingFields();
	fields->AddUInt32("OutputBufferCount", cOutputBufferCount);
	fields->AddUInt32("Flags", dwFlags);
	fields->AddTimeSpan("PresentationTime", Windows::Foundation::TimeSpan() = { presentationTime });

	return g_LogChannel->StartActivity("ProcessOutput", fields);
}

void Trace::Log_SetInputType(DWORD dwStreamID, IMFMediaType *pMediaType)
{
	LoggingFields ^fields = GetMediaProperties(pMediaType);
	g_LogChannel->LogEvent("SetInputType", fields);
}

void Trace::Log_SetOutputType(DWORD dwStreamID, IMFMediaType *pMediaType)
{
	LoggingFields ^fields = GetMediaProperties(pMediaType);
	g_LogChannel->LogEvent("SetOutputType", fields);
}
