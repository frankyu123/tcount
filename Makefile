TCOUNTEXE = tcount
RESULT = result.rec

build: libjieba.a
	gcc -g main.c lib/hash.c -o $(TCOUNTEXE) -L./ -ljieba -lstdc++ -lm

test:
	./$(TCOUNTEXE) ../dataset/ettoday.rec

libjieba.a:
	g++ -o jieba.o -c -DLOGGING_LEVEL=LL_WARNING -I./lib/cjieba/deps/ ./lib/cjieba/lib/jieba.cpp
	ar rs libjieba.a jieba.o

clean:
ifeq ($(TCOUNTEXE), $(wildcard $(TCOUNTEXE)))
	rm $(TCOUNTEXE)
endif

ifeq ($(TCOUNTEXE).dSYM, $(wildcard $(TCOUNTEXE).dSYM))
	rm -rf $(TCOUNTEXE).dSYM
endif