TARGETS = scheduling_simulator
CC = gcc
CFLAGS += -std=gnu99 -Wall
OBJS = scheduling_simulator.o task.o

all:$(TARGETS)

$(TARGETS):$(OBJS)
	$(CC) $(CFLAGS) -o scheduling_simulator $(OBJS)

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o scheduling_simulator
