/*
SELF GENERATIVE MIDI MUSIC GENERATOR
ALAN SMITH - 2016
*/

#include <FreqPeriod.h>
#include <MIDI.h>
#include <Adafruit_QDTech.h> // Display Hardware-specific library
#include <TimedAction.h>

//control wires for display 
#define sclk 13 // Don't change
#define mosi 11 // Don't change
#define cs 9
#define dc 8
#define rst 12

Adafruit_QDTech display = Adafruit_QDTech(cs, dc, rst); // Invoke custom library

struct midiEvent {
	byte ch = 255;
	byte mnote = 0;
	byte velocity = 0;
	byte playpos = 0;
	byte nkey = 0;
	long len;
	long ts;
};

const byte noteArraySize = 64;
const byte mmArraySize = noteArraySize;
struct midiEvent midiMessage[mmArraySize];
double lfrq;
long int pp;
int index = 0;
int channel = 1;
const int8_t chans = 16;
byte chanControl[chans][2];
int tempoDelay = 0;
byte note = 0;
int scale = 0;
const String space = " ";
const String dash = "-";
int8_t key = 0;
int phase = 0;
int playRange = 16;
int leadChannel = random(chans);
byte leadNoteMem[2];
int speedFactor = 1;
int beat = 0;
bool isSynced = true;


TimedAction timedMidi = TimedAction(250, DoMidiStuff);

//TimedAction timedMidi2 = TimedAction(555, DoControlStuff);


void setup() {
	MIDI.begin(MIDI_CHANNEL_OMNI);
	FreqPeriod::begin();
	display.init();
	display.setRotation(1); // 0 - Portrait, 1 - Lanscape
	display.setTextSize(1);
	display.setTextColor(QDTech_GREEN, QDTech_BLACK);
	display.fillScreen(QDTech_BLACK);
	//display.print(__FILE__);
	for (byte ch = 0; ch < chans; ch++) {
		chanControl[ch][0] = 64; // volumes
		chanControl[ch][1] = random(56, 76); // pans
		MIDI.sendControlChange(7, chanControl[ch][0], ch + 1);
		if (ch != 9) {
			MIDI.sendControlChange(10, chanControl[ch][1], ch + 1);
		}
		MIDI.sendControlChange(123, 0, ch + 1);
		delay(5);
		MIDI.sendControlChange(121, 0, ch + 1);
		delay(5);
		display.print(ch + dash);
	}
	delay(1000);
	randomSeed(analogRead(7));
	display.fillScreen(QDTech_BLACK);
	display.setCursor(0, 0);
	if (isSynced) {
		MIDI.setHandleClock(handleClock);
	}
	display.print("sync...");
	timedMidi.disable();
	// end of setup
}

void handleClock()
{
	if (beat % 6 == 0) {
		DoMidiStuff();
	}
	beat++;
}

void DoMidiStuff() {

	digitalWrite(10, HIGH);

	//store in the note array and then form the midi message
	if (midiMessage[index].mnote <= 0) {
		note = ScaleFilter(scale, byte(GetNote()), key);
		midiMessage[index].ch = index % chans;// %leadChannel;
		if (note >= 36 && note < 127) {
			midiMessage[index].mnote = note;   //main listening function
		}
		else {
			//midiMessage[index].mnote = 0;
		}
		midiMessage[index].playpos = random(playRange); // index % playRange;
		midiMessage[index].velocity = random(40, 127);
		midiMessage[index].nkey = key;
		midiMessage[index].len = long(random((tempoDelay - speedFactor) / 4, (tempoDelay - speedFactor) * 4));
		midiMessage[index].ts = long(millis()); //timestamp
	}

	//Start to replace notes when the array is 3/4 full
	if (random(100) > 95) {
		MIDI.sendNoteOff(midiMessage[index].mnote + midiMessage[index].nkey, 0, midiMessage[index].ch + 1);
		midiMessage[index].mnote = 0;
		if (!isSynced) {
			DisplayStuff(index, 0);
		}
	}

	/*if (midiMessage[index].len < tempoDelay * 4) {
		midiMessage[index].len += 30L;
	}*/

	// Underlying Drumbeat

	if (index % random(2,4) == 0) { // HH  
		MIDI.sendNoteOn(42, random(40, 80), 10);
		MIDI.sendNoteOff(42, random(40, 70), 10);
	}

	if (midiMessage[index].mnote > 0) {

		if (index % 4 == 0) { // KICK
			MIDI.sendNoteOn(36, random(70, 110), 10);
			MIDI.sendNoteOff(36, random(70, 110), 10);
		}
		if (index % (random(8, 10)) == 0) { // SNARE
			MIDI.sendNoteOn(38, random(70, 100), 10);
			MIDI.sendNoteOff(38, random(70, 100), 10);
		}
		if (index % 16 == 15) { // ANYTHING CRASH
			int dd = random(36, 60);
			MIDI.sendNoteOn(dd, random(70, 100), 10);
			MIDI.sendNoteOff(dd, random(70, 110), 10);
		}
	}

	

	//Send the notes out

	//lead note stuff
	leadNoteMem[0] = midiMessage[index].mnote + midiMessage[index].nkey + 24;
	if (ScaleFilter(scale, leadNoteMem[0], key) > 24) {  //&& (index % 2) == 0
		
	    //******* TODO - implement melody note lengths
		MIDI.sendNoteOn(leadNoteMem[1], 0, leadChannel + 1);
		MIDI.sendNoteOn(leadNoteMem[0], random(40, 120), leadChannel + 1);
		leadNoteMem[1] = leadNoteMem[0];
	}

	for (byte c = 0; c < noteArraySize; c++) {

		if (midiMessage[c].mnote > 0
			&& midiMessage[c].playpos == (index % playRange)) {
			MIDI.sendNoteOn(byte(midiMessage[c].mnote + midiMessage[c].nkey), midiMessage[c].velocity - random(50), midiMessage[c].ch + 1);
			//MIDI.sendNoteOn(byte(midiMessage[c].mnote + midiMessage[c].nkey+12), midiMessage[c].velocity, leadChannel + 1);
			if (!isSynced) {
				DisplayStuff(c, midiMessage[c].mnote * 0x0F00);
			}

		}
		else {
			//delayMicroseconds(25);
		}

	}

	//Scan for note offs
	for (byte i = 0; i < noteArraySize; i++) {
		if (millis() >long(midiMessage[i].ts + midiMessage[i].len)) {
			MIDI.sendNoteOn(byte(midiMessage[i].mnote + midiMessage[i].nkey), 0, midiMessage[i].ch + 1);
			//MIDI.sendNoteOff(byte(midiMessage[i].mnote + midiMessage[i].nkey+12), 127, leadChannel + 1);
			if (!isSynced) {
				DisplayStuff(i, 1);
			}
			midiMessage[i].nkey = key;
			if (ScaleFilter(scale, midiMessage[i].mnote, key) > 0) {
				midiMessage[i].mnote = ScaleFilter(scale, midiMessage[i].mnote, key);
			}
			midiMessage[i].ts = long(millis() + midiMessage[i].len);
		}
		else {
			
			
			//delayMicroseconds(25);
		}
	}

	/*midiMessage[index].len += 25L;
	if (midiMessage[index].len < long(speedFactor) ) {
		MIDI.sendNoteOn(byte(midiMessage[index].mnote + midiMessage[index].nkey), 0, midiMessage[index].ch + 1);
		midiMessage[index].mnote = 0;
	}*/



	//// Randomize the channel if adjacent member is the same.
	//if (index > 0 && (midiMessage[index].ch == midiMessage[index - 1].ch)) {
	//	midiMessage[index].ch = random(chans);
	//}
	//// Randomize the play position if adjacent member is the same.
	//if (index > 0 && (midiMessage[index].playpos == midiMessage[index - 1].playpos)) {
	//	midiMessage[index].playpos = random(playRange);
	//}

	//Random Articulations for Kontakt (if required)
	if (index % 8 == 0) {
		int articnote = random(23, 29);
		MIDI.sendNoteOn(articnote, 1, midiMessage[index].ch + 1);
		MIDI.sendNoteOff(articnote, 1, midiMessage[index].ch + 1);
	}

	//Random mixing // Volumes
	if (midiMessage[index].ch != 9) {
		if (chanControl[midiMessage[index].ch][0] > 60) {
			chanControl[midiMessage[index].ch][0] -= random(10);
		}
		if (chanControl[midiMessage[index].ch][0] < 90) {
			chanControl[midiMessage[index].ch][0] += random(10);
		}
		MIDI.sendControlChange(7, chanControl[midiMessage[index].ch][0], midiMessage[index].ch + 1);
		//MIDI.sendControlChange(123,0,7);

		//Random mixing // PANS
		if (chanControl[midiMessage[index].ch][1] > 10) {
			chanControl[midiMessage[index].ch][1] -= random(18);
		}
		if (chanControl[midiMessage[index].ch][1] < 110) {
			chanControl[midiMessage[index].ch][1] += random(18);
		}
		MIDI.sendControlChange(10, chanControl[midiMessage[index].ch][1], midiMessage[index].ch + 1);
	}

	if (midiMessage[index].ch != 9) {
		chanControl[midiMessage[index].ch][0] = AGC();
	}

	//increment main counter
	index++;

	if (index % 8 == 0) {
		scale = random(8);
		MIDI.sendControlChange(123, 0, leadChannel + 1);
		if (random(3) == 1) {
			leadChannel = random(chans);
			chanControl[leadChannel][0] = 64;
			chanControl[leadChannel][1] = 64;

		}
	}

	//check and reset index counter
	if (index >= noteArraySize) {
		//MIDI.sendNoteOff(byte(midiMessage[index].mnote + midiMessage[index].nkey), 127, midiMessage[index].ch + 1);
		index = 0;

		//playRange = (phase + 1) * 4;


		MIDI.sendControlChange(123, 0, leadChannel + 1);
		if (random(3) == 1) {
			leadChannel = random(chans);
			chanControl[leadChannel][0] = 64;
			chanControl[leadChannel][1] = 64;

		}
		scale = random(8);
		phase++;
		// change key
		if (phase % 4 == 0) {
			MIDI.sendControlChange(123, 0, leadChannel + 1);
			key = random(12);



		}
		// change scale
		if (phase > playRange / 2) {
			MIDI.sendControlChange(123, 0, leadChannel + 1);
			scale = random(8);
			phase = 0;
			key = 0;
			speedFactor+=5;

		}
		display.setTextColor(QDTech_YELLOW, QDTech_BLACK); display.setTextSize(2);
		display.setCursor(0, 0);
		display.print((leadChannel + 1) + space + key + space + scale + space + countNotes() + space + space);
	}

	// other randomization stuff

	if (random(10) == 5 && index % 8 == 0) {
		juggleChannels();
	}

	/*if (random(30) == 5 && index == 0) {
		jugglePositions();
	}*/

	/*if (random(30) == 5 && index % 8 == 0) {
		pumpNoteUp();
	}

	if (random(30) == 25 && index % 8 == 0) {
		pumpNoteDown();
	}*/

}

void juggleChannels() {
	for (byte c = 0; c < noteArraySize; c++) {
		if (midiMessage[c].mnote != 0) {
			MIDI.sendNoteOn(byte(midiMessage[c].mnote + midiMessage[c].nkey), 0, midiMessage[c].ch + 1);
			midiMessage[c].ch = random(chans);
		}
	}
}

int countNotes() {
	int nc = 0;
	for (byte c = 0; c < noteArraySize; c++) {
		if (midiMessage[c].mnote > 0) {
			nc++;
		}
	}
	return nc;
}

void jugglePositions() {
	for (byte c = 0; c < noteArraySize; c++) {
		if (midiMessage[c].mnote != 0) {
			MIDI.sendNoteOn(byte(midiMessage[c].mnote + midiMessage[c].nkey), 0, midiMessage[c].ch + 1);
			midiMessage[c].playpos = random(playRange);
		}
	}
}

byte AGC() {
	int x = 100 - map(analogRead(7), 0, 1023, 0, 80);
	return x;
}

void pumpNoteUp() {
	for (byte c = 0; c < noteArraySize; c++) {
		if (midiMessage[c].mnote != 0) {
			if (midiMessage[c].playpos % 4 == 0) {
				MIDI.sendNoteOn(byte(midiMessage[c].mnote + midiMessage[c].nkey), 0, midiMessage[c].ch + 1);
				midiMessage[c].mnote = ScaleFilter(scale, 36 + c, key);
			}
		}
	}
}

void pumpNoteDown() {
	for (byte c = 0; c < noteArraySize; c++) {
		if (midiMessage[c].mnote != 0) {
			if (midiMessage[c].playpos % 4 == 0) {
				MIDI.sendNoteOn(byte(midiMessage[c].mnote + midiMessage[c].nkey), 0, midiMessage[c].ch + 1);
				midiMessage[c].mnote = ScaleFilter(scale, 96 - c, key);
			}
		}
	}
}

void loop() {
	if (isSynced == true && digitalRead(2) == HIGH) {
		isSynced = false;
		display.setCursor(0, 0);
		timedMidi.enable();
		//timedMidi2.enable();
		display.fillScreen(QDTech_BLACK);
		display.print("Int Sync");
		delay(2000);
		display.fillScreen(QDTech_BLACK);
	}



	if (isSynced) {
		MIDI.read();
	}
	else {
		timedMidi.check();
		tempoDelay = map(analogRead(4), 0, 1023, 20, 1000);
		timedMidi.setInterval(tempoDelay - speedFactor);
	}
	//end of loop
}

float GetNote() {
	byte sample = 0;
	float gnote = 0.0;
	pp = FreqPeriod::getPeriod();
	for (sample = 0; sample < 5; sample++) {
		if (pp) {
			digitalWrite(10, LOW);
			lfrq = 16000000.0 / (pp); // 16000000/256 Mhz - fine tuning
			gnote += log(lfrq / 440.0) / log(2) * 12 + 69;
		}
	}
	gnote = gnote / sample;  //get average
	return gnote - 24 ;
	digitalWrite(10, HIGH);
}

void DisplayStuff(int c, int pen) {
	/*display.drawPixel
	(
		2 + (display.width() / playRange)*midiMessage[c].playpos,
		(display.height() / noteArraySize)*c,
		(pen)
	);*/

	/*if (midiMessage[c].velocity > 100) {
		display.drawFastHLine(0, display.height() / noteArraySize*c, display.width(), pen);
		display.drawFastVLine(2 + (display.width() / playRange)*midiMessage[c].playpos, 0, display.height(), pen);
	}*/
	display.drawCircle
	(
		2 + (display.width() / playRange)*midiMessage[c].playpos,
		(display.height() / noteArraySize)*c,
		2,
		(pen)
	);
}


int ScaleFilter(int scale, int n, int key) {
	switch (scale) {
	case 0: //Harmonic Minor
		if ((n - (key % 12)) % 12 == 0
			|| (n - (key % 12)) % 12 == 2
			|| (n - (key % 12)) % 12 == 3
			|| (n - (key % 12)) % 12 == 5
			|| (n - (key % 12)) % 12 == 7
			|| (n - (key % 12)) % 12 == 8
			|| (n - (key % 12)) % 12 == 11)
		{
			return n; break;
		}
		else {
			return 0;
		}
		break;

	case 1: //Minor Melodic
		if ((n - (key % 12)) % 12 == 0
			|| (n - (key % 12)) % 12 == 2
			|| (n - (key % 12)) % 12 == 3
			|| (n - (key % 12)) % 12 == 5
			|| (n - (key % 12)) % 12 == 7
			|| (n - (key % 12)) % 12 == 9
			|| (n - (key % 12)) % 12 == 11)
		{
			return n; break;
		}
		else {
			return 0;
		}
		break;

	case 2: //Diminished
		if ((n - (key % 12)) % 12 == 0
			|| (n - (key % 12)) % 12 == 2
			|| (n - (key % 12)) % 12 == 3
			|| (n - (key % 12)) % 12 == 5
			|| (n - (key % 12)) % 12 == 6
			|| (n - (key % 12)) % 12 == 8
			|| (n - (key % 12)) % 12 == 9
			|| (n - (key % 12)) % 12 == 11)
		{
			return n; break;
		}
		else {
			return 0;
		}

		break;

	case 3: //Blues
		if ((n - (key % 12)) % 12 == 0
			|| (n - (key % 12)) % 12 == 3
			|| (n - (key % 12)) % 12 == 5
			|| (n - (key % 12)) % 12 == 6
			|| (n - (key % 12)) % 12 == 7
			|| (n - (key % 12)) % 12 == 10
			|| (n - (key % 12)) % 12 == 11)
		{
			return n; break;
		}
		else {
			return 0;
		}
		break;

	case 4: //Major Pentatonic

		if ((n - (key % 12)) % 12 == 0
			|| (n - (key % 12)) % 12 == 2
			|| (n - (key % 12)) % 12 == 4
			|| (n - (key % 12)) % 12 == 7
			|| (n - (key % 12)) % 12 == 9)
		{
			return n; break;
		}
		else {
			return 0;
		}
		break;

	case 5: //Minor Pentatonic

		if ((n - (key % 12)) % 12 == 0
			|| (n - (key % 12)) % 12 == 2
			|| (n - (key % 12)) % 12 == 3
			|| (n - (key % 12)) % 12 == 4
			|| (n - (key % 12)) % 12 == 5
			|| (n - (key % 12)) % 12 == 6
			|| (n - (key % 12)) % 12 == 7
			|| (n - (key % 12)) % 12 == 9
			|| (n - (key % 12)) % 12 == 10
			|| (n - (key % 12)) % 12 == 11)
		{
			return n; break;
		}
		else {
			return 0;
		}
		break;

	case 6: //Gypsy

		if ((n - (key % 12)) % 12 == 0
			|| (n - (key % 12)) % 12 == 1
			|| (n - (key % 12)) % 12 == 4
			|| (n - (key % 12)) % 12 == 5
			|| (n - (key % 12)) % 12 == 7
			|| (n - (key % 12)) % 12 == 8
			|| (n - (key % 12)) % 12 == 9)
		{
			return n; break;
		}
		else {
			return 0;
		}
		break;

	case 7: //Augmented
		if ((n - (key % 12)) % 12 == 0
			|| (n - (key % 12)) % 12 == 2
			|| (n - (key % 12)) % 12 == 4
			|| (n - (key % 12)) % 12 == 6
			|| (n - (key % 12)) % 12 == 8
			|| (n - (key % 12)) % 12 == 9
			|| (n - (key % 12)) % 12 == 11)
		{
			return n; break;
		}
		else {
			return 0;
		}
		break;
	}
}

