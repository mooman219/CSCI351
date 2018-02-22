CC      = clang
CFLAGS  = -g -Wall
PROGRAM = peerchat
OBJECTS = peerchat.o peerchat_utility.o peerchat_packet.o peerchat_user.o

peerchat: $(OBJECTS)
	$(CC) $(CFLAGS) -o $(PROGRAM) $(OBJECTS)

clean:
	rm -f $(PROGRAM) $(OBJECTS)

peerchat.o: peerchat_packet.h peerchat_user.h peerchat_utility.h
peerchat_utility.o: peerchat_utility.h
peerchat_packet.o: peerchat_packet.h peerchat_user.h peerchat_utility.h
peerchat_user.o: peerchat_user.h peerchat_utility.h
