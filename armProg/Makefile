CXX = g++
RM = rm -rf

APP_NAME = str750prog

CFLAGS  += -Os -Wall
LDFLAGS += -s

CXX_SRCS = main.cpp

$(APP_NAME): $(CXX_SRCS)
	$(CXX) -o $@ $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $<


clean:
	$(RM) $(APP_NAME)

.SUFFIXES: .cpp
.PHONY: clean
