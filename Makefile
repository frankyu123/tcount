TCOUNTEXE = tcount
RESULT = result.rec

build: libjieba.a
ifeq ($(OS), Darwin)
	gcc -g main.c lib/hash.c -o $(TCOUNTEXE) -L./ -ljieba -lstdc++ -lm
else
	gcc -g -pthread main.c lib/hash.c -o $(TCOUNTEXE) -L./ -ljieba -lstdc++ -lm
endif

test:
	./$(TCOUNTEXE) -thread 5 ../dataset/ettoday.rec

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

	rm libjieba.a jieba.o