#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mqtt.h"
#include "parse_config.h"

#define ILL_BOUND 1000
#define CO2_BOUND 50
#define PML_BOUND 50


#define CTRL_PUB_TOPIC "1663401252952/APP2AIOTSIM"
#define SUB_TOPIC "1663401252952/AIOTSIM2APP"


#define LAMP_ON    "{\"lamp\":true}"
#define LAMP_OFF   "{\"lamp\":false}"
#define ALARM_ON    "{\"alarm\":true}"
#define ALARM_OFF   "{\"alarm\":false}"
#define SUNSHADE_ON    "{\"sunshade\": \"forward\"}"
#define SUNSHADE_IDLE  "{\"sunshade\": \"stop\"}"
#define SUNSHADE_OFF   "{\"sunshade\": \"reverse\"}"
#define FAN_ON    "{\"fan\":true}"
#define FAN_OFF   "{\"fan\":false}"
#define DOOR_ON   "{\"doorLock\":true}"
#define DOOR_OFF  "{\"doorLock\":false}"
#define IRRI_ON   "{\"irrigation\":true}"
#define IRRI_OFF  "{\"irrigation\":false}"
#define VCMD1     "{\"speech\":\"Opening sunshade.\"}"
#define VCMD2     "{\"speech\":\"Closing sunshade.\"}"



const char KEY[] = "0908900800";
const char FACEGROUP[] = "owner";
const char CMD1[] = "open";
const char CMD2[] = "close";
const char CMD3[] = "sunshade";


void emergency_exit(int infrd){
	if (infrd){
		mqtt_publish(CTRL_PUB_TOPIC, DOOR_ON);
		puts("Door is unlocked for escape!");
	}
}

void report(int status, Exchange data){
	if (status&&(!(status&1))){
		puts("\n!!Attention!!\n! WARNING: Certain value critical, see below:\n");
		if (status & 2)
			puts("FLAMMABLE GAS DETECTED!\n");
		if (status & 4)
			puts("FLAME DETECTED!\n");
		if (status & 8)
			puts("SMOKE DETECTED!\n");
		if (status & 16){
			puts("CO2 LEVEL Critical!");
			printf("CO2: %f\n\n", data.co2);	
		}
		if (status & 32){
			puts("PM2.5 LEVEL Critical!");
			printf("PM2.5: %f\n\n", data.pm25);	
		}
	}else{
		puts("Report: All data within normal condition");
		printf("Light: %f \t co2: %f \t pm2.5: %f \n", data.light, data.co2, data.pm25);
	}
}

int main(int argc, char *argv[])
{
	Exchange data;
	int status  = 0;

	 if (0 != mqtt_init()){
        puts("Fail to instantiate mqtt");
        return -1;
    }

    if(mqtt_subscribe(SUB_TOPIC) < 0){
        printf("Topic subscribe failed\n");
        return -1;
    }

	while(1){
		puts("\n");
		sleep(3);
		data = get_virtual_env();
		// 2^0 - light
		if(data.light > ILL_BOUND){
			if(!(status&1)){
				mqtt_publish(CTRL_PUB_TOPIC, SUNSHADE_ON);
				sleep(2);
				mqtt_publish(CTRL_PUB_TOPIC, SUNSHADE_IDLE);
				puts("Sunshade is lowered due to high light intensity");
				status |= 1;
			}
		}else{
			if(status&1){
				status ^= 1;
				mqtt_publish(CTRL_PUB_TOPIC, SUNSHADE_OFF);
				sleep(2);
				mqtt_publish(CTRL_PUB_TOPIC, SUNSHADE_IDLE);
				puts("Light condition normal, sunshade off.");
			}
		}
		// 2^1 - flamGas
		if (data.flamGas){
			if(!(status&2)){
				mqtt_publish(CTRL_PUB_TOPIC, ALARM_ON);
				emergency_exit(data.infrared);
				status |= 2;
			}
		}else{
			if(status&2){
				status ^= 2;
				mqtt_publish(CTRL_PUB_TOPIC, ALARM_OFF);
			}
		}
		// 2^2 - flame
		if (data.flame){
			if(!(status&4)){
				mqtt_publish(CTRL_PUB_TOPIC, ALARM_ON);
				mqtt_publish(CTRL_PUB_TOPIC, IRRI_ON);
				emergency_exit(data.infrared);
				status |= 4;
			}
		}else{
			if(status&4){
				status ^= 4;
				mqtt_publish(CTRL_PUB_TOPIC, ALARM_OFF);
				mqtt_publish(CTRL_PUB_TOPIC, IRRI_OFF);
			}
		}
		// 2^3 - smoke
		if (data.smoke){
			if(!(status&8)){
				mqtt_publish(CTRL_PUB_TOPIC, ALARM_ON);
				mqtt_publish(CTRL_PUB_TOPIC, IRRI_ON);
				emergency_exit(data.infrared);
				status |= 8;
			}
		}else{
			if(status&8){
				status ^= 8;
				mqtt_publish(CTRL_PUB_TOPIC, ALARM_OFF);
				mqtt_publish(CTRL_PUB_TOPIC, IRRI_OFF);
			}
		}
		// 2^4 - co2
		if(data.co2 > CO2_BOUND){
			if(!(status&16)){
				mqtt_publish(CTRL_PUB_TOPIC, FAN_ON);
				mqtt_publish(CTRL_PUB_TOPIC, ALARM_ON);
				emergency_exit(data.infrared);
				status |= 16;
			}
		}else{
			if(status&16){
				status ^= 16;
				mqtt_publish(CTRL_PUB_TOPIC, FAN_OFF);
				mqtt_publish(CTRL_PUB_TOPIC, ALARM_OFF);
			}
		}
		// 2^5 - pm2.5
		if (data.pm25 > PML_BOUND){
			if(!(status&32)){
				mqtt_publish(CTRL_PUB_TOPIC, FAN_ON);
				mqtt_publish(CTRL_PUB_TOPIC, ALARM_ON);
				emergency_exit(data.infrared);
				status |= 32;
			}
		}else {
			if(status&32){
				status ^= 32;
				mqtt_publish(CTRL_PUB_TOPIC, FAN_OFF);
				mqtt_publish(CTRL_PUB_TOPIC, ALARM_OFF);
			}
		}
		
		if (data.RFID){
			if (!strcmp(data.RFID, KEY)){
				mqtt_publish(CTRL_PUB_TOPIC, DOOR_OFF);
				puts("Key matched, door unlocked for 3 sec.");
				sleep(3);
				mqtt_publish(CTRL_PUB_TOPIC, DOOR_ON);
				puts("Door is re-locked");
			}else{
				puts("Key not matched.");
			}
		}
		
		if (data.FaceID){
			if(!strcmp(data.FaceID, FACEGROUP)){
				mqtt_publish(CTRL_PUB_TOPIC, DOOR_OFF);
				puts("Key matched, door unlocked for 3 sec.");
				sleep(3);
				mqtt_publish(CTRL_PUB_TOPIC, DOOR_ON);
				puts("Door is re-locked");
			}		
		}

		if (data.Voice){
			int l = strlen(data.Voice);
			int flag = 0;
			if(l){
				// target priority;
				for(int i = 0; i<l-7; i++){
					flag = 1;
					for(int j = 0; j<8; j++)
						if(data.Voice[i+j] != CMD3[j]){
							flag = 0;
							break;
						}
				}
				if(flag){
					for(int i = 0; i<l-3; i++){
						flag = 1;
						for(int j = 0; j<4; j++)
							if(data.Voice[i+j] != CMD1[j]){
								flag = 0;
								break;
							}
						}
					if(!flag){
						for(int i = 0; i<l-4; i++){
							flag = 2;
							for(int j = 0; j<5; j++)
								if(data.Voice[i+j] != CMD2[j]){
									flag = 0;
									break;
								}
						}
					}
					if(flag==1){
						mqtt_publish(CTRL_PUB_TOPIC, SUNSHADE_ON);
						puts("Voice command reveiced. Sunshade is now opened.");
						sleep(2);
						mqtt_publish(CTRL_PUB_TOPIC, VCMD1);
					}else if(flag==2){
						mqtt_publish(CTRL_PUB_TOPIC, SUNSHADE_ON);
						puts("Voice command reveiced. Sunshade is now closed.");
						sleep(2);
						mqtt_publish(CTRL_PUB_TOPIC, VCMD2);
						
					}
				}
			}
		}
		report(status, data);
	}

	exit_mqtt();

	return 0;
}
