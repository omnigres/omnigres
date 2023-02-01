PGVERS=15 14 13
ROOT_DIR=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

test: $(PGVERS)

$(PGVERS):
	mkdir -p .b-$@
	cd .b-$@ && cmake "${ROOT_DIR}" -DPGVER=$@ && CTEST_PARALLEL_LEVEL=8 $(MAKE) -j 8 all test

