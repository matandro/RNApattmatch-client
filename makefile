#BUILD=-g
BUILD=-O

SOURCE_FOLDER=src
INCLUDE_FOLDER=include
BINARY_FOLDER=bin
CC=g++
TARGET=RNAPattMatch RNAdsBuilder
TARGET_OBJECTS=$(addsuffix .o ,$(addprefix $(BINARY_FOLDER)/,$(TARGET)))
SOURCES=$(wildcard $(SOURCE_FOLDER)/*.cpp)
CCFLAGS=-Wall $(BUILD) -I$(INCLUDE_FOLDER) -std=c++11 -pthread
CCLIBS=
LDFLAGS=-std=c++11
LDLIBS=-lboost_regex -lboost_system -lboost_thread -lpthread
OBJECTS=$(filter-out $(TARGET_OBJECTS) ,$(addprefix $(BINARY_FOLDER)/,$(notdir $(SOURCES:.cpp=.o))))


all: $(BINARY_FOLDER) $(TARGET)

$(BINARY_FOLDER):
	mkdir $(BINARY_FOLDER)
 
$(BINARY_FOLDER)/%.o: $(SOURCE_FOLDER)/%.cpp
	@echo 'Building object ' $@
	$(CC) $(CCFLAGS) -c $< -o $@ $(CCLIBS)
	@echo 'Finished building object ' $@

RNAPattMatch: $(OBJECTS) $(BINARY_FOLDER)/RNAPattMatch.o
	@echo 'Started linking binary ' RNAPattMatch
	$(CC) $(LDFLAGS) -o $(BINARY_FOLDER)/$@ $^ $(LDLIBS)
	@echo 'Finished linking binary'
	
RNAdsBuilder: $(OBJECTS) $(BINARY_FOLDER)/RNAdsBuilder.o
	@echo 'Started linking binary ' RNAdsBuilder
	$(CC) $(LDFLAGS) -o $(BINARY_FOLDER)/$@ $^ $(LDLIBS)
	@echo 'Finished linking binary'
 
clean:
	rm -f $(BINARY_FOLDER)/*
