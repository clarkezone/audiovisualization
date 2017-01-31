#pragma once
class AudioProcessor {
public:
	HRESULT BuildVectorBuffer(Microsoft::WRL::ComPtr<IMFSample> pInput, DirectX::XMVECTOR** vectorBytes);

	void CalculateVolumes(DirectX::XMVECTOR* input);

	HRESULT BuildFloatBuffer(Microsoft::WRL::ComPtr<IMFSample> pInput, UINT channels, UINT sampleRate, std::queue<float> *floatBuffer, double *durationMs);
	void CalcRMS(std::queue<float>* floatBuffer, int samplesPerVBlank, std::queue<double>* rmsList);
};