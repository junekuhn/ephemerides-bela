#include <render.h>


//converting from midi key to index
// int displaceIndex(int key) {
// 	int offset = NUM_KEYS - 1;
// 	int c = 0;
	
// 	if((int) gDeviceState.divisor % 2 == 0) {
// 		offset = NUM_KEYS;
// 		c = (offset - gDeviceState.divisor) / 2 + 1;
// 	} else {
// 		c = (offset - gDeviceState.divisor) / 2;
// 	}
	
	
// 	return key - c;
// }

// void storePreset(DeviceState currentState, int index) {
// 	if(index < 10 && index >= 0) {
// 		gPresets[index] = currentState;
// 	}
// }

//daisy generated
void sendRmsBuffer(void*) {
    if(!gRmsReady.load()) return;

    float outBuffer[NUM_OSCS+1];
    outBuffer[0] = gTimestampMs;
    for(int i = 1; i<NUM_OSCS+1; i++ ) {
    	outBuffer[i] = gRmsResults[i];
    }

    gui.sendBuffer(RMS, outBuffer, NUM_OSCS+1);

    gRmsReady.store(false);
}

// DeviceState recallPreset(int index) {
// 	return gPresets[index];
// }

void printTask(float myFloat) {
 //   if(!gNewData) return;

    rt_printf("=== DeviceState ===\n");

    rt_printf("div, n, d:\n");
    rt_printf("%f \n", gDeviceState.recButton.load());

  //  gNewData = false;
   	usleep(5000);
}

float calculateRatio(algorithms algo, int index) {
	float ratio = 1;
	
	switch(algo) {
		case ARITHMETIC_DIVISION:
			ratio = ((float) index) * (gDeviceState.numerator - gDeviceState.denominator) / gDeviceState.divisor + gDeviceState.denominator;
			break;
		case RATIO_DIVISION:
			ratio = ((float) index ) * (gDeviceState.numerator/gDeviceState.denominator) / gDeviceState.divisor;
			break;
		case TET:
			ratio = pow( pow(gDeviceState.numerator / gDeviceState.denominator, 1 / gDeviceState.divisor), (float) index);
			break;
		default:
			printf("error, invalid algorithm selection");
			break;
	}
	
	return ratio;
	
}

void calculateAvailability(Oscillator oscs[]) {
	
	for(int i = 0; i< NUM_OSCS; i++) {
		//if you modulate to something out of range, it turns off
		if(oscs[i].targetFrequency > freq_max || 
		oscs[i].targetFrequency < freq_min ) {
			oscs[i].available = false;
			oscs[i].state = RELEASING;
		}

	}
	
	//calculate frequences outside of Oscillators and update index range 
	// find lowest note and adjust range 
	int oscIndex = 0;
	bool newIndexFlag = false;
	while(oscIndex > -32) {
		
		float newFreq = calculateRatio(gDeviceState.selectedAlgorithm, oscIndex) * gDeviceState.targetBaseFrequency;
		//offset? 
		if (newFreq < freq_min) {
			touch_min = oscIndex;
			newIndexFlag = true;
			break;
		}
		
		oscIndex--;
	}
	
	if(!newIndexFlag) touch_min = -32;
	
	oscIndex = 0;
	newIndexFlag = false;
	
	while(oscIndex < 63) {
		
		float newFreq = calculateRatio(gDeviceState.selectedAlgorithm, oscIndex) * gDeviceState.targetBaseFrequency;
		if(newFreq > freq_max) {
			touch_max = oscIndex;
			newIndexFlag = true;
			break;
		}
		
		oscIndex++;
		
	}
	
	if(!newIndexFlag) touch_max = 63;
	
	
//	rt_printf("printing touch minmax\n");
//	rt_printf("%i %i", touch_min, touch_max);
}


void loop(void*)
{
    while(!Bela_stopRequested())
    {
        // ---- Get all buffers ----
        DataBuffer& potsBuf    = gui.getDataBuffer(POT);
        DataBuffer& touchBuf   = gui.getDataBuffer(TOUCH);
        DataBuffer& registerBuf= gui.getDataBuffer(REGISTER);
        DataBuffer& bigBuf     = gui.getDataBuffer(BIG);
        DataBuffer& freqBuf    = gui.getDataBuffer(FREQ);
        DataBuffer& recBuf     = gui.getDataBuffer(REC_SEND);
        DataBuffer& panningBuf = gui.getDataBuffer(PANNING);

        // ----------------------------------------------------------------
        // TOUCH — full array of 16 floats, one per oscillator
        // ----------------------------------------------------------------
        if(touchBuf.getNumElements() >= NUM_OSCS) {
            float* touchData = touchBuf.getAsFloat();

            for(int i = 0; i < NUM_OSCS; i++) {
                bool touched = touchData[i] > 0.5f;

                if(touched) {
                    // Option B: only retrigger phase if coming from IDLE
                    if(gOscillators[i].state == IDLE) {
                        gOscillators[i].phase = 0.0f;
                    }
                    gOscillators[i].state      = ACTIVE;
                    gOscillators[i].targetGain = 1.0f;

                } else {
                    // Only trigger release if currently active
                    if(gOscillators[i].state == ACTIVE) {
                        gOscillators[i].state      = RELEASING;
                        gOscillators[i].targetGain = 0.0f;
                    }
                }
            }

            touchBuf.getBuffer()->resize(0);
        }

        // ----------------------------------------------------------------
        // PANNING — full array of 16 floats
        // ----------------------------------------------------------------
        if(panningBuf.getNumElements() >= NUM_OSCS) {
            float* panData = panningBuf.getAsFloat();

            for(int i = 0; i < NUM_OSCS; i++)
                gOscillators[i].panning = panData[i];

            panningBuf.getBuffer()->resize(0);
        }

        // ----------------------------------------------------------------
        // POTS — full array of 16 floats
        // ----------------------------------------------------------------
        if(potsBuf.getNumElements() >= 16) {
            float* potsData = potsBuf.getAsFloat();

            // All writes go to target values — render() smooths toward them
            gDeviceState.targetSynthGain    = potsData[0]  * 2.0f;
            gDeviceState.targetHiFreqBoost  = potsData[1]  * 0.5f;
            gDeviceState.filterQ            = potsData[2]  * 200.0f + 50.0f;
            gDeviceState.filterGain         = potsData[3]  * 100.0f;
            gDeviceState.targetLpCutoff     = potsData[4]  * 4000.0f + 2000.0f;
            gDeviceState.targetMicGain      = potsData[5]  * 10.0f;
            // potsData[6] unassigned
            gDeviceState.limiterThresh      = potsData[7]  * 3.0f + 0.8f;
            gDeviceState.limiterLookahead   = potsData[8]  * 50.0f + 5.0f;
            gDeviceState.limiterRelease     = potsData[9]  * 100.0f + 5.0f;
            gDeviceState.targetBaseFrequency= potsData[10] * 1000.0f;
            gDeviceState.targetFbAmount     = potsData[11] * 40.0f;
            gDeviceState.glideAmount        = potsData[12] * 1000.0f;
            // potsData[13] unassigned
            // potsData[14] unassigned
            // potsData[15] panning — handled via PANNING buffer

            potsBuf.getBuffer()->resize(0);
            gNeedsFreqUpdate = true;
            gNeedsLimiterUpdate = true;
            gNeedsFilterUpdate = true;
        }

        // ----------------------------------------------------------------
        // REGISTERS — full array of 5 floats
        // ----------------------------------------------------------------
        if(registerBuf.getNumElements() >= 5) {
            float* registerData = registerBuf.getAsFloat();

            // +1 offsets preserve existing behaviour (1-indexed values)
            gDeviceState.numerator          = (int)registerData[0] + 1;
            gDeviceState.denominator        = (int)registerData[1] + 1;
            gDeviceState.divisor            = (int)registerData[2] + 1;
            gDeviceState.offset             = (int)registerData[3];
            gDeviceState.selectedAlgorithm  = (algorithms)(int)registerData[4];

            registerBuf.getBuffer()->resize(0);
            gNeedsFreqUpdate = true;
        }

        // ----------------------------------------------------------------
        // BIG BUTTONS — bitmask
        // ----------------------------------------------------------------
        if(bigBuf.getNumElements() >= 1) {
			float* bigData = bigBuf.getAsFloat();
			uint32_t mask = (uint32_t)bigData[0];
			
			gDeviceState.savePresetButton   = (mask >> 0) & 1;
			gDeviceState.setReferenceButton = (mask >> 1) & 1;
			gDeviceState.toggleOutputButton = (mask >> 2) & 1;
			gDeviceState.droneModeButton    = (mask >> 3) & 1;

            // Preset save handled here — just log for now
            if(gDeviceState.savePresetButton) {
                rt_printf("Save preset %d\n", gDeviceState.activePreset);
                gDeviceState.savePresetButton = false;
            }

            bigBuf.getBuffer()->resize(0);
        }

        // ----------------------------------------------------------------
        // FREQ BUTTONS — bitmask
        // ----------------------------------------------------------------
        if(freqBuf.getNumElements() >= 1) {
            float* freqData = freqBuf.getAsFloat();
            uint32_t mask;
            memcpy(&mask, &freqData[0], sizeof(uint32_t));

            gDeviceState.micButton          = (mask >> 0) & 1;
            gDeviceState.keyboardButton     = (mask >> 1) & 1;
            gDeviceState.lastSelectedButton = (mask >> 2) & 1;

            freqBuf.getBuffer()->resize(0);
        }

        // ----------------------------------------------------------------
        // RECORD BUTTON
        // ----------------------------------------------------------------
        if(recBuf.getNumElements() >= 1) {
            float* recData = recBuf.getAsFloat();
            bool nowRecording = recData[0] > 0.5f;
            gDeviceState.recButton.store(nowRecording);

            if(nowRecording) {
                float elapsedMs = (gSampleCount / 44100.0f) * 1000.0f;
                gRecordStartMs = elapsedMs;
            }

            recBuf.getBuffer()->resize(0);
        }

        // ----------------------------------------------------------------
        // PRESET LOAD — trigger crossfade
        // ----------------------------------------------------------------
        DataBuffer& presetBuf = gui.getDataBuffer(PRESET);
        if(presetBuf.getNumElements() >= 1) {
            float* presetData = presetBuf.getAsFloat();
            gDeviceState.activePreset = (int)presetData[0];

            // Signal render() to begin crossfade
            gPresetLoading.store(true);
            gTargetMasterGain = 0.0f;

            rt_printf("Loading preset %d\n", gDeviceState.activePreset);

            presetBuf.getBuffer()->resize(0);
        }

        usleep(5000); // 5ms loop rate — faster than before for tighter response
    }
}

bool setup(BelaContext *context, void *userData)
{
    gInverseSampleRate = 1.0f / context->audioSampleRate;

    // ---- Ramp rates ----
    // How much to change gain per sample to reach 0 in RELEASE_MS
    gReleaseRampRate   = 1.0f / (RELEASE_MS   * 0.001f * context->audioSampleRate);
    gCrossfadeRampRate = 1.0f / (CROSSFADE_MS * 0.001f * context->audioSampleRate);

    // ---- RMS window ----
    gWindowSize = (int)((RMS_INTERVAL_MS / 1000.0f) * context->audioSampleRate);

    // ---- Scope ----
    scope.setup(NUM_OSCS, context->audioSampleRate);

    // ---- GUI ----
    gui.setup(context->projectName);

    // Buffer declarations must match buffers enum order:
    // POT, TOUCH, REGISTER, BIG, FREQ, REC_SEND, PANNING
    gui.setBuffer('f', NUM_OSCS);  // POT
    gui.setBuffer('f', NUM_OSCS);  // TOUCH
    gui.setBuffer('f', 5);   // REGISTER
    gui.setBuffer('f', 1);   // BIG
    gui.setBuffer('f', 1);   // FREQ
    gui.setBuffer('f', 1);   // REC_SEND
    gui.setBuffer('f', NUM_OSCS);  // PANNING
    // Send buffers
    gui.setBuffer('f', NUM_OSCS + 1);   // RMS
    gui.setBuffer('f', 1);   // LEFT
    gui.setBuffer('f', 1);   // RIGHT
    gui.setBuffer('f', 1);   // PRESET
    gui.setBuffer('f', 2);   // availability/ranges 

    // ---- Oscillator bank init ----
    for(int i = 0; i < NUM_OSCS; i++) {
        gOscillators[i].state            = IDLE;
        gOscillators[i].gain             = 0.0f;
        gOscillators[i].targetGain       = 0.0f;
        gOscillators[i].phase            = 0.0f;
        gOscillators[i].panning          = 0.5f;

        // Compute starting frequency from ratio algorithm
        float ratio = calculateRatio(gDeviceState.selectedAlgorithm, i + gDeviceState.offset);
        gOscillators[i].targetFrequency  = ratio * gDeviceState.targetBaseFrequency;
        gOscillators[i].currentFrequency = gOscillators[i].targetFrequency;
    }

    // ---- Filter bank init ----
    s.fs         = context->audioSampleRate;
    s.q          = gDeviceState.filterQ;
    s.peakGainDb = gDeviceState.filterGain;
    s.type       = Biquad::bandpass;

    for(int i = 0; i < NUM_OSCS; i++) {
        s.cutoff = gOscillators[i].currentFrequency;
        filterBank[i].setup(s);
    }

    // ---- Lowpass filter init ----
    lps.fs         = context->audioSampleRate;
    lps.q          = 0.707f;
    lps.peakGainDb = 0.0f;
    lps.type       = Biquad::lowpass;
    lps.cutoff     = gDeviceState.targetLpCutoff;
    lpfilter.setup(lps);

    // ---- Limiters ----
    inputLimiter.setup(context->audioSampleRate,
                       gDeviceState.limiterThresh,
                       gDeviceState.limiterLookahead,
                       gDeviceState.limiterRelease);

    outputLimiter.setup(context->audioSampleRate,
                        gDeviceState.limiterThresh,
                        gDeviceState.limiterLookahead,
                        gDeviceState.limiterRelease);

    // ---- Digital pins ----
    pinMode(context, 0, gTriggerButtonPin, INPUT);
    pinMode(context, 0, gModeButtonPin,    INPUT);

    // ---- Analog check ----
    if(context->analogFrames == 0 || context->analogFrames > context->audioFrames) {
        rt_printf("Error: analog must be enabled with 4 or 8 channels\n");
        return false;
    }
    gAudioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;

    // ---- Auxiliary tasks ----
    sendTask = Bela_createAuxiliaryTask(sendRmsBuffer, 50, "rms-send-task");
    if(!sendTask) {
        rt_printf("Error creating RMS send task\n");
        return false;
    }

    Bela_runAuxiliaryTask(loop);

    rt_printf("Setup complete. Sample rate: %f\n", context->audioSampleRate);
    rt_printf("Release ramp rate: %f\n", gReleaseRampRate);
    rt_printf("Window size: %d samples\n", gWindowSize);

    return true;
}

// void render(BelaContext *context, void *userData)
// {
	
// 				    //We create an auxiliary counter variable that will indicate when to send the buffer
// 	static unsigned int count = 0;
	
// 	for(unsigned int n = 0; n < context->audioFrames; n++) {
		
// 	    //count will increase at each audio frame by one
// 		count++;
// 		gSampleCount++;
		
		
// 	//	float out = 0;
// 		float outArray[context->audioOutChannels];
// 		float input = 0;
// 		float feedback = 0;
// 		float amp = 0;
// 		outArray[0] = 0;
// 		outArray[1] = 0;

// 		//input
// 		if(gAudioFramesPerAnalogFrame && !(n % gAudioFramesPerAnalogFrame)) {
// 			//for quantifying frequencies
// 			//float multiplier = floor(map(analogRead(context, n/gAudioFramesPerAnalogFrame, analogBaseFreq), 0, 1, 1, 20));
// 			//gDeviceState.baseFrequency = 110 * pow( pow( 2.0 , 0.083333), multiplier);
// 			//gAmplitude = map(analogRead(context, n/gAudioFramesPerAnalogFrame, analogAmplitude), 0, 1, 0.1, 1);

// 		}
		
		
// 		if(gMicOn) {
// 		//mic input on channel 0
// 	     input = audioRead(context, n, 0);	

// 		//input gain
// 		input  = input * gDeviceState.micGain;
// 		}

// 	//	gOutput[0] = input;

		
// 		//limit/compress input


// 	//	gOutput[1] = input;
		
// 		static unsigned int printCount = 0;


// 		for(int i = 0; i<NUM_OSCS; i++) {

// 			//copy input to each channel
// 			filterChannels[i] = 0;
			
// 			//filter input does it in place
// 			filterChannels[i] = filterBank[i].process(input);
// 		//	filterChannels[i] = filterBank[i].process((float)rand() / (float)RAND_MAX);
// 		//	filterChannels[i] *= (i * gDeviceState.hiFreqBoost) + 1;
// 			filterChannels[i] = inputLimiter.processSample(filterChannels[i]);
			
// 			//RMS capture
// 			gSumOfSquares[i] += filterChannels[i]*filterChannels[i];
			
// 			if(gFeedbackOn) {
// 				feedback += filterChannels[i] * gDeviceState.fbAmount * ((i * gDeviceState.hiFreqBoost) + 1.f);
// 			} else {
// 				//just mic
// 				feedback += input / ((float) NUM_OSCS);
// 			}
			
// 			if(gDeviceState.toggleOutputButton) {
// 				amp =  gDeviceState.synthGain * 1./ ((float) NUM_OSCS);
// 			} else {
// 				amp = 0;
// 			}
			

			
			
// 			//out += sinf();

// 			// switch(gOscType) {
// 			// 	default:
// 			// 	// SINEWAVE
// 			// 	case sine:
// 			// 		out += sinf(gPhases[i]);
// 			// 		break;
// 			// 	// TRIANGLE WAVE
// 			// 	case triangle:
// 			// 		if (gPhases[i] > 0) {
// 			// 		      out += -1 + (2 * gPhases[i] / (float)M_PI);
// 			// 		} else {
// 			// 		      out += -1 - (2 * gPhases[i]/ (float)M_PI);
// 			// 		}
// 			// 		break;
// 			// 	// SQUARE WAVE
// 			// 	case square:
// 			// 		if (gPhases[i] > 0) {
// 			// 		      out += 1;
// 			// 		} else {
// 			// 		      out += -1;
// 			// 		}
// 			// 		break;
// 			// 	// SAWTOOTH
// 			// 	case sawtooth:
// 			// 		out += 1 - (1 / (float)M_PI * gPhases[i]);
// 			// 		break;
// 			// 	}
// 			//glide 
// 			gFrequencies[i] = gFrequencies[i] + ((gDeviceState.targetRatio[i]* gDeviceState.baseFrequency) - gFrequencies[i]) / (gDeviceState.glideAmount*gDeviceState.glideAmount);

	
// 			// Compute phase
// 		//	if(gDeviceState.oscillatorsOn[i]){
// 			gPhases[i] += 2.0f * (float)M_PI * gFrequencies[i] * gInverseSampleRate;

// 			if(gPhases[i] > M_PI)
// 			{	gPhases[i] -= 2.0f * (float)M_PI; }
				
// 			outArray[0] += sinf(gPhases[i]) * amp * (1 - gDeviceState.panning[i]) + feedback;
// 			outArray[1] += sinf(gPhases[i]) * amp * gDeviceState.panning[i] + feedback;
// 			//}
				
// 			//output to scope
// 			//gOutput[i] = out * NUM_OSCS;
			
// 		}

// 	   // gOutput[2] = finalOut;

// 		//feedback
// 	//	out += input;
// 		//output limiter
		
// 		outArray[0] = lpfilter.process(outArray[0]);
// 		outArray[1] = lpfilter.process(outArray[1]);
// 		outArray[0] = outputLimiter.processSample(outArray[0]);
// 		outArray[1] = outputLimiter.processSample(outArray[1]);
		
// 	//	out = outputLimiter.processSample(out);

// 		gOutput[0] = outArray[0];
// 		scope.log(input, feedback, outArray[0]);

		

// 		// Write output to different channels
// 		for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
// 			 audioWrite(context, n, channel, outArray[channel]);
// 		//	audioWrite(context, n, channel, out);
// 		}
		
// 		//send to gui
// 		if (count >= gTimePeriod*context->audioSampleRate)
// 		{
			
// 			gui.sendBuffer(0, abs(input));
// 			gui.sendBuffer(1, abs(feedback));
			
// 			//rt_printf("%f", out);

// 			//and we reset the counter
// 			count = 0;
// 		}
		
// 		//send to gui every 50 ms 
// 		if(gSampleCount >= gWindowSize) {
			
			
// 			if(gDeviceState.recButton.load() && !gRmsReady.load()) {
//                 // Compute RMS per channel
//                 for(int ch = 0; ch < NUM_OSCS; ch++) {
//                         gRmsResults[ch] = sqrtf(gSumOfSquares[ch] / gWindowSize);
//                 }

//                 // Compute elapsed ms from record start using audio clock
//                 float elapsedSamples = (float)context->audioFramesElapsed + n;
//                 gTimestampMs = ((elapsedSamples / context->audioSampleRate) * 1000.0f)
//                               - gRecordStartMs;

//                 // Signal aux task
//                 gRmsReady.store(true);
//                 Bela_scheduleAuxiliaryTask(sendTask);
//             }

//             // Reset accumulation regardless of recording state
//             memset(gSumOfSquares, 0, sizeof(gSumOfSquares));
//             gSampleCount = 0;
// 		}
// 	}
	
// 	 if (gNeedsUpdate) {
        	
// 		//	rt_printf("Update: ");
        	
        	
// 			// loop through the oscillator bank
// 			for(int l = 0; l<NUM_OSCS; l++) {
// 				//if osc is on, update

// 				//calculate new frequency
// 				// printf("update=ing %i\n", l);
// 					gDeviceState.targetRatio[l] = calculateRatio(gDeviceState.selectedAlgorithm, gDeviceState.noteIndex[l] + gDeviceState.offset);	
// 									//pull current frequencies and update filters
// 					s.fs = context->audioSampleRate;
// 					s.type = Biquad::bandpass;
// 					s.q = gDeviceState.filterQ;
// 					s.peakGainDb = gDeviceState.filterGain;
// 					//instead of gfreqs, just advance it for now
// 				    s.cutoff = gDeviceState.targetRatio[l] * gDeviceState.baseFrequency;

// 					filterBank[l].setup(s);		
// 				//	filterBank[l].clean();
					
// 				//	rt_printf("%d ", gDeviceState.oscillatorsOn[l]);

				
// 			//	rt_printf("\n");
	

//         	}
        	
//         	inputLimiter.setup(context->audioSampleRate, gDeviceState.limiterThresh, gDeviceState.limiterLookahead, gDeviceState.limiterRelease);
// 			outputLimiter.setup(context->audioSampleRate, gDeviceState.limiterThresh, gDeviceState.limiterLookahead, gDeviceState.limiterRelease);

// 			gNeedsUpdate = false;
//         }
	
// }

void render(BelaContext *context, void *userData)
{
    static unsigned int count = 0;

    // ----------------------------------------------------------------
    // BLOCK BOUNDARY — apply filter/limiter updates
    // Only runs when loop() signals a change
    // ----------------------------------------------------------------
 // ---- Frequency update ----
	// Only recalculates target frequencies - no filter state reset
	if(gNeedsFreqUpdate) {
	    for(int i = 0; i < NUM_OSCS; i++) {
	        float ratio = calculateRatio(
	            gDeviceState.selectedAlgorithm,
	            i + gDeviceState.offset
	        );
	        gOscillators[i].targetFrequency = ratio * gDeviceState.targetBaseFrequency;
	    }
	    
	    //update availability 
	    calculateAvailability(gOscillators);
	    
	    //send touch availability
	    float availArray[2];
	    availArray[0] = (float) touch_min;
	    availArray[1] = (float) touch_max;
	    gui.sendBuffer(AVAIL, availArray);
	    
	    
	    
	    gNeedsFreqUpdate = false;
	}
	
	// ---- Filter update ----
	// Only rebuild when filter Q or gain changed
	if(gNeedsFilterUpdate) {
	    s.fs         = context->audioSampleRate;
	    s.type       = Biquad::bandpass;
	    s.q          = gDeviceState.filterQ;
	    s.peakGainDb = gDeviceState.filterGain;
	
	    for(int i = 0; i < NUM_OSCS; i++) {
	        s.cutoff = gOscillators[i].targetFrequency;
	        filterBank[i].setup(s);
	        filterBank[i].clean(); // clear state after coefficient change
	    }
	    gNeedsFilterUpdate = false;
	}
	
	// ---- Limiter update ----
	// Only rebuild when limiter params changed
	if(gNeedsLimiterUpdate) {
	    inputLimiter.setup(context->audioSampleRate,
	                       gDeviceState.limiterThresh,
	                       gDeviceState.limiterLookahead,
	                       gDeviceState.limiterRelease);
	    outputLimiter.setup(context->audioSampleRate,
	                        gDeviceState.limiterThresh,
	                        gDeviceState.limiterLookahead,
	                        gDeviceState.limiterRelease);
	    gNeedsLimiterUpdate = false;
	}

    // ----------------------------------------------------------------
    // SAMPLE LOOP
    // ----------------------------------------------------------------
    for(unsigned int n = 0; n < context->audioFrames; n++) {
    	

        count++;
        gSampleCount++;

        float outArray[2] = {0.0f, 0.0f};
        float input       = 0.0f;
        float feedback    = 0.0f;

        // ---- Analog read ----
        if(gAudioFramesPerAnalogFrame && !(n % gAudioFramesPerAnalogFrame)) {
            // Analog reads available here if needed
        }


        if(gMicOn) {
            input = audioRead(context, n, 0);
        }

        // ---- Smooth mic gain ----
        gDeviceState.currentMicGain += gSmoothAlpha *
            (gDeviceState.targetMicGain - gDeviceState.currentMicGain);
        input *= gDeviceState.currentMicGain;

        // ---- Smooth global parameters ----
        gDeviceState.currentSynthGain += gSmoothAlpha *
            (gDeviceState.targetSynthGain - gDeviceState.currentSynthGain);

        gDeviceState.currentFbAmount += gSmoothAlpha *
            (gDeviceState.targetFbAmount - gDeviceState.currentFbAmount);

        gDeviceState.currentHiFreqBoost += gSmoothAlpha *
            (gDeviceState.targetHiFreqBoost - gDeviceState.currentHiFreqBoost);

        gDeviceState.currentBaseFrequency += gSmoothAlpha *
            (gDeviceState.targetBaseFrequency - gDeviceState.currentBaseFrequency);

        // ---- Master crossfade ----
        // Ramp master gain toward target
        if(gMasterGain < gTargetMasterGain) {
            gMasterGain += gCrossfadeRampRate;
            if(gMasterGain >= gTargetMasterGain) {
                gMasterGain = gTargetMasterGain;
            }
        } else if(gMasterGain > gTargetMasterGain) {
            gMasterGain -= gCrossfadeRampRate;
            if(gMasterGain <= gTargetMasterGain) {
                gMasterGain = gTargetMasterGain;

                // Master gain reached 0 — apply preset params and ramp back up
                if(gPresetLoading.load() && gMasterGain == 0.0f) {
                    // All target values already written by loop()
                    // Just trigger filter/limiter update and ramp back up
                    gNeedsLimiterUpdate       = true;
                    gTargetMasterGain  = 1.0f;
                    gPresetLoading.store(false);
                }
            }
        }


        for(int i = 0; i < NUM_OSCS; i++) {

            filterChannels[i] = filterBank[i].process(input);
            filterChannels[i] = inputLimiter.processSample(filterChannels[i]);

            // ---- RMS accumulation ----
            gSumOfSquares[i] += filterChannels[i] * filterChannels[i];

            // ---- Feedback accumulation ----
            if(gFeedbackOn) {
                feedback += filterChannels[i]
                            * gDeviceState.currentFbAmount
                            * ((i * gDeviceState.currentHiFreqBoost) + 1.0f);
            } else {
                feedback += input / (float)NUM_OSCS;
            }

            // ---- Gain envelope ----
            // Ramp gain toward targetGain
            if(gOscillators[i].gain < gOscillators[i].targetGain) {
                gOscillators[i].gain += gReleaseRampRate;
                if(gOscillators[i].gain >= gOscillators[i].targetGain)
                    gOscillators[i].gain = gOscillators[i].targetGain;

            } else if(gOscillators[i].gain > gOscillators[i].targetGain) {
                gOscillators[i].gain -= gReleaseRampRate;
                if(gOscillators[i].gain <= 0.0f) {
                    gOscillators[i].gain = 0.0f;

                    // Gain reached 0 during RELEASING → go IDLE
                    if(gOscillators[i].state == RELEASING) {
                        gOscillators[i].state = IDLE;
                        gOscillators[i].phase = 0.0f; // safe to reset — silent
                    }
                }
            }

            // ---- Frequency glide ----
            gOscillators[i].currentFrequency +=
                (gOscillators[i].targetFrequency - gOscillators[i].currentFrequency)
                / (gDeviceState.glideAmount * gDeviceState.glideAmount + 1.0f);
            // +1 prevents division by zero if glideAmount is 0

            // ---- Phase advance ----
            gOscillators[i].phase += 2.0f * (float)M_PI
                                     * gOscillators[i].currentFrequency
                                     * gInverseSampleRate;

            if(gOscillators[i].phase > M_PI)
                gOscillators[i].phase -= 2.0f * (float)M_PI;

            // ---- Output ----
            // Only contribute if not IDLE
            if(gOscillators[i].state != IDLE) {
                float amp = gDeviceState.currentSynthGain
                            * gOscillators[i].gain
                            * gMasterGain
                            / (float)NUM_OSCS;

                float sig = sinf_neon(gOscillators[i].phase) * amp;

                if(gDeviceState.toggleOutputButton) {
                    outArray[0] += sig * (1.0f - gOscillators[i].panning) + feedback;
                    outArray[1] += sig * gOscillators[i].panning           + feedback;
                }
            }
        }
        
        // After oscillator loop, before post processing
		if(outArray[0] > 0.9f || outArray[0] < -0.9f)
		    rt_printf("clipping: %.4f\n", outArray[0]);

        // ---- Post processing ----
        outArray[0] = lpfilter.process(outArray[0]);
        outArray[1] = lpfilter.process(outArray[1]);
        outArray[0] = outputLimiter.processSample(outArray[0]);
        outArray[1] = outputLimiter.processSample(outArray[1]);

        // ---- Scope ----
        scope.log(input, feedback, outArray[0]);

        // ---- Write output ----
        for(unsigned int ch = 0; ch < context->audioOutChannels; ch++) {
            audioWrite(context, n, ch, outArray[ch]);
        }

        // ---- Send left/right meters to GUI ----
        if(count >= gTimePeriod * context->audioSampleRate) {
            gui.sendBuffer(LEFT,  outArray[0]);
            gui.sendBuffer(RIGHT, outArray[1]);
            count = 0;
        }
    }

    // ----------------------------------------------------------------
    // RMS — outside sample loop, once per block
    // ----------------------------------------------------------------
    if(gSampleCount >= gWindowSize) {

        if(gDeviceState.recButton.load() && !gRmsReady.load()) {

            for(int ch = 0; ch < NUM_OSCS; ch++) {
                gRmsResults[ch] = sqrtf(gSumOfSquares[ch] / gSampleCount);
            }

            float elapsedSamples = (float)context->audioFramesElapsed;
            gTimestampMs = ((elapsedSamples / context->audioSampleRate) * 1000.0f)
                           - gRecordStartMs;

            gRmsReady.store(true);
            Bela_scheduleAuxiliaryTask(sendTask);
        }

        memset(gSumOfSquares, 0, sizeof(gSumOfSquares));
        gSampleCount = 0;
    }
}

void cleanup(BelaContext *context, void *userData)
{
    gui.cleanup();
}



