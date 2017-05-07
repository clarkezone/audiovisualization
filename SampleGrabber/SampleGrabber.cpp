#include "pch.h"
#include "SampleGrabber.h"
#include "Trace.h"
#include <windows.media.core.interop.h>

#define PROCESS_SAMPLES_ASYNC

CSampleGrabber::CSampleGrabber() : 
	m_pSample(nullptr), 
	m_pOutputType(nullptr), 
	m_pInputType(nullptr),
	m_AnalysisStepCallback(this,&CSampleGrabber::OnAnalysisStep),
	m_fOutputSampleRate(60.0f),
	m_fSampleOverlapPercentage(0.5f),
	m_bIsLogFScale(false),
	m_FFTLength(4096),
	m_InputSampleRate(48000)
{
	Trace::Initialize();
	SetLinearFScale(m_FFTLength/2);
	m_Analyzer.SetLogAmplitudeScale(-100.0f, 100.0f);
	TRACE(L"construct");
	m_hWQAccess = CreateSemaphore(nullptr, 1, 1, nullptr);
}

CSampleGrabber::~CSampleGrabber()
{
	m_pAttributes.Reset();
	CloseHandle(m_hWQAccess);
}

// Initialize the instance.
STDMETHODIMP CSampleGrabber::RuntimeClassInitialize()
{
	if (m_hWQAccess == INVALID_HANDLE_VALUE)
		return E_FAIL;

	// Create the attribute store.
	return MFCreateAttributes(m_pAttributes.GetAddressOf(), 3);
}

// IMediaExtension methods

//-------------------------------------------------------------------
// SetProperties
// Sets the configuration of the effect
//-------------------------------------------------------------------
HRESULT CSampleGrabber::SetProperties(ABI::Windows::Foundation::Collections::IPropertySet *pConfiguration)
{
	// com rule is esential comm (don box)
	// for a synchronous mehtod the caller ensures that the property set is alive for the duration of the call
	// rule 2 when a funtion returns an interface pointer it is already addref
	// having QI'd i now own the reference, i must call release on it
	// only add a reference if keeping alive after function.
	// putting it in WRL will incur cost of add ref
	// if it was an async method you would do that immediately
	
	Microsoft::WRL::ComPtr<ABI::Windows::Foundation::Collections::IMap<HSTRING, IInspectable *>> pMap;
	HRESULT hr = pConfiguration->QueryInterface(IID_PPV_ARGS(&pMap));
	if (FAILED(hr))
		return hr;

	Microsoft::WRL::Wrappers::HStringReference name(L"samplegrabber");

	IInspectable* reference;

	//QI will addref, we put the addreff'd one in the property set
	hr = this->QueryInterface(IID_PPV_ARGS(&reference));

	if (FAILED(hr))
		return hr;

	boolean replaced;

	hr = pMap->Insert(name.Get(), reference, &replaced);

	if (FAILED(hr))
		return hr;

	return S_OK;
}

HRESULT CSampleGrabber::GetVector(ABI::Windows::Foundation::Collections::IVectorView<ABI::SampleGrabber::Data> **pConfiguration) {
	return E_FAIL;
}

HRESULT CSampleGrabber::GetSingleData(ABI::SampleGrabber::Data *pData) {
	ABI::SampleGrabber::Data data;
	data.VariableOne = 256.999f;
	pData = &data;
	return S_OK;
}

HRESULT CSampleGrabber::GetFrame(ABI::Windows::Media::IAudioFrame **ppResult)
{
	using namespace Microsoft::WRL;
	using namespace Microsoft::WRL::Wrappers;
	using namespace ABI::Windows::Media;
	using namespace ABI::Windows::Foundation;

	// Get current presentation position
	MFTIME currentPosition = -1;
	HRESULT hr = S_OK;
	if (m_spPresentationClock != nullptr) {
		m_spPresentationClock->GetTime(&currentPosition);
	}

	Microsoft::WRL::ComPtr<ABI::Windows::Foundation::Diagnostics::ILoggingActivity> logActivity;

	Trace::Log_StartGetFrame(&logActivity, currentPosition,m_AnalyzerOutput.unsafe_size());
	
	CLogActivityHelper activity(logActivity.Get());

	if (currentPosition == -1)
	{
		*ppResult = nullptr;
		return S_OK;
	}

	// Forward the output queue to the current presentation position and return that sample
	// Remove samples that are in the front and not matching the current presentation time
	ComPtr<IMFSample> spSample;
	
	auto lock = m_csOutputQueueAccess.Lock();

	while (!m_AnalyzerOutput.empty())
	{
		auto item = m_AnalyzerOutput.unsafe_begin();
		REFERENCE_TIME sampleTime = 0, sampleDuration = 0;
		hr = item->sample->GetSampleTime(&sampleTime);
		if (FAILED(hr))
			return hr;
		hr = item->sample->GetSampleDuration(&sampleDuration);
		if (FAILED(hr))
			return hr;

		// Add 5uS (about half sample time @96k) to avoid int time math rounding errors
		Trace::Log_TestFrame(currentPosition, sampleTime, sampleTime + sampleDuration);
		if (currentPosition < sampleTime)
		{
			break;	// Current position is before the frames in visualization queue - wait until we catch up
		}

		if (currentPosition <= sampleDuration + sampleTime + 50L)
		{
			Trace::Log_FrameFound(sampleTime, sampleDuration);	// Sample is found
			spSample = m_AnalyzerOutput.unsafe_begin()->sample;
			break;
		}
		else
		{
			// Current position is after the item in the queue - remove and continue searching
			sample_queue_item item;
			m_AnalyzerOutput.try_pop(item);
		}
	}
	// Position not found in the queue
	if (spSample == nullptr)
	{
		Trace::Log_FrameNotFound();
		*ppResult = nullptr;
		return S_OK;

	}
	
	MULTI_QI qiFactory[1] = { { &__uuidof(IAudioFrameNativeFactory),nullptr,S_OK } };
	hr = CoCreateInstanceFromApp(CLSID_AudioFrameNativeFactory, nullptr, CLSCTX_INPROC_SERVER, nullptr, 1, qiFactory);
	if (FAILED(hr))
		return hr;
	if (FAILED(qiFactory[0].hr))
		return qiFactory[0].hr;

	ComPtr<IAudioFrameNativeFactory> spNativeFactory = (IAudioFrameNativeFactory *)qiFactory[0].pItf;


	// Now use the factory to create frame out of IMFSample
	hr = spNativeFactory->CreateFromMFSample(spSample.Get(), false, IID_PPV_ARGS(ppResult));

	return S_OK;
}

HRESULT CSampleGrabber::Configure(float outputSampleRate, float overlapPercentage, unsigned long fftLength)
{
	Trace::Log_Configure(outputSampleRate, overlapPercentage, fftLength);
	// Allow 1- 60fps
	if (outputSampleRate > 60.0 || outputSampleRate < 1.0)
	{
		return E_INVALIDARG;
	}
	if (overlapPercentage < 0.0 || overlapPercentage > 1.0)
	{
		return E_INVALIDARG;
	}
	if ((fftLength & (fftLength - 1)) != 0 || fftLength < 256 || fftLength > 0x20000)	// Set some limits
	{
		return E_INVALIDARG;
	}
	if (m_pInputType != nullptr)
	{
		UINT32 sampleRate = 0;
		m_pInputType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &sampleRate);
		unsigned inputLength = (unsigned) ((sampleRate / outputSampleRate) * (1 + overlapPercentage));
		if (fftLength < inputLength)
			return E_INVALIDARG;
	}
	m_fOutputSampleRate = outputSampleRate;
	m_fSampleOverlapPercentage = overlapPercentage;
	m_FFTLength = fftLength;

	if (m_pInputType != nullptr)
	{
		ConfigureAnalyzer();
	}

	return S_OK;
}

HRESULT CSampleGrabber::GetStreamLimits(
	DWORD   *pdwInputMinimum,
	DWORD   *pdwInputMaximum,
	DWORD   *pdwOutputMinimum,
	DWORD   *pdwOutputMaximum
	)
{
	if ((pdwInputMinimum == NULL) ||
		(pdwInputMaximum == NULL) ||
		(pdwOutputMinimum == NULL) ||
		(pdwOutputMaximum == NULL))
	{
		return E_POINTER;
	}

	// This MFT has a fixed number of streams.
	*pdwInputMinimum = 1;
	*pdwInputMaximum = 1;
	*pdwOutputMinimum = 1;
	*pdwOutputMaximum = 1;
	return S_OK;
}

//-------------------------------------------------------------------
// GetStreamCount
// Returns the actual number of streams.
//-------------------------------------------------------------------

HRESULT CSampleGrabber::GetStreamCount(
	DWORD   *pcInputStreams,
	DWORD   *pcOutputStreams
	)
{
	if ((pcInputStreams == NULL) || (pcOutputStreams == NULL))

	{
		return E_POINTER;
	}

	// This MFT has a fixed number of streams.
	*pcInputStreams = 1;
	*pcOutputStreams = 1;
	return S_OK;
}

//-------------------------------------------------------------------
// GetStreamIDs
// Returns stream IDs for the input and output streams.
//-------------------------------------------------------------------

HRESULT CSampleGrabber::GetStreamIDs(
	DWORD   dwInputIDArraySize,
	DWORD   *pdwInputIDs,
	DWORD   dwOutputIDArraySize,
	DWORD   *pdwOutputIDs
	)
{
	// It is not required to implement this method if the MFT has a fixed number of
	// streams AND the stream IDs are numbered sequentially from zero (that is, the
	// stream IDs match the stream indexes).

	// In that case, it is OK to return E_NOTIMPL.
	return E_NOTIMPL;
}

//-------------------------------------------------------------------
// GetInputStreamInfo
// Returns information about an input stream.
//-------------------------------------------------------------------

HRESULT CSampleGrabber::GetInputStreamInfo(
	DWORD                     dwInputStreamID,
	MFT_INPUT_STREAM_INFO *   pStreamInfo
	)
{
	if (pStreamInfo == NULL)
	{
		return E_POINTER;
	}

	auto lock = m_cs.Lock();

	if (dwInputStreamID!=0)
	{
		return MF_E_INVALIDSTREAMNUMBER;
	}

	//TODO: may need to do something to the stream info here

	return S_OK;
}

//-------------------------------------------------------------------
// GetOutputStreamInfo
// Returns information about an output stream.
//-------------------------------------------------------------------

HRESULT CSampleGrabber::GetOutputStreamInfo(
	DWORD                     dwOutputStreamID,
	MFT_OUTPUT_STREAM_INFO *  pStreamInfo
	)
{
	if (pStreamInfo == NULL)
	{
		return E_POINTER;
	}

	auto lock = m_cs.Lock();

	if (dwOutputStreamID!=0)
	{
		return MF_E_INVALIDSTREAMNUMBER;
	}

	// NOTE: This method should succeed even when there is no media type on the
	//       stream. If there is no media type, we only need to fill in the dwFlags
	//       member of MFT_OUTPUT_STREAM_INFO. The other members depend on having a
	//       a valid media type.

	
	// Flags
	pStreamInfo->dwFlags =
		MFT_OUTPUT_STREAM_WHOLE_SAMPLES |         // Output buffers contain complete audio frames.
		MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES |   // The MFT can allocate output buffers, or use caller-allocated buffers.
		MFT_OUTPUT_STREAM_FIXED_SAMPLE_SIZE;      // Samples (ie, audio frames) are fixed size.

	pStreamInfo->cbSize = 0;   // If no media type is set, use zero.
	pStreamInfo->cbAlignment = 0;

	if (m_pOutputType != nullptr){
		pStreamInfo->cbSize = MFGetAttributeUINT32(m_pOutputType.Get(), MF_MT_AUDIO_BLOCK_ALIGNMENT, 0);
	}

	return S_OK;
}

//-------------------------------------------------------------------
// GetAttributes
// Returns the attributes for the MFT.
//-------------------------------------------------------------------

HRESULT CSampleGrabber::GetAttributes(IMFAttributes** ppAttributes)
{
	if (ppAttributes == NULL)
	{
		return E_POINTER;
	}

	auto lock = m_cs.Lock();

	ASSERT(ppAttributes);
	m_pAttributes.CopyTo(ppAttributes);

	return S_OK;
}

//-------------------------------------------------------------------
// GetInputStreamAttributes
// Returns stream-level attributes for an input stream.
//-------------------------------------------------------------------

HRESULT CSampleGrabber::GetInputStreamAttributes(
	DWORD           dwInputStreamID,
	IMFAttributes   **ppAttributes
	)
{
	// This MFT does not support any stream-level attributes, so the method is not implemented.
	return E_NOTIMPL;
}


//-------------------------------------------------------------------
// GetOutputStreamAttributes
// Returns stream-level attributes for an output stream.
//-------------------------------------------------------------------

HRESULT CSampleGrabber::GetOutputStreamAttributes(
	DWORD           dwOutputStreamID,
	IMFAttributes   **ppAttributes
	)
{
	// This MFT does not support any stream-level attributes, so the method is not implemented.
	return E_NOTIMPL;
}


//-------------------------------------------------------------------
// DeleteInputStream
//-------------------------------------------------------------------

HRESULT CSampleGrabber::DeleteInputStream(DWORD dwStreamID)
{
	// This MFT has a fixed number of input streams, so the method is not supported.
	return E_NOTIMPL;
}


//-------------------------------------------------------------------
// AddInputStreams
//-------------------------------------------------------------------

HRESULT CSampleGrabber::AddInputStreams(
	DWORD   cStreams,
	DWORD   *adwStreamIDs
	)
{
	// This MFT has a fixed number of output streams, so the method is not supported.
	return E_NOTIMPL;
}



//-------------------------------------------------------------------
// GetInputAvailableType
// Returns a preferred input type.
//-------------------------------------------------------------------

HRESULT CSampleGrabber::GetInputAvailableType(
	DWORD           dwInputStreamID,
	DWORD           dwTypeIndex, // 0-based
	IMFMediaType    **ppType
	)
{
	if (ppType == NULL)
	{
		return E_INVALIDARG;
	}

	auto lock = m_cs.Lock();

	if (dwInputStreamID!=0)
	{
		return MF_E_INVALIDSTREAMNUMBER;
	}

	HRESULT hr = S_OK;

	// If the output type is set, return that type as our preferred input type.
	if (m_pOutputType == NULL)
	{
		// The output type is not set. Create a partial media type.

		Microsoft::WRL::ComPtr<IMFMediaType> pmt;

		HRESULT hr = MFCreateMediaType(pmt.GetAddressOf());
		if (FAILED(hr))
		{
			return hr;
		}

		if (dwTypeIndex == 0) {
			hr = pmt->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
			if (FAILED(hr))
			{
				return hr;
			}

			hr = pmt->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float);
			if (FAILED(hr))
			{
				return hr;
			}
		}
		else 
		{
			return MF_E_NO_MORE_TYPES;	// We really want float PCM
		}

		
		pmt.CopyTo(ppType);
		return S_OK;


	}
	else if (dwTypeIndex > 0)
	{
		hr = MF_E_NO_MORE_TYPES;
	}
	else
	{
		ASSERT(m_pOutputType);
		m_pOutputType.CopyTo(ppType);
	}

	return hr;
}

//-------------------------------------------------------------------
// GetOutputAvailableType
// Returns a preferred output type.
//-------------------------------------------------------------------

HRESULT CSampleGrabber::GetOutputAvailableType(
	DWORD           dwOutputStreamID,
	DWORD           dwTypeIndex, // 0-based
	IMFMediaType    **ppType
	)
{
	if (ppType == NULL)
	{
		return E_INVALIDARG;
	}

	auto lock = m_cs.Lock();

	if (dwOutputStreamID!=0)
	{
		return MF_E_INVALIDSTREAMNUMBER;
	}

	HRESULT hr = S_OK;

	// MFMediaType_Audio
	// MFAudioFormat_PCM
	Microsoft::WRL::ComPtr<IMFMediaType> wav;
	//hr = MFCreateMediaType(&wav);


	if (m_pInputType == NULL)
	{
		// The input type is not set. Create a partial media type.
		ASSERT(0);
		//hr = OnGetPartialType(dwTypeIndex, ppType);
	}
	else if (dwTypeIndex > 0)
	{
		hr = MF_E_NO_MORE_TYPES;
	}
	else
	{
		ASSERT(m_pInputType);
		hr = ConvertAudioTypeToFloat32(m_pInputType.Get(), wav.GetAddressOf());
		wav.CopyTo(ppType);
		//m_pInputType.CopyTo(ppType);
	}

	return hr;
}




//-------------------------------------------------------------------
// SetInputType
//-------------------------------------------------------------------

HRESULT CSampleGrabber::SetInputType(
	DWORD           dwInputStreamID,
	IMFMediaType    *pType, // Can be NULL to clear the input type.
	DWORD           dwFlags
	)
{
	// Validate flags.
	if (dwFlags & ~MFT_SET_TYPE_TEST_ONLY)
	{
		return E_INVALIDARG;
	}

	Trace::Log_SetInputType(dwInputStreamID, pType);

	auto lock = m_cs.Lock();

	if (dwInputStreamID!=0)
	{
		return MF_E_INVALIDSTREAMNUMBER;
	}

	// Does the caller want us to set the type, or just test it?
	BOOL bReallySet = ((dwFlags & MFT_SET_TYPE_TEST_ONLY) == 0);

	// If we have an input sample, the client cannot change the type now.
	if (m_pSample!=nullptr)
	{
		return MF_E_TRANSFORM_CANNOT_CHANGE_MEDIATYPE_WHILE_PROCESSING;
	}

	// Validate the type, if non-NULL.
	if (pType) {
		if (m_pOutputType != nullptr)
		{
			DWORD flags = 0;
			auto hr = pType->IsEqual(m_pOutputType.Get(), &flags);

			// IsEqual can return S_FALSE. Treat this as failure.
			if (hr != S_OK)
			{
				return MF_E_INVALIDMEDIATYPE;
			}
		}
		else {
			GUID major_type;
			HRESULT hr = pType->GetGUID(MF_MT_MAJOR_TYPE, &major_type);
			if (FAILED(hr)) return E_FAIL;
			if (major_type != MFMediaType_Audio) return MF_E_INVALIDMEDIATYPE;
			GUID minor_type;
			hr = pType->GetGUID(MF_MT_SUBTYPE, &minor_type);
			if (FAILED(hr)) return E_FAIL;
			if (minor_type != MFAudioFormat_Float) return MF_E_INVALIDMEDIATYPE;

			//TODO: may need to do more stringent test
		}
	}

	// The type is OK. Set the type, unless the caller was just testing.
	if (bReallySet)
	{
		//m_pInputType.Attach(pType);		
		m_pInputType = pType;		

		// Configure the analyzer
		HRESULT hr = ConfigureAnalyzer();
		if (FAILED(hr))
			return MF_E_INVALIDMEDIATYPE;
		// When the type changes, end streaming.
		return EndStreaming();
	}
	return S_OK;
}

HRESULT CSampleGrabber::SetOutputType(
	DWORD           dwOutputStreamID,
	IMFMediaType    *pType, // Can be NULL to clear the output type.
	DWORD           dwFlags
	)
{
	// Validate flags.
	if (dwFlags & ~MFT_SET_TYPE_TEST_ONLY)
	{
		return E_INVALIDARG;
	}

	Trace::Log_SetOutputType(dwOutputStreamID, pType);

	auto lock = m_cs.Lock();

	if (dwOutputStreamID!=0)
	{
		return MF_E_INVALIDSTREAMNUMBER;
	}

	BOOL bReallySet = ((dwFlags & MFT_SET_TYPE_TEST_ONLY) == 0);

	// If we have an input sample, the client cannot change the type now.
	if (m_pSample != nullptr)
	{
		return MF_E_TRANSFORM_CANNOT_CHANGE_MEDIATYPE_WHILE_PROCESSING;
	}

	if (pType)
	{
		if (m_pInputType != nullptr)
		{
			DWORD flags = 0;
			auto hr = pType->IsEqual(m_pInputType.Get(), &flags);

			// IsEqual can return S_FALSE. Treat this as failure.
			if (hr != S_OK)
			{
				return MF_E_INVALIDMEDIATYPE;
			}
			m_pOutputType = pType;
		}
		else {
			GUID major_type;
			HRESULT hr = pType->GetGUID(MF_MT_MAJOR_TYPE, &major_type);
			if (FAILED(hr)) return E_FAIL;
			if (major_type != MFMediaType_Audio) return MF_E_INVALIDMEDIATYPE;
			//TODO: may need to do more stringent test
			return S_OK;
		}
	}

	return S_OK;
}

//-------------------------------------------------------------------
// GetInputCurrentType
// Returns the current input type.
//-------------------------------------------------------------------

HRESULT CSampleGrabber::GetInputCurrentType(
	DWORD           dwInputStreamID,
	IMFMediaType    **ppType
	)
{
	if (ppType == NULL)
	{
		return E_POINTER;
	}

	HRESULT hr = S_OK;

	auto lock = m_cs.Lock();

	if (dwInputStreamID!=0)
	{
		hr = MF_E_INVALIDSTREAMNUMBER;
	}
	else if (!m_pInputType)
	{
		hr = MF_E_TRANSFORM_TYPE_NOT_SET;
	}
	else
	{
		m_pInputType.CopyTo(ppType);
	}
	return hr;
}

//-------------------------------------------------------------------
// GetOutputCurrentType
// Returns the current output type.
//-------------------------------------------------------------------

HRESULT CSampleGrabber::GetOutputCurrentType(
	DWORD           dwOutputStreamID,
	IMFMediaType    **ppType
	)
{
	if (ppType == NULL)
	{
		return E_POINTER;
	}

	HRESULT hr = S_OK;

	auto lock = m_cs.Lock();

	if (dwOutputStreamID!=0)
	{
		hr = MF_E_INVALIDSTREAMNUMBER;
	}
	else if (!m_pOutputType)
	{
		hr = MF_E_TRANSFORM_TYPE_NOT_SET;
	}
	else
	{
		m_pOutputType.CopyTo(ppType);
	}

	return hr;
}


//-------------------------------------------------------------------
// GetInputStatus
// Query if the MFT is accepting more input.
//-------------------------------------------------------------------

HRESULT CSampleGrabber::GetInputStatus(
	DWORD           dwInputStreamID,
	DWORD           *pdwFlags
	)
{
	if (pdwFlags == NULL)
	{
		return E_POINTER;
	}

	auto lock = m_cs.Lock();

	if (dwInputStreamID==0)
	{
		return MF_E_INVALIDSTREAMNUMBER;
	}

	// If an input sample is already queued, do not accept another sample until the 
	// client calls ProcessOutput or Flush.

	// NOTE: It is possible for an MFT to accept more than one input sample. For 
	// example, this might be required in a video decoder if the frames do not 
	// arrive in temporal order. In the case, the decoder must hold a queue of 
	// samples. For the video effect, each sample is transformed independently, so
	// there is no reason to queue multiple input samples.

	if (m_pSample == NULL)
	{
		*pdwFlags = MFT_INPUT_STATUS_ACCEPT_DATA;
	}
	else
	{
		*pdwFlags = 0;
	}

	return S_OK;
}

//-------------------------------------------------------------------
// GetOutputStatus
// Query if the MFT can produce output.
//-------------------------------------------------------------------

HRESULT CSampleGrabber::GetOutputStatus(DWORD *pdwFlags)
{
	if (pdwFlags == NULL)
	{
		return E_POINTER;
	}

	auto lock = m_cs.Lock();

	// The MFT can produce an output sample if (and only if) there an input sample.
	if (m_pSample != NULL)
	{
		*pdwFlags = MFT_OUTPUT_STATUS_SAMPLE_READY;
	}
	else
	{
		*pdwFlags = 0;
	}

	return S_OK;
}

//-------------------------------------------------------------------
// SetOutputBounds
// Sets the range of time stamps that the MFT will output.
//-------------------------------------------------------------------

HRESULT CSampleGrabber::SetOutputBounds(
	LONGLONG        hnsLowerBound,
	LONGLONG        hnsUpperBound
	)
{
	// Implementation of this method is optional.
	return E_NOTIMPL;
}

//-------------------------------------------------------------------
// ProcessEvent
// Sends an event to an input stream.
//-------------------------------------------------------------------

HRESULT CSampleGrabber::ProcessEvent(
	DWORD              dwInputStreamID,
	IMFMediaEvent      *pEvent
	)
{
	Trace::Log_ProcessEvent(dwInputStreamID, pEvent);
	return S_OK;
}


//-------------------------------------------------------------------
// ProcessMessage
//-------------------------------------------------------------------

HRESULT CSampleGrabber::ProcessMessage(
	MFT_MESSAGE_TYPE    eMessage,
	ULONG_PTR           ulParam
	)
{
	auto lock = m_cs.Lock();

	HRESULT hr = S_OK;

	Trace::Log_ProcessMessage(eMessage, ulParam);
	switch (eMessage)
	{
	case MFT_MESSAGE_COMMAND_FLUSH:
		// Flush the MFT.
		hr = OnFlush();
		break;

	case MFT_MESSAGE_COMMAND_DRAIN:
		// Drain: Tells the MFT to reject further input until all pending samples are
		// processed. That is our default behavior already, so there is nothing to do.
		//
		// For a decoder that accepts a queue of samples, the MFT might need to drain
		// the queue in response to this command.
		break;

	case MFT_MESSAGE_SET_D3D_MANAGER:
		// Sets a pointer to the IDirect3DDeviceManager9 interface.

		// The pipeline should never send this message unless the MFT sets the MF_SA_D3D_AWARE 
		// attribute set to TRUE. Because this MFT does not set MF_SA_D3D_AWARE, it is an error
		// to send the MFT_MESSAGE_SET_D3D_MANAGER message to the MFT. Return an error code in
		// this case.

		// NOTE: If this MFT were D3D-enabled, it would cache the IDirect3DDeviceManager9 
		// pointer for use during streaming.

		hr = E_NOTIMPL;
		break;

	case MFT_MESSAGE_NOTIFY_BEGIN_STREAMING:
		hr = BeginStreaming();
		break;

	case MFT_MESSAGE_NOTIFY_END_STREAMING:
		hr = EndStreaming();
		break;

		// The next two messages do not require any action from this MFT.

	case MFT_MESSAGE_NOTIFY_END_OF_STREAM:
		break;

	case MFT_MESSAGE_NOTIFY_START_OF_STREAM:
		m_Analyzer.Reset();
		break;
	}

	return hr;
}

//-------------------------------------------------------------------
// ProcessInput
// Process an input sample.
//-------------------------------------------------------------------

HRESULT CSampleGrabber::ProcessInput(
	DWORD               dwInputStreamID,
	IMFSample           *pSample,
	DWORD               dwFlags
	)
{
	REFERENCE_TIME presentationTime = -1;
	if (m_spPresentationClock != nullptr)
	{
		m_spPresentationClock->GetTime(&presentationTime);
	}
	Microsoft::WRL::ComPtr<ABI::Windows::Foundation::Diagnostics::ILoggingActivity> logActivity;
	Trace::Log_ProcessInput(&logActivity, dwInputStreamID, pSample, dwFlags, presentationTime);
	CLogActivityHelper activity(logActivity.Get());

	// Check input parameters.
	if (pSample == NULL)
	{
		return E_POINTER;
	}

	if (dwFlags != 0)
	{
		return E_INVALIDARG; // dwFlags is reserved and must be zero.
	}

	HRESULT hr = S_OK;

	auto lock = m_cs.Lock();

	// Validate the input stream number.
	if (dwInputStreamID!=0)
	{
		return MF_E_INVALIDSTREAMNUMBER;
	}

	// Check for valid media types.
	// The client must set input and output types before calling ProcessInput.
	if (!m_pInputType || !m_pOutputType)
	{
		return MF_E_NOTACCEPTING;
	}

	// Check if an input sample is already queued.
	if (m_pSample != NULL)
	{
		return MF_E_NOTACCEPTING;   // We already have an input sample.
	}

	// Initialize streaming.
	hr = BeginStreaming();
	if (FAILED(hr))
	{
		return hr;
	}

	// Cache the sample. We do the actual work in ProcessOutput.
	m_pSample = pSample;

	return hr;
}

//-------------------------------------------------------------------
// ProcessOutput
// Process an output sample.
//-------------------------------------------------------------------

HRESULT CSampleGrabber::ProcessOutput(
	DWORD                   dwFlags,
	DWORD                   cOutputBufferCount,
	MFT_OUTPUT_DATA_BUFFER  *pOutputSamples, // one per stream
	DWORD                   *pdwStatus
	)
{
	REFERENCE_TIME presentationTime = -1;
	if (m_spPresentationClock != nullptr)
	{
		m_spPresentationClock->GetTime(&presentationTime);
	}
	Microsoft::WRL::ComPtr<ABI::Windows::Foundation::Diagnostics::ILoggingActivity> logActivity;
	Trace::Log_ProcessOutput(&logActivity, dwFlags, cOutputBufferCount, pOutputSamples, presentationTime);
	CLogActivityHelper activity(logActivity.Get());

	if (dwFlags != 0)
	{
		return E_INVALIDARG;
	}

	if (pOutputSamples == NULL || pdwStatus == NULL)
	{
		return E_POINTER;
	}

	// There must be exactly one output buffer.
	if (cOutputBufferCount != 1)
	{
		return E_INVALIDARG;
	}

	HRESULT hr = S_OK;


	auto lock = m_cs.Lock();

	if (m_pSample == nullptr)
	{
		return MF_E_TRANSFORM_NEED_MORE_INPUT;
	}

	// Initialize streaming.

	hr = BeginStreaming();
	if (FAILED(hr))
	{
		return hr;
	}

#pragma region MyRegion

	// Queue the audio frame in the analyzer buffer
	hr = m_Analyzer.AppendInput(m_pSample.Get());
	BeginAnalysis();

#pragma endregion

	if (pOutputSamples->pSample != nullptr)
	{
		ASSERT(false);
	}
	else {
		//pOutputSamples[0].pSample = m_pSample.Detach();
		pOutputSamples->pSample = m_pSample.Detach();
	}

	// Set status flags.
	pOutputSamples[0].dwStatus = 0;
	*pdwStatus = 0;
	
	return hr;
}

HRESULT CSampleGrabber::EndStreaming()
{
	m_bStreamingInitialized = false;
	return S_OK;
}

// Initialize streaming parameters.
//
// This method is called if the client sends the MFT_MESSAGE_NOTIFY_BEGIN_STREAMING
// message, or when the client processes a sample, whichever happens first.

HRESULT CSampleGrabber::BeginStreaming()
{
	HRESULT hr = S_OK;

	if (!m_bStreamingInitialized)
	{
		m_bStreamingInitialized = true;
	}

	return hr;
}

// Flush the MFT.

HRESULT CSampleGrabber::OnFlush()
{
	// Release input sample and reset the analyzer and queues
	m_Analyzer.Reset();
	auto lock = m_csOutputQueueAccess.Lock();
	m_AnalyzerOutput.clear();

	m_pSample.Reset();

	return S_OK;
}

HRESULT CSampleGrabber::BeginAnalysis()
{
	Trace::Log_BeginAnalysis();
#ifdef PROCESS_SAMPLES_ASYNC
	// See if the access semaphore is signaled
	DWORD dwWaitResult = WaitForSingleObject(m_hWQAccess, 0);
	if (dwWaitResult == WAIT_OBJECT_0) // 
	{
		return MFPutWorkItem2(MFASYNC_CALLBACK_QUEUE_MULTITHREADED, 0, &m_AnalysisStepCallback, nullptr);
	}
	else if (dwWaitResult == WAIT_TIMEOUT)
	{
		Trace::Log_AnalysisAlreadyRunning();
		return S_FALSE;	// Analysis is already running
	}
	else
		return E_FAIL;
#else
	HRESULT hr = S_OK;
	while (hr == S_OK)
	{
		Microsoft::WRL::ComPtr<ABI::Windows::Foundation::Diagnostics::ILoggingActivity> spActivity;
		Trace::Log_StartAnalyzerStep(&spActivity);

		Microsoft::WRL::ComPtr<IMFSample> spSample;
		HRESULT hr = m_Analyzer.Step(&spSample);
		REFERENCE_TIME timestamp = -1;

		if (spSample != nullptr)
			spSample->GetSampleTime(&timestamp);

		Trace::Log_StopAnalyzerStep(spActivity.Get(), timestamp, hr);

		if (hr == S_OK)
		{
			Microsoft::WRL::ComPtr<ABI::Windows::Foundation::Diagnostics::ILoggingActivity> spPushActivity;
			Trace::Log_StartOutputQueuePush(&spPushActivity, timestamp);
			CLogActivityHelper pushActivity(spPushActivity.Get());
			auto lock = m_csOutputQueueAccess.Lock();
			m_AnalyzerOutput.push(spSample.Get());
		}
	}
	return S_OK;
#endif
	
}
HRESULT CSampleGrabber::OnAnalysisStep(IMFAsyncResult *pResult)
{
	Microsoft::WRL::ComPtr<ABI::Windows::Foundation::Diagnostics::ILoggingActivity> spActivity;
	Trace::Log_StartAnalyzerStep(&spActivity);
	Microsoft::WRL::ComPtr<IMFSample> spSample;
	HRESULT hr = m_Analyzer.Step(&spSample);
	REFERENCE_TIME timestamp = -1;
	
	if (spSample != nullptr)
		spSample->GetSampleTime(&timestamp);

	Trace::Log_StopAnalyzerStep(spActivity.Get(), timestamp, hr);

	if (hr == S_OK)
	{
		Microsoft::WRL::ComPtr<ABI::Windows::Foundation::Diagnostics::ILoggingActivity> spPushActivity;
		Trace::Log_StartOutputQueuePush(&spPushActivity,timestamp);
		CLogActivityHelper pushActivity(spPushActivity.Get());
		auto lock =  m_csOutputQueueAccess.Lock();
		m_AnalyzerOutput.push(spSample.Get());
	}
	else
	{
		// Work is done, queue is empty, release the semaphore
		ReleaseSemaphore(m_hWQAccess, 1, nullptr);
		return S_OK;
	}

	// Schedule next work item
	return MFPutWorkItem2(MFASYNC_CALLBACK_QUEUE_MULTITHREADED, 0, &m_AnalysisStepCallback, nullptr);
}

HRESULT CSampleGrabber::ConfigureAnalyzer()
{
	if (m_pInputType == nullptr)
		return E_FAIL;

	m_pInputType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &m_InputSampleRate);

	UINT32 samplesPerAnalyzerOutputFrame = (UINT32) (m_InputSampleRate / m_fOutputSampleRate);
	UINT32 overlap = (UINT32) (samplesPerAnalyzerOutputFrame * m_fSampleOverlapPercentage);	// 50% overlap

	HRESULT hr = m_Analyzer.Configure(m_pInputType.Get(), samplesPerAnalyzerOutputFrame, overlap, m_FFTLength);
	Trace::Log_ConfigureAnalyzer(samplesPerAnalyzerOutputFrame, overlap, m_FFTLength, hr);
	return hr;
}

STDMETHODIMP CSampleGrabber::SetLogFScale(float lowFrequency, float highFrequency, unsigned long numberOfBins)
{
	Trace::Log_SetLogFScale(lowFrequency, highFrequency, numberOfBins);
	if (lowFrequency <= 0 || lowFrequency >= highFrequency || numberOfBins == 0 || highFrequency > m_InputSampleRate)
		return E_INVALIDARG;

	m_bIsLogFScale = true;
	m_fLowFrequency = lowFrequency;
	m_fHighFrequency = highFrequency;
	m_FrequencyBins = numberOfBins;
	m_fFrequencyStep = pow(m_fHighFrequency / m_fLowFrequency, 1.f / m_FrequencyBins);
	m_Analyzer.SetLogFScale(m_fLowFrequency, m_fHighFrequency, m_FrequencyBins);
	return S_OK;
}
STDMETHODIMP CSampleGrabber::SetLinearFScale(unsigned long numberOfBins)
{
	Trace::Log_SetLinearScale(numberOfBins);
	m_bIsLogFScale = false;
	m_fLowFrequency = 0.0f;
	m_fHighFrequency = (float) (m_InputSampleRate >> 1);
	m_FrequencyBins = numberOfBins == 0 ? m_FFTLength >> 1 : numberOfBins;
	m_fFrequencyStep = (float) m_InputSampleRate / (float) 2*m_FrequencyBins;
	m_Analyzer.SetLinearFScale(m_FrequencyBins);
	return S_OK;
}
