#include <render.h>


//converting from midi key to index
int displaceIndex(int key) {
	int offset = NUM_KEYS - 1;
	int c = 0;
	
	if((int) gDeviceState.divisor % 2 == 0) {
		offset = NUM_KEYS;
		c = (offset - gDeviceState.divisor) / 2 + 1;
	} else {
		c = (offset - gDeviceState.divisor) / 2;
	}
	
	
	return key - c;
}

void printTask(float myFloat) {
    if(!gNewData) return;

    rt_printf("=== DeviceState ===\n");

    rt_printf("div, n, d:\n");
    rt_printf("%f \n", myFloat);

    gNewData = false;
   	usleep(50000);
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

void loop(void*)
{
	// int numBits;
	// int speed = 0;
	while(!Bela_stopRequested())
	{

		
		//receiving buffer data 
		DataBuffer& potsBuf = gui.getDataBuffer(0);
		DataBuffer& presetBuf = gui.getDataBuffer(1);
		DataBuffer& touchBuf = gui.getDataBuffer(2);
		DataBuffer& registerBuf = gui.getDataBuffer(3);
		DataBuffer& bigBuf = gui.getDataBuffer(4);
		DataBuffer& freqBuf = gui.getDataBuffer(5);
		
		// Retrieve contents of the buffer as floats
			//store in struct 
		unsigned int blength = touchBuf.getNumElements();
		
		if(touchBuf.getNumElements()>0){
			float* touchData = touchBuf.getAsFloat();
			gBufferState.touchIndex = (int) touchData[0];
			gBufferState.touchValue =  touchData[1] != 0;
			
				    	//update noteIndex
        	if(gBufferState.touchIndex < NUM_OSCS) {
        		gDeviceState.noteIndex[gBufferState.touchIndex] = gBufferState.touchIndex;
        		gDeviceState.oscillatorsOn[gBufferState.touchIndex] = gBufferState.touchValue;
        		gDeviceState.lastTouched = gBufferState.touchIndex;
        	}
			
			//reset buffer
		    touchBuf.getBuffer()->resize(0);
		    gNeedsUpdate = true;
		}
		
		if(potsBuf.getNumElements()>0){
			float* potsData = potsBuf.getAsFloat();
			gBufferState.potIndex = (int) potsData[0];
			gBufferState.potValue = potsData[1];
			
			switch(gBufferState.potIndex) {
				case 0:
					gDeviceState.synthGain = gBufferState.potValue;
					break;
				case 2:
					gDeviceState.filterQ = gBufferState.potValue * 10.;
					break;
				case 3:
					gDeviceState.filterGain = gBufferState.potValue * 20.;
					break;
				case 7:
					gDeviceState.limiterThresh = gBufferState.potValue;
					break;
				case 8:
					gDeviceState.limiterLookahead = gBufferState.potValue;
					break;			
				case 9:
					gDeviceState.limiterRelease = gBufferState.potValue;
					break;
				case 10:
					gDeviceState.baseFrequency = gBufferState.potValue * 1000.;
					break;
				case 11:
					gDeviceState.fbAmount = gBufferState.potValue;
					break;
				case 12:
					gDeviceState.glideAmount = gBufferState.potValue * 100.;
					break;
				case 15:
					gDeviceState.panning[gDeviceState.lastTouched] = gBufferState.potValue;
					break;
				default:
					break;
			}
			
			printTask(gBufferState.potValue);
			
			//reset buffer
		    potsBuf.getBuffer()->resize(0);
		    gNeedsUpdate = true;
		}
			
		if(presetBuf.getNumElements()>0){
			gBufferState.activePreset = (int) presetBuf.getAsFloat()[0];
						
			//reset buffer
		    presetBuf.getBuffer()->resize(0);
		    gNeedsUpdate = true;
		}
		
		if(registerBuf.getNumElements()>0){
			float* registerData = registerBuf.getAsFloat();
			gBufferState.registerIndex = (int) registerData[0];
			gBufferState.registerValue = (int) registerData[1];
			
			switch(gBufferState.registerIndex) {
				//numerator has to be 1 or higher
				case 0:
					gDeviceState.numerator = gBufferState.registerValue+1;
					break;
				case 1:
					gDeviceState.denominator = gBufferState.registerValue+1;
					break;
				case 2:
					gDeviceState.divisor = gBufferState.registerValue+1;
					break;
				case 3:
					gDeviceState.offset = gBufferState.registerValue;
					break;
				case 4:
					gDeviceState.selectedAlgorithm = (algorithms) gBufferState.registerValue;
					break;
				default:
					break;
			}
			
			//reset buffer
		    registerBuf.getBuffer()->resize(0);
		    gNeedsUpdate = true;
		    
		    gNewData = true;

		}

		if(bigBuf.getNumElements()>0){	    	
	    	float* bigData = bigBuf.getAsFloat();
			uint32_t mask2 = (uint32_t) bigData[0];
	    	gDeviceState.savePresetButton = (mask2 >> 0) & 1;
	    	gDeviceState.setReferenceButton = (mask2 >> 1) & 1;
	    	gDeviceState.toggleOutputButton = (mask2 >> 2) & 1;
	    	gDeviceState.droneModeButton = (mask2 >> 2) & 1;
	    	
	    	
	    	//reset buffer
		    bigBuf.getBuffer()->resize(0);
		    gNeedsUpdate = true;
		}

		if(freqBuf.getNumElements()>0){	    	
	        float* freqData = freqBuf.getAsFloat();
			uint32_t mask = (uint32_t) freqData[0];
	    	gDeviceState.micButton = (mask >> 0) & 1;
	    	gDeviceState.keyboardButton = (mask >> 1) & 1;
	    	gDeviceState.lastSelectedButton = (mask >> 2) & 1;
	    	
	    	//reset buffer
		    freqBuf.getBuffer()->resize(0);
		    gNeedsUpdate = true;
		}

		
        if (gNeedsUpdate) {
        	

        	
        	
			// loop through the oscillator bank
			for(int l = 0; l<NUM_OSCS; l++) {
				//if osc is on, update

			//calculate new frequency
			// printf("update=ing %i\n", l);
			if(gDeviceState.oscillatorsOn[l]) {
				gDeviceState.targetRatio[l] = calculateRatio(gDeviceState.selectedAlgorithm, gDeviceState.noteIndex[l] + gDeviceState.offset);	
			}

			//pull current frequencies and update filters
		 //   s.cutoff = gFrequencies[l];
		//	filterBank[l].setup(s);		
			gNeedsUpdate = false;
        	}
        }
                
		usleep(50000);
	}
}

bool setup(BelaContext *context, void *userData)
{
	gInverseSampleRate = 1.0 / context->audioSampleRate;

	scope.setup(NUM_OSCS, context->audioSampleRate);
	
	rt_printf("Num Channels Out \n");
	rt_printf("%f", context->audioOutChannels);
	
	gui.setup(context->projectName);
	
	//gui set buffers
	gui.setBuffer('f', 2);  
	gui.setBuffer('f', 1);
	gui.setBuffer('f', 2);
	gui.setBuffer('f', 2);
	gui.setBuffer('f', 1);
	gui.setBuffer('f', 1);
	

	//setup filter settings
	s.fs = context->audioSampleRate;
	s.q = 3.0;
	s.peakGainDb = 8;
	s.type = Biquad::peak;

	
	for(int k = 0; k<NUM_OSCS; k++) {
		gPhases[k] = 0.0;
		gFrequencies[k] = 300.0 * (k+1);
		gDeviceState.synthGain = 0.8f;
		filterChannels[k] = 0;
	}

	//setup filters
	for(int i = 0;i<NUM_OSCS;i++){
		s.cutoff = gFrequencies[i];
		s.peakGainDb = 20+3*(i+1);
		filterBank[i].setup(s);	
		
	//	rt_printf("%f /n",filterBank[i].type);
	}



	// Set buttons pins as inputs
	pinMode(context, 0, gTriggerButtonPin, INPUT);
	pinMode(context, 0, gModeButtonPin, INPUT);
	
		// Check if analog channels are enabled
	if(context->analogFrames == 0 || context->analogFrames > context->audioFrames) {
		rt_printf("Error: this example needs analog enabled, with 4 or 8 channels\n");
		return false;
	}
		// Useful calculations
	gAudioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;

	// setup limiter params
	inputLimiter.setup(context->audioSampleRate);
	outputLimiter.setup(context->audioSampleRate);

	Bela_runAuxiliaryTask(loop);
	
	return true;
}

void render(BelaContext *context, void *userData)
{
	
				    //We create an auxiliary counter variable that will indicate when to send the buffer
	static unsigned int count = 0;
	
	for(unsigned int n = 0; n < context->audioFrames; n++) {
		
	    //count will increase at each audio frame by one
		count++;
		
		float out = 0;
		float outArray[context->audioOutChannels];
		float input = 0;
		float feedback = 0;

		//input
		if(gAudioFramesPerAnalogFrame && !(n % gAudioFramesPerAnalogFrame)) {
			//for quantifying frequencies
			//float multiplier = floor(map(analogRead(context, n/gAudioFramesPerAnalogFrame, analogBaseFreq), 0, 1, 1, 20));
			//gDeviceState.baseFrequency = 110 * pow( pow( 2.0 , 0.083333), multiplier);
			//gAmplitude = map(analogRead(context, n/gAudioFramesPerAnalogFrame, analogAmplitude), 0, 1, 0.1, 1);

			
	
		}
		//mic input on channel 0
	     input = audioRead(context, n, 0);	

		//input gain
		input *= 3.0;

	//	gOutput[0] = input;

		
		//limit/compress input


	//	gOutput[1] = input;
		


		for(int i = 0; i<NUM_OSCS; i++) {

			//copy input to each channel
			filterChannels[i] = 0;
			filterChannels[i] = input;
			
			
			//filter input does it in place
			//filterBank[i].process(input);
			filterChannels[i] = filterBank[i].process(filterChannels[i]);
			filterChannels[i] = inputLimiter.processSample(filterChannels[i]);
			//out += filterChannels[i] ;
			feedback += filterChannels[i];
			
			float amp =  gDeviceState.synthGain * 1./ ((float) NUM_OSCS);

		//	float amp = gAmplitude;
			//* envelopes[i].process();
			// float amp = gAmplitudes[i] * 0.8;
			

			out += sinf(gPhases[i]) * amp;
			
			outArray[0] += sinf(gPhases[i]) * amp * (1 - gDeviceState.panning[i]);
			outArray[1] += sinf(gPhases[i]) * amp * gDeviceState.panning[i];
			
			
			//out += sinf();

			// switch(gOscType) {
			// 	default:
			// 	// SINEWAVE
			// 	case sine:
			// 		out += sinf(gPhases[i]);
			// 		break;
			// 	// TRIANGLE WAVE
			// 	case triangle:
			// 		if (gPhases[i] > 0) {
			// 		      out += -1 + (2 * gPhases[i] / (float)M_PI);
			// 		} else {
			// 		      out += -1 - (2 * gPhases[i]/ (float)M_PI);
			// 		}
			// 		break;
			// 	// SQUARE WAVE
			// 	case square:
			// 		if (gPhases[i] > 0) {
			// 		      out += 1;
			// 		} else {
			// 		      out += -1;
			// 		}
			// 		break;
			// 	// SAWTOOTH
			// 	case sawtooth:
			// 		out += 1 - (1 / (float)M_PI * gPhases[i]);
			// 		break;
			// 	}
		
			//out += sinf(jPhase);

			//feedback
			//out += filterChannels[i];


			
			//glide 
			gFrequencies[i] = gFrequencies[i] + ((gDeviceState.targetRatio[i]* gDeviceState.baseFrequency) - gFrequencies[i]) / (gDeviceState.glideAmount*gDeviceState.glideAmount);

	
			// Compute phase
			if(gDeviceState.oscillatorsOn[i]){
			gPhases[i] += 2.0f * (float)M_PI * gFrequencies[i] * gInverseSampleRate;
			//gPhases[i] += 2.0f * (float)M_PI * 300 * gInverseSampleRate;
			if(gPhases[i] > M_PI)
				gPhases[i] -= 2.0f * (float)M_PI;
			}
				
			//output to scope
			//gOutput[i] = out * NUM_OSCS;
			
		}

	   // gOutput[2] = finalOut;

		//feedback
	//	out += input;
		//output limiter
	//	outL = outputLimiter.processSample(outL);
	//	outR = outputLimiter.processSample(outR);
		out = outputLimiter.processSample(out);

//		gOutput[0] = finalOut;
//		scope.log(gOutput);

		

		// Write output to different channels
		for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
			 audioWrite(context, n, channel, outArray[channel]);
		//	audioWrite(context, n, channel, out);
		}
		
		//send to gui
		if (count >= gTimePeriod*context->audioSampleRate)
		{
			
			gui.sendBuffer(0, abs(feedback));
			gui.sendBuffer(1, abs(out));
			
			//rt_printf("%f", out);

			//and we reset the counter
			count = 0;
		}
	}
}

void cleanup(BelaContext *context, void *userData)
{
}



