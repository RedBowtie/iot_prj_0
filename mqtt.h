#ifndef __MQTT_SMART_H_
#define __MQTT_SMART_H_

typedef struct Storage{
    float light;
    float co2;
    float pm25;
    int flamGas;
    int infrared;
    int smoke;
    int flame;
    char *RFID;
    char *Voice;
    char *FaceID;
}Exchange;

int mqtt_init();
void exit_mqtt();
int mqtt_subscribe(const char*topic);
int mqtt_publish(const char *topic, char *msg);
Exchange get_virtual_env();



#endif
