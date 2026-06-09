
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
#include <atomic>



#define NUM_CAP_CHANNELS 30
#define NUM_OSCS 16
#define RMS_INTERVAL_MS 50
#define RECORD_BUTTON_CHANNEL 0  // which analog input the button is on

#define RELEASE_MS 50
#define CROSSFADE_MS 10 


//UTILITY GLOBALS
bool gMicOn = true;
bool gFeedbackOn = true;
int numOscillatorsOn = 0;
float gSmoothAlpha = 0.001f;

//Instantiate a scope
Scope scope;

//create a limiter
Limiter inputLimiter;
Limiter outputLimiter;


//Trill touchSensor;
Pipe gPipe;
Gui gui;
AuxiliaryTask sendTask;

//scope
float gOutput[NUM_OSCS];

// Sleep time for auxiliary task
unsigned int gTaskSleepTime = 12000; // microseconds

//int bitResolution = 12;

int gButtonValue = 0;

float filterChannels[NUM_OSCS];

// typedef enum {
// 	kPrescaler,
// 	kBaseline,
// 	kNoiseThreshold,
// 	kNumBits,
// 	kMode,
// } ids_t;

// typedef enum {
// 	One,
// 	Two,
// 	Three,
// 	Four,
// 	Five,
// } guiParams;

typedef enum {
	POT,
	TOUCH,
	REGISTER,
	BIG,
	FREQ,
	REC_SEND,
	PANNING,
	RMS,   
	LEFT,
	RIGHT,
	PRESET,
	AVAIL,
} buffers;

enum OscState {
    IDLE,
    ACTIVE,
    RELEASING
};

struct Oscillator {
    OscState state           = IDLE;
    float gain               = 0.0f;
    float targetGain         = 0.0f;
    float currentFrequency   = 0.0f;
    float targetFrequency    = 0.0f;
    float phase              = 0.0f;
    float panning            = 0.5f;
    float available			= true;
};

Oscillator gOscillators[NUM_OSCS];


// struct Command {
// 	ids_t id;
// 	float value;
// };

// struct SendParam {
// 	guiParams id;
// 	float value;
// };

typedef enum {
	ARITHMETIC_DIVISION,
	RATIO_DIVISION,
	TET,
} algorithms;


// std::vector<std::pair<std::wstring, guiParams>> gParams =
// {
// 	{L"one", One},
// 	{L"two", Two},
// 	{L"three", Three},
// 	{L"four", Four},
// 	{L"five", Five},
// };

// const std::map<std::string, int> configs {
// 	{"ATTACK", 5},
// 	{"DECAY", 2},
// 	{"SUSTAIN", 3},
// 	{"RELEASE", 4},
// 	{"TOP", 29},
// 	{"DIVISOR", 28},
// 	{"BOTTOM", 0},
// 	{"ALGORITHM", 22},
// 	{"GLIDE", 23},
// 	{"HOLD", 24}
// };

struct BufferState {

    // From Gui
    int   potIndex;         // Buffer 0
    float potValue;
    int   activePreset;     // Buffer 1 
    bool touchValue = false;   	// Buffer 2 
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


// ---- Ramp rates (computed in setup()) ----
float gReleaseRampRate   = 0.0f;  // per sample, during RELEASING
float gCrossfadeRampRate = 0.0f;  // per sample, during preset crossfade

// ---- Master crossfade ----
float gMasterGain        = 1.0f;
float gTargetMasterGain  = 1.0f;
std::atomic<bool> gPresetLoading{false};

// ranges for registers
int numerator_min = 1;
int numerator_max = 16;
int denominator_min = 1;
int denominator_max = 16;
int index_min = 0;
int index_max = 63;
int touch_min = 0;
int touch_max = 63;
int num_options = 3; // size of algorithms enum 
int div_min = 1; // no divisions 
int div_max = 32; // max 32 EDO 
float freq_min = 10.f;
float freq_max = 10000.f;


struct DeviceState {

    bool micButton           = false;
    bool keyboardButton      = false;
    bool lastSelectedButton  = false;
    bool savePresetButton    = false;
    bool setReferenceButton  = false;
    bool toggleOutputButton  = false;
    bool droneModeButton     = false;
    std::atomic<bool> recButton{false};


    float targetSynthGain    = 0.5f;
    float currentSynthGain   = 0.5f;

    float targetMicGain      = 3.0f;
    float currentMicGain     = 3.0f;

    float targetFbAmount     = 20.0f;
    float currentFbAmount    = 20.0f;

    float targetLpCutoff     = 5000.0f;
    float currentLpCutoff    = 5000.0f;

    float targetBaseFrequency = 220.0f;
    float currentBaseFrequency = 220.0f;

    float targetHiFreqBoost  = 0.0f;
    float currentHiFreqBoost = 0.0f;

    float filterQ            = 100.0f;
    float filterGain         = 10.0f;
    float limiterThresh      = 0.95f;
    float limiterLookahead   = 0.4f;
    float limiterRelease     = 10.0f;
    float glideAmount        = 10.0f;

    float numerator          = 2.0f;
    float denominator        = 1.0f;
    float divisor            = 4.0f;
    int   offset             = 0;
    algorithms selectedAlgorithm = ARITHMETIC_DIVISION;

    int   lastTouched        = 0;
    int   activePreset       = 0;
    float freqDisplay        = 0.0f;
};

BufferState gBufferState;
DeviceState gDeviceState;
DeviceState gPresets[10];


ADSR envelopes[NUM_OSCS]; // ADSR envelope

float gAttack = 0.03; // Envelope attack (seconds)
float gDecay = 0.05; // Envelope decay (seconds)
float gRelease = 0.5; // Envelope release (seconds)
float gSustain = 0.9; // Envelope sustain level

//float gFrequencies[NUM_OSCS]; // Oscillator frequency (Hz)
//float gPhases[NUM_OSCS]; // Oscillator phase

float gInverseSampleRate;

float jPhase = 0.0;
float jFreq = 1.0;

//DEFAULT CONFIG VALUES
float ratios[NUM_OSCS] = {1,1,1,1,1, 1, 1, 1};


float thresholdTouch = 0.1;
int voiceCount = 1;
bool holdMode = false;



bool gNeedsUpdate = false;
// In header
bool gNeedsFilterUpdate  = false;
bool gNeedsLimiterUpdate = false;
bool gNeedsFreqUpdate = false;

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
Biquad lpfilter;

Biquad::Settings s;
Biquad::Settings lps;

float gTimePeriod = 0.1;

bool gNewData = false;


///// RMS capture
float gSumOfSquares[NUM_OSCS] = {0};
int gSampleCount = 0;
int gWindowSize = 0; 

// ---- Shared between audio thread and aux task ----
float gRmsResults[NUM_OSCS] = {0};   // latest computed RMS values
float gTimestampMs = 0.0f;               // elapsed ms at time of capture
std::atomic<bool> gRmsReady{false};      // lock-free handoff flag

// ---- Recording state ----

float gRecordStartMs = 0.0f;             // Bela time when record started
bool gLastButtonState = false;           // for debounce
int gDebounceCounter = 0;
#define DEBOUNCE_FRAMES 100              // ~2ms at 44100Hz



