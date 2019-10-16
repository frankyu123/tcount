# tcount
A tool for counting terms from large texts based on external hash table.

## External Hash Table
* concurrent insert terms in N threads
* write external key buffer when current mem usage >= limit mem usage
* merge M files with K chunks (winner tree)

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
| **-m** | limit memory usage for tcount | -m 400000000 |
| **-s** | expected key buffer size | -s 1024 |
| **-h** | hash table size | -h 3000 |
| **-chunk** | number of external chunks using in merge | -chunk 4 |
| **-parallel** | number of threads needed | -parallel 4 |
| **-o** | output in specific file | -o result.rec |
| **--help** | show tcount information | --help |