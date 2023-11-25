#pragma once
#include <rack.hpp>


using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;

// Declare each Model, defined in each module source file
extern Model* modelReverb;
extern Model* modelFilter;
extern Model* modelDispersion;
extern Model* modelSawtooth;
extern Model* modelSine;
