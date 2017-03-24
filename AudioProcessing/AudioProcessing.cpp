#include "pch.h"
#include "AudioProcessing.h"

HRESULT AudioProcessor::BuildVectorBuffer(Microsoft::WRL::ComPtr<IMFSample> pSample, DirectX::XMVECTOR** vectorBytes)
{
	LONGLONG duration; //duration in units of 100-nanosecond
	HRESULT hr = pSample->GetSampleDuration(&duration);

	Microsoft::WRL::ComPtr<IMFMediaBuffer> pInput;

	hr = pSample->ConvertToContiguousBuffer(&pInput);
	if (FAILED(hr))
	{
		return hr;
	}

	//TODO: check that this is a Floating point sample
	//TODO: Get sample rate

	DWORD currentLengthIn(0);
	DWORD maxLengthIn(0);
	DWORD currentLengthOut(0);
	DWORD maxLengthOut(0);

	BYTE* pInputBytes = nullptr;
	BYTE* pOutputBytes = nullptr;

	hr = pInput->Lock(&pInputBytes, &maxLengthIn, &currentLengthIn);
	if (FAILED(hr))
	{
		return hr;
	}

	DirectX::XMVECTOR* vectorInputBuffer = (DirectX::XMVECTOR*)_aligned_malloc(currentLengthIn / 4 * sizeof(DirectX::XMVECTOR), 16);

	for (int i = 0; i < currentLengthIn / 4; i++) {
		vectorInputBuffer[i] = DirectX::XMLoadFloat((float*)(pInputBytes + (i * 4)));
	}

	//_aligned_free(vectorInputBuffer);

	vectorBytes = &vectorInputBuffer;
}

void AudioProcessor::CalcRMSVec(DirectX::XMVECTOR *input)
{
	//TODO: port rms to vector math

	//int inputLengthSamples = (int)input.Length / sizeof(DirectX::XMVECTOR);

	//int samplesPervBlank = (int)((float)currentEncodingProperties.SampleRate / (float)videoFrameRate);

	//int numVBlanksForCurrentAudioBuffer = (int)Math.Ceiling(((float)context.InputFrame.Duration.Value.Milliseconds / ((1.0f / (float)videoFrameRate) * 1000)));
}

HRESULT AudioProcessor::BuildFloatBuffer(Microsoft::WRL::ComPtr<IMFSample> pSample, UINT channels, UINT sampleRate, std::queue<float>* floatBuffer, double *durationMs)
{	
	LONGLONG duration; //duration in units of 100-nanosecond
	HRESULT hr = pSample->GetSampleDuration(&duration);

	Microsoft::WRL::ComPtr<IMFMediaBuffer> pInput;

	hr = pSample->ConvertToContiguousBuffer(&pInput);
	if (FAILED(hr))
	{
		return hr;
	}

	DWORD currentLengthIn(0);
	DWORD maxLengthIn(0);
	DWORD currentLengthOut(0);
	DWORD maxLengthOut(0);

	BYTE* pInputBytes = nullptr;
	BYTE* pOutputBytes = nullptr;

	hr = pInput->Lock(&pInputBytes, &maxLengthIn, &currentLengthIn);
	if (FAILED(hr))
	{
		return hr;
	}

	for (int i = 0; i < currentLengthIn / 4; i++) {
		float sampleData = *(float*)(pInputBytes + (i * 4));
		if (i % 2 == 0) {
			(*floatBuffer).push(sampleData);
		}
	}

	*durationMs = (duration * 100.0f) / 1000000.0f;

	return S_OK;

	/*Duration 3276190 units of 100 nanoseconds
	327619000 nanoseconds
	1 millisecond = 1000000 nanoseconds
	= > Duration == 327.61900 miliseconds

	Num samples = 115, 584 bytes
	= 155, 584 / 4 floats = 38, 896 floats
	= 19, 448 mono samples

	number in buffer at end is 14, 448 <=
	44100 samples per second = 44.1 samples per milisecond 32.76 miliseconds = 14, 447.9 <= samples
	
	327.61900ms worth of audio samples => 327.61900 / (1000/60) 19.65714 vblanks worth of samples
	
	14,448 samples / 19.65714 vblanks = 735.0001 samples per vbank
	*/
}

void AudioProcessor::CalcRMS(std::queue<float>* floatBuffer, int samplesPerVBlank, std::queue<double>* rmsList) {
	//TODO: calc stereo, store in a RMS struct with vblanknumber, left and right RMS values
	float sum = 0.0f;
	while (floatBuffer->size() >= samplesPerVBlank) {
		for (int i = 0; i < samplesPerVBlank; i++) {
			auto value = floatBuffer->front();
			floatBuffer->pop();
			sum += value * value;
		}
		double rms = sqrt(sum / (samplesPerVBlank));
		rmsList->push(rms);
	}
}
