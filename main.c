#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mqtt.h"
#include "parse_config.h"

#define ILL_OVER_THROUGH      30001
#define ILL_UNDER_THROUGH     30000

#define CTRL_PUB_TOPIC "1662600472837/ScratchToSoftWare"
#define SUB_TOPIC "1662600472837/SoftWareToScratch"


#define LAMP_ON    "{\"lamp\":true}"
#define LAMP_OFF   "{\"lamp\":false}"
#define ALARM_ON    "{\"alarm\":true}"
#define ALARM_OFF   "{\"alarm\":false}"

#define LIGHT   "{\"light\":%f}"

int main(int argc, char *argv[])
{
	int ret;
	char data[1024] = {0};
	float light = 0;
	 if (0 != mqtt_init())
    {
        puts("Fail to instantiate mqtt");
        return -1;
    }

    if(mqtt_subscribe(SUB_TOPIC) < 0)
    {
        printf("Topic subscribe failed\n");
        return -1;
    }

	while(1)
	{
		sleep(3);
		light = get_virtual_env();
		if(light >= ILL_OVER_THROUGH)
		{
			mqtt_publish(CTRL_PUB_TOPIC, LAMP_OFF);
			mqtt_publish(CTRL_PUB_TOPIC, ALARM_ON);
			printf("Phrase 1\n");
		}
		else if(light <= ILL_UNDER_THROUGH)
		{
			mqtt_publish(CTRL_PUB_TOPIC, LAMP_ON);
			mqtt_publish(CTRL_PUB_TOPIC, ALARM_OFF);
			printf("Phrase 2\n");
		}
		else
		{	
		}
		printf("Light condition:%f \n", light);
	}

	exit_mqtt();

	return 0;
}
