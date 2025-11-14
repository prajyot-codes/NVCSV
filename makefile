CC=nvcc
CFLAGS=-O3 -Xptxas -v -use_fast_math -arch=sm_30
SOURCES=nvcsv.cu
BINNAME=nvcsv

ifeq ($(MYSQL_UPLOAD),1)
SOURCES += mysql_uploader.cpp
LDFLAGS += -lmysqlclient
CFLAGS += -DMYSQL_UPLOAD
endif

$(BINNAME): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) $(LDFLAGS) -o $(BINNAME)

clean:
	rm $(BINNAME)
