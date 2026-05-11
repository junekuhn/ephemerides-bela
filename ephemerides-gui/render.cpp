#include <render.h>

int displaceIndex(int key) {
	int offset = NUM_KEYS - 1;
	int c = 0;
	
	if((int) divisor % 2 == 0) {
		offset = NUM_KEYS;
		c = (offset - divisor) / 2 + 1;
	} else {
		c = (offset - divisor) / 2;
	}
	
	
	return key - c;
}

float calculateRatio(algorithms algo, int index) {
	float ratio = 1;
	
	switch(algo) {
		case ARITHMETIC_DIVISION:
			ratio = ((float) index + 1.f) * (topValue - bottomValue) / divisor + bottomValue;
			break;
		case RATIO_DIVISION:
			ratio = ((float) index + 1.f) * (topValue/bottomValue) / divisor;
			break;
		case TET:
			ratio = pow( pow(topValue / bottomValue, 1 / divisor), (float) index + 1.f);
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
		
       // if (needsUpdate) {
			// loop through the oscillator bank
			for(int l = 0; l<NUM_OSCS; l++) {
				//if osc is on, update

			//calculate new frequency
			// printf("update=ing %i\n", l);
		//	targetRatio[l] = calculateRatio(selectedAlgorithm, displaceIndex( oscillatorsOn[l] ));

			//pull current frequencies and update filters
		 //   s.cutoff = gFrequencies[l];
		//	filterBank[l].setup(s);		
			
        }
                
		usleep(50000);
	}
}

bool setup(BelaContext *context, void *userData)
{
	gInverseSampleRate = 1.0 / context->audioSampleRate;

	scope.setup(NUM_OSCS, context->audioSampleRate);
	
	gui.setup(context->projectName);

	//setup filter settings
	s.fs = context->audioSampleRate;
	s.q = 3.0;
	s.peakGainDb = 8;
	s.type = Biquad::peak;

	
	for(int k = 0; k<NUM_OSCS; k++) {
		gPhases[k] = 0.0;
		gFrequencies[k] = 300.0 * (k+1);
		gAmplitude = 0.8f;
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
		float finalOut = 0;
		float input = 0;
		float feedback = 0;

		//input
		if(gAudioFramesPerAnalogFrame && !(n % gAudioFramesPerAnalogFrame)) {
			//for quantifying frequencies
			//float multiplier = floor(map(analogRead(context, n/gAudioFramesPerAnalogFrame, analogBaseFreq), 0, 1, 1, 20));
			//baseFrequency = 110 * pow( pow( 2.0 , 0.083333), multiplier);
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
			out += filterChannels[i] ;
			feedback += filterChannels[i];
			

			float amp = gAmplitude * 1./ ((float) NUM_OSCS);
		//	float amp = gAmplitude;
			//* envelopes[i].process();
			// float amp = gAmplitudes[i] * 0.8;

			out += sinf(gPhases[i]) * amp;
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
			//gFrequencies[i] = gFrequencies[i] + ((targetRatio[i]* baseFrequency) - gFrequencies[i]) / (glideAmount*glideAmount);

	
			// Compute phase
			gPhases[i] += 2.0f * (float)M_PI * gFrequencies[i] * gInverseSampleRate;
			//gPhases[i] += 2.0f * (float)M_PI * 300 * gInverseSampleRate;
			if(gPhases[i] > M_PI)
				gPhases[i] -= 2.0f * (float)M_PI;
				
			//finalOut += out;

			//output to scope
			//gOutput[i] = out * NUM_OSCS;
			
		}

	   // gOutput[2] = finalOut;

		//feedback
	//	out += input;
		//output limiter
		out = outputLimiter.processSample(out);

//		gOutput[0] = finalOut;

//		scope.log(gOutput);

		// Write output to different channels
		for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
			audioWrite(context, n, channel, out);
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
