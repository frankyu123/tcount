TCOUNTEXE = tcount
UNAME = $(shell uname -s)

build:
ifeq ($(UNAME), Darwin)
	gcc -Iinclude src/tcount.c src/hash.c src/mergesort.c src/winner_tree.c -o $(TCOUNTEXE)
else
	gcc -Iinclude -pthread src/tcount.c src/hash.c src/mergesort.c src/winner_tree.c -o $(TCOUNTEXE)
endif

run:
	./$(TCOUNTEXE) -m 400000000 ../dataset/small_term_list.txt > result.txt

clean:
ifeq ($(TCOUNTEXE), $(wildcard $(TCOUNTEXE)))
	rm $(TCOUNTEXE)
endif

ifeq ($(TCOUNTEXE).dSYM, $(wildcard $(TCOUNTEXE).dSYM))
	rm -rf $(TCOUNTEXE).dSYM
endif