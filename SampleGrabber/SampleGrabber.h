#pragma once
#include "SampleGrabber_h.h"

#include <mfapi.h>
#include <mftransform.h>
#include <mfidl.h>
#include <mferror.h>

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mfplat.lib")

#include <wrl\implements.h>
#include <wrl\module.h>
#include <windows.media.h>
#include <wrl\wrappers\corewrappers.h>
#include "DirectXMath.h"
#include <malloc.h>
#include "MFHelpers.h"
#include "SpectrumAnalyzer.h"
#include "AsyncCallback.h"

class CSampleGrabber
	: public Microsoft::WRL::RuntimeClass<
	Microsoft::WRL::RuntimeClassFlags< Microsoft::WRL::RuntimeClassType::WinRtClassicComMix >,
	ABI::Windows::Media::IMediaExtension, ABI::SampleGrabber::IMyInterface,
	IMFTransform,IMFClockConsumer >
{
	InspectableClass(RuntimeClass_SampleGrabber_SampleGrabberTransform, BaseTrust)

public:
	CSampleGrabber();

	~CSampleGrabber();

	STDMETHOD(RuntimeClassInitialize)();

	// IMediaExtension
	STDMETHODIMP SetProperties(ABI::Windows::Foundation::Collections::IPropertySet *pConfiguration);

	// IMyInterface
	STDMETHODIMP GetVector(ABI::Windows::Foundation::Collections::IVectorView<ABI::SampleGrabber::Data> **pConfiguration);

	STDMETHODIMP GetSingleData(ABI::SampleGrabber::Data* result);

	STDMETHODIMP GetFrame(ABI::Windows::Media::IAudioFrame **pResult);

	STDMETHODIMP Configure(float outputSampleRate, float overlapPercent, unsigned long fftLength);
	STDMETHODIMP SetLogFScale(float lowFrequency, float highFrequency, unsigned long numberOfBins);
	STDMETHODIMP SetLinearFScale(unsigned long numberOfBins);
	STDMETHODIMP get_IsLogFrequencyScale(boolean *pResult)
	{
		if (pResult == nullptr)
			return E_FAIL;

		*pResult = m_bIsLogFScale;
		return S_OK;
	}

	STDMETHODIMP get_LowFrequency(float *pResult)
	{
		if (pResult == nullptr)
			return E_FAIL;

		*pResult = m_fLowFrequency;
		return S_OK;
	}
	STDMETHODIMP get_HighFrequency(float *pResult)
	{
		if (pResult == nullptr)
			return E_FAIL;

		*pResult = m_fHighFrequency;
		return S_OK;
	}
	STDMETHODIMP get_FrequencyStep(float *pResult)
	{
		if (pResult == nullptr)
			return E_FAIL;

		*pResult = m_fFrequencyStep;
		return S_OK;
	}
	STDMETHODIMP get_Channels(unsigned long *pResult)
	{
		if (pResult == nullptr)
			return E_FAIL;

		*pResult = m_Channels;
		return S_OK;
	}
	STDMETHODIMP get_FrequencyBinCount(unsigned long *pResult)
	{
		if (pResult == nullptr)
			return E_FAIL;

		*pResult = m_FFTLength;
		return S_OK;
	}



	// IMFTransform
	STDMETHODIMP GetStreamLimits(
		DWORD   *pdwInputMinimum,
		DWORD   *pdwInputMaximum,
		DWORD   *pdwOutputMinimum,
		DWORD   *pdwOutputMaximum
		);

	STDMETHODIMP GetStreamCount(
		DWORD   *pcInputStreams,
		DWORD   *pcOutputStreams
		);

	STDMETHODIMP GetStreamIDs(
		DWORD   dwInputIDArraySize,
		DWORD   *pdwInputIDs,
		DWORD   dwOutputIDArraySize,
		DWORD   *pdwOutputIDs
		);

	STDMETHODIMP GetInputStreamInfo(
		DWORD                     dwInputStreamID,
		MFT_INPUT_STREAM_INFO *   pStreamInfo
		);

	STDMETHODIMP GetOutputStreamInfo(
		DWORD                     dwOutputStreamID,
		MFT_OUTPUT_STREAM_INFO *  pStreamInfo
		);

	STDMETHODIMP GetAttributes(IMFAttributes** pAttributes);

	STDMETHODIMP GetInputStreamAttributes(
		DWORD           dwInputStreamID,
		IMFAttributes   **ppAttributes
		);

	STDMETHODIMP GetOutputStreamAttributes(
		DWORD           dwOutputStreamID,
		IMFAttributes   **ppAttributes
		);

	STDMETHODIMP DeleteInputStream(DWORD dwStreamID);

	STDMETHODIMP AddInputStreams(
		DWORD   cStreams,
		DWORD   *adwStreamIDs
		);

	STDMETHODIMP GetInputAvailableType(
		DWORD           dwInputStreamID,
		DWORD           dwTypeIndex, // 0-based
		IMFMediaType    **ppType
		);

	STDMETHODIMP GetOutputAvailableType(
		DWORD           dwOutputStreamID,
		DWORD           dwTypeIndex, // 0-based
		IMFMediaType    **ppType
		);

	STDMETHODIMP SetInputType(
		DWORD           dwInputStreamID,
		IMFMediaType    *pType,
		DWORD           dwFlags
		);

	STDMETHODIMP SetOutputType(
		DWORD           dwOutputStreamID,
		IMFMediaType    *pType,
		DWORD           dwFlags
		);

	STDMETHODIMP GetInputCurrentType(
		DWORD           dwInputStreamID,
		IMFMediaType    **ppType
		);

	STDMETHODIMP GetOutputCurrentType(
		DWORD           dwOutputStreamID,
		IMFMediaType    **ppType
		);

	STDMETHODIMP GetInputStatus(
		DWORD           dwInputStreamID,
		DWORD           *pdwFlags
		);

	STDMETHODIMP GetOutputStatus(DWORD *pdwFlags);

	STDMETHODIMP SetOutputBounds(
		LONGLONG        hnsLowerBound,
		LONGLONG        hnsUpperBound
		);

	STDMETHODIMP ProcessEvent(
		DWORD              dwInputStreamID,
		IMFMediaEvent      *pEvent
		);

	STDMETHODIMP ProcessMessage(
		MFT_MESSAGE_TYPE    eMessage,
		ULONG_PTR           ulParam
		);

	STDMETHODIMP ProcessInput(
		DWORD               dwInputStreamID,
		IMFSample           *pSample,
		DWORD               dwFlags
		);

	STDMETHODIMP ProcessOutput(
		DWORD                   dwFlags,
		DWORD                   cOutputBufferCount,
		MFT_OUTPUT_DATA_BUFFER  *pOutputSamples, // one per stream
		DWORD                   *pdwStatus
		);

	HRESULT STDMETHODCALLTYPE SetPresentationClock(_In_opt_ IMFPresentationClock *pPresentationClock) 
	{
		m_spPresentationClock = pPresentationClock;
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE GetPresentationClock(_COM_Outptr_opt_result_maybenull_ IMFPresentationClock **pPresentationClock) 
	{
		return E_NOTIMPL;
	}

private:
	// Analyzer configuration
	float m_fOutputSampleRate;
	float m_fSampleOverlapPercentage;
	bool m_bIsLogFScale;
	float m_fLowFrequency;
	float m_fHighFrequency;
	float m_fFrequencyStep;
	unsigned m_FrequencyBins;
	unsigned m_Channels;
	unsigned m_FFTLength;
	unsigned m_InputSampleRate;

	Microsoft::WRL::ComPtr<IMFAttributes>		m_pAttributes;
	Microsoft::WRL::ComPtr<IMFMediaType>		m_pInputType;              // Input media type.
	Microsoft::WRL::ComPtr<IMFMediaType>		m_pOutputType;             // Output media type.
	Microsoft::WRL::Wrappers::CriticalSection	m_cs;
	Microsoft::WRL::ComPtr<IMFSample>			m_pSample;
	Microsoft::WRL::ComPtr <IMFMediaType>		m_pMediaType;
	bool										m_bStreamingInitialized;
	Microsoft::WRL::ComPtr<IMFPresentationClock>	m_spPresentationClock;
	HRESULT BeginStreaming();
	HRESULT EndStreaming();
	HRESULT OnFlush();

	CSpectrumAnalyzer			m_Analyzer;
	const size_t cMaxOutputQueueSize = 600;	// Allow maximum 10sec worth of elements

	Microsoft::WRL::Wrappers::CriticalSection m_csOutputQueueAccess;
	std::deque<Microsoft::WRL::ComPtr<IMFSample>> m_AnalyzerOutput;
	HANDLE m_hWQAccess;		// Semaphore for work queue access
	HANDLE m_hResetWorkQueue;	// Event which signals that the work queue is being reset - while signalled no operations should be performed on output queue. Set in ProcessMessage and reset in ProcessInput
	AsyncCallback<CSampleGrabber> m_AnalysisStepCallback;
	HRESULT BeginAnalysis();
	HRESULT ConfigureAnalyzer();
	HRESULT OnAnalysisStep(IMFAsyncResult *pResult);
	HRESULT FastForwardQueueToPosition(REFERENCE_TIME position, IMFSample **ppSample);	// Removes samples before position and return ppSample if found
	HRESULT FlushOutputQueue();
};

