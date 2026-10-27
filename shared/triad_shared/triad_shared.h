/*
BEGIN_JUCE_MODULE_DECLARATION

 ID:                 triad_shared
 vendor:             Triad
 version:            1.0.0
 name:               Triad shared plugin code
 description:        Branding, look-and-feel and spectrum analyzer shared by the Sonic Lab plugins
 license:            Proprietary
 dependencies:       juce_gui_basics juce_dsp
 minimumCppStandard: 17

END_JUCE_MODULE_DECLARATION
*/

#pragma once
#define TRIAD_SHARED_H_INCLUDED

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "dsp/SpectrumTap.h"
#include "ui/TriadLookAndFeel.h"
#include "ui/TriadHeader.h"
#include "ui/SpectrumAnalyzer.h"
