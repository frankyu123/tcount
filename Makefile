TCOUNTEXE = tcount
NORMEXE = a.out

build:
ifeq ($(OS), Darwin)
	gcc -pthread main.c lib/hash.c lib/mergesort.c lib/winner_tree.c -o $(TCOUNTEXE)
else
	gcc main.c lib/hash.c lib/mergesort.c lib/winner_tree.c -o $(TCOUNTEXE)
endif

test:
	./$(TCOUNTEXE) -s 1024 -h 5000 -m 70000000 ../segmentor/result.txt > result.rec

clean:
ifeq ($(TCOUNTEXE), $(wildcard $(TCOUNTEXE)))
	rm $(TCOUNTEXE)
endif

ifeq ($(NORMEXE), $(wildcard $(NORMEXE)))
	rm $(NORMEXE)
endif

ifeq ($(TCOUNTEXE).dSYM, $(wildcard $(TCOUNTEXE).dSYM))
	rm -rf $(TCOUNTEXE).dSYM
endif