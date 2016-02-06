/* Flash pin 13 10 times and toggle a latching relay on pins 11 and 12 */

// DEFINE DEBUG to produce readable (ASCII) command strings
//#define DEBUG 1

// For UP...CAM2, the constants correspond to Arduino inputs

#define STOP	0
#define UNDEF1	1
#define UNDEF2	2

#define UP	3   // Input pin D3
#define DOWN	4
#define LEFT	5
#define RIGHT	6
#define IN	7
#define OUT	8
#define CAM1	9
#define CAM2	10  // Input pin D10

#define CAM        0  // Location of cameraID in command string
#define PAN_SPEED  3  // Pan Speed byte
#define TILT_SPEED 4  // Tilt speed byte

int lastCommand = -1;
int relayState  = 0;
int speed[2]    =  { 0x20, 0x20 } ;

#ifdef DEBUG
byte pelco_stop[5]  = "czsss";
byte pelco_right[5] = "czrss";
byte pelco_left[5]  = "czlss";
byte pelco_up[5]    = "czuss";
byte pelco_down[5]  = "czdss";
byte pelco_in[5]    = "cziss";
byte pelco_out[5]   = "czoss";
#else
byte pelco_stop[5]  = { 0x01, 0x00, 0x00, 0x20, 0x20 };
byte pelco_right[5] = { 0x01, 0x00, 0x02, 0x20, 0x20 };
byte pelco_left[5]  = { 0x01, 0x00, 0x04, 0x20, 0x20 };
byte pelco_up[5]    = { 0x01, 0x00, 0x08, 0x20, 0x20 };
byte pelco_down[5]  = { 0x01, 0x00, 0x10, 0x20, 0x20 };
byte pelco_in[5]    = { 0x01, 0x00, 0x20, 0x20, 0x20 };
byte pelco_out[5]   = { 0x01, 0x00, 0x40, 0x20, 0x20 };
#endif

byte *msg[9] =	{
	pelco_stop,
	pelco_stop,  // spare
	pelco_stop,  // spare
	pelco_up,
	pelco_down,
	pelco_left,
	pelco_right,
	pelco_in,
	pelco_out
};

void setup()
{
        delay(2000);
	Serial.begin(2400, SERIAL_8N1); /* default */
#ifdef DEBUG
        Serial.print("hello, world.\n");
#endif
	for (int i = 3; i < 11; i++ ) {
		pinMode(i, INPUT);
		digitalWrite(i,1);
	}
	pinMode(11, OUTPUT);
	digitalWrite(11,1);
	pinMode(12, OUTPUT);
	digitalWrite(12,1);
}

/* If this command was already sent, just return
 * If previous command was not STOP: send STOP first
 * Finally, send the command
 */
void sendIfNeeded(int command)
{
    if (command == lastCommand) // Command was already sent
	return;
    if (lastCommand != 0 || command == STOP) {  // need a STOP
	send(msg[STOP]);
	delay(100);
    }
    if (command != STOP)
	send(msg[command]);    // Send the new command
    lastCommand = command; // and remember it.
}

void pulse(int pin, int del)
{
	digitalWrite(pin,0);	delay(del);
	digitalWrite(pin,1);	delay(del);
}

int relay(int onOff)
{
	if (onOff) pulse(11, 8);
	else 	   pulse(12, 8);
	return onOff;
}

void toggle(void) { relayState = relay(!relayState); }

void send(byte *msg)
{
byte chksum = 0;
	/*
	 * Before sending a command string:
	 * 1) Put the current camera address into byte zero
	 * 2) fill in the Pan/Tilt speeds for that camera
	 */
	if (relayState) msg[CAM] = 0x03;
	else            msg[CAM] = 0x07;
#ifdef DEBUG
	Serial.print("-");
	msg[PAN_SPEED] = msg[TILT_SPEED] = 's';
#else
	Serial.write(0xFF);
	msg[PAN_SPEED] = msg[TILT_SPEED] = speed[relayState];
#endif
	for( int i=0; i < 5; i++)
	{
		Serial.write(msg[i]);
		chksum += msg[i];
	}
#ifdef DEBUG
	Serial.print("\n");
#else
	Serial.write(chksum);
#endif
}

void loop() {

	pulse(13, 50);

// If one switch is down, we stop looking at the others.
// This is why a stuck switch will disable most, if not all of the other functions.
// Yet some functions might work (like up,down) before the stuck switch is checked

	if      ( !digitalRead(UP)  )	sendIfNeeded(UP);
	else if ( !digitalRead(DOWN))	sendIfNeeded(DOWN);
	else if ( !digitalRead(LEFT))	sendIfNeeded(LEFT);
	else if ( !digitalRead(RIGHT))	sendIfNeeded(RIGHT);
	else if ( !digitalRead(IN)  )  { sendIfNeeded(IN);
					 if (speed[relayState] > 0x10)
						speed[relayState]--;
	} else if ( !digitalRead(OUT)) { sendIfNeeded(OUT);
					 if (speed[relayState] < 0x20)
						speed[relayState]++;
	} else if ( !digitalRead(CAM1))	relayState = relay(1);
	  else if ( !digitalRead(CAM2))	relayState = relay(0);
	  else 				sendIfNeeded(STOP);

	delay(50);
}

