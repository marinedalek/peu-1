//------------------------------------------------------------------------
// Copyright(c) 2026 marinedalek.
//------------------------------------------------------------------------

#include "Peu1processor.h"
#include "Peu1cids.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/base/futils.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "public.sdk/source/vst/vstaudioprocessoralgo.h"

using namespace Steinberg;

namespace Marinedalek {
//------------------------------------------------------------------------
// Peu1Processor
//------------------------------------------------------------------------
Peu1Processor::Peu1Processor ()
{
	//--- set the wanted controller for our processor
	setControllerClass (kPeu1ControllerUID);
}

//------------------------------------------------------------------------
Peu1Processor::~Peu1Processor ()
{}

//------------------------------------------------------------------------
tresult PLUGIN_API Peu1Processor::initialize (FUnknown* context)
{
	// Here the Plug-in will be instantiated
	
	//---always initialize the parent-------
	tresult result = AudioEffect::initialize (context);
	// if everything Ok, continue
	if (result != kResultOk)
	{
		return result;
	}

	//--- create Audio IO ------
	addAudioInput (STR16 ("Stereo In"), Steinberg::Vst::SpeakerArr::kStereo);
	addAudioOutput (STR16 ("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo);

	lowPass12.push_back({});
	lowPass12.push_back({});
	lowPass6.push_back({});
	lowPass6.push_back({});

	highPass12.push_back({});
	highPass12.push_back({});
	highPass6.push_back({});
	highPass6.push_back({});
	
	/* If you don't need an event bus, you can remove the next line */
	addEventInput (STR16 ("Event In"), 1);

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Peu1Processor::terminate ()
{
	// Here the Plug-in will be de-instantiated, last possibility to remove some memory!
	
	//---do not forget to call parent ------
	return AudioEffect::terminate ();
}

//------------------------------------------------------------------------
tresult PLUGIN_API Peu1Processor::setActive (TBool state)
{
	//--- called when the Plug-in is enable/disable (On/Off) -----
	return AudioEffect::setActive (state);
}

//------------------------------------------------------------------------
tresult PLUGIN_API Peu1Processor::process (Vst::ProcessData& data)
{
	mBypassFadeTarget = this->processSetup.sampleRate / 100;

	//--- First : Read inputs parameter changes-----------

	if (data.inputParameterChanges) {
		// for each parameter defined by its ID
		int32 numParamsChanged = data.inputParameterChanges->getParameterCount();
		for (int32 index = 0; index < numParamsChanged; ++index) {
			// for this parameter we could iterate the list of value changes
			// in this example, we get only the last value (getPointCount - 1)
			Vst::IParamValueQueue* paramQueue = data.inputParameterChanges->getParameterData(index);
			if (paramQueue) {
				Vst::ParamValue value;
				int32 sampleOffset;
				int32 numPoints = paramQueue->getPointCount();
				switch (paramQueue->getParameterId()) {
				case Peu1Params::kParamGainId:
					if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
						mGain = value;
					break;
				case Peu1Params::kParamTopCutId:
					if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue) {
						mTopCutKnob = value;
					}
					break;
				case Peu1Params::kParamBassCutId:
					if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue) {
						mBassCutKnob = value;
					}
					break;
				case Peu1Params::kParamBypassId:
					if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue) {
						mBypass = (value > 0.5f);
					}
					break;
				}
			}
		}
	}
	auto topCutIndex = FromNormalized<Vst::ParamValue>(mTopCutKnob, 5);
	mTopCutF = mTopCutDetents[topCutIndex].f;
	mTopCutActive = mTopCutDetents[topCutIndex].active;
	auto bassCutIndex = FromNormalized<Vst::ParamValue>(mBassCutKnob, 4);
	mBassCutF = mBassCutDetents[bassCutIndex].f;
	mBassCutActive = mBassCutDetents[bassCutIndex].active;
	//-- Flush case: we only need to update parameter, noprocessing possible
	if (data.numInputs == 0 || data.numSamples == 0)
		return kResultOk;

	//--- Here, you have to implement your processing
	int32 numChannels = data.inputs[0].numChannels;

	//---get audio buffers using helper-functions(vstaudioprocessoralgo.h)------
	uint32 sampleFramesSize = getSampleFramesSizeInBytes(processSetup, data.numSamples);
	void** in = getChannelBuffersPointer(processSetup, data.inputs[0]);
	void** out = getChannelBuffersPointer(processSetup, data.outputs[0]);

	// Here could check the silent flags
	//---check if silence---------------
	// normally we have to check each channel (simplification)
	
	if ((data.inputs[0].silenceFlags != 0)
		&& (data.inputs[1].silenceFlags != 0)
		&& lowPass12[0].isIdle()
		&& lowPass12[1].isIdle()
		&& lowPass6[0].isIdle()
		&& lowPass6[1].isIdle()
		&& highPass12[0].isIdle()
		&& highPass12[1].isIdle()
		&& highPass6[0].isIdle()
		&& highPass6[1].isIdle()) {
		// mark output silence too
		data.outputs[0].silenceFlags = data.inputs[0].silenceFlags;

		// the plug-in has to be sure that if it sets the flags silence that the output buffers are clear
		for (int32 i = 0; i < numChannels; i++) {
			// do not need to be cleared if the buffers are the same (in this case input buffers are
			// already cleared by the host)
			if (in[i] != out[i]) {
				memset(out[i], 0, sampleFramesSize);
			}
		}
		// nothing to do at this point
		return kResultOk;
	}

	//--- We assume that the fade for each channel is synced.
	if (mBypass && mBypassFade[0] >= mBypassFadeTarget) {
		for (int32 i = 0; i < numChannels; i++) {
			if (in[i] != out[i]) {
				memcpy(out[i], in[i], sampleFramesSize);
			}
		}
		return kResultOk;
	}

	// now we will produce the output
	// mark our outputs as not silent
	data.outputs[0].silenceFlags = 0;

	/* static Vst::ParamValue mOldTopCutF{6400};
	static Vst::ParamValue mOldBassCutF{ 110 };
	

	static bool mOldTopCutActive{ false };
	static bool mOldBassCutActive{ false };
	

	static ExpInterpolate<Vst::ParamValue> mTopCutFSmooth[2];
	static ExpInterpolate<Vst::ParamValue> mBassCutFSmooth[2];

	static CosInterpolate<float> mTopCutActiveSmooth[2];
	static CosInterpolate<float> mBassCutActiveSmooth[2];
	*/

	// TODO De-magic-number this bit
	int interpolate_samples = 0.01 * this->processSetup.sampleRate;

	if (mTopCutF != mOldTopCutF) {
		if (mTopCutFSmooth[0].isDone()) {
			mTopCutFSmooth[0].setup(mOldTopCutF, mTopCutF, interpolate_samples);
			mTopCutFSmooth[1].setup(mOldTopCutF, mTopCutF, interpolate_samples);
		}
		else {
			mTopCutFSmooth[0].setup(mTopCutFSmooth[0].read(), mTopCutF, interpolate_samples);
			mTopCutFSmooth[1].setup(mTopCutFSmooth[1].read(), mTopCutF, interpolate_samples);
		}
		mOldTopCutF = mTopCutF;
	}

	if (mBassCutF != mOldBassCutF) {
		if (mBassCutFSmooth[0].isDone()) {
			mBassCutFSmooth[0].setup(mOldBassCutF, mBassCutF, interpolate_samples);
			mBassCutFSmooth[1].setup(mOldBassCutF, mBassCutF, interpolate_samples);
		}
		else {
			mBassCutFSmooth[0].setup(mBassCutFSmooth[0].read(), mBassCutF, interpolate_samples);
			mBassCutFSmooth[1].setup(mBassCutFSmooth[1].read(), mBassCutF, interpolate_samples);
		}
		mOldBassCutF = mBassCutF;
	}

	if (mTopCutActive != mOldTopCutActive) {
		if (mTopCutActiveSmooth[0].isDone()) {
			mTopCutActiveSmooth[0].setup(
				mOldTopCutActive ? 1. : 0., mTopCutActive ? 1. : 0., interpolate_samples);
			mTopCutActiveSmooth[1].setup(
				mOldTopCutActive ? 1. : 0., mTopCutActive ? 1. : 0., interpolate_samples);
		}
		else {
			mTopCutActiveSmooth[0].setup(
				mTopCutActiveSmooth->read(), mTopCutActive ? 1. : 0., interpolate_samples);
			mTopCutActiveSmooth[1].setup(
				mTopCutActiveSmooth->read(), mTopCutActive ? 1. : 0., interpolate_samples);
		}
		mOldTopCutActive = mTopCutActive;
	}

	if (mBassCutActive != mOldBassCutActive) {
		for (int i{ 0 }; i < 2; ++i) {
			if (mBassCutActiveSmooth[i].isDone()) {
				mBassCutActiveSmooth[i].setup(
					mOldBassCutActive ? 1. : 0., mBassCutActive ? 1. : 0., interpolate_samples);
			}
			else {
				mBassCutActiveSmooth[i].setup(
					mBassCutActiveSmooth[i].read(), mBassCutActive ? 1. : 0., interpolate_samples);
			}
		}
		mOldBassCutActive = mBassCutActive;
	}

	float gain = mGain;
	// for each channel (left and right)
	for (int32 i = 0; i < numChannels; ++i) {
		int32 samples = data.numSamples;
		Vst::Sample32* ptrIn = (Vst::Sample32*)in[i];
		Vst::Sample32* ptrOut = (Vst::Sample32*)out[i];
		auto fLowPass = mTopCutFSmooth[i].read() / this->processSetup.sampleRate;
		lowPass12[i].setCoefficients(fLowPass, 1.0);
		lowPass6[i].setCoefficients(fLowPass);
		auto fHighPass = mBassCutFSmooth[i].read() / this->processSetup.sampleRate;
		highPass12[i].setCoefficients(fHighPass, 1.0);
		highPass6[i].setCoefficients(fHighPass);

		// for each sample in this channel
		while (--samples >= 0) {

			if (!mTopCutFSmooth[i].isDone()) {
				mTopCutFSmooth[i].run();
				fLowPass = mTopCutFSmooth[i].read() / this->processSetup.sampleRate;
				lowPass12[i].setCoefficients(fLowPass, 1.0);
				lowPass6[i].setCoefficients(fLowPass);
			}

			if (!mBassCutFSmooth[i].isDone()) {
				mBassCutFSmooth[i].run();
				fHighPass = mBassCutFSmooth[i].read() / this->processSetup.sampleRate;
				highPass12[i].setCoefficients(fHighPass, 1.0);
				highPass6[i].setCoefficients(fHighPass);
			}

			if (!mTopCutActiveSmooth[i].isDone()) {
				mTopCutActiveSmooth[i].run();
			}

			if (!mBassCutActiveSmooth[i].isDone()) {
				mBassCutActiveSmooth[i].run();
			}

			Vst::Sample32 start = (*ptrIn++);
			
			Vst::Sample32 middle;
			if (mTopCutActiveSmooth[i].read() > 0. 
				|| !lowPass12[i].isIdle() 
				|| !lowPass6[i].isIdle()) {
				lowPass12[i].run(start);
				lowPass6[i].run(lowPass12[i].lpf);
				auto mix = mTopCutActiveSmooth[i].read();
				middle = lowPass6[i].lpf * mix + start * (1. - mix);
			}
			else {
				middle = start;
			}
			Vst::Sample32 end;
			if (mBassCutActiveSmooth[i].read() > 0.
				|| !highPass12[i].isIdle()
				|| !highPass6[i].isIdle()) {
				highPass12[i].run(middle);
				highPass6[i].run(highPass12[i].hpf);
				auto mix = mBassCutActiveSmooth[i].read();
				end = highPass6[i].hpf * mix + middle * (1. - mix);
			} 
			else {
				end = middle;
			}

			//--- Handle bypass fade in/out ---
			if (mBypass && mBypassFade[i] < mBypassFadeTarget) {
				double mix = cos(mBypassFade[i] * 1. / mBypassFadeTarget * 3.1415926538) / 2. + .5;
				end = end * mix + start * (1. - mix);
				++mBypassFade[i];
			}
			else if (mBypass && mBypassFade[i] >= mBypassFadeTarget) {
				end = start;
			}
			else if (!mBypass && mBypassFade[i] > 0) {
				double mix = cos(mBypassFade[i] * 1. / mBypassFadeTarget * 3.1415926538) / 2. + .5;
				end = end * mix + start * (1. - mix);
				--mBypassFade[i];
			}

			(*ptrOut++) = end;
		}
	}

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Peu1Processor::setupProcessing (Vst::ProcessSetup& newSetup)
{
	//--- called before any processing ----
	return AudioEffect::setupProcessing (newSetup);
}

//------------------------------------------------------------------------
tresult PLUGIN_API Peu1Processor::canProcessSampleSize (int32 symbolicSampleSize)
{
	// by default kSample32 is supported
	if (symbolicSampleSize == Vst::kSample32)
		return kResultTrue;

	// disable the following comment if your processing support kSample64
	/* if (symbolicSampleSize == Vst::kSample64)
		return kResultTrue; */

	return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Peu1Processor::setState (IBStream* state)
{
	if (!state)
		return kResultFalse;
	// called when we load a preset, the model has to be reloaded
	IBStreamer streamer (state, kLittleEndian);
	float savedParam1 = 0.f;
	if (streamer.readFloat(savedParam1) == false)
		return kResultFalse;
	mGain = savedParam1;

	float savedTopCutKnob = 0.f;
	if (streamer.readFloat(savedTopCutKnob) == false)
		return kResultFalse;
	mTopCutKnob = savedTopCutKnob;

	float savedBassCutKnob = 0.f;
	if (streamer.readFloat(savedBassCutKnob) == false)
		return kResultFalse;
	mBassCutKnob = savedBassCutKnob;

	bool savedBypass = false;
	if (streamer.readBool(savedBypass) == false)
		return kResultFalse;
	mBypass = savedBypass;
	
	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Peu1Processor::getState (IBStream* state)
{
	// here we need to save the model
	float toSaveParam1 = mGain;
	float toSaveTopCutKnob = mTopCutKnob;
	float toSaveBassCutKnob = mBassCutKnob;
	IBStreamer streamer (state, kLittleEndian);
	streamer.writeFloat(toSaveParam1);
	streamer.writeFloat(toSaveTopCutKnob);
	streamer.writeFloat(toSaveBassCutKnob);
	streamer.writeBool(mBypass);

	return kResultOk;
}

//------------------------------------------------------------------------
} // namespace Marinedalek
