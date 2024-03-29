PREFIX = /usr
PYFILES = $(wildcard ./*.py)
BINARIES = $(PYFILES:.py=)
SERVICE_FILE = auto-ryzenadjd.service

.PHONY: install uninstall install-systemd uninstall-systemd

install: $(BINARIES)

$(BINARIES):
	cp $@.py $(PREFIX)/bin/$@
	chmod +x $(PREFIX)/bin/$@

install-systemd: install
	cp $(SERVICE_FILE) /etc/systemd/system/

uninstall:
	rm -f $(addprefix $(PREFIX)/bin/,$(BINARIES))

uninstall-systemd:
	rm -f /etc/systemd/system/$(SERVICE_FILE)