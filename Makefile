CXX = c++
CXXFLAGS = -O2
#LDFLAGS = 
PREFIX = /usr/local
SERVICE_FILES = $(wildcard *.service)

.PHONY: all build_cli build_daemon install install_cli install_daemon systemd uninstall uninstall_cli uninstall_daemon clean

all: build_cli build_daemon

build_cli:
	make -f./cli.mk build CXX=$(CXX) CXXFLAGS+=$(CXXFLAGS) LDFLAGS+=$(LDFLAGS) PREFIX=$(PREFIX)

build_daemon:
	make -f./daemon.mk build CXX=$(CXX) CXXFLAGS+=$(CXXFLAGS) LDFLAGS+=$(LDFLAGS) PREFIX=$(PREFIX)

install: install_cli install_daemon systemd

install_cli: build_cli
	make -f./cli.mk install CXX=$(CXX) CXXFLAGS+=$(CXXFLAGS) LDFLAGS+=$(LDFLAGS) PREFIX=$(PREFIX)

install_cli: build_daemon
	make -f./daemon.mk install CXX=$(CXX) CXXFLAGS+=$(CXXFLAGS) LDFLAGS+=$(LDFLAGS) PREFIX=$(PREFIX)

systemd: build_cli build_daemon
	cp $(SERVICE_FILES) /etc/systemd/system/

uninstall: uninstall_cli uninstall_daemon

uninstall_cli:
	make -f./cli.mk uninstall CXX=$(CXX) CXXFLAGS+=$(CXXFLAGS) LDFLAGS+=$(LDFLAGS) PREFIX=$(PREFIX)

uninstall_cli:
	make -f./daemon.mk uninstall CXX=$(CXX) CXXFLAGS+=$(CXXFLAGS) LDFLAGS+=$(LDFLAGS) PREFIX=$(PREFIX)

clean:
	make -f./cli.mk clean CXX=$(CXX) CXXFLAGS+=$(CXXFLAGS) LDFLAGS+=$(LDFLAGS) PREFIX=$(PREFIX)
	make -f./daemon.mk clean CXX=$(CXX) CXXFLAGS+=$(CXXFLAGS) LDFLAGS+=$(LDFLAGS) PREFIX=$(PREFIX)

