CXX = c++
CXXFILES = $(wildcard src/cli/*.cpp)
override CXXFLAGS += -Wall
#override LDFLAGS += -ltomlplusplus
PREFIX = /usr/local
EXECUTABLE = auto-ryzenadjctl

.PHONY: build install uninstall clean run debug 

build:
	$(CXX) $(CXXFILES) $(LDFLAGS) $(CXXFLAGS) -o $(EXECUTABLE).out

install: ./$(EXECUTABLE).out
	cp ./$(EXECUTABLE).out $(PREFIX)/bin/$(EXECUTABLE)

uninstall: $(PREFIX)/bin/$(EXECUTABLE)
	rm $(PREFIX)/bin/$(EXECUTABLE)

clean:
	rm ./*.out 2> /dev/null || true
	rm ./vgcore* 2> /dev/null || true

run: build
	./$(EXECUTABLE).out

debug: clean
	$(CXX) -DDEBUG $(CXXFILES) -o $(EXECUTABLE)_debug.out $(LDFLAGS) $(CXXFLAGS) -g
	./$(EXECUTABLE)_debug.out $(RUNARGS)

