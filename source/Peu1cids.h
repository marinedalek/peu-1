//------------------------------------------------------------------------
// Copyright(c) 2026 marinedalek.
//------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

enum Peu1Params : Steinberg::Vst::ParamID {
	kParamGainId = 102,
	kParamTopCutId,
	kParamBassCutId,
	kParamBypassId,
};

namespace Marinedalek {
//------------------------------------------------------------------------
static const Steinberg::FUID kPeu1ProcessorUID (0xCFBFD7B1, 0x7EA055A5, 0xB270DCC1, 0xA2012A5C);
static const Steinberg::FUID kPeu1ControllerUID (0x86A61BDB, 0x3E95590F, 0x868BF430, 0x1C63CB0F);

#define Peu1VST3Category "Fx|Filter"

//------------------------------------------------------------------------
} // namespace Marinedalek
