#include "pch.h"
#include "SpectrumAnalyzer.h"
#include <algorithm>
#include <XDSP.h>
#include "Trace.h"
#include "cubic_spline.h"

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
	m_InputReadPtrSampleIndex(-1),
	m_ExpectedInputSampleOffset(-1),
	m_pFftUnityTable(nullptr),
	m_pFftReal(nullptr),
	m_pFftBuffers(nullptr),
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

	// Allocate data for the input data ring buffer
	m_InputReadPtrSampleIndex = 0;

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

	m_pFftReal = static_cast<XMVECTOR *>(_aligned_malloc(m_FFTLength * sizeof(XMVECTOR) * m_AudioChannels, 16));	// For real data allocate space for all channels
	m_pFftBuffers = static_cast<XMVECTOR *>(_aligned_malloc(3 * m_FFTLength * sizeof(XMVECTOR), 16));	// Preallocate buffers for FFT calculation

	if (!(m_pFftUnityTable && m_pFftReal && m_pFftBuffers))
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
	if (m_pFftBuffers != nullptr)
		_aligned_free(m_pFftBuffers);
}

// Copy data from the sample to the input ring buffer
HRESULT CSpectrumAnalyzer::AppendInput(IMFSample * pSample)
{
	REFERENCE_TIME sampleTime;
	HRESULT hr = pSample->GetSampleTime(&sampleTime);
	if (FAILED(hr))
		return hr;

	long sampleOffset = m_AudioChannels * (long)((sampleTime * m_InputSampleRate + 5000000) / 10000000);	// Avoid int arithmetic problems by rounding

	// If there is some data in the buffer, validate that the input data is contiguous
	// Otherwise empty the buffer, read pointer lock needs to be aquired only when clearing the buffer
	if (!m_InputBuffer.empty() && sampleOffset != m_ExpectedInputSampleOffset)
	{
		auto lock = m_csReadPtr.Lock();
		m_InputBuffer.clear();
		m_InputReadPtrSampleIndex = -1;	// Get the index for reader
	}

	if (m_InputReadPtrSampleIndex == -1)
		m_InputReadPtrSampleIndex = sampleOffset;	// Set the index for empty buffer

	ComPtr<IMFMediaBuffer> spBuffer;
	hr = pSample->ConvertToContiguousBuffer(&spBuffer);
	if (FAILED(hr))
		return hr;

	float *pBufferData = nullptr;
	DWORD cbCurrentLength = 0;
	hr = spBuffer->Lock((BYTE **)&pBufferData, nullptr, &cbCurrentLength);
	if (FAILED(hr))
		return hr;

	for (size_t sampleIndex = 0; sampleIndex < cbCurrentLength / sizeof(float); sampleIndex++)
	{
		*(m_InputBuffer.writer()++) = pBufferData[sampleIndex];
	}

	m_ExpectedInputSampleOffset = sampleOffset + cbCurrentLength / sizeof(float);	// Calculate the next sample offset

	hr = spBuffer->Unlock();
	if (FAILED(hr))
		return hr;

	return S_OK;
}

void CSpectrumAnalyzer::CalculateFft(DirectX::XMVECTOR *pReal, DirectX::XMVECTOR *pOutData, float *pRMS,DirectX::XMVECTOR *pBuffers)
{
	using namespace DirectX;
	using namespace XDSP;
	size_t fftVectorLength = m_FFTLength >> 2;
	XMVECTOR *pImag = pBuffers;
	XMVECTOR *pRealUnswizzled = pBuffers + fftVectorLength;
	XMVECTOR *pImagUnswizzled = pBuffers + fftVectorLength * 2;
	
	memset(pImag, 0, sizeof(float)*m_FFTLength);	// Imaginary values are 0 for input

	FFT(pReal, pImag, m_pFftUnityTable, m_FFTLength);
	FFTUnswizzle(pRealUnswizzled, pReal, m_FFTLengthPow2);
	FFTUnswizzle(pImagUnswizzled, pImag, m_FFTLengthPow2);

	XMVECTOR vRMS = XMVectorZero();
	float fftScaler = 2.0f / m_FFTLength;

	// Calculate abs value first half of FFT output and copy to output
	for (size_t vIndex = 0; vIndex < fftVectorLength >> 1; vIndex++)	// vector length is 4 times shorter, copy only positive frequency values
	{
		XMVECTOR vRR = XMVectorMultiply(pRealUnswizzled[vIndex], pRealUnswizzled[vIndex]);
		XMVECTOR vII = XMVectorMultiply(pImagUnswizzled[vIndex], pImagUnswizzled[vIndex]);
		XMVECTOR vRRplusvII = XMVectorAdd(vRR, vII);
		XMVECTOR vAbs = XMVectorSqrtEst(vRRplusvII);
		pOutData[vIndex] = XMVectorScale(vAbs, fftScaler);
		vRMS += pOutData[vIndex];
	}
	*pRMS = vRMS.m128_f32[0] + vRMS.m128_f32[1] + vRMS.m128_f32[2] + vRMS.m128_f32[3];
}


DirectX::XMVECTOR g_vLog10DbScaler = DirectX::XMVectorReplicate(8.68588f); // This is 20.0/LogE(10)
																		   // {3F692E37-FC20-48DD-93D2-2234E1B1AA23}
// GUID to pass the aligned buffer size
static const GUID g_PropBufferStep =
{ 0x3f692e37, 0xfc20, 0x48dd,{ 0x93, 0xd2, 0x22, 0x34, 0xe1, 0xb1, 0xaa, 0x23 } };


HRESULT CSpectrumAnalyzer::GetRingBufferData(float *pData,REFERENCE_TIME *pTimeStamp)
{
	auto readerLock = m_csReadPtr.Lock();	// Get lock on modifying the reader pointer

											// Not enough samples in ring buffer
	if (m_InputBuffer.size() < m_StepFrameCount)
		return S_FALSE;

	auto src = m_InputBuffer.reader() - (m_StepFrameOverlap * m_AudioChannels);	// Offset by overlap

	// Now window, de-interleave and copy the data
	for (size_t index = 0; index < m_FFTLength; index++)
	{
		for (size_t channelIndex = 0,dataIndex = index; channelIndex < m_AudioChannels; channelIndex++,dataIndex += m_FFTLength)
		{
			if (index < m_StepFrameCount + m_StepFrameOverlap)
				pData[dataIndex] = m_pWindow[index] * (*(src++));
			else
				pData[dataIndex] = 0;
		}
	}
	m_InputBuffer.reader() += m_StepFrameCount * m_AudioChannels;	// These are samples not frames

	*pTimeStamp = 10000000 * (long long)m_InputReadPtrSampleIndex / m_InputSampleRate;
	m_InputReadPtrSampleIndex += m_StepFrameCount;

	return S_OK;
}
HRESULT CSpectrumAnalyzer::Step(IMFSample **ppSample)
{
	using namespace DirectX;

	// Not initialized
	if (m_AudioChannels == 0)
		return E_NOT_VALID_STATE;

	auto lock = m_csConfigAccess.Lock();	// Lock object for config changes
	
	REFERENCE_TIME hnsDataTime = 0;
	GetRingBufferData((float *)m_pFftReal, &hnsDataTime);

	// Create output media sample
	ComPtr<IMFMediaBuffer> spBuffer;

	size_t outputLength = m_bUseLogFScale ? m_logElementsCount : m_FFTLength;

	// Create aligned buffer for vector math + 16 bytes for 8 floats for RMS values
	// To use vector math allocate buffer for each channel which is rounded up to the next 16 byte boundary
	size_t vOutputLength = (outputLength + 3) >> 2;
	HRESULT hr = MFCreateAlignedMemoryBuffer((sizeof(XMVECTOR)*vOutputLength*m_AudioChannels) + 2 * sizeof(XMVECTOR), 16, &spBuffer);
	if (FAILED(hr))
		return hr;

	spBuffer->SetCurrentLength(sizeof(XMVECTOR)*(2 + m_AudioChannels * vOutputLength));

	ComPtr<IMFSample> spSample;
	hr = MFCreateSample(&spSample);
	if (FAILED(hr))
		return hr;

	spSample->AddBuffer(spBuffer.Get());
	spSample->SetSampleDuration((long long)10000000L * m_StepFrameCount / m_InputSampleRate);
	spSample->SetSampleTime(hnsDataTime);

	// Append the channel buffer length as a property
	spSample->SetUINT32(g_PropBufferStep, vOutputLength << 2);

	float *pData;
	hr = spBuffer->Lock((BYTE **)&pData, nullptr, nullptr);
	if (FAILED(hr))
		return hr;

	float *pRMSData = (float *)(pData + m_AudioChannels * (vOutputLength << 2));
	*((XMVECTOR *)pRMSData) = DirectX::g_XMZero;	// Zero RMS values as vector value

	// Process each channel now. FFT values are in the m_pFftReal
	for (size_t channelIndex = 0; channelIndex < m_AudioChannels; channelIndex++)
	{
		// Output data goes directly into the allocated buffer
		// One channel output is fftLength / 2, array vector size is fftLength / 4 hence fftLength >> 3
		float *pOutData = pData + channelIndex * (vOutputLength << 2);

		DirectX::XMVECTOR *pFftData = m_pFftReal + channelIndex * (m_FFTLength >> 2);
		if (m_bUseLogFScale)
		{
			CalculateFft(pFftData, pFftData, pRMSData + channelIndex, m_pFftBuffers);
			float fromFreq = 2 * m_fLogMin / m_InputSampleRate;
			float toFreq = 2 * m_fLogMax / m_InputSampleRate;
			mapToLogScale((float *)pFftData, m_FFTLength >> 1, pOutData, m_logElementsCount, fromFreq, toFreq);
		}
		else
		{
			CalculateFft(pFftData, (XMVECTOR *) pOutData, pRMSData + channelIndex, m_pFftBuffers);
		}
	}
	// Scale the output value, we can use vector math as there is space after the buffer and it is aligned
	if (m_bUseLogAmpScale)
	{

		XMVECTOR *pvData = (XMVECTOR *)pData;
		// Process all amplitude and RMS values in one pass
		for (size_t vIndex = 0; vIndex < m_AudioChannels * vOutputLength + 2; vIndex++)
		{
			XMVECTOR vClamped = XMVectorClamp(pvData[vIndex], DirectX::g_XMZero, XMVectorReplicate(std::numeric_limits<float>::max()));
			XMVECTOR vLog = XMVectorLogE(vClamped) * g_vLog10DbScaler;
			pvData[vIndex] = XMVectorClamp(vLog, m_vClampAmpLow, m_vClampAmpHigh);
		}
	}

	spBuffer->Unlock();

	spSample.CopyTo(ppSample);
	return S_OK;
}

void CSpectrumAnalyzer::Reset()
{
	m_InputBuffer.clear();
	// Clean up any state from buffer copying
	m_CopiedFrameCount = 0;
	m_InputWriteIndex = 0;
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

