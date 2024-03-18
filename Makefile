CXX = g++
CXXFLAGS = -std=c++14 -O2 -Wall -g

TARGET = server
OBJS = \
	./log/*.cpp \
	./http/*.cpp \
	./timer/*.cpp \
	./buffer/*.cpp \
	./server/*.cpp \
	./pool/*.cpp \
	./main.cpp

all: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) \
	-o ./bin/$(TARGET) \
	-pthread \
	-lmysqlclient

clean:
	rm -rf ./bin/$(OBJS) $(TARGET)