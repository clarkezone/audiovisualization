#include "pch.h"
#include "SpectrumAnalyzer.h"
#include <algorithm>
#include <XDSP.h>
#include "Trace.h"

using namespace Microsoft::WRL;
#undef max
#undef min

CSpectrumAnalyzer::CSpectrumAnalyzer() :
	m_AudioChannels(0),
	m_StepFrameCount(0),
	m_StepFrameOverlap(0),
	m_StepTotalFrames(0),
	m_pWindow(nullptr),
	m_pInputBuffer(nullptr),
	m_CopiedFrameCount(0),
	m_InputWriteIndex(0),
	m_CurrentBufferSampleIndex(0),
	m_pFftUnityTable(nullptr),
	m_pFftReal(nullptr),
	m_pFftImag(nullptr),
	m_pFftRealUnswizzled(nullptr),
	m_pFftImagUnswizzled(nullptr),
	m_hnsCurrentBufferTime(0L),
	m_hnsOutputFrameTime(0L),
	m_bUseLogFScale(false),
	m_fLogMin(20.0f),
	m_fLogMax(20000.f),
	m_logElementsCount(200),
	m_bUseLogAmpScale(false),
	m_vClampAmpLow(DirectX::XMVectorReplicate(std::numeric_limits<float>::min())),
	m_vClampAmpHigh(DirectX::XMVectorReplicate(std::numeric_limits<float>::min()))
{
}


CSpectrumAnalyzer::~CSpectrumAnalyzer()
{
	FreeBuffers();
}

HRESULT CSpectrumAnalyzer::Configure(IMFMediaType * pInputType, unsigned stepFrameCount, unsigned stepFrameOverlap, unsigned fftLength)
{
	auto lock = m_csConfigAccess.Lock();	// Lock object for config changes
	// Validate arguments first
	if (pInputType == nullptr)
		return E_INVALIDARG;

	GUID majorType;
	// Validate for supported media type
	HRESULT hr = pInputType->GetMajorType(&majorType);
	if (FAILED(hr))
		return hr;

	if (memcmp(&majorType, &MFMediaType_Audio, sizeof(GUID)) != 0)
	{
		return E_INVALIDARG;
	}
	GUID subType;
	hr = pInputType->GetGUID(MF_MT_SUBTYPE, &subType);
	if (FAILED(hr))
		return hr;

	if (memcmp(&subType, &MFAudioFormat_Float, sizeof(GUID)) != 0)
	{
		return E_INVALIDARG;
	}

	if (fftLength <= stepFrameCount + stepFrameOverlap || (fftLength & (fftLength - 1)) != 0)
		return E_INVALIDARG;

	// Get the number of audio channels
	hr = pInputType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &m_AudioChannels);

	if (FAILED(hr))
		return hr;

	hr = pInputType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &m_InputSampleRate);
	if (FAILED(hr))
		return hr;

	m_StepFrameCount = stepFrameCount;
	m_StepFrameOverlap = stepFrameOverlap;
	m_StepTotalFrames = stepFrameCount + stepFrameOverlap;

	m_FFTLength = fftLength;
	m_FFTLengthPow2 = 1;
	while ((size_t)1 << m_FFTLengthPow2 != m_FFTLength)
		m_FFTLengthPow2++;

	hr = AllocateBuffers();
	if (FAILED(hr))
		return hr;

	return S_OK;
}

HRESULT CSpectrumAnalyzer::AllocateBuffers()
{
	using namespace DirectX;
	// Input buffer will need to keep samples for the step and the overlap
	m_pInputBuffer = static_cast<float *>(malloc(m_StepTotalFrames * m_AudioChannels * sizeof(float)));
	memset(m_pInputBuffer, 0, m_StepTotalFrames * m_AudioChannels * sizeof(float));
	m_CopiedFrameCount = 0;
	m_InputWriteIndex = 0;

	m_pWindow = static_cast<float *>(malloc(m_StepTotalFrames * sizeof(float)));

	// Initialize window, use Blackman-Nuttall window for low sidelobes
	float a0 = 0.3635819f, a1 = 0.4891775f, a2 = 0.1365995f, a3 = 0.0106411f;
	float pi = 3.14159f;
	for (size_t index = 0; index < m_StepTotalFrames; index++)
	{
		m_pWindow[index] = a0 - a1 * cosf(2 * pi * index / (m_StepTotalFrames - 1)) +
			a2 * cosf(4 * pi * index / (m_StepTotalFrames - 1)) -
			a3 * cosf(6 * pi * index / (m_StepTotalFrames - 1));
	}

	m_pFftUnityTable = static_cast<XMVECTOR *> (_aligned_malloc(2 * m_FFTLength * sizeof(XMVECTOR), 16));	// Complex values 
	XDSP::FFTInitializeUnityTable(m_pFftUnityTable, m_FFTLength);

	m_pFftReal = static_cast<XMVECTOR *>(_aligned_malloc(m_FFTLength * sizeof(XMVECTOR), 16));
	m_pFftImag = static_cast<XMVECTOR *>(_aligned_malloc(m_FFTLength * sizeof(XMVECTOR), 16));
	m_pFftRealUnswizzled = static_cast<XMVECTOR *>(_aligned_malloc(m_FFTLength * sizeof(XMVECTOR), 16));
	m_pFftImagUnswizzled = static_cast<XMVECTOR *>(_aligned_malloc(m_FFTLength * sizeof(XMVECTOR), 16));

	if (!(m_pFftUnityTable && m_pFftReal && m_pFftImag && m_pFftRealUnswizzled && m_pFftImagUnswizzled))
		return E_OUTOFMEMORY;

	return S_OK;
}

void CSpectrumAnalyzer::FreeBuffers()
{
	free(m_pWindow);
	free(m_pInputBuffer);

	if (m_pFftUnityTable != nullptr)
		_aligned_free(m_pFftUnityTable);
	if (m_pFftReal != nullptr)
		_aligned_free(m_pFftReal);
	if (m_pFftImag != nullptr)
		_aligned_free(m_pFftImag);
	if (m_pFftRealUnswizzled != nullptr)
		_aligned_free(m_pFftRealUnswizzled);
	if (m_pFftImagUnswizzled != nullptr)
		_aligned_free(m_pFftImagUnswizzled);
}

HRESULT CSpectrumAnalyzer::QueueInput(IMFSample *pSample)
{
	m_InputQueue.push(pSample);
	Trace::Log_QueueInput(pSample);
	return S_OK;
}

HRESULT CSpectrumAnalyzer::GetNextSample()
{
	sample_queue_item item;
	if (m_InputQueue.try_pop(item))
	{
		m_hnsCurrentBufferTime = item.time;
		m_spCurrentBuffer = item.buffer;
		m_CurrentBufferSampleIndex = 0;
		Trace::Log_QIPop(m_spCurrentBuffer.Get(), m_hnsCurrentBufferTime);
	}
	else
	{
		return S_FALSE;
	}

	return S_OK;
}

void CSpectrumAnalyzer::AnalyzeData(DirectX::XMVECTOR *pData, float *pRMS)
{
	using namespace DirectX;
	using namespace XDSP;
	FFT(m_pFftReal, m_pFftImag, m_pFftUnityTable, m_FFTLength);
	FFTUnswizzle(m_pFftRealUnswizzled, m_pFftReal, m_FFTLengthPow2);
	FFTUnswizzle(m_pFftImagUnswizzled, m_pFftImag, m_FFTLengthPow2);

	XMVECTOR vRMS = XMVectorZero();
	float fftScaler = 2.0f / m_FFTLength;
	// Calculate abs value first half of FFT output and copy to output
	for (size_t vIndex = 0; vIndex < m_FFTLength >> 3; vIndex++)	// vector length is 4 times shorter, copy only positive frequency values
	{
		XMVECTOR vRR = XMVectorMultiply(m_pFftRealUnswizzled[vIndex], m_pFftRealUnswizzled[vIndex]);
		XMVECTOR vII = XMVectorMultiply(m_pFftImagUnswizzled[vIndex], m_pFftImagUnswizzled[vIndex]);
		XMVECTOR vRRplusvII = XMVectorAdd(vRR, vII);
		XMVECTOR vAbs = XMVectorSqrtEst(vRRplusvII);
		pData[vIndex] = XMVectorScale(vAbs, fftScaler);
		vRMS += pData[vIndex];
	}
	*pRMS = vRMS.m128_f32[0] + vRMS.m128_f32[1] + vRMS.m128_f32[2] + vRMS.m128_f32[3];
}

HRESULT CSpectrumAnalyzer::CopyDataFromInputBuffer()
{
	using namespace Microsoft::WRL;
	using namespace ABI::Windows::Foundation::Diagnostics;

	ComPtr<ILoggingActivity> spActivity;
	Trace::Log_StartCopyDataFromInput(&spActivity, m_spCurrentBuffer.Get(),m_CopiedFrameCount, m_StepFrameCount,m_CurrentBufferSampleIndex / m_AudioChannels);

	while (m_CopiedFrameCount < m_StepFrameCount)
	{
		if (m_spCurrentBuffer == nullptr && GetNextSample() != S_OK)
		{
			Trace::Log_StopCopyDataFromInput(spActivity.Get(), m_CopiedFrameCount, m_CurrentBufferSampleIndex / m_AudioChannels, __LINE__, S_FALSE);
			return S_FALSE;	// Not enough input samples
		}

		// Copy next m_StepSampleCount samples from buffer and perform analyzes
		DWORD cbCurrentLength;
		float *pBuffer = nullptr;
		HRESULT hr = m_spCurrentBuffer->Lock((BYTE **)&pBuffer, nullptr, &cbCurrentLength);
		if (FAILED(hr))
		{
			Trace::Log_StopCopyDataFromInput(spActivity.Get(), m_CopiedFrameCount, m_CurrentBufferSampleIndex / m_AudioChannels, __LINE__, hr);
			return hr;
		}

		DWORD samplesInBuffer = cbCurrentLength / sizeof(float);
		Trace::Log_LockInputBuffer(m_spCurrentBuffer.Get());


		ComPtr<ILoggingActivity> spCopyActivity;
		Trace::Log_StartCopyLoop(&spCopyActivity, m_CopiedFrameCount, m_StepFrameCount, m_CurrentBufferSampleIndex, samplesInBuffer,m_AudioChannels);
		// Copy frames from source buffer to input buffer all copied or source buffer depleted
		while (m_CopiedFrameCount < m_StepFrameCount && m_CurrentBufferSampleIndex < samplesInBuffer)
		{
			if (m_CopiedFrameCount == 0)
			{
				// Set the timestamp when copying the first sample
				m_hnsOutputFrameTime = m_hnsCurrentBufferTime + 10000000L * ((long long)m_CurrentBufferSampleIndex / m_AudioChannels) / m_InputSampleRate;
				Trace::Log_SetCopiedFrameTime(m_hnsOutputFrameTime);
			}
			for (size_t channelIndex = 0; channelIndex < m_AudioChannels; channelIndex++, m_InputWriteIndex++, m_CurrentBufferSampleIndex++)
			{
				m_pInputBuffer[m_InputWriteIndex] = pBuffer[m_CurrentBufferSampleIndex];
			}

			m_CopiedFrameCount++;

			// Wrap write index over the end if needed
			if (m_InputWriteIndex >= m_StepTotalFrames * m_AudioChannels)
				m_InputWriteIndex = 0;
		}
		Trace::Log_StopCopyLoop(spCopyActivity.Get(), m_CopiedFrameCount, m_StepFrameCount, m_CurrentBufferSampleIndex, samplesInBuffer, m_AudioChannels);
		m_spCurrentBuffer->Unlock();

		if (m_CurrentBufferSampleIndex >= samplesInBuffer)
		{
			m_spCurrentBuffer = nullptr; // Flag to get next sample from input
			Trace::Log_CopyNextSample();
		}
	}
	m_CopiedFrameCount = 0;	// Start all over next time
	Trace::Log_StopCopyDataFromInput(spActivity.Get(), m_CopiedFrameCount, m_CurrentBufferSampleIndex / m_AudioChannels, __LINE__, S_OK);
	return S_OK;
}
void CSpectrumAnalyzer::CopyDataToFftBuffer(int readIndex, float *pReal)
{
	for (size_t fftIndex = 0; fftIndex < m_FFTLength; fftIndex++)
	{
		if (fftIndex < m_StepTotalFrames)
		{
			pReal[fftIndex] = m_pWindow[fftIndex] * m_pInputBuffer[readIndex];
			readIndex += m_AudioChannels;
			if (readIndex >= m_StepTotalFrames * m_AudioChannels)	// Wrap the read index over end
				readIndex -= m_StepTotalFrames * m_AudioChannels;
		}
		else
			pReal[fftIndex] = 0;		// Pad with zeros
	}

}

DirectX::XMVECTOR g_vLog10DbScaler = DirectX::XMVectorReplicate(8.68588f); // This is 20.0/LogE(10)
																		   // {3F692E37-FC20-48DD-93D2-2234E1B1AA23}
// GUID to pass the aligned buffer size
static const GUID g_PropBufferStep =
{ 0x3f692e37, 0xfc20, 0x48dd,{ 0x93, 0xd2, 0x22, 0x34, 0xe1, 0xb1, 0xaa, 0x23 } };


HRESULT CSpectrumAnalyzer::Step(IMFSample **ppSample)
{
	using namespace DirectX;

	// Not initialized
	if (m_AudioChannels == 0)
		return E_NOT_VALID_STATE;

	auto lock = m_csConfigAccess.Lock();	// Lock object for config changes

	// Copy data to the input buffer from sample queue
	HRESULT hr = CopyDataFromInputBuffer();
	if (hr != S_OK)
		return hr;

	// Create media sample
	ComPtr<IMFMediaBuffer> spBuffer;

	size_t outputLength = m_bUseLogFScale ? m_logElementsCount : m_FFTLength;
	
	// Create aligned buffer for vector math + 16 bytes for 8 floats for RMS values
	// To use vector math allocate buffer for each channel which is rounded up to the next 16 byte boundary
	size_t vOutputLength = (outputLength + 3) >> 2;
	hr = MFCreateAlignedMemoryBuffer((sizeof(XMVECTOR)*vOutputLength*m_AudioChannels) + 2*sizeof(XMVECTOR), 16, &spBuffer);
	if (FAILED(hr))
		return hr;

	spBuffer->SetCurrentLength(sizeof(XMVECTOR)*(2 + m_AudioChannels * vOutputLength));

	ComPtr<IMFSample> spSample;
	hr = MFCreateSample(&spSample);
	if (FAILED(hr))
		return hr;

	spSample->AddBuffer(spBuffer.Get());
	spSample->SetSampleDuration((long long)10000000L * m_StepFrameCount / m_InputSampleRate);
	spSample->SetSampleTime(m_hnsOutputFrameTime);

	// Append the channel buffer length as a property
	spSample->SetUINT32(g_PropBufferStep, vOutputLength << 2);

	float *pData;
	hr = spBuffer->Lock((BYTE **)&pData, nullptr, nullptr);
	if (FAILED(hr))
		return hr;

	float *pRMSData = (float *)(pData + m_AudioChannels * (vOutputLength << 2));
	*((XMVECTOR *)pRMSData) = DirectX::g_XMZero;	// Zero RMS values as vector value

	// For each channel copy data to FFT buffer
	for (size_t channelIndex = 0; channelIndex < m_AudioChannels; channelIndex++)
	{
		size_t readIndex = channelIndex + m_InputWriteIndex;	// After previous copy the write index will point to the starting point of the overlapped area

		CopyDataToFftBuffer(readIndex, (float *)m_pFftReal);
		memset(m_pFftImag, 0, sizeof(float)*m_FFTLength);	// Imaginary values are 0 for input

		// One channel output is fftLength / 2, array vector size is fftLength / 4 hence fftLength >> 3
		float *pOutData = pData + channelIndex * (vOutputLength << 2);

		if (m_bUseLogFScale)
		{
			AnalyzeData(m_pFftReal, pRMSData + channelIndex);
			float fromFreq = 2 * m_fLogMin / m_InputSampleRate;
			float toFreq = 2 * m_fLogMax / m_InputSampleRate;
			mapToLogScale((float *)m_pFftReal, m_FFTLength >> 1, pOutData, m_logElementsCount, fromFreq, toFreq);
		}
		else
		{
			AnalyzeData((XMVECTOR *)pOutData, pRMSData + channelIndex);
		}
	}
	// Scale the output value, we can use vector math as there is space after the buffer and it is aligned
	if (m_bUseLogAmpScale)
	{

		XMVECTOR *pvData = (XMVECTOR *) pData;
		// Process all amplitude and RMS values in one pass
		for (size_t vIndex = 0; vIndex < m_AudioChannels * vOutputLength + 2; vIndex++)
		{
			XMVECTOR vLog = XMVectorLogE(pvData[vIndex]) * g_vLog10DbScaler;
			pvData[vIndex] = XMVectorClamp(vLog, m_vClampAmpLow, m_vClampAmpHigh);
		}
	}

	spBuffer->Unlock();

	spSample.CopyTo(ppSample);
	return S_OK;
}

void CSpectrumAnalyzer::Reset()
{
	m_InputQueue.clear();
	m_CurrentBufferSampleIndex = 0;
	m_spCurrentBuffer = nullptr;
	// Clean up any state from buffer copying
	memset(m_pInputBuffer, 0, m_StepTotalFrames * m_AudioChannels * sizeof(float));
	m_CopiedFrameCount = 0;
	m_InputWriteIndex = 0;
}

HRESULT CSpectrumAnalyzer::Skip(REFERENCE_TIME timeStamp)
{
	while (true)
	{
		if (m_spCurrentBuffer == nullptr && GetNextSample() != S_OK)
		{
			return S_FALSE;	// Not enough input samples
		}

		DWORD cbBufferLength;
		HRESULT hr = m_spCurrentBuffer->GetCurrentLength(&cbBufferLength);
		if (FAILED(hr))
			return hr;
		REFERENCE_TIME duration = 10000000L * (long long)(cbBufferLength / sizeof(float) / m_AudioChannels) / m_InputSampleRate;
		if (timeStamp >= m_hnsCurrentBufferTime && timeStamp < m_hnsCurrentBufferTime + duration)
		{
			// Clean input buffer
			memset(m_pInputBuffer, 0, m_StepTotalFrames * m_AudioChannels * sizeof(float));
			m_CopiedFrameCount = 0;
			m_InputWriteIndex = 0;
			// And set copy pointer to the requested location
			m_CurrentBufferSampleIndex = (unsigned)(m_AudioChannels * ((m_InputSampleRate*(timeStamp - m_hnsCurrentBufferTime) + (m_InputSampleRate >> 1)) / 10000000L));	// Add half sample rate for appropriate rounding to avoid int math rounding errors
			return S_OK;
		}
		m_spCurrentBuffer = nullptr;
	}
}

void CSpectrumAnalyzer::SetLinearFScale()
{
	auto lock = m_csConfigAccess.Lock();	// Lock object for config changes
	m_bUseLogFScale = false;
}

void CSpectrumAnalyzer::SetLogAmplitudeScale(float clampToLow, float clampToHigh)
{
	auto lock = m_csConfigAccess.Lock();	// Lock object for config changes
	m_bUseLogAmpScale = true;
	m_vClampAmpLow = DirectX::XMVectorReplicate(clampToLow);
	m_vClampAmpHigh = DirectX::XMVectorReplicate(clampToHigh);
}

void CSpectrumAnalyzer::SetLinearAmplitudeScale()
{
	auto lock = m_csConfigAccess.Lock();	// Lock object for config changes
	m_bUseLogAmpScale = false;
	m_vClampAmpLow = DirectX::XMVectorReplicate(std::numeric_limits<float>::min());
	m_vClampAmpHigh = DirectX::XMVectorReplicate(std::numeric_limits<float>::max());
}

void CSpectrumAnalyzer::SetLogFScale(float lowFrequency, float highFrequency, size_t binCount)
{
	auto lock = m_csConfigAccess.Lock();	// Lock object for config changes
	m_bUseLogFScale = true;
	m_fLogMin = lowFrequency;
	m_fLogMax = highFrequency;
	m_logElementsCount = binCount;
}

