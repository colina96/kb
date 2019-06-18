#include <stdio.h>
#include <wiringPi.h>

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

char *kbval[NROWS][NCOLS] = {
	{ "XX","esc","f1","f2","f3","f4","f5","f6","f7","f8","f9","f10","f11","f12","f13"},
	{ "XX2","`","1","2","3","4","5","6","7","8","9","0","-","=","del"},
	{"XX3","tab","q","w","e","r","t","y","u","i","o","p","[","]","\\"},
	{"XX4""caps","a","s","d","f","g","h","j","k","l",";","'","return","na"},
	{"XX5""lshift","z","x","c","v","b","n","m",",",".","/","rshift"},
	{"XX6","fn","ctl","opt","com","space","rcom","ropt","X","x2","x3","x4"}
};


typedef struct key_summary {
	char *kbval;
	int count;
} key_summary;

key_summary keys[NROWS][NCOLS];

void init_key_summary()
{
int i,j;

	for (i = 0; i < NROWS; i++) {
		for (j = 0; j < NCOLS; j++) {
			keys[i][j].count = 0;
			keys[i][j].kbval = kbval [i][i];
		}
	}
}

char *get_kbval(int row,int col)
{
	char *s;
	if (row >=0 && row <= NROWS && col >= 0 && col < NCOLS) {
		s = kbval[row ][col];
		if (!s) s = "not defined";
	}
	else s = "error";
	return(s);
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
	return(count);
}

int scan_inputs(int col)
{
	char *kb_str;

	int i,count = 0;
	for (i = 0; i < nscan_pins; i++) {
		if (digitalRead(scan_pins[i])) {
			kb_str = get_kbval(i,col);
			if (keys[i][col].count == 1) {
				printf("%s (%d,%d)\n",kb_str,col,i);
			}
			keys[i][col].kbval = kb_str; // shouldn't be needed - something wrong in init
			keys[i][col].count++;
		}
		else {
			if (keys[i][col].count) {
				printf ("%s up (%d,%d, count %d)\n",keys[i][col].kbval,col,i,keys[i][col].count);
				keys[i][col].count = 0;
			}
		}
	}
	return(count);
}
