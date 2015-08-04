CXX = /usr/bin/gcc
CXXFLAGS = -fPIC -Wall -Wno-unknown-pragmas -Iinc -O6
LDFLAGS = -shared -lhidapi-libusb -lpthread -lm
APPCXXFLAGS = -Wall -Wno-unknown-pragmas -Iinc -O6
APPLDFLAGS = -Wl,--no-as-needed -Llib -lenergymon-default -lhidapi-libusb -lpthread -lm
TESTCXXFLAGS = -Wall -Iinc -g -O0
TESTLDFLAGS = -Wl,--no-as-needed -Llib -lenergymon-default -lhidapi-libusb -lpthread -lm

INSTALL_PREFIX = /usr/local

INCDIR = inc
SRCDIR = src
LIBDIR = lib
BINDIR = bin
PCDIR = pkgconfig
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

ifndef DEFAULT
DEFAULT = dummy
endif

DEFAULT_IMPL = $(DEFAULT)
DEFAULT_FLAGS =
ifeq ($(DEFAULT), osp-polling)
DEFAULT_IMPL = osp
DEFAULT_FLAGS = -DENERGYMON_OSP_USE_POLLING
endif

all: $(LIBDIR) libs $(APPS) $(TESTS)

# Power/energy monitors
libs: $(LIBDIR)/libenergymon-default.so $(LIBDIR)/libenergymon-dummy.so $(LIBDIR)/libenergymon-msr.so $(LIBDIR)/libenergymon-odroid.so $(LIBDIR)/libenergymon-osp.so $(LIBDIR)/libenergymon-osp-polling.so

$(LIBDIR)/libenergymon-default.so: $(SRCDIR)/em-$(DEFAULT_IMPL).c $(INCDIR)/energymon.h $(INCDIR)/energymon-$(DEFAULT_IMPL).h
	$(CXX) $(CXXFLAGS) -DENERGYMON_DEFAULT $(DEFAULT_FLAGS) $(LDFLAGS) -Wl,-soname,$(@F) -o $@ $^
	sed -e s/energymon-$(DEFAULT)/energymon-default/g $(PCDIR)/energymon-$(DEFAULT).pc > $(PCDIR)/energymon-default.pc

$(LIBDIR)/libenergymon-dummy.so: $(SRCDIR)/em-dummy.c $(INCDIR)/energymon.h $(INCDIR)/energymon-dummy.h
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -Wl,-soname,$(@F) -o $@ $^

$(LIBDIR)/libenergymon-msr.so: $(SRCDIR)/em-msr.c $(INCDIR)/energymon.h $(INCDIR)/energymon-msr.h
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -Wl,-soname,$(@F) -o $@ $^

$(LIBDIR)/libenergymon-odroid.so: $(SRCDIR)/em-odroid.c $(INCDIR)/energymon.h $(INCDIR)/energymon-odroid.h
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -Wl,-soname,$(@F) -o $@ $^

$(LIBDIR)/libenergymon-osp.so: $(SRCDIR)/em-osp.c $(INCDIR)/energymon.h $(INCDIR)/energymon-osp.h
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -Wl,-soname,$(@F) -o $@ $^

$(LIBDIR)/libenergymon-osp-polling.so: $(SRCDIR)/em-osp.c $(INCDIR)/energymon.h $(INCDIR)/energymon-osp.h
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -DENERGYMON_OSP_USE_POLLING -Wl,-soname,$(@F) -o $@ $^

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
	install -m 0644 $(LIBDIR)/*.so $(INSTALL_PREFIX)/lib/
	install -d $(INSTALL_PREFIX)/include/energymon
	install -m 0644 $(INCDIR)/* $(INSTALL_PREFIX)/include/energymon/
	install -d $(INSTALL_PREFIX)/lib/pkgconfig
	install -m 0644 $(PCDIR)/*.pc $(INSTALL_PREFIX)/lib/pkgconfig

uninstall:
	rm -f $(INSTALL_PREFIX)/lib/libenergymon*.so
	rm -f $(INSTALL_PREFIX)/bin/energymon
	rm -rf $(INSTALL_PREFIX)/include/energymon/
	rm -f $(INSTALL_PREFIX)/lib/pkgconfig/energymon*.pc

## cleaning
clean:
	rm -rf $(LIBDIR) $(BINDIR) *.log *~ $(SRCDIR)/*~
	rm -f $(PCDIR)/energymon-default.pc
