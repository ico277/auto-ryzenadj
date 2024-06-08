CXX = c++
CXXFLAGS = -O2
PREFIX = /usr/local
SERVICE_FILES = $(wildcard *.service)

ifdef DEBUG
    override CXXFLAGS = -g -DDEBUG
endif

.PHONY: all cli daemon install install-cli install_daemon systemd uninstall uninstall_cli uninstall_daemon clean

all: cli daemon

cli:
	make -f./cli.mk build CXX="$(CXX)" CXXFLAGS+="$(CXXFLAGS)" PREFIX="$(PREFIX)"

daemon:
	make -f./daemon.mk build CXX="$(CXX)" CXXFLAGS+="$(CXXFLAGS)" PREFIX="$(PREFIX)"

install: install-cli install-daemon 

install-cli: cli
	make -f./cli.mk install CXX="$(CXX)" CXXFLAGS+="$(CXXFLAGS)" PREFIX="$(PREFIX)"

install-daemon: daemon
	make -f./daemon.mk install CXX="$(CXX)" CXXFLAGS+="$(CXXFLAGS)" PREFIX="$(PREFIX)"

install-systemd: install 
	cp $(SERVICE_FILES) /etc/systemd/system/

uninstall: uninstall-cli uninstall_daemon

uninstall-cli:
	make -f./cli.mk uninstall CXX="$(CXX)" CXXFLAGS+="$(CXXFLAGS)" PREFIX="$(PREFIX)"

uninstall-daemon:
	make -f./daemon.mk uninstall CXX="$(CXX)" CXXFLAGS+="$(CXXFLAGS)" PREFIX="$(PREFIX)"

clean:
	make -f./cli.mk clean CXX="$(CXX)" CXXFLAGS+="$(CXXFLAGS)" PREFIX="$(PREFIX)"
	make -f./daemon.mk clean CXX="$(CXX)" CXXFLAGS+="$(CXXFLAGS)" PREFIX="$(PREFIX)"

