#paths
MODULES = 
INCLUDE = 

# compiler
CC = g++

# Compile options. Το -I<dir> λέει στον compiler να αναζητήσει εκεί include files
CPPFLAGS = -lrt -g -Wall -Werror -lm -pthread

# Αρχεία .o
OBJS = Bank.o Server.o RecordEditor.o Semaphore.o SegmentLock.o

# Το εκτελέσιμο πρόγραμμα
EXEC = Bank

# Παράμετροι για δοκιμαστική εκτέλεση
ARGS = accounts100000.bin 25 100 20

$(EXEC): $(OBJS) 
	$(CC) $(OBJS) -o $(EXEC) $(CPPFLAGS)

	g++ -o Reader Reader.cpp RecordEditor.o SegmentLock.o Semaphore.o $(CPPFLAGS)
	g++ -o Writer Writer.cpp RecordEditor.o SegmentLock.o Semaphore.o $(CPPFLAGS)

run: $(EXEC)
	./$(EXEC) $(ARGS)

clean:
	rm -f $(OBJS) $(EXEC)
	rm -f LocalAccounts.bin Reader Writer Log.txt

debug: $(EXEC)
	valgrind ./$(EXEC) $(ARGS)
	ipcs