/**
		aaaaaaa        P1_4 右侧公共正极 P1_5 左侧公共正极
		bb   dd        P1_1 Rd    P1_2 Ra    P1_3 LRb
		bb   dd        P1_6 Ld    P1_7 La    P0_1 Rg
		ccccccc        P0_2 LRc   P0_3 Rf    P0_4 Re
		ee   gg        P0_6 Lg    P0_7 Lf    P4_6 Le
		ee   gg
		fffffff
0x0abcdefg
  **/
#include <stc12.h>
#define INIT 0
#define IDLE 1
#define SETTIMER 2
#define RUNNING 3
#define PAUSE 4
//secondT 20times=1seconds 1200times=1minutes
#define secondTH 0x3C
#define secondTL 0xB0

unsigned char display_ram=0;
unsigned char display_ram_L = 0;
unsigned char display_ram_R = 0;
unsigned char Timer1Mod=0;
__bit display_enable=1;
__bit isButtonDown=0;
unsigned char ACT=INIT; //current display state
unsigned char config_set_minutes;
unsigned char var_set_minutes;
unsigned char running_minutes;
unsigned int timer_t0_counter=0;
__code unsigned char initAnimation[]={
	/*
	//a,d,g,f,e
	0x40,0x08,0x01,0x02,0x04,
	//b,eb,ebc,cdg,cdgaf
	0x20,0x24,0x34,0x19,0x5B,
	//cdg,c,bce,abcef,ff
	0x19,0x10,0x34,0x76,0x7f,
	//00,ff,00
	0x00,0x7f,0x00,*/
	//a,bd,c,eg,f
	0x40,0x28,0x10,0x05,0x02,
	//eg,c,bd,a,abd
	0x05,0x10,0x28,0x40,0x68,
	//abdc,abdceg,abdcegf,0,ff,0
	0x78,0x7D,0x7F,0x00,0x7f,0x7f,0x7f,0x00
};
unsigned char animationIndex=0;

__code unsigned char display_data[]={
	//0,1,2,3,4
	0x6f,0x09,0x5E,0x5B,0x39,
	//5,6,7,8,9
	0x73,0x77,0x49,0x7F,0x7B
};

void DisplayNum(unsigned char a){
	if(a<10){
		display_ram_L=display_data[0];
		display_ram_R=display_data[a];
	}else{
		display_ram_L = display_data[a/10];
		display_ram_R = display_data[a%10];
	}
}

void Timer1(void) __interrupt 3
{
	//Timer1 Show Animation
	unsigned char index;
	animationIndex++;
	if(ACT==INIT){//Timer1Mod==0){
		//Initial Animation
		if(animationIndex%5==0){
			index=animationIndex/5-1;
			if(index<sizeof(initAnimation)){
				display_ram_R = display_ram_L = initAnimation[index];
			}else{
				//Timer1Mod=1;
				//TR1=0;//just stop timer1
				ACT=IDLE;
			}

		}
	}
	if(ACT==IDLE)return; // empty animation
	if(Timer1Mod==2){
		//blink show
		if(animationIndex%2==0){
			//reverse display
			display_enable=!display_enable;
		}
	}
}
void Timer0(void) __interrupt 1{
	timer_t0_counter ++;
	if(timer_t0_counter==1200){
		//1 minutes
		running_minutes--;
		timer_t0_counter=0;
	}
	if(running_minutes==0){
		TR0=0;
	}
	TH0=secondTH;TL0=secondTL;//reload secondTimer;
}
void parseButton(){
	if(!isButtonDown){
		if(!P3_3){
			//button down ,start timer;
			isButtonDown=1;
			animationIndex=0;
		}
		return;
	}
	//already down
	if(P3_3){
		//release button;
		//counting timer;
		if(animationIndex<8){
			//short click
			if(ACT==IDLE){
				ACT=RUNNING;
				running_minutes=config_set_minutes;
				TR0=1;//start counting timer;
				// connect relay
				P4_4=0;
			}else if(ACT==SETTIMER){
				//add ram timer
				var_set_minutes++;
				if(var_set_minutes>99)var_set_minutes=0;
				animationIndex=0;//restart return timer;
			}else if(ACT==RUNNING){
				//pause and resume
				ACT=PAUSE;
				TR0=0;//pause counting timer;
				//disconnect relay
				P4_4=1;
			}else if(ACT==PAUSE){
				ACT=RUNNING;
				TR0=1;//restart counting timer;
				//connect relay
				P4_4=0;
			}
		}else{
			//long click
			if(ACT==IDLE){
				ACT=SETTIMER;
				animationIndex=0;//restart return timer;
				var_set_minutes=config_set_minutes; //set var timer
			}else if(ACT==SETTIMER){
				//save ram timer
				config_set_minutes=var_set_minutes;
				IAP_CMD=3;//earse eeprom
				IAP_ADDRH=IAP_ADDRL=0;
				IAP_TRIG=0x5A;IAP_TRIG=0xA5;
				IAP_CMD=2;//write eeprom
				IAP_DATA=var_set_minutes;
				IAP_TRIG=0x5A;IAP_TRIG=0xA5;

				ACT=IDLE;
			}else if(ACT==RUNNING){
				//stop
				ACT=IDLE;
			}else if(ACT==PAUSE){
				//stop
				ACT=IDLE;
			}
		}
		isButtonDown=0;
	}
}
void delayAndClear(){
	unsigned char i;
	for(i=255;i!=0;i--);
	P1|=0xCF;//除了P1.4/P1.5其余复位
	P0=0xFF;
	P4_6=1;
}
void display(){
	unsigned char ram=display_ram;
	if(ram&0x80){
		//right
		P1_4=1;P1_5=0;
	}else{
		//left
		P1_4=0;P1_5=1;
	}
	ram<<=1;
	if(ram&0x80){
		//a
		P1_2=0;P1_7=0;
	}
	ram<<=1;
	delayAndClear();

	if(ram&0x80){
		//b
		P1_3=0;
	}
	ram<<=1;
	delayAndClear();

	if(ram&0x80){
		//c
		P0_2=0;
	}
	ram<<=1;
	delayAndClear();


	if(ram&0x80){
		//d
		P1_1=0;P1_6=0;
	}
	ram<<=1;
	delayAndClear();


	if(ram&0x80){
		//e
		P0_4=0;P4_6=0;
	}
	ram<<=1;
	delayAndClear();

	if(ram&0x80){
		//f
		P0_3=0;P0_7=0;
	}
	ram<<=1;
	delayAndClear();

	if(ram&0x80){
		//g
		P0_6=0;P0_1=0;
	}
	ram<<=1;
	delayAndClear();
}
void delay1s(void){
	unsigned char i,j,k;
	for(i=255;i!=0;i--)
		for(j=255;j!=0;j--)
			for(k=30;k!=0;k--);
}
void DisplayAct(){
	if(ACT==INIT)return;
	if(ACT==IDLE)DisplayNum(config_set_minutes);
	if(ACT==RUNNING){
		DisplayNum(running_minutes);
		if(animationIndex%15==0){//blink
			display_ram_L=0;
			display_ram_R=0;
		}
		if(running_minutes==0){//over
			ACT=IDLE;
			//disconnect relay
			P4_4=1;
		}
	}
	if(ACT==PAUSE){
		display_ram_L= 0x7C;//P
		display_ram_R= 0x7D;//A
	}
	if(ACT==SETTIMER){
		DisplayNum(var_set_minutes);
		if(animationIndex%2==0){//blink
			display_ram_L=0;
			display_ram_R=0;
		}
		if(animationIndex==255)ACT=IDLE;//reset idle drop modify
	}
}
void main(void){
	P1M1=0;
	P1M0=0x30; //0b00110000;
	P4SW|=0x50;//0b01010000;P4.6/P4.4 as IO
	display_ram=0x7f; //00000100;
	//init animation
	TMOD = 0x10; //T1 16bit timer
	ET1 = 1;
	EA = 1;
	TH1=TL1=0;
	TR1=1;
	//end init animation
	//init minute timer
	TMOD |= 1; //T0 16bit timer
	ET0 = 1;
	TH0 = secondTH;
	TL0 = secondTL;
	TR0 = 0;
	//end minute timer
	//init int1 button 
	//TCON |= 0x04; //downedge interrupt
	//EX1=1; //Int1 interrupt enabled
	//end init int1 button
	//load initial data from eeprom
	IAP_CONTR=0x83; //enable eeprom
	IAP_CMD=1; //cmd read
	IAP_ADDRH=IAP_ADDRL=0; //address 0
	IAP_TRIG=0x5A;
	IAP_TRIG=0xA5;
	config_set_minutes=IAP_DATA;

	while(1){
		parseButton();
		DisplayAct();
		display_ram = display_ram_L;
		display();
		display_ram = display_ram_R;
		display_ram |= 0x80;
		display();
	}
}
