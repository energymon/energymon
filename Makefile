CXX = /usr/bin/gcc
CXXFLAGS = -fPIC -Wall -Wno-unknown-pragmas -Iinc -O6
LDFLAGS = -shared -lhidapi-libusb -lpthread -lrt -lm
DEFAULT_IMPL = -lem-dummy
TESTCXXFLAGS = -Wall -Iinc -g -O0
TESTLDFLAGS = -Llib $(DEFAULT_IMPL) -lpthread -lrt -lm

INCDIR = inc
SRCDIR = src
TESTDIR = test
LIBDIR = lib
BINDIR = bin

TEST_SOURCES = $(wildcard $(TESTDIR)/*.c)
TEST_OBJECTS = $(patsubst $(TESTDIR)/%.c,$(BINDIR)/%.o,$(TEST_SOURCES))
TESTS = $(patsubst $(TESTDIR)/%.c,$(BINDIR)/%,$(TEST_SOURCES))

all: $(LIBDIR) energy $(TESTS)

# Power/energy monitors
energy: $(LIBDIR)/libem.so $(LIBDIR)/libem-dummy.so $(LIBDIR)/libem-msr.so $(LIBDIR)/libem-odroid.so $(LIBDIR)/libem-osp.so $(LIBDIR)/libem-osp-polling.so

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

# Build test object files
$(BINDIR)/%.o : $(TESTDIR)/%.c | $(BINDIR)
	$(CXX) -c $(TESTCXXFLAGS) $< -o $@

# Build test binaries
$(BINDIR)/% : $(BINDIR)/%.o
	$(CXX) $< $(TESTLDFLAGS) -o $@

$(BINDIR) $(LIBDIR) :
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
