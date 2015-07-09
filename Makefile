CXX = /usr/bin/gcc
CXXFLAGS = -fPIC -Wall -Wno-unknown-pragmas -Iinc -O6
LDFLAGS = -shared -lhidapi-libusb -lpthread -lrt -lm
APP_IMPL = -lem-dummy
APPCXXFLAGS = -Wall -Wno-unknown-pragmas -Iinc -O6
APPLDFLAGS = -Llib $(APP_IMPL)
TEST_IMPL = -lem-dummy
TESTCXXFLAGS = -Wall -Iinc -g -O0
TESTLDFLAGS = -Llib $(TEST_IMPL)

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

all: $(LIBDIR) libs $(APPS) $(TESTS)

# Power/energy monitors
libs: $(LIBDIR)/libem.so $(LIBDIR)/libem-dummy.so $(LIBDIR)/libem-msr.so $(LIBDIR)/libem-odroid.so $(LIBDIR)/libem-osp.so $(LIBDIR)/libem-osp-polling.so

$(LIBDIR)/libem.so: $(SRCDIR)/*.c
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -Wl,-soname,$(@F) -o $@ $^

$(LIBDIR)/libem-dummy.so: $(SRCDIR)/em-dummy.c $(INCDIR)/energymon.h $(INCDIR)/em-dummy.h
	$(CXX) $(CXXFLAGS) -DEM_DEFAULT $(LDFLAGS) -Wl,-soname,$(@F) -o $@ $^

$(LIBDIR)/libem-msr.so: $(SRCDIR)/em-msr.c $(INCDIR)/energymon.h $(INCDIR)/em-msr.h
	$(CXX) $(CXXFLAGS) -DEM_DEFAULT $(LDFLAGS) -Wl,-soname,$(@F) -o $@ $^

$(LIBDIR)/libem-odroid.so: $(SRCDIR)/em-odroid.c $(INCDIR)/energymon.h $(INCDIR)/em-odroid.h
	$(CXX) $(CXXFLAGS) -DEM_DEFAULT $(LDFLAGS) -Wl,-soname,$(@F) -o $@ $^

$(LIBDIR)/libem-osp.so: $(SRCDIR)/em-odroid-smart-power.c $(INCDIR)/energymon.h $(INCDIR)/em-odroid-smart-power.h
	$(CXX) $(CXXFLAGS) -DEM_DEFAULT $(LDFLAGS) -Wl,-soname,$(@F) -o $@ $^

$(LIBDIR)/libem-osp-polling.so: $(SRCDIR)/em-odroid-smart-power.c $(INCDIR)/energymon.h $(INCDIR)/em-odroid-smart-power.h
	$(CXX) $(CXXFLAGS) -DEM_DEFAULT $(LDFLAGS) -DEM_ODROID_SMART_POWER_USE_POLLING -Wl,-soname,$(@F) -o $@ $^

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
	mkdir -p /usr/local/include/energymon
	install -m 0644 $(INCDIR)/* /usr/local/include/energymon/

uninstall:
	rm -f /usr/local/lib/libem-*.so
	rm -rf /usr/local/include/energymon/

## cleaning
clean:
	-rm -rf $(LIBDIR) $(BINDIR) *.log *~ $(SRCDIR)/*~
