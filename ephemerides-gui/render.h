
#include <Bela.h>
// #include <libraries/Trill/Trill.h>
#include <libraries/Gui/Gui.h>
#include <libraries/Pipe/Pipe.h>
#include <cmath>
#include <libraries/ADSR/ADSR.h>
#include <stdlib.h>
#include <libraries/Biquad/Biquad.h>
#include <vector>
#include <libraries/Fft/Fft.h>
#include <stdexcept>
#include <libraries/Scope/Scope.h>
#include <lookahead.h>
#include <libraries/math_neon/math_neon.h>



#define NUM_CAP_CHANNELS 30
#define NUM_OSCS 8
#define NUM_KEYS 12
#define analogBaseFreq 0 
#define analogAmplitude 1

// dark purple - 3.3v
// grey - gnd 
// white - sda 
// black. - sdl 

//Instantiate a scope
Scope scope;

//create a limiter
Limiter inputLimiter;
Limiter outputLimiter;


//Trill touchSensor;
Pipe gPipe;
Gui gui;

//scope
float gOutput[NUM_OSCS];

// Sleep time for auxiliary task
unsigned int gTaskSleepTime = 12000; // microseconds

int bitResolution = 12;

int gButtonValue = 0;

float filterChannels[NUM_OSCS];

typedef enum {
	kPrescaler,
	kBaseline,
	kNoiseThreshold,
	kNumBits,
	kMode,
} ids_t;

typedef enum {
	One,
	Two,
	Three,
	Four,
	Five,
} guiParams;


struct Command {
	ids_t id;
	float value;
};

struct SendParam {
	guiParams id;
	float value;
};

typedef enum {
	ARITHMETIC_DIVISION,
	RATIO_DIVISION,
	TET,
} algorithms;


std::vector<std::pair<std::wstring, guiParams>> gParams =
{
	{L"one", One},
	{L"two", Two},
	{L"three", Three},
	{L"four", Four},
	{L"five", Five},
};

const std::map<std::string, int> configs {
	{"ATTACK", 5},
	{"DECAY", 2},
	{"SUSTAIN", 3},
	{"RELEASE", 4},
	{"TOP", 29},
	{"DIVISOR", 28},
	{"BOTTOM", 0},
	{"ALGORITHM", 22},
	{"GLIDE", 23},
	{"HOLD", 24}
};

struct BufferState {

    // From Gui
    int   potIndex;         // Buffer 0
    float potValue;
    int   activePreset;     // Buffer 1 
    float touchValue;   	// Buffer 2 
    int   touchIndex;
    int   registerIndex;	// Buffer 3
    int   registerValue;
    float bigButtons;		// Buffer 4 
    float freqButtons;		// Buffer 5
    

    // to Gui... h for hardware
    int   hPotIndex;        // Buffer 6
    float hPotValue;
    int   hActivePreset;    // Buffer 7 
    int   hTouchIndex;      // Buffer 8
    float hTouchValue;
    float hAvailability;	
    float hSevenSegment;	// Buffer 9 
    int   hRegisterIndex;	// Buffer 10 
    int   hRegisterValue;
    float hBigButtons;		// Buffer 11
    float hFreqButtons;		// Buffer 12
    
    // filter Channel volumes
    float volumes[32];		//Buffer 13
	
};

// i think that this struct would serve as preset data 
struct DeviceState {
	
	//buttons
	bool micButton;
	bool keyboardButton;
	bool lastSelectedButton;
	bool savePresetButton;
	bool setReferenceButton;
	bool toggleOutputButton;
	bool droneModeButton;
	
	//potentiometers
	float hiFreqBoost;
	float filterQ;
	float filterGain;
	float lpCutoff;
	float micGain;
	float filterChannelGain;
	float limiterThresh;
	float limiterLookahead;
	float limiterRelease;
	float baseFrequency = 220;
	float fbAmount;
	float glideAmount = 10;
	
	//touch data 
	
	//preset data 
	int activePreset;
	
	//freq display
	float freqDisplay;
	
	//config data
	float numerator = 2;
	float denominator = 1;
	float divisor = 4;
	algorithms selectedAlgorithm = ARITHMETIC_DIVISION;
	
};

BufferState gBufferState;
DeviceState gDeviceState;

// int keys[NUM_KEYS] = {8,9,10,11,12,13,14,15,16,17,18,19,20,21};
//with two broken keys
int keys[NUM_KEYS] = {8,9,11,12,13,14,15,16,17,18,19,21};
int oscillatorsOn[NUM_OSCS] = {0, 1, 2, 3, 4, 5, 6, 7};

ADSR envelopes[NUM_OSCS]; // ADSR envelope

float gAttack = 0.03; // Envelope attack (seconds)
float gDecay = 0.05; // Envelope decay (seconds)
float gRelease = 0.5; // Envelope release (seconds)
float gSustain = 0.9; // Envelope sustain level

float gFrequencies[NUM_OSCS]; // Oscillator frequency (Hz)
float gPhases[NUM_OSCS]; // Oscillator phase
float targetRatio[NUM_OSCS] = {0,0,0,0,0,0, 0, 0};
float gInverseSampleRate;

float jPhase = 0.0;
float jFreq = 1.0;

//DEFAULT CONFIG VALUES
float ratios[NUM_OSCS] = {1,1,1,1,1, 1, 1, 1};


float thresholdTouch = 0.1;
//std::string configMode = "OFF";
int voiceCount = 1;
bool holdMode = false;



bool needsUpdate = false;



int gAudioFramesPerAnalogFrame = 0;

// Set the analog channels to read from
int gSensorInputFrequency = 0;
int gSensorInputAmplitude = 1;
int gSensorInputSlider = 2;


// Oscillator type
enum osc_type
{
	sine,		// 0
	triangle,	// 1
	square,		// 2
	sawtooth,	// 3
	numOscTypes
};

unsigned int gOscType = 0; // Selected oscillator type
float gAmplitude; // Amplitude scaling for oscillator's envelope

int gTriggerButtonPin = 0; // Digital pin to which gate button should be connected
int gTriggerButtonLastStatus = 0; // Last status of gate button

int gModeButtonPin = 1; // Digital pin to which oscillator selection button should be connected
int gModeButtonLastStatus = 0; // Last status of oscillator selection button

//biquad filtering
std::vector<Biquad> filterBank(NUM_OSCS);

Biquad::Settings s;

float gTimePeriod = 0.1;

bool gNewData = false;



