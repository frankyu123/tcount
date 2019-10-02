TCOUNTEXE = tcount
RESULT = result.rec

build:
ifeq ($(OS), Darwin)
	gcc -g -pthread main.c lib/hash.c lib/mergesort.c lib/winner_tree.c -o $(TCOUNTEXE)
else
	gcc -g main.c lib/hash.c lib/mergesort.c lib/winner_tree.c -o $(TCOUNTEXE)
endif

test:
	./$(TCOUNTEXE) -parallel 5 -s 1024 -h 5000 -m 500000000 term_list.txt

clean:
ifeq ($(TCOUNTEXE), $(wildcard $(TCOUNTEXE)))
	rm $(TCOUNTEXE)
endif

ifeq ($(TCOUNTEXE).dSYM, $(wildcard $(TCOUNTEXE).dSYM))
	rm -rf $(TCOUNTEXE).dSYM
endif