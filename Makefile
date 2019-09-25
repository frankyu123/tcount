TCOUNTEXE = tcount
RESULT = result.rec

build:
ifeq ($(OS), Darwin)
	gcc -g main.c lib/hash.c -o $(TCOUNTEXE)
else
	gcc -g -pthread main.c lib/hash.c -o $(TCOUNTEXE)
endif

test:
	./$(TCOUNTEXE) -thread 5 term_list_sm.txt

clean:
ifeq ($(TCOUNTEXE), $(wildcard $(TCOUNTEXE)))
	rm $(TCOUNTEXE)
endif

ifeq ($(TCOUNTEXE).dSYM, $(wildcard $(TCOUNTEXE).dSYM))
	rm -rf $(TCOUNTEXE).dSYM
endif