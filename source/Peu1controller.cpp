//------------------------------------------------------------------------
// Copyright(c) 2026 marinedalek.
//------------------------------------------------------------------------

#include "Peu1controller.h"
#include "pluginterfaces/base/ibstream.h"
#include "Peu1cids.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "pluginterfaces/base/ustring.h"
#include "base/source/fstreamer.h"
#include "base/source/fstring.h"

using namespace Steinberg;

namespace Marinedalek {

//------------------------------------------------------------------------
// Peu1Controller Implementation
//------------------------------------------------------------------------
tresult PLUGIN_API Peu1Controller::initialize (FUnknown* context)
{
	using namespace Steinberg::Vst;
	// Here the Plug-in will be instantiated

	//---do not forget to call parent ------
	tresult result = EditControllerEx1::initialize (context);
	if (result != kResultOk)
	{
		return result;
	}

	// Here you could register some parameters
	auto* topCutParam = new StringListParameter(STR16("Top Cut"), kParamTopCutId, STR16("c/s"));
	topCutParam->appendString(STR16("NIL"));
	topCutParam->appendString(STR16("3200"));
	topCutParam->appendString(STR16("1600"));
	topCutParam->appendString(STR16("800"));
	topCutParam->appendString(STR16("400"));
	topCutParam->appendString(STR16("200"));
	parameters.addParameter(topCutParam);

	auto* bassCutParam = new StringListParameter(STR16("Bass Cut"), kParamBassCutId, STR16("c/s"));
	bassCutParam->appendString(STR16("NIL"));
	bassCutParam->appendString(STR16("220"));
	bassCutParam->appendString(STR16("440"));
	bassCutParam->appendString(STR16("880"));
	bassCutParam->appendString(STR16("1760"));
	parameters.addParameter(bassCutParam);

	parameters.addParameter(STR16("Bypass"), nullptr, 1, 0,
		ParameterInfo::kCanAutomate | ParameterInfo::kIsBypass, kParamBypassId);

	return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Peu1Controller::terminate ()
{
	// Here the Plug-in will be de-instantiated, last possibility to remove some memory!

	//---do not forget to call parent ------
	return EditControllerEx1::terminate ();
}

//------------------------------------------------------------------------
tresult PLUGIN_API Peu1Controller::setComponentState (IBStream* state)
{
	// Here you get the state of the component (Processor part)
	if (!state)
		return kResultFalse;

	IBStreamer streamer(state, kLittleEndian);
	// sync with our parameter
	float savedTopCutKnob = 0.f;
	if (streamer.readFloat(savedTopCutKnob) == false)
		return kResultFalse;
	if (auto param = parameters.getParameter(Peu1Params::kParamTopCutId))
		param->setNormalized(savedTopCutKnob);

	float savedBassCutKnob = 0.f;
	if (streamer.readFloat(savedBassCutKnob) == false)
		return kResultFalse;
	if (auto param = parameters.getParameter(Peu1Params::kParamBassCutId))
		param->setNormalized(savedBassCutKnob);

	bool bypassState = 0;
	if (streamer.readBool(bypassState) == false)
		return kResultFalse;
	if (auto param = parameters.getParameter(Peu1Params::kParamBypassId))
		param->setNormalized(bypassState ? 1 : 0);

	return kResultOk;
}
/*
//------------------------------------------------------------------------
tresult PLUGIN_API Peu1Controller::setState (IBStream* state)
{
	// Here you get the state of the controller
`
	return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Peu1Controller::getState (IBStream* state)
{
	// Here you are asked to deliver the state of the controller (if needed)
	// Note: the real state of your plug-in is saved in the processor

	return kResultTrue;
}
*/

//------------------------------------------------------------------------
IPlugView* PLUGIN_API Peu1Controller::createView (FIDString name)
{
	// Here the Host wants to open your editor (if you have one)
	if (FIDStringsEqual (name, Vst::ViewType::kEditor))
	{
		// create your editor here and return a IPlugView ptr of it
		auto* view = new VSTGUI::VST3Editor (this, "view", "Peu1editor.uidesc");
		return view;
	}
	return nullptr;
}

//------------------------------------------------------------------------
} // namespace Marinedalek
