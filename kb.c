/*
 * File : kb.c
 * Created: June 2019
 * By: Colin Atkinson, EVOZ Pty Ltd
 *
 * Notes: This littpe program scans a sonder keyboard and creates an HID report.
 * The keyboard is arranged in rows and columns. A pair of 74HS959 shift registers are used to set each column line
 * high then each row is read. Each row is connected directly to a gpio pin and is enumberated in scan_pins.
 * The rpi scan pins do not equate to the same gpio pin. eg gpio pin 17 is mapped to rpi pin 0
 *
 * At the end of a complete scan a HID report is generated and sent
 *
 * HID reports are 8 bytes long.
 * The first bype is the modifier (shift, ctrl etc). zero if not used
 * The second byte is reserved and not used
 * The next 6 bytes are the HID coded allowing for 'chording'. If more than 6 keys are depressed an error is generated
 * Auto repeat is performed in the host which means that if no null report is sent after a key is released the host
 * will assume the key is still pressed.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <wiringPi.h>
#include "usb_hid_keycodes.h"

/***************************************************************
pin 0 => 17
pin 1 => 18
pin 2 => 27
pin 3 => 22
pin 4 => 23
pin 5 => 24
pin 6 => 25
pin 7 => 4
pin 8 => 2
pin 9 => 3
pin 10 => 8
pin 11 => 7
pin 12 => 10
pin 13 => 9
pin 14 => 11
pin 15 => 14
pin 16 => 15
pin 17 => 28
pin 18 => 29
pin 19 => 30
pin 20 => 31
pin 21 => 5
pin 22 => 6
pin 23 => 13
pin 24 => 19
pin 25 => 26
******************************************************************/



#define PIN_SER 2
#define PIN_RCLOCK 0
#define PIN_SRCLK 3
int scan_pins[] = {1,4,5,6,15,10,11};
int nscan_pins = 7;
int scan_inputs(int row);
int scan();
void set_one(int pin);
void set_all(int pin);
#define NCOLS 16
#define NROWS 9
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#define MIN_KEY_SCAN 1 // number of consecutive times a key has to be scanned to eliminate key bounce
#define MAX_SLOTS 6 // number of slots for key presses in hid report

char *kbstr[NROWS][NCOLS] = {
	{ "XX","esc","f1","f2","f3","f4","f5","f6","f7","f8","f9","f10","f11","f12","f13"},
	{ "XX2","`","1","2","3","4","5","6","7","8","9","0","-","=","del"},
	{"XX3","tab","q","w","e","r","t","y","u","i","o","p","[","]","\\"},
	{"XX4""caps","a","s","d","f","g","h","j","k","l",";","'","return","na"},
	{"XX5""lshift","z","x","c","v","b","n","m",",",".","/","rshift"},
	{"XX6","fn","ctl","opt","com","space","rcom","ropt","X","x2","x3","x4"}
};

unsigned char hidval[NROWS][NCOLS] = {
	{ 0,KEY_ESC,KEY_F1,KEY_F2,KEY_F3,KEY_F4,KEY_F5,KEY_F6,KEY_F7,KEY_F8,KEY_F9,KEY_F10,KEY_F11,KEY_F12,KEY_F13},
	{ 0,KEY_APOSTROPHE,KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,KEY_7,KEY_8,KEY_9,KEY_0,KEY_MINUS,KEY_EQUAL,KEY_BACKSPACE},
	{0,KEY_TAB,KEY_Q,KEY_W,KEY_E,KEY_R,KEY_T,KEY_Y,KEY_U,KEY_I,KEY_O,KEY_P,KEY_LEFTBRACE,KEY_RIGHTBRACE,KEY_BACKSLASH},
	{0,KEY_CAPSLOCK,KEY_Q,KEY_S,KEY_D,KEY_F,KEY_G,KEY_H,KEY_J,KEY_K,KEY_L,KEY_SEMICOLON,KEY_APOSTROPHE,KEY_ENTER,0},
	{0,KEY_LEFTSHIFT,KEY_Z,KEY_X,KEY_C,KEY_V,KEY_B,KEY_N,KEY_M,KEY_COMMA,KEY_DOT,KEY_SLASH,KEY_RIGHTSHIFT},
/*	{0,KEY_fn,KEY_ctl,KEY_opt,KEY_com,KEY_space,KEY_rcom,KEY_ropt,KEY_X,KEY_x2,KEY_x3,KEY_x4} */
};


typedef struct key_summary {
	char *kbstr;
	char hidval;
	int modifier; // modifies other keys
	int count; // number of times key press has been scanned
	int slot; // location in HID REPORT - 0 to 5
	char up; // was pressed last scan, not this one. May not be needed
} key_summary;

key_summary keys[NROWS][NCOLS];
char hid_report[8]; // HID report sent to host

void clear_report()
{
int i;
    for (i = 0; i < 8; i++) hid_report[i] = 0;
}
void init_key_summary()
{
int i,j;

	for (i = 0; i < NROWS; i++) {
		for (j = 0; j < NCOLS; j++) {
			keys[i][j].count = 0;
			keys[i][j].kbstr = kbstr [i][i];
			keys[i][j].up = FALSE;
			keys[i][j].modifier = 0;
		}
	}
	// define modifier keys
	keys[4][1].hidval = KEY_MOD_LSHIFT; // 0x02
}

char *get_kbstr(int row,int col)
{
	char *s;
	if (row >=0 && row <= NROWS && col >= 0 && col < NCOLS) {
		s = kbstr[row ][col];
		if (!s) s = "not defined";
	}
	else s = "error";
	return(s);
}

char get_hidval(int row,int col)
{
	char hid;
	if (row >=0 && row <= NROWS && col >= 0 && col < NCOLS) {
		hid = hidval[row ][col];

	}
	else hid = 0;
	return(hid);
}

int send_hid_report()
{
	int i;

    int fd = open("/dev/hidg0", O_RDWR);
   printf("sending report : ");
	for (i = 0; i < 8; i++) printf("%02x ",hid_report[i]);
	printf("\n");
	if (!fd) {
		printf(" could not open fd\n");
		return(0);
	}
	write(fd,&hid_report[0],8);

	close (fd);
	return(TRUE);
}
int send_error_report()
{ // TODO
	printf("ERROR!!!!!!!\n\n\n\n");
	clear_report();
	send_hid_report();
	return(0);
}

int construct_hid_report()
{
	int i,j,k,error = 0;
	int next_slot = 0;
	int change = FALSE;
	for (i = 0; i < MAX_SLOTS; i++) {
		if (hid_report[i + 2] == 0)
			next_slot = i;
	}
	if (next_slot == -1) { // no free slots - too many meys pressed. Send error report
		printf("too many keys pressed\n");
		send_error_report();
		return(TRUE);
	}
	for (i = 0; i < NROWS; i++) {
		for (j = 0; j < NCOLS; j++) {
			if (keys[i][j].modifier > 0 && keys[i][j].count >= MIN_KEY_SCAN) {
				hid_report[0] |= keys[i][j].modifier;
			}
			if (keys[i][j].count == MIN_KEY_SCAN) {
				keys[i][j].slot = next_slot;
				for (k = next_slot + 1; k < MAX_SLOTS; k++) {
					if (hid_report[k + 2] == 0) {
						next_slot = k;
					}
				}
				if (next_slot < MAX_SLOTS) {
					hid_report[next_slot + 2] = keys[i][j].hidval;
				}
				else {
					printf("not free slots - too many keys pressed\n");
					send_error_report();
					return(TRUE);
				}
				change = TRUE;
			}
			else if (keys[i][j].up == TRUE) {
				hid_report[keys[i][j].slot + 2] = 0;
				keys[i][j].slot = -1;
				keys[i][j].up = FALSE;
				change = TRUE;
			}
		}
	}
	if (change) send_hid_report();
	return(error);
}

void show_pins()
{
int i,pinno;

	for (i = 0; i < 26; i++) {
		pinno = wpiPinToGpio(i);
		printf("pin %d => %d\n",i,pinno);
	}
}

int main (void)
{
int i;
int pinno;
char c;
int debug = 0;

	wiringPiSetup () ;
	for (i = 0; i < 26; i++) {
			pinno = wpiPinToGpio(i);
			printf("pin %d => %d\n",i,pinno);
	}
	pinMode (PIN_SER, OUTPUT) ;
	pinMode (PIN_RCLOCK, OUTPUT) ;
	pinMode (PIN_SRCLK, OUTPUT) ;
	for (i = 0; i < nscan_pins; i++) {
		pinMode (scan_pins[i], INPUT) ;
		pullUpDnControl(scan_pins[i],PUD_DOWN);
	}

	for (;;)
	{
		delay(1);
		if (debug) {
			c = getc(stdin);
	//		printf("got %d\n",c);
			switch (c) {
				case 'D': digitalWrite(PIN_SER,1); break;
				case 'd': digitalWrite(PIN_SER,0); break;
				case 'C': digitalWrite(PIN_RCLOCK,1); break;
				case 'c': digitalWrite(PIN_RCLOCK,0); break;
				case 'S': digitalWrite(PIN_SRCLK,1); break;
				case 's': digitalWrite(PIN_SRCLK,0); break;
				case 'A': set_all(1); break;
				case 'a': set_all(0); break;
				case 10: break; // return char
				case 'q': scan(); break;
				case 'w': scan_inputs(0); break;
				case 'z': show_pins(); break;
				case 'r': printf("running kb scan\n"); debug = 0; break;
				default :
					if (c >= '1' && c <= '9') set_one(c - '1');
					else printf("unknown command chars allows DdCcSs\n");
			}
		}
		else scan();
		// delay(10);
		// digitalWrite (0, HIGH) ; delay (500) ;
	}
	return 0 ;
}

/*
 * set_all - sets all the outputs of the shift registed high or low by clocking in val.
 */
void set_one(int pin)
{
int i;

	printf("setting pin %d\n",pin);
	set_all(0);
	digitalWrite(PIN_RCLOCK, LOW);
	digitalWrite(PIN_SRCLK, LOW);
	delay(1); // not sure this is needed but little harm
	digitalWrite(PIN_SER, HIGH);
	delay(1); // not sure this is needed but little harm
	digitalWrite(PIN_RCLOCK, HIGH);
	digitalWrite(PIN_SRCLK, HIGH);
	delay(1); // not sure this is needed but little harm
	digitalWrite(PIN_SRCLK, LOW);
	digitalWrite(PIN_RCLOCK, LOW);
	delay(1); // not sure this is needed but little harm
	digitalWrite(PIN_SER, LOW);
	for (i = 0; i < pin ;i ++) {
		digitalWrite(PIN_RCLOCK, LOW);
		digitalWrite(PIN_SRCLK, LOW);
		delay(1); // not sure this is needed but little harm
		digitalWrite(PIN_RCLOCK, HIGH);
		digitalWrite(PIN_SRCLK, HIGH);
		delay(1); // not sure this is needed but little harm
	}
	digitalWrite(PIN_RCLOCK, LOW);
}
void set_all(int val)
{
int i;
	digitalWrite(PIN_SER, val ? HIGH: LOW);
	digitalWrite(PIN_RCLOCK, HIGH);
	digitalWrite(PIN_SRCLK, HIGH);
	delay(1); // not sure this is needed but little harm
	for (i = 0; i < 16;i ++) {
		digitalWrite(PIN_RCLOCK, LOW);
		digitalWrite(PIN_SRCLK, LOW);
		delay(1); // not sure this is needed but little harm
		digitalWrite(PIN_RCLOCK, HIGH);
		digitalWrite(PIN_SRCLK, HIGH);
		delay(1); // not sure this is needed but little harm
	}
}

int scan ()
{
/*
rpi seems to need delays between setting pins - might be because the pi does no gaurantee gpio operation order ????
*/
	// need to clear??
	set_all(0);
	int i,count = 0;;
	digitalWrite(PIN_RCLOCK, LOW);
	digitalWrite(PIN_SRCLK, LOW);
	delay(1); // not sure this is needed but little harm
	digitalWrite(PIN_SER,	HIGH);
	delay(1); // not sure this is needed but little harm
	digitalWrite(PIN_RCLOCK, HIGH);
	delay(1); // not sure this is needed but little harm
	digitalWrite(PIN_SRCLK, HIGH);
	delay(1); // not sure this is needed but little harm
	digitalWrite(PIN_RCLOCK, LOW);
	digitalWrite(PIN_SRCLK, LOW);
	delay(1); // not sure this is needed but little harm
	digitalWrite(PIN_SER,	LOW);
	delay(1); // not sure this is needed but little harm
	for (i = 0; i < 16;i ++) {
		 count += scan_inputs(i);
/*
		 digitalWrite(PIN_RCLOCK, HIGH);
		delay(1); // not sure this is needed but little harm
		digitalWrite(PIN_SRCLK, HIGH);
		delay(1); // not sure this is needed but little harm
		digitalWrite(PIN_RCLOCK, LOW);
		delay(1); // not sure this is needed but little harm
		digitalWrite(PIN_SRCLK, LOW);
		delay(1); // not sure this is needed but little harm
*/
		 digitalWrite(PIN_RCLOCK, HIGH);
		digitalWrite(PIN_RCLOCK, LOW);
		delay(1); // not sure this is needed but little harm
		digitalWrite(PIN_SRCLK, HIGH);
		digitalWrite(PIN_SRCLK, LOW);
	}
	construct_hid_report();
	return(count);
}

int scan_inputs(int col)
{
int i,count = 0;
	for (i = 0; i < nscan_pins; i++) {
		if (digitalRead(scan_pins[i])) {
			keys[i][col].kbstr = get_kbstr(i,col);
			if (keys[i][col].count == 1) {
				printf("%s (%d,%d)\n",keys[i][col].kbstr,col,i);
			}
			keys[i][col].hidval = get_hidval(i,col);; // shouldn't be needed - something wrong in init
			keys[i][col].count++;
			keys[i][col].up = FALSE;
		}
		else {
			if (keys[i][col].count) {
				printf ("%s up (%d,%d, count %d)\n",keys[i][col].kbstr,col,i,keys[i][col].count);
				keys[i][col].count = 0;
				keys[i][col].up = TRUE;
			}
		}
	}
	return(count);
}
