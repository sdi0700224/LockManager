# paths
INCLUDE = include

# folders
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin
FILES_DIR = files

# compiler
CC = g++

# Compile options. Το -I<dir> λέει στον compiler να αναζητήσει εκεί include files
CPPFLAGS = -I$(INCLUDE) -lrt -g -Wall -Werror -lm -pthread

# Source files
SRCS = $(SRC_DIR)/Bank.cpp $(SRC_DIR)/Server.cpp $(SRC_DIR)/RecordEditor.cpp $(SRC_DIR)/MySemaphore.cpp $(SRC_DIR)/SegmentLock.cpp
READER_SRCS = $(SRC_DIR)/Reader.cpp $(SRC_DIR)/RecordEditor.cpp $(SRC_DIR)/SegmentLock.cpp $(SRC_DIR)/MySemaphore.cpp
WRITER_SRCS = $(SRC_DIR)/Writer.cpp $(SRC_DIR)/RecordEditor.cpp $(SRC_DIR)/SegmentLock.cpp $(SRC_DIR)/MySemaphore.cpp

# Object files
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))
READER_OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(READER_SRCS))
WRITER_OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(WRITER_SRCS))

# Το εκτελέσιμο πρόγραμμα
EXEC = $(BIN_DIR)/Bank
READER_EXEC = $(BIN_DIR)/Reader
WRITER_EXEC = $(BIN_DIR)/Writer

# Παράμετροι για δοκιμαστική εκτέλεση
ARGS = $(FILES_DIR)/accounts100000.bin 25 100 20

all: $(EXEC) $(READER_EXEC) $(WRITER_EXEC)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CC) -c $< -o $@ $(CPPFLAGS)

$(EXEC): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(OBJS) -o $(EXEC) $(CPPFLAGS)

$(READER_EXEC): $(READER_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $^ -o $@ $(CPPFLAGS)

$(WRITER_EXEC): $(WRITER_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $^ -o $@ $(CPPFLAGS)

run: all
	./$(EXEC) $(ARGS)

clean:
	rm -rf $(BUILD_DIR)/*.o $(BIN_DIR)/*
	rm -f LocalAccounts.bin Log.txt

debug: $(EXEC)
	valgrind ./$(EXEC) $(ARGS)
