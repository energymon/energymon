BUILDDIR = _build
MAKE_PARAMS = --no-print-directory

ifndef DEFAULT
DEFAULT = dummy
endif

all: $(BUILDDIR) cmake libs

$(BUILDDIR):
	mkdir -p $@

cmake: $(BUILDDIR)
	cd $(BUILDDIR) && cmake -DCMAKE_BUILD_TYPE=Release -DDEFAULT:STRING=$(DEFAULT) ..

libs: cmake
	cd $(BUILDDIR) && make $(MAKE_PARAMS)

energymon-dummy \
energymon-msr \
energymon-odroid \
energymon-osp \
energymon-osp-polling \
energymon-rapl \
energymon-default \
energymon-dummy-static \
energymon-msr-static \
energymon-odroid-static \
energymon-osp-static \
energymon-osp-polling-static \
energymon-rapl-static \
energymon-default-static: cmake
	cd $(BUILDDIR) && make $@ $(MAKE_PARAMS)

install: all
	cd $(BUILDDIR) && make install $(MAKE_PARAMS)

uninstall:
	cd $(BUILDDIR) && make uninstall $(MAKE_PARAMS)

clean:
	rm -rf $(BUILDDIR)
