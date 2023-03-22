make clobber
make -j12  EXTRA_CFLAGS+=-DRUN_COUNT=1
make     EXTRA_CFLAGS+=-DRUN_COUNT=1
cat builddatastore.sh
