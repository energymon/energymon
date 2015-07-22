CXX = /usr/bin/gcc
CXXFLAGS = -fPIC -Wall -Wno-unknown-pragmas -Iinc -O6
LDFLAGS = -shared -lhidapi-libusb -lpthread -lrt -lm
IMPL = libenergymon-dummy.so
APPCXXFLAGS = -Wall -Wno-unknown-pragmas -Iinc -O6
APPLDFLAGS = -Llib -lenergymon -lhidapi-libusb -lpthread -lrt -lm
TESTCXXFLAGS = -Wall -Iinc -g -O0
TESTLDFLAGS = -Llib -lenergymon -lhidapi-libusb -lpthread -lrt -lm

INCDIR = inc
SRCDIR = src
LIBDIR = lib
BINDIR = bin
APPDIR = $(SRCDIR)/app
APPBINDIR = $(BINDIR)/app
TESTDIR = test
TESTBINDIR = $(BINDIR)/test

APP_SOURCES = $(wildcard $(APPDIR)/*.c)
APP_OBJECTS = $(patsubst $(APPDIR)/%.c,$(APPBINDIR)/%.o,$(APP_SOURCES))
APPS = $(patsubst $(APPDIR)/%.c,$(APPBINDIR)/%,$(APP_SOURCES))

TEST_SOURCES = $(wildcard $(TESTDIR)/*.c)
TEST_OBJECTS = $(patsubst $(TESTDIR)/%.c,$(TESTBINDIR)/%.o,$(TEST_SOURCES))
TESTS = $(patsubst $(TESTDIR)/%.c,$(TESTBINDIR)/%,$(TEST_SOURCES))

all: $(LIBDIR) libs impl $(APPS) $(TESTS)

# Power/energy monitors
libs: $(LIBDIR)/libenergymon-all.so $(LIBDIR)/libenergymon-dummy.so $(LIBDIR)/libenergymon-msr.so $(LIBDIR)/libenergymon-odroid.so $(LIBDIR)/libenergymon-osp.so $(LIBDIR)/libenergymon-osp-polling.so

$(LIBDIR)/libenergymon-all.so: $(SRCDIR)/*.c
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -Wl,-soname,$(@F) -o $@ $^

$(LIBDIR)/libenergymon-dummy.so: $(SRCDIR)/em-dummy.c $(INCDIR)/energymon.h $(INCDIR)/em-dummy.h
	$(CXX) $(CXXFLAGS) -DEM_DEFAULT $(LDFLAGS) -Wl,-soname,$(@F) -o $@ $^

$(LIBDIR)/libenergymon-msr.so: $(SRCDIR)/em-msr.c $(INCDIR)/energymon.h $(INCDIR)/em-msr.h
	$(CXX) $(CXXFLAGS) -DEM_DEFAULT $(LDFLAGS) -Wl,-soname,$(@F) -o $@ $^

$(LIBDIR)/libenergymon-odroid.so: $(SRCDIR)/em-odroid.c $(INCDIR)/energymon.h $(INCDIR)/em-odroid.h
	$(CXX) $(CXXFLAGS) -DEM_DEFAULT $(LDFLAGS) -Wl,-soname,$(@F) -o $@ $^

$(LIBDIR)/libenergymon-osp.so: $(SRCDIR)/em-odroid-smart-power.c $(INCDIR)/energymon.h $(INCDIR)/em-odroid-smart-power.h
	$(CXX) $(CXXFLAGS) -DEM_DEFAULT $(LDFLAGS) -Wl,-soname,$(@F) -o $@ $^

$(LIBDIR)/libenergymon-osp-polling.so: $(SRCDIR)/em-odroid-smart-power.c $(INCDIR)/energymon.h $(INCDIR)/em-odroid-smart-power.h
	$(CXX) $(CXXFLAGS) -DEM_DEFAULT $(LDFLAGS) -DEM_ODROID_SMART_POWER_USE_POLLING -Wl,-soname,$(@F) -o $@ $^

impl : | libs
	ln -sfr $(LIBDIR)/$(IMPL) $(LIBDIR)/libenergymon.so

# Build app object files
$(APPBINDIR)/%.o : $(APPDIR)/%.c | $(APPBINDIR)
	$(CXX) -c $(APPCXXFLAGS) $< -o $@

# Build app binaries
$(APPBINDIR)/% : $(APPBINDIR)/%.o
	$(CXX) $< $(APPLDFLAGS) -o $@

# Build test object files
$(TESTBINDIR)/%.o : $(TESTDIR)/%.c | $(TESTBINDIR)
	$(CXX) -c $(TESTCXXFLAGS) $< -o $@

# Build test binaries
$(TESTBINDIR)/% : $(TESTBINDIR)/%.o
	$(CXX) $< $(TESTLDFLAGS) -o $@

$(LIBDIR) $(BINDIR) $(APPBINDIR) $(TESTBINDIR) :
	mkdir -p $@

# Installation
install: all
	install -m 0644 $(LIBDIR)/*.so /usr/local/lib/
	ln -sfr /usr/local/lib/$(IMPL) /usr/local/lib/libenergymon.so
	mkdir -p /usr/local/include/energymon
	install -m 0644 $(INCDIR)/* /usr/local/include/energymon/

uninstall:
	rm -f /usr/local/lib/libenergymon*.so
	rm -rf /usr/local/include/energymon/

## cleaning
clean:
	-rm -rf $(LIBDIR) $(BINDIR) *.log *~ $(SRCDIR)/*~
