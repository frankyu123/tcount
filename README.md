# tcount
A tool for counting terms from large texts based on external hash table.

## External Hash Table
* insert terms in hash ( open addressing )
* write external files when memory usage >= limit memory or node table is full ( default size : 10000000 )
* merge M files with K chunks ( winner tree )

## Usage
**Install tcount**
```
make
```
**Run tcount**
* Using Makefile
```bash
#edit make run in Makefile
make run
```
* Command
```bash
./tcount [OPTION]... [FILE]...
```

## Command
| Command | Description | Example |
| ---             | ---    | ---       |
| **-m** | total memory limit | -m 400000000 test.txt |
| **-s** | expected key buffer size | -s 1024 test.txt |
| **-h** | hash table size | -h 3000 test.txt |
| **-chunk** | number of external chunks using in merge | -chunk 4 test.txt |
| **-parallel** | number of threads needed | -parallel 4 test.txt |
| **-o** | output in specific file | -o result.rec test.txt |
| **--help** | show tcount information | --help |