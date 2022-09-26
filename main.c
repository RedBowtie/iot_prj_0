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
#define SUNSHADE_OFF   "{\"sunshade\": \"reverse\"}"
#define FAN_ON    "{\"fan\":true}"
#define FAN_OFF   "{\"fan\":false}"
#define DOOR_ON   "{\"doorLock\":true}"
#define DOOR_OFF  "{\"doorLock\":false}"
#define IRRI_ON   "{\"irrigation\":true}"
#define IRRI_OFF  "{\"irrigation\":false}"

#define LIGHT   "{\"light\":%f}"



const char KEY[] = "0908900800";
const char FACEGROUP[] = "owner";
const char CMD1[] = "open";
const char CMD2[] = "close";
const char CMD3[] = "sunshade";


void emergency_exit(int infrd){
	if (infrd){
		mqtt_publish(CTRL_PUB_TOPIC, DOOR_ON);
	}
}

void report(int status, Exchange data){
	if (status){
		puts("WARNING: Certain value critical, see below:");
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

	while(1)
	{
		puts("");
		sleep(3);
		data = get_virtual_env();
		
		if(data.light > ILL_BOUND){
			mqtt_publish(CTRL_PUB_TOPIC, SUNSHADE_ON);
			printf("Too much light, lower sunshade.\n");
		}else{
			mqtt_publish(CTRL_PUB_TOPIC, SUNSHADE_OFF);
			printf("Light condition normal, sunshade off.\n");
		}
		
		if (data.flamGas){
			mqtt_publish(CTRL_PUB_TOPIC, ALARM_ON);
			// printf("Flammable gas detected, turn on alarm.\n");
			emergency_exit(data.infrared);
			status |= 2;
		}else{
			if(status&2)
				status ^= 2;
		}

		if (data.flame){
			mqtt_publish(CTRL_PUB_TOPIC, ALARM_ON);
			mqtt_publish(CTRL_PUB_TOPIC, IRRI_ON);
			// printf("Flame detected, turn on alarm.\n");
			emergency_exit(data.infrared);
			status |= 4;
		}else{
			if(status&4)
				status ^= 4;
		}

		if (data.smoke){
			mqtt_publish(CTRL_PUB_TOPIC, ALARM_ON);
			mqtt_publish(CTRL_PUB_TOPIC, IRRI_ON);
			// printf("Smoke detected, turn on alarm.\n");
			emergency_exit(data.infrared);
			status |= 8;
		}else{
			if(status&8)
				status ^= 8;
		}

		if(data.co2 > CO2_BOUND){
			mqtt_publish(CTRL_PUB_TOPIC, FAN_ON);
			mqtt_publish(CTRL_PUB_TOPIC, ALARM_ON);
			// printf("CO2 level high, turn on fans and alarms.\n");
			emergency_exit(data.infrared);
			status |= 16;
		}else{
			if(status&16)
				status ^= 16;
		}

		if (data.pm25 > PML_BOUND){
			mqtt_publish(CTRL_PUB_TOPIC, FAN_ON);
			mqtt_publish(CTRL_PUB_TOPIC, ALARM_ON);
			// printf("PM2.5 high, turn on fans and alarms.\n");
			emergency_exit(data.infrared);
			status |= 32;
		}else {
			if(status&32)
				status ^= 32;
		}
		
		if (data.RFID){
			if (!strcmp(data.RFID, KEY)){
				mqtt_publish(CTRL_PUB_TOPIC, DOOR_OFF);
				printf("Key matched, door unlocked.\n");
			}else{
				printf("Key not matched.\n");
			}
		}
		
		if (data.FaceID){
			if(!strcmp(data.FaceID, FACEGROUP)){
				mqtt_publish(CTRL_PUB_TOPIC, DOOR_OFF);
				printf("Face matched, door unlocked.\n");
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
					}else if(flag==2){
						mqtt_publish(CTRL_PUB_TOPIC, SUNSHADE_OFF);
					}
				}
			}
		}
		report(status, data);
	}

	exit_mqtt();

	return 0;
}
