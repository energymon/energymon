CXX = /usr/bin/gcc
CXXFLAGS = -fPIC -Wall -Wno-unknown-pragmas -Iinc -Llib -O6
DBG = -g
DEFINES ?=
LDFLAGS = -shared -lpthread -lrt -lm

DOCDIR = doc
LIBDIR = lib
INCDIR = ./inc
SRCDIR = ./src

all: $(LIBDIR) energy

$(LIBDIR):
	-mkdir -p $(LIBDIR)

# Power/energy monitors
energy: $(LIBDIR)/libem.so $(LIBDIR)/libem-dummy.so $(LIBDIR)/libem-msr.so $(LIBDIR)/libem-odroid.so $(LIBDIR)/libem-odroid-smart-power.so

$(LIBDIR)/libem.so: $(SRCDIR)/em-dummy.c $(SRCDIR)/em-msr.c $(SRCDIR)/em-odroid.c $(SRCDIR)/em-odroid-smart-power.c
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -Wl,-soname,$(@F) -o $@ $^

$(LIBDIR)/libem-dummy.so: $(SRCDIR)/em-dummy.c
	$(CXX) $(CXXFLAGS) -DEM_GENERIC $(LDFLAGS) -Wl,-soname,$(@F) -o $@ $^

$(LIBDIR)/libem-msr.so: $(SRCDIR)/em-msr.c
	$(CXX) $(CXXFLAGS) -DEM_GENERIC $(LDFLAGS) -Wl,-soname,$(@F) -o $@ $^

$(LIBDIR)/libem-odroid.so: $(SRCDIR)/em-odroid.c
	$(CXX) $(CXXFLAGS) -DEM_GENERIC $(LDFLAGS) -Wl,-soname,$(@F) -o $@ $^

$(LIBDIR)/libem-odroid-smart-power.so: $(SRCDIR)/em-odroid-smart-power.c
	$(CXX) $(CXXFLAGS) -DEM_GENERIC $(LDFLAGS) -lhidapi-libusb -Wl,-soname,$(@F) -o $@ $^

# Installation
install: all
	install -m 0644 $(LIBDIR)/*.so /usr/local/lib/
	mkdir -p /usr/local/include/energymon
	install -m 0644 $(INCDIR)/* /usr/local/include/energymon/

uninstall:
	rm -f /usr/local/lib/libem-*.so
	rm -rf /usr/local/include/energymon/

## cleaning
clean:
	-rm -rf $(LIBDIR) *.log *~ $(SRCDIR)/*~
