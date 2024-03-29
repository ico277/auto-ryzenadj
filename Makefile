PREFIX = /usr
PYFILES = $(wildcard ./*.py)
BINARIES = $(PYFILES:.py=)
SERVICE_FILE = auto-ryzenadjd.service

.PHONY: install uninstall install-service uninstall-service

install: $(BINARIES) install-service

$(BINARIES):
	cp $@.py $(PREFIX)/bin/$@
	chmod +x $(PREFIX)/bin/$@

install-service:
	cp $(SERVICE_FILE) /etc/systemd/system/

uninstall: uninstall-service
	rm -f $(addprefix $(PREFIX)/bin/,$(BINARIES))

uninstall-service:
	rm -f /etc/systemd/system/$(SERVICE_FILE)