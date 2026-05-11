/*
For sending a buffer from p5.js to Bela we use the function:
```
Bela.data.sendBuffer(0, 'float', buffer);
```
where the first argument (0) is the index of the sent buffer, second argument ('float') is the type of the
data, and third argument (buffer) is the data array to be sent.
*/

let outerArc;
let backgroundColor;
let numPoints = 16;
let pots, presetButtons, bigButtons, freqButtons, registerButtons;
let meters;
let segmentDisplay;
let channelGains = [0.5, 0.2, 0.99,0.3, 0.2, 0.4, 0.2, 0.2, 0.3];
let config;
let leftMeter, rightMeter, leftGain, rightGain;
let labels = {
  pots: ["Synth Gain", "High Frequency Boost", "Filter Q", "Filter Gain", "Lowpass Cutoff", "Mic Gain", "Filter Channel Gain", "Limiter Threshold", "Limiter Lookahead", "Limiter Release", "Reference Frequency", "Feedback Amount", "Glide", "Synth Attack", "Synth Release", ""],
  presetButtons: [],
  big: ["Save Preset", "Set Reference", "Output Toggle"],
  freq: ["Mic Input", "Keyboard Input", "Last Selected"],
  register: ["Numerator", "Denominator", "Divisor", "Index", "Algorithm"]
}




function setup() {
  createCanvas(800, 500);

  //fixing bela auto css issues
  document.querySelector("body").style = "position: absolute; height: auto;"
  
  backgroundColor = color(255, 255, 255);
  
  outerArc = new arcTouch(0,0,300,300, numPoints);
  
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
    createVector(-4, 6),
    createVector(-7, 8),
    createVector(-5, 8),
    createVector(-3, 8)
  ];
  presetButtons = new buttonArray(presetCoords, presetCoords.length, 12,12, type="PRESET");
  
  let bigButtonCoords = [
    createVector(-3, 10),
    createVector(0, 10),
    createVector(3, 10)
  ]
  bigButtons = new buttonArray(bigButtonCoords, bigButtonCoords.length, 12, 16, type="BIG");
  
  let freqButtonCoords = [
    createVector(4, 2),
    createVector(6, 2),
    createVector(8, 2)
  ]
  freqButtons = new buttonArray(freqButtonCoords, freqButtonCoords.length, 12, 12, type="FREQ");
  
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
  freqButtons.drawShapes();
  registerButtons.drawShapes();
  
  //pull first two buffers to meters
  leftMeter.drawShapes(Bela.data.buffers[0]);
  rightMeter.drawShapes(Bela.data.buffers[1]);
  
  fill(0,255,0);
  rectMode(CORNER)
  channelGains.map((channelGain, index) => {
    let channelSpacing = 300 / channelGains.length;
    rect(width/4 + index*channelSpacing, height*0.4, channelSpacing-4, -60*channelGain )
  })
  rectMode(CENTER)
  
  //segment display
  noFill();
  stroke(0);
  rect(6*12, 4*12, 7*12, 2*12);
  textAlign(CENTER, CENTER);
  textSize(15);
  text("1 0. 3 0 4", 6*12, 4*12 );
  

  
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
      touched: new Array(32).fill(false),
      elements: []
    }
    this.maxTouchPoints = 4;
    this.init();
  }
  
  init() {
    for(let point = 0; point<this.numPoints*2; point++) {
      this.state.colors.push(color(255, 0, 0));
      this.state.touched.push(false);
      
      this.state.elements.push(createInput(`${point}`, "checkbox"));
      this.state.elements[point].position(10 + 22*point, height-20);
      this.state.elements[point].size(30);
      this.state.elements[point].elt.addEventListener("change", updateArc);
    }
  }
  
  update(e) {

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
    fill(this.state.colors[this.numPoints+point]);
    arc(this.x, this.y, this.w*0.8, this.h*0.8, angle, nextAngle, PIE);
    fill(backgroundColor);
    arc(this.x, this.y, this.w*0.7, this.h*0.7, angle, nextAngle, PIE);
      
    }
 
  }
  
}

class buttonArray {
  constructor(coords, numButtons, spacing, length = 12, type = "TOGGLE") {
    this.numButtons = numButtons;
    this.spacing = spacing;
    this.length = length;
    this.state = {
      value: [],
      colors: [],
      elements: [],
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
          this.state.elements.push(createSlider(0, 255, 0, 10));
          this.state.elements[button].position(width*.5, 40 + button*20);
          this.state.elements[button].size(100);

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
      } else if (this.type == "FREQ") {
          this.state.elements.push(createInput(`${button}`, "checkbox"));
          this.state.elements[button].position(width*0.8, 240+button*20);
          this.state.elements[button].size(40);
          this.state.elements[button].elt.addEventListener("change", updateFreq);
      }
    }
    
    //outsite for loop
    if (this.type == "PRESET") {
          this.state.elements.push(createSlider(1, 12, 0, 1));
          this.state.elements[0].position(width*.8, 320);
          this.state.elements[0].size(100);
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
      } else if (this.type =="FREQ") {
        text(labels.freq[button], width*0.6, -height/2 + 250+  button *20 )
      }
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
      //go through all points and colors and indicators based on state changes
    let index = e.target.value;
    outerArc.state.colors[index] = color(e.target.checked ? 0:255, 0, 0)
    outerArc.state.touched[index] = e.target.checked;
    
    //send to bela 
    Bela.data.sendBuffer(index, 'bool', outerArc.state.touched);
}

function updateBig(e) {
  let index = e.target.value;
  bigButtons.state.colors[index] = color(e.target.checked ? 255:0, 0, 0);
}

function updateRegister(e) {
  let index = e.target.value;
  registerButtons.state.colors[index] = color(e.target.checked ? 255:0, 0, 0);
}

function updateFreq(e) {
  let index = e.target.value;
  freqButtons.state.colors[index] = color(e.target.checked ? 255:0, 0, 0);
}