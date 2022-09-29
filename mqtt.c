#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTClient.h"
#include <pthread.h>
#include "mqtt.h"
#include "parse_config.h"
#include "cJSON.h"

#define ADDRESS "tcp://192.168.114.107:1883" 

#define CLIENTID_PUB "ExampleClientPub"
#define CLIENTID_SUB "ExampleClientSub"
#define QOS 1
#define TIMEOUT 10000L
//#define PUB_TOPIC "ScratchToSoftWare" 
#define MQTT_SUB_TOPIC "1663401252952/AIOTSIM2APP" 

#define FAN_OFF "{'fan':false}"
#define FAN_ON "{'fan':true}"

#define ILL "light"
#define CO2 "co2"
#define PML "PM2.5"
#define FLG "flamGas"
#define INF "infrared"
#define SMK "smog"
#define FLA "flame"
#define RF "RFID_125K"
#define VC "speech"
#define FC "faceId"

const char KEY[] = "0908900800";
const char FACEGROUP[] = "owner";


MQTTClient client;
int fan_state = 0;
volatile MQTTClient_deliveryToken deliveredtoken;

char virtual_data[1024] = {0};
char *pend;
volatile Exchange env_data;

int transfer_virtual_data()
{
	// printf("Status:%s\n", virtual_data);
	
	cJSON *root = NULL;
	cJSON *item = NULL;
	
	root = cJSON_Parse(virtual_data);
	if (!root)
	{
		printf("parse err\n");
		return -1;
	}

	item = cJSON_GetObjectItem(root, RF);
	if((item != NULL)){
		if (!strcmp(item->valuestring, KEY))
			env_data.RFID = 1;
		else 
			env_data.RFID = -1;
		return 0;
	}else
		env_data.RFID = 0;

	item = cJSON_GetObjectItem(root, FC);
	if((item != NULL)){
		if (!strcmp(item->valuestring, FACEGROUP))
			env_data.FaceID = 1;
		else
			env_data.FaceID = -1;
		return 0;
	}else
		env_data.FaceID = 0;

	item = cJSON_GetObjectItem(root, ILL);
	if(item != NULL){
		env_data.light = item->valuedouble;
		return 0;
	}

	item = cJSON_GetObjectItem(root, CO2);
	if(item != NULL){
		env_data.co2 = item->valuedouble;
		return 0;
	}

	item = cJSON_GetObjectItem(root, PML);
	if(item != NULL){
		env_data.pm25 = item->valuedouble;
		return 0;
	}

	item = cJSON_GetObjectItem(root, FLG);
	if(item != NULL){
		env_data.flamGas = item->valueint;
		return 0;
	}

	item = cJSON_GetObjectItem(root, INF);
	if(item != NULL){
		env_data.infrared = item->valueint;
		return 0;
	}

	item = cJSON_GetObjectItem(root, SMK);
	if(item != NULL){
		env_data.smoke = item->valueint;
		return 0;
	}

	item = cJSON_GetObjectItem(root, FLA);
	if(item != NULL){
		env_data.flame = item->valueint;
	}

	/*
	item = cJSON_GetObjectItem(root, VC);
	if((item != NULL)&&(!env_data.Voice)){
		env_data.Voice = item->valuestring;
		return 0;
	}else{
		env_data.Voice = NULL;
	}*/
}

Exchange get_virtual_env()
{
	return env_data;
}
void delivered(void *context, MQTTClient_deliveryToken dt)
{
	printf("Message with token value %d delivery confirmed\n", dt);
	deliveredtoken = dt;
}
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
	memset(virtual_data,0,sizeof(virtual_data)); // empty msg
	memcpy(virtual_data,message->payload,message->payloadlen); // store the subscribe msg
	transfer_virtual_data(); // interpret the msg, get it to main()
	MQTTClient_freeMessage(&message); // release resource
	MQTTClient_free(topicName);
	return 1;
}
void connlost(void *context, char *cause)
{
	printf("\nConnection lost\n");
	printf(" cause: %s\n", cause);
}

int mqtt_publish(const char *topic, char *msg)
{
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
	MQTTClient_deliveryToken token;
    pubmsg.payload = msg;
    pubmsg.payloadlen = (int)strlen(msg);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    return MQTTClient_publishMessage(client, topic, &pubmsg, &token);
}

void exit_mqtt()
{
	int rc;
	if ((rc = MQTTClient_disconnect(client, 10000)) != MQTTCLIENT_SUCCESS)
		printf("Failed to disconnect, return code %d\n", rc);

	MQTTClient_destroy(&client);
}
int mqtt_subscribe(const char *topic)
{
    return MQTTClient_subscribe(client, topic, QOS);
}

int mqtt_init()
{
	char uri[128] = {0};
	
    GetProfileString(DEF_CONF_FILE,"mqtt","uri",uri);	
	
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;
	
    //MQTTClient_create(&client, ADDRESS, CLIENTID_PUB, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTClient_create(&client, uri, CLIENTID_PUB, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        return -1;
    }
    printf("mqtt connect success\n");

    return 0;
}
