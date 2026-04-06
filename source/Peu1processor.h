//------------------------------------------------------------------------
// Copyright(c) 2026 marinedalek.
//------------------------------------------------------------------------

#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include <vector>

namespace Marinedalek {
template<typename SampleType>
class StateVariableFilter {
public:
	void setCoefficients(SampleType f, SampleType q)
	{
		w = 2 * tan(3.1415926538 * f);
		a = w / q;
		b = w * w;
		c1 = (a + b) / (1 + a / 2 + b / 4);
		c2 = b / (a + b);
		d0_lpf = c1 * c2 / 4;
		d0_hpf = 1 - c1 / 2 + c1 * c2 / 4;
	}
	void run(SampleType input)
	{
		auto x = input - z1 - z2;
		hpf = d0_hpf * x;
		z2 += c2 * z1;
		lpf = d0_lpf * x + z2;
		z1 += c1 * x;
	}
	bool isIdle() 
	{
		return ((z1 < 1.0e-10) && (z2 < 1.0e-10));
	}

	SampleType lpf = 0;
	SampleType hpf = 0;
private:
	SampleType w;
	SampleType a;
	SampleType b;
	SampleType c1;
	SampleType c2;
	SampleType d0_lpf;
	SampleType d0_hpf;
	SampleType z1 = 0;
	SampleType z2 = 0;
};

template<typename SampleType>
class ButterworthFilter {
public:
	void setCoefficients(SampleType f)
	{
		w = 2 * 3.1415926538 * f;
		b = cos(w) / (1 + sin(w));
		a = (1 - b) / 2;
	}
	void run(SampleType input)
	{
		lpf = a * (input + lastInput) + b * lastOutput;
		hpf = input - lpf;
		lastInput = input;
		lastOutput = lpf;
	}
	bool isIdle()
	{
		return ((lastInput < 1.0e-10) && (lastOutput < 1.0e-10));
	}
	SampleType lpf = 0;
	SampleType hpf = 0;
private:
	SampleType w{ 0 };
	SampleType a{ 0 };
	SampleType b{ 0 };
	SampleType lastInput{ 0 };
	SampleType lastOutput{ 0 };
};

template<typename SampleType>
class ExpInterpolate {
public:
	void setup (SampleType origin, SampleType target, int steps)
	{
		mAccumulator = origin;
		mTarget = target;
		if (steps > 0) {
			// this only works because I know I'm not going to call it with
			// negative/zero origin/targets
			mMultiplier = exp((log(target) - log(origin)) / steps);
			mCounter = steps;
		}
		else {
			mMultiplier = 1;
			mCounter = 0;
		}
	}

	SampleType run()
	{
		if (isDone())
		{
			return mTarget;
		}
		else {
			mAccumulator *= mMultiplier;
			--mCounter;
			return mAccumulator;
		}
	}

	SampleType read()
	{
		if (isDone()) {
			return mTarget;
		}
		else {
			return mAccumulator;
		}
	}

	bool isDone()
	{
		return mCounter <= 0;
	}

private:
	SampleType mTarget{ 0 };
	SampleType mAccumulator{ 0 };
	SampleType mMultiplier{ 1 };
	int mCounter{ 0 };
};

template<typename SampleType>
class CosInterpolate {
public:
	void setup(SampleType origin, SampleType target, int steps)
	{
		mOrigin = origin;
		mTarget = target;
		mSteps = steps;
		mCounter = steps;
	}

	SampleType run()
	{
		if (!isDone()) {
			--mCounter;
		}
		return read();
	}

	SampleType read()
	{
		if (!isDone()) {
			auto elapsed = mSteps - mCounter;
			auto fade = cos(3.1415926538 * elapsed / mSteps) / -2. + .5;
			return (mTarget - mOrigin) * fade + mOrigin;
		}
		return mTarget;
	}
	bool isDone()
	{
		return (mCounter <= 0);
	}
private:
	SampleType mOrigin{ 0 };
	SampleType mTarget{ 0 };
	int mSteps{ 0 };
	int mCounter{ 0 };
};

struct CutDetent {
	bool active;
	Steinberg::Vst::ParamValue f;
};

//------------------------------------------------------------------------
//  Peu1Processor
//------------------------------------------------------------------------
class Peu1Processor : public Steinberg::Vst::AudioEffect {
public:
	Peu1Processor();
	~Peu1Processor() SMTG_OVERRIDE;

	// Create function
	static Steinberg::FUnknown* createInstance(void* /*context*/)
	{
		return (Steinberg::Vst::IAudioProcessor*)new Peu1Processor;
	}

	//--- ---------------------------------------------------------------------
	// AudioEffect overrides:
	//--- ---------------------------------------------------------------------
	/** Called at first after constructor */
	Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;

	/** Called at the end before destructor */
	Steinberg::tresult PLUGIN_API terminate() SMTG_OVERRIDE;

	/** Switch the Plug-in on/off */
	Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) SMTG_OVERRIDE;

	/** Will be called before any process call */
	Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& newSetup) SMTG_OVERRIDE;

	/** Asks if a given sample size is supported see SymbolicSampleSizes. */
	Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolicSampleSize) SMTG_OVERRIDE;

	/** Here we go...the process call */
	Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;

	/** For persistence */
	Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) SMTG_OVERRIDE;

	//------------------------------------------------------------------------
protected:
	//---- Parameters ----
	Steinberg::Vst::ParamValue mGain = 1.;
	Steinberg::Vst::ParamValue mTopCutKnob = 0.;
	Steinberg::Vst::ParamValue mBassCutKnob = 0.;
	bool mBypass = false;

	bool mTopCutActive = false;
	Steinberg::Vst::ParamValue mTopCutF = 3200.;
	bool mBassCutActive = false;
	Steinberg::Vst::ParamValue mBassCutF = 220.;
	CutDetent const mTopCutDetents[6] = {
		{false,3200},
		{true,3200},
		{true,1600},
		{true,800},
		{true,400},
		{true,200}
	};
	CutDetent const mBassCutDetents[5] = {
		{false,220},
		{true,220},
		{true,440},
		{true,880},
		{true,1760}
	};
	std::vector<StateVariableFilter<Steinberg::Vst::Sample32>> lowPass12, highPass12;
	std::vector<ButterworthFilter<Steinberg::Vst::Sample32>> lowPass6, highPass6;
	Steinberg::int32 mBypassFade[2] = { 0,0 };
	Steinberg::int32 mBypassFadeTarget = 0;

	bool mOldTopCutActive{ false };
	CosInterpolate<Steinberg::Vst::Sample32> mTopCutActiveSmooth[2];
	Steinberg::Vst::ParamValue mOldTopCutF{ 6400 };
	ExpInterpolate<Steinberg::Vst::ParamValue> mTopCutFSmooth[2];

	bool mOldBassCutActive{ false };
	CosInterpolate<Steinberg::Vst::Sample32> mBassCutActiveSmooth[2];
	Steinberg::Vst::ParamValue mOldBassCutF{ 110 };
	ExpInterpolate<Steinberg::Vst::ParamValue> mBassCutFSmooth[2];
};

//------------------------------------------------------------------------
} // namespace Marinedalek
