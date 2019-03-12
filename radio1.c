#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "lcd.h"

#define buttonPinR 3
#define buttonPinB 12
#define purpSpin 1
#define purpApin 15
#define purpBpin 16
#define MAX 100



#define lcdADDR 0x27
#define TopLine 0x80
#define BottomLine 0xC0
#define ENABLE 0b00000100
#define LCD_CHR  1 
#define LCD_CMD  0 
#define lcdBacklight 0x08

static volatile double globalFreq;
int fd;
void tune_radio(char f[5]){
    unsigned char radio[5] = {0};
    //int fd;
    int dID = 0x60;                         // i2c Channel the device is on
    unsigned char frequencyH = 0;
    unsigned char frequencyL = 0;
    unsigned int frequencyB;
    printf("Playing Radio Station character %s",f);
    double frequency = strtod(f,NULL);

    printf("Playing Radio Station %f",frequency);


    frequencyB=4*(frequency*1000000+225000)/32768; //calculating PLL word
    frequencyH=frequencyB>>8;
    frequencyL=frequencyB&0XFF;


    radio[0]=frequencyH; //FREQUENCY H
    radio[1]=frequencyL; //FREQUENCY L
    radio[2]=0xB0; //3 byte (0xB0): high side LO injection is on,. 
    radio[3]=0x10; //4 byte (0x10) : Xtal is 32.768 kHz 
    radio[4]=0x00; //5 byte (0x00)

    if((fd=wiringPiI2CSetup(dID))<0){
      printf("error opening i2c channel\n\r");  //check access to board
    }

    write (fd, (unsigned int)radio, 5) ;
    close (fd);
}



void mute_radio(void){

    unsigned char radio[5] = {0};
  //  int fd;
    int dID = 0x60;                 // i2c Channel the device is on
    double frequency = 0;
    unsigned char frequencyH = 0;
    unsigned char frequencyL = 0;
    unsigned int frequencyB;

    if((fd=wiringPiI2CSetup(dID))<0){
        printf("error opening i2c channel\n\r");   //check access to board
    }
    read(fd,radio,5);

  
    frequency=((((radio[0]&0x3F)<<8)+radio[1])*32768/4-225000)/10000;
    frequency = round(frequency * 10.0)/1000.0;
    frequency = round(frequency * 10.0)/10.0;
  


    frequencyB=4*(frequency*1000000+225000)/32768; //calculating PLL word
    frequencyH=frequencyB>>8;
    frequencyH=frequencyH|0x80;   // mutes the radio
    frequencyL=frequencyB&0XFF;


    radio[0]=frequencyH; //FREQUENCY H
    radio[1]=frequencyL; //FREQUENCY L
    radio[2]=0xB0; //3 byte (0xB0): high side LO injection is on,. 
    radio[3]=0x10; //4 byte (0x10) : Xtal is 32.768 kHz 
    radio[4]=0x00; //5 byte (0x00)

    write (fd, (unsigned int)radio, 5) ;

    close(fd);
}

void unmute_radio(void){ 
    unsigned char radio[5] = {0};
    //int fd;
    int dID = 0x60; // i2c Channel the device is on
    double frequency = 0;
    unsigned char frequencyH = 0;
    unsigned char frequencyL = 0;
    unsigned int frequencyB;

    //open access to the board, send error msg if fails
    if((fd=wiringPiI2CSetup(dID))<0){
        printf("error opening i2c channel\n\r");
    }
    read(fd,radio,5);

    frequency=((((radio[0]&0x3F)<<8)+radio[1])*32768/4-225000)/10000;
    frequency = round(frequency * 10.0)/1000.0;
    frequency = round(frequency * 10.0)/10.0;

    frequencyB=4*(frequency*1000000+225000)/32768; //calculating PLL word
    frequencyH=frequencyB>>8;
    frequencyL=frequencyB&0XFF;

    radio[0]=frequencyH; //FREQUENCY H
    radio[1]=frequencyL; //FREQUENCY L
    radio[2]=0xB0; //3 byte (0xB0): high side LO injection is on,. 
    radio[3]=0x10; //4 byte (0x10) : Xtal is 32.768 kHz 
    radio[4]=0x00; //5 byte0x00)

    write (fd, (unsigned int)radio, 5) ;

  close(fd);
}



void lcd_clear(void){
    lcd_byte(0x01, LCD_CMD);
    lcd_byte(0x02, LCD_CMD);
}

void lcd_location(int line){
    lcd_byte(line, LCD_CMD);
}

void lcd_string(const char s[]){
    while(*s) lcd_byte( *(s++), LCD_CHR);
}

void lcd_toggle_en(int b){
    delay(200);
    wiringPiI2CReadReg8(fd, (b | ENABLE));
    delay(200);
    wiringPiI2CReadReg8(fd, (b & ~ENABLE));
    delay(200);
}

void lcd_byte(int b, int m){
    int highBit;
    int lowBit;

    highBit = m|(b & 0xF0)|(0x08);
    lowBit = m|((b<<4) & 0xF0)|(0x08);

    wiringPiI2CReadReg8(fd, highBit);
    lcd_toggle_en(highBit);

  
    wiringPiI2CReadReg8(fd, lowBit);
    lcd_toggle_en(lowBit);
}

void lcd_init(void){
  lcd_byte(0x33, LCD_CMD); 
  lcd_byte(0x32, LCD_CMD); 
  lcd_byte(0x06, LCD_CMD); 
  lcd_byte(0x0C, LCD_CMD); 
  lcd_byte(0x28, LCD_CMD); 
  lcd_byte(0x01, LCD_CMD); 
  delay(100);
}



//red button at wiringpi(3) blue button at wiringpi(2)

int main(void){


    double temp;
    char *pter;
    double frequency  = 0;

	int i = 0;
  
    lcd_init();
    lcd_clear();

	if(wiringPiSetup() == -1){
		printf("WiringPi Setup Failed\n");
		return 1;
	}
    fd = wiringPiI2CSetup(lcdADDR);
    lcd_init();
    lcd_clear();
    lcd_location(TopLine);
    lcd_string("Ready");

	pinMode(buttonPinR,INPUT);
	pinMode(buttonPinB,INPUT);
    pinMode(purpApin,INPUT);
    pinMode(purpBpin,INPUT);
    pinMode(purpSpin,INPUT);

    unsigned int clkstateIni,clkstate,dtstate,RePress;
    double temp2;
    int lcdcheck;


    char stations2[6][7];
    int si = 0;


    FILE *stations_file;


    char string[7];
    stations_file = fopen("stations.txt","rt");
    if(stations_file == NULL){
        printf("Could not open file");
    }



    
    
    while(    fgets(stations2[si],7,stations_file) != NULL){
    
        printf("si = %d\n", si);  
        printf("stations2[%d] string is %s",si,stations2[si]);
        si++;
        if(si >5){
            break;
        }
    }

    fclose(stations_file);
	printf("\nSoftware Desgined Radio by Team Radblock 2019\n");




	while (1){
	    int a;
  	    char f[4]; 

	    scanf("%d", &a);
    if(a == 0){ 
        break;
    }
    if(a == 1){
        scanf("%s", f);
        printf("\nTuning to %s frequency\n",f);
        tune_radio(f);

    }
    if(a == 2){
        printf("\nUnmuting Radio\n");
        unmute_radio();
    }
    if(a == 3){
        printf("\nMuting Radio\n");
        mute_radio();
    }

    if(a == 4){
        tune_radio(stations2[0]);
        i=0;
        
	    while(1){
            
	
	        if(digitalRead(buttonPinR) == HIGH){	

    	        printf("\nRed button is down\n");
      	      	if(digitalRead(buttonPinR) == 0){
                    i -= 1;
                    if(i < 0) {
                        i = 5; 
                    }
                    tune_radio(stations2[i]);
        			printf("\nRed button was released\n");
                    temp = strtod(stations2[i],NULL);
                    frequency += temp;
                    lcdcheck = 1;

        		}
	        } 

	        if(digitalRead(buttonPinB) == HIGH){
    		    printf("\nBlue button is down\n");
    		    if(digitalRead(buttonPinB) == 0){
                    i += 1;
                    if(i > 5) {
                        i = 0;
                    }
                    tune_radio(stations2[i]);
        			printf("\nBlue button was released\n");
                    temp = strtod(stations2[i],NULL);
                    frequency += temp;
                    lcdcheck = 1;
			    }
	        }


            if(lcdcheck == 1){
                delay(100);
                printf("Change Screen");
                fd = wiringPiI2CSetup(lcdADDR); 
                lcd_clear();
                lcd_location(TopLine);
                lcd_string(stations2[i]);
                lcdcheck = 0;
            }
            
	    }
    }
    if(a==5){
        tune_radio("104.3");
        globalFreq = 104.3;
        clkstateIni = digitalRead(purpApin);
	    while(1){
            RePress = digitalRead(purpSpin);
            temp2 = globalFreq;
	    	clkstate = digitalRead(purpApin);
	    	dtstate = digitalRead(purpBpin);
	    
	    	if(clkstate != clkstateIni){
	    		if(dtstate!=clkstate){
                    globalFreq += 0.1;
	    		}
	    		else{
                    globalFreq -= 0.1;
	     		}
	    	}

            if(globalFreq != temp2){
                sprintf(f,"%f",globalFreq);
                printf("%s\n",f);
                tune_radio(f);
            }

            if(RePress == 0){
                printf("\nDial is press down");
            }            
	    	clkstateIni = clkstate;
	    
	    	delay(10);
	    }
    }
}
printf("\nProgram end\n");
return 0;
}
