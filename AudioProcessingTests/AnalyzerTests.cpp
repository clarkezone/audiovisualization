#include "pch.h"
#include "CppUnitTest.h"
#include"..\AudioProcessing\SpectrumAnalyzer.h"
#include "MFHelpers.h"
#include <algorithm>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Microsoft::WRL;

namespace AudioProcessingTests
{
	TEST_CLASS(AnalyzerTests)
	{
	public:
		TEST_METHOD(SpectrumAnalyzer_Configure)
		{
			using namespace AudioProcessing;
			using namespace Microsoft::WRL;
			CSpectrumAnalyzer analyzer;

			ComPtr<IMFMediaType> mediaType;
			MFCreateMediaType(&mediaType);
			// Test for media type error conditions
			mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
			Assert::IsTrue(FAILED(analyzer.Configure(mediaType.Get(), 4, 2, 64)), L"Only audio type should be supported");
			mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
			mediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
			Assert::IsTrue(FAILED(analyzer.Configure(mediaType.Get(), 4, 2, 64)), L"Only float type should be supported");
			// Set float type, 48000Hz sample rate, 2 channels
			mediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float);
			mediaType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000u);
			mediaType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, 2u);

			Assert::IsTrue(FAILED(analyzer.Configure(mediaType.Get(), 32, 48, 64)), L"Overall input length needs to be less than fft");
			Assert::IsTrue(FAILED(analyzer.Configure(mediaType.Get(), 4, 2, 63)), L"FFT length needs to be power of 2");

			Assert::IsTrue(SUCCEEDED(analyzer.Configure(mediaType.Get(), 800, 400, 4096)), L"Configure 60fps output with 50% overlap and 2^12 fft length");
		}
		TEST_METHOD(SpectrumAnalyzer_Run)
		{
			using namespace AudioProcessing;
			using namespace Microsoft::WRL;
			CSpectrumAnalyzer analyzer;
			ComPtr<IMFMediaType> mediaType;
			const unsigned sampleRate = 48000;
			const unsigned fftLength = 4096;
			const float pi = 3.14159f;

			CreateFloat32AudioType(sampleRate, 32, 2, &mediaType);

			Assert::IsTrue(SUCCEEDED(analyzer.Configure(mediaType.Get(), 800, 400, fftLength)));

			// No samples queued, step needs to return S_FALSE
			Assert::AreEqual(analyzer.Step(nullptr,nullptr), S_FALSE);

			// To test border conditions create 3 samples with lengths set to 1200,400 and 800 samples
			unsigned sampleLengths[5] = { 1200,400,800,1200,1200 };
			unsigned sampleOffset = 0;
			// Set one channel frequency at 1/32th (1500Hz) of sample rate and another one to 1/128th (375Hz)
			const unsigned T1 = 32, T2 = 128;	// Indicates one period length in samples
			float w1 = 2.0f * pi * (1.0f / T1), w2 = 2.0f * pi * (1.0f/T2);
			for each (unsigned sampleLength in sampleLengths)
			{
				ComPtr<IMFSample> sample;
				HRESULT hr = MFCreateSample(&sample);
				sample->SetSampleTime((10000000L * (long long)sampleOffset) / sampleRate);	// Need to cast to long long to avoid overflow
				sample->SetSampleDuration((10000000L * (long long)sampleLength) / sampleRate);

				ComPtr<IMFMediaBuffer> buffer;
				hr = MFCreateMemoryBuffer(sampleLength * sizeof(float) * 2, &buffer);	// 2 for stereo
				float *pBuffer;
				hr = buffer->Lock((BYTE **)&pBuffer, nullptr, nullptr);

				for (size_t sampleIndex = 0; sampleIndex < sampleLength; sampleIndex++)
				{
					pBuffer[sampleIndex * 2] = sinf(w1*(sampleIndex+sampleOffset));
					pBuffer[sampleIndex * 2 + 1] = cosf(w2*(sampleIndex + sampleOffset));
				}

				hr = buffer->Unlock();
				hr = buffer->SetCurrentLength(sampleLength * sizeof(float) * 2);
				hr = sample->AddBuffer(buffer.Get());
				sampleOffset += sampleLength;
				Assert::IsTrue(SUCCEEDED(analyzer.QueueInput(sample.Get())));
			}

			REFERENCE_TIME hnsExpectedTimes[] = { 0L,166666L,333333L };
			REFERENCE_TIME hnsTime = 0;
			float f[fftLength];

			// Now run three steps
			for (size_t step = 0; step < 3; step++)
			{
				Assert::AreEqual(analyzer.Step(&hnsTime, f), S_OK);
				float peak_volume[2] = { 0.0f,0.0f };
				int peak_index[2] = { -1, -1 };
				float total[2] = { f[0],f[1] };

				for (size_t channel = 0; channel < 2; channel++)
				{
					for (size_t index = channel; index < fftLength; index += 2)
					{
						total[channel] += f[index];
						// Find maximum
						if (f[index] > peak_volume[channel])
						{
							peak_index[channel] = (index) >> 1;
							peak_volume[channel] = f[index];
						}
					}
				}
				// Validate the timing info
				Assert::IsTrue(hnsExpectedTimes[step] == hnsTime);
				// Make sure the spectrum maximums are at defined 
				Assert::AreEqual(peak_index[0],(int) (fftLength / T1));
				Assert::AreEqual(peak_index[1],(int) (fftLength / T2));
			}
			REFERENCE_TIME skipTo = 833333;	// This should be frame 4000
			Assert::IsTrue(analyzer.Skip(skipTo)==S_OK);	// Skip to sample 4000
			// This should leave just 800 frames in input queue
			Assert::IsTrue(analyzer.Step(&hnsTime, f) == S_OK);
			Assert::IsTrue(hnsTime == skipTo);
		}
	};
}