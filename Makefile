CXXFLAGS =	-O0 -g -std=c++17  -Wall $(INC)

OBJS =		S_main.o

LIBS =-L. -lws2_32 -Lc:/libevent-2.1.12-stable/build/lib -levent_core -levent_extra

INC  = -Ic:/libevent-2.1.12-stable/include -Ic:/libevent-2.1.12-stable

TARGET =	S_main.exe

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET)

clean:
	del $(OBJS) $(TARGET)    #win
# 	rem -f $(OBJS) $(TARGET) #bsd