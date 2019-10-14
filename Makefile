TCOUNTEXE = tcount
UNAME = $(shell uname -s)

build:
ifeq ($(UNAME), Darwin)
	gcc main.c lib/hash.c lib/mergesort.c lib/winner_tree.c -o $(TCOUNTEXE)
else
	gcc -pthread main.c lib/hash.c lib/mergesort.c lib/winner_tree.c -o $(TCOUNTEXE)
endif

run:
	./$(TCOUNTEXE) -m 400000000 ../auto-term-generator/term.txt > result_news_v2.rec

clean:
ifeq ($(TCOUNTEXE), $(wildcard $(TCOUNTEXE)))
	rm $(TCOUNTEXE)
endif

ifeq ($(TCOUNTEXE).dSYM, $(wildcard $(TCOUNTEXE).dSYM))
	rm -rf $(TCOUNTEXE).dSYM
endif