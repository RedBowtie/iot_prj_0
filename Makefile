CC = gcc
# args, -c to generate *.o files
# -g to generate debug info
# -m32 to generate 32-bit code
AFLAGS = -c -g
# -lpthread to link pthread library
# -lpaho-mqtt3c -lrt -lm to link paho-mqtt3c library
LDFLAGS = -lpthread -m32 -lpaho-mqtt3c -lrt -lm

# Dependencies of excutable
OBJS = main.o  mqtt.o parse_config.o cJSON.o

# Compile procedures, $@ is the target, $^ are dependencies, 
smart_home:$(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# Build .o files
$(OBJS):%.o:%.c
	$(CC) $(AFLAGS) $< -o $@


.PHONY: clean lib
# Clean all .o files and excutable
clean:
	rm *.o smart_home

lib:
	ar crs libmylib.a 
#gcc main.c -o server -L.. -lmylib -lpthread
