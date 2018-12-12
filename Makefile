TARGETS = scheduling_simulator
CC = gcc
CFLAGS += -std=gnu99 -Wall
OBJS = scheduling_simulator.o task.o

GIT_HOOKS := .git/hooks/applied

all:$(TARGETS) $(GIT_HOOKS)

$(GIT_HOOKS):
	@.githooks/install-git-hooks
	@echo

$(TARGETS):$(OBJS)
	$(CC) $(CFLAGS) -o scheduling_simulator $(OBJS)

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o scheduling_simulator
