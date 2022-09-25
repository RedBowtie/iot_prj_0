#ifndef __MQTT_SMART_H_
#define __MQTT_SMART_H_

int mqtt_init();
void exit_mqtt();
int mqtt_subscribe(const char*topic);
int mqtt_publish(const char *topic, char *msg);
float get_virtual_env();

#endif
