CC = g++
CFLAGS = -g -Wall -O0

LDFLAGS = -g -lpthread

LISTENER = listener 
LISTENER_OBJ = listener.o socket.o

CHORD_SYS = chord_sys 
CHORD_SYS_OBJ = chord_sys.o

NODE = node
NODE_OBJ = node.o socket.o sha1.o node_class.o

all: $(INTRODUCER) $(LISTENER) $(CHORD_SYS) $(NODE)

$(INTRODUCER): $(INTRODUCER_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

$(LISTENER): $(LISTENER_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

$(CHORD_SYS): $(CHORD_SYS_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

$(NODE): $(NODE_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o : %.cc
	$(CC) $(CFLAGS) -c $< -o $@

clean :
	-rm -rf *.o $(INTRODUCER) $(LISTENER) $(CHORD_SYS) $(NODE)
