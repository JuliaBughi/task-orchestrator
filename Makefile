CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -Iinclude `pkg-config --cflags glib-2.0`
LDFLAGS = `pkg-config --libs glib-2.0` -lcurses -lm

all: folders server client
server: bin/orchestrator
client: bin/client

folders:
	@mkdir -p src include obj bin tmp output
bin/orchestrator: obj/orchestrator.o
	$(CC) $^ $(LDFLAGS) -o $@
bin/client: obj/client.o
	$(CC) $^ $(LDFLAGS) -o $@
obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@
test_file:
	dd if=/dev/zero of=test_file.txt bs=3M count=1000
clean_fifos:
	rm -f tmp/fifo*
clean_output:
	rm -f output/*
clean_build:
	rm -f obj/* bin/*
clean:
	rm -f obj/* tmp/* bin/* output/* test3m_file.txt test5m_file.txt test6m_file.txt test6m2_file.txt