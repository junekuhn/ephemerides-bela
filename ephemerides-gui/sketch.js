/*
For sending a buffer from p5.js to Bela we use the function:
```
Bela.data.sendBuffer(0, 'float', buffer);

old buffer structure
| gui           | to                 | bela |                                  |
| ------------- | ------------------ | ---- | -------------------------------- |
| 0             | pot values         | 2    | index and value floats           |
| 1             | preset index       | 1    | float                            |
| 2             | 32 touch values    | 3    | float and index and panning                            |
| 3             | register keys      | 2    | index and value floats           |
| 4             | big buttons.       | 1    | float(bitmask)                   |
| 5             | freq buttons       | 1    | float(bitmask)                   |

new buffer structure
| gui           | to                 | bela |                                  |
| ------------- | ------------------ | ---- | -------------------------------- |
| 0             | pot values         | 16    |  value floats           | SEND
| 2             | 32 touch values    | 16    | float and index and panning     | SEND
| 3             | register keys      | 5    |  value floats           | SEND 
| 4             | big buttons.       | 1    | float(bitmask)                   | SEND
| 5             | freq buttons       | 1    | float(bitmask)                   | SEND 
| 2             | panning		     | 16    | float and index and panning       SEND 

| 1             | preset index       | 1    | float                            | RECEIVE
| 1             | preset index       | 1    | float                            | RECEIVE
| 1             | preset index       | 1    | float                            | RECEIVE
| 1             | preset index       | 1    | float                            | RECEIVE
| 
```
where the first argument (0) is the index of the sent buffer, second argument ('float') is the type of the
data, and third argument (buffer) is the data array to be sent.

error handling is mostly on bela side 

*/

let outerArc;
let backgroundColor;
let pots, presetButtons, bigButtons, registerButtons;
let meters;
let segmentDisplay;
let channelGains = [0.5, 0.2, 0.99,0.3, 0.2, 0.4, 0.2, 0.2];
let config;
let leftMeter, rightMeter, leftGain, rightGain;
let labels = {
  pots: ["Synth Gain", "High Frequency Boost", "Filter Q", "Filter Gain", "Lowpass Cutoff", "Mic Gain", "Unassigned", "Limiter Threshold", "Limiter Lookahead", "Limiter Release", "Reference Frequency", "Feedback Amount", "Glide", "Unassigned", "Unassigned", "Panning"],
  presetButtons: [],
  big: ["Save Preset", "Set Reference", "Output Toggle", "Drone Mode"],
//  freq: ["Mic Input", "Keyboard Input", "Last Selected"],
  register: ["Numerator", "Denominator", "Divisor", "Index", "Algorithm"]
}

// send buffers
const MAX_CHANNELS = 16;
const BUFFER_POT = 0;
const BUFFER_TOUCH = 1;
const BUFFER_REGISTER = 2;
const BUFFER_BIG = 3;
const BUFFER_FREQ = 4;
const BUFFER_REC_SEND = 5;
const BUFFER_PANNING = 6;

//receive buffers
const BUFFER_RMS = 7;   
const BUFFER_LEFT = 8;
const BUFFER_RIGHT = 9;
const BUFFER_PRESET = 10;
const BUFFER_AVAIL = 11;

//registers 
const NUMERATOR = 0;
const DENOMINATOR = 1;
const DIVISOR = 2;
const INDEX = 3;
const OPTIONS = 4;

//not implemented yet
const PAGE_UP = 5;
const PAGE_DOWN = 6;

let numPoints = MAX_CHANNELS;

 

// ---- Recording state ----
let isRecording    = false;
let logData        = [];   // accumulated JSON entries

// ---- GUI elements ----
let recordButton;
let downloadRTData;
let statusText;
let channelMeters = [];    // visual RMS meters per channel

//presets 
let presetManager;
let fileInput;

function setup() {
  createCanvas(800, 500);

  //fixing bela auto css issues
  document.querySelector("body").style = "position: absolute; height: auto;"
  
  backgroundColor = color(255, 255, 255);
  
  outerArc = new arcTouch(0,0,300,300, MAX_CHANNELS);
  
  presetManager = new PresetManager(9);
  
	  // Download bank
	let downloadBank = createButton('Download Bank');
	downloadBank.position(width-300, height);
	downloadBank.mousePressed(() => presetManager.downloadBank());
	
	let uploadButton = createButton('Upload Presets');
	uploadButton.position(width-450, height);
	uploadButton.mousePressed(() => {
	    fileInput = createFileInput((file) => {
	        presetManager.loadBankFromFile(file.file);
	    });
	})
  
  let potCoords = [
      createVector(-2, -7),
      createVector(0,-7),
      createVector(2, -7),
      createVector(-4, -5),
      createVector(-2, -5),
      createVector(0, -5),
      createVector(2, -5),
      createVector(4, -5),
      createVector(-6, -3),
      createVector(-4, -3),
      createVector(4, -3),
      createVector(6, -3),
      createVector(-6, -1),
      createVector(-4, -1),
      createVector(4, -1),
      createVector(6, -1)
    ];
  pots = new buttonArray(potCoords, potCoords.length,12, length = 12, type = "POT");
  
  let presetCoords = [
    createVector(-10, 2),
    createVector(-8, 2),
    createVector(-6, 2),
    createVector(-9, 4),
    createVector(-7, 4),
    createVector(-5, 4),
    createVector(-8, 6),
    createVector(-6, 6),
    createVector(-4, 6)
  ];
  presetButtons = new buttonArray(presetCoords, presetCoords.length, 12,12, type="PRESET");
  
  let bigButtonCoords = [
    createVector(-4, 10),
    createVector(-1, 10),
    createVector(2, 10),
    createVector(5, 10)
  ]
  bigButtons = new buttonArray(bigButtonCoords, bigButtonCoords.length, 12, 16, type="BIG");
  
  //let freqButtonCoords = [
  //  createVector(4, 2),
  //  createVector(6, 2),
  //  createVector(8, 2)
  //]
  //freqButtons = new buttonArray(freqButtonCoords, freqButtonCoords.length, 12, 12, type="FREQ");
  
  let registerButtonCoords = [
    createVector(-1, -2),
    createVector(1, -2),
    createVector(-1, 0),
    createVector(1, 0),
    createVector(0, 2)
  ]
  registerButtons = new buttonArray(registerButtonCoords, registerButtonCoords.length, 12, 15, type="REGISTER");
  
  leftMeter = new Meter(-1*12, 8*12, 12, 60, 8);
  rightMeter = new Meter(12, 8*12, 12, 60, 8);
  
  
    recordButton = createButton('Record');
    recordButton.position(width, height);
    recordButton.mousePressed(onRecordButtonPressed);

    downloadRTData = createButton('Download JSON');
    downloadRTData.position(width-150, height);
    downloadRTData.attribute('disabled', '');
    downloadRTData.mousePressed(onDownloadPressed);
  
	
	setTimeout(() => {
		 //after presets loaded, send
		presetManager.loadPreset(0);
	}, 1000);
	
	// Status text
    statusText = createP('Stopped');
    statusText.position(240, 14);
 
  
}

function draw() {
  background(backgroundColor);
  translate(width/4, height/2);
  
  noFill();
  stroke(0);
  textSize(25);
  textAlign(CENTER)
  text("EPHEMERIDES", 5, -180);
  ellipse(0,0,300,300);
  
  outerArc.drawShapes();
  pots.drawShapes();
  presetButtons.drawShapes();
  bigButtons.drawShapes();
 // freqButtons.drawShapes();
  registerButtons.drawShapes();
  
  //pull first two buffers to meters
  leftMeter.drawShapes(Bela.data.buffers[BUFFER_LEFT]);
  rightMeter.drawShapes(Bela.data.buffers[BUFFER_RIGHT]);
  
  fill(0,255,0);
  rectMode(CORNER)
  //channelGains.map((channelGain, index) => {
  //  let channelSpacing = 300 / channelGains.length;
  //  rect(width/4 + index*channelSpacing, height*0.4, channelSpacing-4, -60*channelGain*100 )
  //})
  rectMode(CENTER)
  
  //segment display
  noFill();
  stroke(0);
  rect(6*12, 4*12, 7*12, 2*12);
  textAlign(CENTER, CENTER);
  textSize(15);
  text("1 0. 3 0 4", 6*12, 4*12 );
  

    // ---- Read incoming RMS buffer ----
    let rmsBuf = Bela.data.buffers[BUFFER_RMS];
    if(rmsBuf && rmsBuf.length == MAX_CHANNELS + 1) {
        let timestamp = rmsBuf[0];
        let rms       = Array.from(rmsBuf.slice(1, 9));
        
        // Update meters regardless of recording state
        for(let i = 0; i < MAX_CHANNELS; i++)
            channelGains[i] = rms[i];

        // Only log if recording
        if(isRecording) {
            logData.push({
                t:   Math.round(timestamp),
                rms: rms.map(v => parseFloat(v.toFixed(4)))
            });
        }
    }
    

}

class Preset {

    constructor() {
        this.index = 0;
        this.sliderPositions = [
            0, // synthGain
            0,   // hiFreqBoost
            0.5, // filterQ
            0.5, // filterGain
            0,   // lpCutoff
            0,   // micGain
            0,   // Unassigned
            0.2,   // limiterThresh
            0.5,   // limiterLookahead
            0.2,   // limiterRelease
            0.4,   // reference
            0.5,   // fbAmount
            0,   // glideAmount
            0,   // Unassigned
            0,   // Unassigned
            0.5  // panning
        ];

        this.registerPositions = [
            3, // Numerator
            1, // Denominator
            4, // Divisor
            0, // offset
            0  // Algorithm
        ];

        this.oscillatorsOn =  new Array(MAX_CHANNELS).fill(false);
        this.panning       = new Array(MAX_CHANNELS).fill(0.5);
    }

    toJSON(data = {sliders: this.sliderPositions, registers: this.registerPositions, oscs: this.oscillatorsOn, panning: this.panning }) {
    	
    	console.log(data.sliders)
    	
    	//pull state first and store here 
    	this.sliderPositions = [...data.sliders];
    	this.registerPositions = [...data.registers];
    	this.oscillatorsOn = [...data.oscs];
    	this.panning = [...data.panning];
    	
        return {
            index:             this.index,
            sliderPositions:   [...this.sliderPositions],   
            registerPositions: [...this.registerPositions],
            oscillatorsOn:     [...this.oscillatorsOn],
            panning:           [...this.panning]
        };
    }

    fromJSON(data) {
        this.index             = data.index             ?? this.index;
        this.sliderPositions   = data.sliderPositions   ?? [...this.sliderPositions];
        this.registerPositions = data.registerPositions ?? [...this.registerPositions];
        this.oscillatorsOn     = data.oscillatorsOn     ?? [...this.oscillatorsOn];
        this.panning           = data.panning           ?? [...this.panning];
        
        return this;
    }

    // ---- Load from a JSON object and send all parameters to Bela ----
    load(data) {
        // Populate fields from JSON
        this.fromJSON(data);
        
        //load into state 
        this.loadPresetToState();

        // Send each parameter group to Bela via matching buffer IDs
        this._sendPots();
        this._sendRegisters();
        this._sendOscillators();

        console.log(`Preset ${this.index} loaded`);
    }
    
    loadPresetToState() {
		outerArc.registerStates = [...this.registerPositions];
		
		this.sliderPositions.map((value, i) => {
			pots.state.elements[i].elt.value = value;
			pots.state.values[i] = value;
		});
		
		this.oscillatorsOn.map((value, i) => {
			outerArc.state.elements[i].elt.checked = value;
			outerArc.state.colors[i] = color(value ? 0:255, 0, 0)
	    	outerArc.state.touched[i] = value;
	    	outerArc.state.panning[i] = this.panning[i];
		});
	
		
		
		console.log(`uploaded preset ${this.index} to state`)
	}

    // ---- Private send helpers ----

    _sendPots() {
        // Send all slider positions - Bela will handle soft takeover
        // Buffer 6: [potIndex, potValue] pairs sent sequentially
        let sendArray = [];
        this.sliderPositions.map(val => {
        	sendArray.push(parseFloat(val));
        });
        Bela.data.sendBuffer(BUFFER_POT, 'float', sendArray);
    }

    _sendRegisters() {
    	let sendArray = [];
		this.registerPositions.map((val, i) => {
        	sendArray.push(parseFloat(val));		
		});
		Bela.data.sendBuffer(BUFFER_REGISTER, 'float', sendArray);	
    }

    _sendOscillators() {
        let oscFloats = this.oscillatorsOn.map(v => v ? 1.0 : 0.0);
    	setTimeout(() => {
			Bela.data.sendBuffer(BUFFER_TOUCH, 'float', oscFloats);
		}, 2000);
    }
}

class PresetManager {

    constructor(bankSize = 9) {
        this.bankSize      = bankSize;
        this.currentIndex  = 0;
        this.bank          = Array.from({ length: bankSize }, (_, i) => {
            let p = new Preset();
            p.index = i;
            return p.toJSON();  // initialise bank with defaults
        });
        this.current = new Preset();
    }

    loadPreset(index) {
        if(index < 0 || index >= this.bankSize) return;
        this.currentIndex = index;
        this.current.load(this.bank[index]);
    }

    // ---- Save current state into bank slot ----
    savePreset(data) {
        this.current.index = this.currentIndex;
        this.bank[this.currentIndex] = this.current.toJSON(data);
        console.log(`Saved to slot ${this.currentIndex}`);
    }

    // ---- Download entire bank as JSON ----
    downloadBank() {
        let json = JSON.stringify({ presets: this.bank }, null, 2);
        let blob = new Blob([json], { type: 'application/json' });
        let url  = URL.createObjectURL(blob);
        let a    = document.createElement('a');
        a.href     = url;
        a.download = `preset_bank_${Date.now()}.json`;
        a.click();
        URL.revokeObjectURL(url);
    }

    // ---- Load bank from JSON file via file input element ----
    loadBankFromFile(file) {
        let reader = new FileReader();

        reader.onload = (e) => {
            try {
                let data = JSON.parse(e.target.result);

                if(!data.presets || !Array.isArray(data.presets)) {
                    console.error('Invalid preset bank file');
                    return;
                }

                // Merge loaded presets into bank
                // ?? fallback keeps existing slot if file is missing that index
                data.presets.forEach((p, i) => {
                    if(i < this.bankSize) this.bank[i] = p;
                });

                // Reload current preset from updated bank
                this.loadPreset(this.currentIndex);
                console.log(`Bank loaded: ${data.presets.length} presets`);

            } catch(e) {
                console.error('Failed to parse preset file:', e);
            }
        };

        reader.readAsText(file);
    }
}


class arcTouch {
  constructor(x, y, w, h, numPoints) {
    this.x = x;
    this.y = y;
    this.w = w; 
    this.h  = h;
    this.numPoints = numPoints;
    this.state = {
      colors: [],
      // this contains both index and value
      touched: new Array(MAX_CHANNELS).fill(false),
      elements: [],
      panning: new Array(MAX_CHANNELS).fill(0.5),
      lastTouched: 0
    }
    this.maxTouchPoints = 4;
    this.init();
    //what states each register is in
    this.registerStates = [1, 0, 3, 0, 1];
    this.registerOn = false;
    this.registerIndex = 0;
  }
  
  init() {
    for(let point = 0; point<this.numPoints; point++) {
      this.state.colors.push(color(255, 0, 0));
      
      this.state.elements.push(createInput(`${point}`, "checkbox"));
      this.state.elements[point].position(10 + 22*point, height-20);
      this.state.elements[point].size(30);
      this.state.elements[point].elt.addEventListener("change", updateArc);
    }
  }
  
  drawShapes() {
    
    textSize(10);
    textAlign(LEFT);
    fill(0);
    noStroke();
    text("Touch Points", -width*0.18,height*.42);
    
    
    stroke(backgroundColor);
    strokeWeight(1);
    let angleInc = Math.PI/this.numPoints;
    
    for(let point = 0; point<this.numPoints; point++) {
      
    let angle = Math.PI + angleInc*point;
    let nextAngle = Math.PI + angleInc*(point+1);
    fill(this.state.colors[point]);
    arc(this.x, this.y, this.w, this.h, angle, nextAngle, PIE);
    fill(backgroundColor);
    arc(this.x, this.y, this.w*0.9, this.h*0.9, angle, nextAngle, PIE);
    // fill(this.state.colors[this.numPoints+point]);
    // arc(this.x, this.y, this.w*0.8, this.h*0.8, angle, nextAngle, PIE);
    // fill(backgroundColor);
    // arc(this.x, this.y, this.w*0.7, this.h*0.7, angle, nextAngle, PIE);
      
    }
  }
}

class buttonArray {
  constructor(coords, numButtons, spacing, length = 12, type = "TOGGLE") {
    this.numButtons = numButtons;
    this.spacing = spacing;
    this.length = length;
    this.state = {
      values: new Array(numButtons).fill(0),
      colors: new Array(numButtons).fill(color(0,0,0)),
      elements: [],
      mask: 0,
    }
    this.type  = type;
    this.coords = coords;
    this.init();
  }
  
  init() {
    for (let button = 0; button<this.numButtons; button++) {
      this.state.colors.push(0);
          
        if(this.type == "POT") {
          //create sliders
          this.state.elements.push(createSlider(0, 1, 0, 0.01));
          this.state.elements[button].elt.setAttribute("data-index", `${button}`);
          this.state.elements[button].position(width*.5, 40 + button*20);
          this.state.elements[button].size(100);
          this.state.elements[button].elt.addEventListener("change", updatePot);

      } else if (this.type == "BIG") {
          this.state.elements.push(createInput(`${button}`, "checkbox"));
          this.state.elements[button].position(width*0.8, 40+button*20);
          this.state.elements[button].size(40);
          this.state.elements[button].elt.addEventListener("change", updateBig);
      } else if (this.type == "REGISTER") {
          this.state.elements.push(createInput(`${button}`, "checkbox"));
          this.state.elements[button].position(width*0.8, 120+button*20);
          this.state.elements[button].size(40);
          this.state.elements[button].elt.addEventListener("change", updateRegister);
      } 
      //else if (this.type == "FREQ") {
      //    this.state.elements.push(createInput(`${button}`, "checkbox"));
      //    this.state.elements[button].position(width*0.8, 240+button*20);
      //    this.state.elements[button].size(40);
      //    this.state.elements[button].elt.addEventListener("change", updateFreq);
      //}
    }
    
    //outsite for loop
    if (this.type == "PRESET") {
          this.state.elements.push(createSlider(1, 12, 0, 1));
          this.state.elements[0].position(width*.8, 320);
          this.state.elements[0].size(100);
          this.state.elements[0].elt.addEventListener("change", updatePreset);
      }
  }
  
  drawShapes() {
    rectMode(CENTER)
    
    for(let button = 0; button<this.numButtons; button++) {
        fill(this.state.colors[button]);
        rect(this.coords[button].x*this.spacing, this.coords[button].y*this.spacing, this.length, this.length);
      if(this.type == "POT") {
        textSize(10)
        textAlign(LEFT)
        text(labels.pots[button], width*0.4, -height/2 + 50+  button *20)
        
        this.state.colors[button] = color(this.state.elements[button].value(), 0, 0);
      } else if (this.type == "BIG") {
        text(labels.big[button], width*0.6, -height/2 + 50+  button *20 )
       // this.state.value = this.state.elements[button].checked;
       // this.state.colors[button] = this.state.value;
      } else if (this.type =="REGISTER") {
        text(labels.register[button], width*0.6, -height/2 + 130+  button *20 )
      } 
      //else if (this.type =="FREQ") {
      //  text(labels.freq[button], width*0.6, -height/2 + 250+  button *20 )
      //}
    }
    
    if (this.type == "PRESET") {
      textSize(10);
      textAlign(LEFT);
      text("PRESET: " + this.state.value, 450,110);
      
       this.state.value = this.state.elements[0].value();
      //change current preset to red
      for(let button = 0; button<this.numButtons; button++){
        if(button == this.state.value-1) {
          this.state.colors[this.state.value-1] = color(255, 0, 0);
        } else {
          this.state.colors[button] = 0;
        }
      }
      
    }
  }
}

class Meter {
  constructor(x, y, w, l, divisions) {
    this.x = x;
    this.y = y;
    this.l = l;
    this.w = w;
    this.divisions = divisions;
  }
  
  drawShapes(percentFilled) {
    let leds = Math.round(this.divisions * percentFilled);
    let ledHeight = this.l/this.divisions;
    stroke(backgroundColor);
    
    for(let led = 0; led<leds; led++) {
      fill(255*led/8, 255-(255*led/8), 0)
      rect(this.x, this.y-ledHeight*led, this.w, ledHeight);
    }
  }
}

function updateArc(e) {
	
	//check if register key is down
	if(outerArc.registerOn) {
		
		outerArc.registerStates[outerArc.registerIndex] = e.target.value;
		
		//turn off register 
	  	registerButtons.state.values[outerArc.registerIndex] = false;
		registerButtons.state.elements[outerArc.registerIndex].elt.checked = false;
		registerButtons.state.colors[outerArc.registerIndex] = color(0,0,0);

		
		
		//resolve arc back to its original state (touched)
		for(let t = 0; t<outerArc.numPoints; t++){
			if(outerArc.state.touched[t]) {
				outerArc.state.colors[t] = color(0,0,0);
				outerArc.state.elements[t].elt.checked = true;
			} else {
				outerArc.state.colors[t] = color(255, 0, 0);
				outerArc.state.elements[t].elt.checked = false;
			}
		}
		
		//send registers to bela
		let sendArray = outerArc.registerStates.map(v => parseFloat(v));
		Bela.data.sendBuffer(BUFFER_REGISTER, 'float', sendArray);
		
		outerArc.registerOn = false;
		
		return;
		
	}
	
	//fill preset checkbox
	bigButtons.state.elements[0].elt.checked = true;
	
    
    
    //go through all points and colors and indicators based on state changes
    let index = e.target.value;
   
    outerArc.state.colors[index] = color(e.target.checked ? 0:255, 0, 0)
    outerArc.state.touched[index] = e.target.checked;
    outerArc.state.lastTouched = index;
    
    //send to bela 
    let oscFloats = outerArc.state.touched.map(v => v ? 1.0 : 0.0);
    Bela.data.sendBuffer(BUFFER_TOUCH, 'float', oscFloats);
}

function updateBig(e) {
  let index = e.target.value;
  
  console.log(pots.state.values)
  
  // if save preset
  if(index == 0) {
  	e.preventDefault();
	// should be filled if unsaved, and different from default
	// should be empty if once saved 
	
	// if filled then update and save and 
	if (e.target.checked == false) {
		// get current preset 
		//save everything to that preset
		let presetData = {
			   	//pull state first and store here 
    	sliders: pots.state.values,
    	registers: outerArc.registerStates,
    	oscs: outerArc.state.touched,
    	panning: outerArc.state.panning
		}
		
		
		presetManager.savePreset(presetData);
	} 
	// if unfilled do nothing
	else {
		return;
	}
	
	return;
  }
  bigButtons.state.colors[index] = color(e.target.checked ? 255:0, 0, 0);
  
  bigButtons.state.mask = updateBitmask(bigButtons.state.mask, index, e.target.checked);

  console.log(bigButtons.state.mask);
  Bela.data.sendBuffer(BUFFER_BIG, 'float', [bigButtons.state.mask])
}

function updateRegister(e) {

//last pressed 
  registerButtons.state.values.map((value, i) => {
  	if(value == true) {
  		registerButtons.state.values[i] = false;
  		registerButtons.state.elements[i].elt.checked = false;
  		registerButtons.state.colors[i] = color(0,0,0);
  	}
  });
  
  
  //these are shift keys, so only one at a time (last pressed), and toucharc needs to check 
  let index = e.target.value;

  
	//fill preset checkbox
	bigButtons.state.elements[0].elt.checked = true;
  
  registerButtons.state.colors[index] = color(e.target.checked ? 255:0, 0, 0);
  registerButtons.state.values[index] = e.target.checked;

	outerArc.registerOn = true;
	outerArc.registerIndex = index;
  
  //update arc with current register data
  for(let f = 0; f<outerArc.numPoints; f++) {
  	if (f == outerArc.registerStates[index]) {
  		outerArc.state.colors[f] = color(0,0,0);
  		outerArc.state.elements[f].elt.checked = true;
  	} else {
  		outerArc.state.colors[f] = color(255, 0, 0);
  		outerArc.state.elements[f].elt.checked = false;
  	}
  }
  
}

// function updateFreq(e) {
//   let index = e.target.value;
//   freqButtons.state.colors[index] = color(e.target.checked ? 255:0, 0, 0);
  
//   freqButtons.state.mask = updateBitmask(freqButtons.state.mask, index, e.target.checked);
  
//   Bela.data.sendBuffer(BUFFER_FREQ, 'float', [freqButtons.state.mask])
// }

function updatePreset(e) {
	presetManager.loadPreset(index);
}

function updatePot(e) {
	// value from 0 255
	let potvalue = e.target.value;
	let potindex = e.target.getAttribute("data-index");
	
	//fill preset checkbox
	bigButtons.state.elements[0].elt.checked = true;
	
	//update panning to state
	if(potindex == 15) {
		// last touched 
		outerArc.state.panning[outerArc.state.lastTouched] = potvalue;
		
		let panningArray = outerArc.state.panning.map(v => parseFloat(v));
		Bela.data.sendBuffer(BUFFER_PANNING, 'float', potsarray);
		
	} else {
		pots.state.values[potindex] = potvalue;
	}

	console.log("sending slider data", potindex, potvalue)
	let potsarray = pots.state.values.map(v => parseFloat(v));
	Bela.data.sendBuffer(BUFFER_POT, 'float', potsarray);
}

// Set or clear a bit by index and boolean value
function updateBitmask(mask, index, value) {
    if(value)
        return (mask | (1 << index)) >>> 0;   
    else
        return (mask & ~(1 << index)) >>> 0;  
}


//RMS handlers 
function onRecordButtonPressed() {
    let newState = !isRecording;
    
    console.log(newState);

    // Send toggle to Bela (convert to float)
    Bela.data.sendBuffer(BUFFER_REC_SEND, 'float', [newState ? 1.0 : 0.0]);

    setRecordingState(newState);
}

function setRecordingState(state) {
    isRecording = state;

    if(isRecording) {
        logData = [];   // clear previous recording
        recordButton.html('Stop');
        statusText.html('Recording...');
        statusText.style('color', 'red');
        downloadRTData.attribute('disabled', '');
    } else {
        recordButton.html('Record');
        statusText.html('Stopped');
        statusText.style('color', 'black');

        // Only enable download if we have data
        if(logData.length > 0)
            downloadRTData.removeAttribute('disabled');
    }
}

function onDownloadPressed() {
    if(logData.length === 0) return;

    let json     = JSON.stringify(logData, null, 2);
    let blob     = new Blob([json], { type: 'application/json' });
    let url      = URL.createObjectURL(blob);
    let a        = document.createElement('a');
    a.href       = url;
    a.download   = `rms_recording_${Date.now()}.json`;
    a.click();

    // Clean up blob URL after download
    URL.revokeObjectURL(url);
}




