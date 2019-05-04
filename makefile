CC = g++

all: buildIndex searchQuery
buildIndex: source/buildIndex.cpp InvertedIndex.o CBtreeFunc.o Btree.o KVstore.o stopwords.o
	$(CC) -o buildIndex source/buildIndex.cpp InvertedIndex.o CBtreeFunc.o Btree.o KVstore.o stopwords.o
searchQuery: source/searchQuery.cpp InvertedIndex.o CBtreeFunc.o Btree.o KVstore.o stopwords.o
	$(CC) -o searchQuery source/searchQuery.cpp InvertedIndex.o CBtreeFunc.o Btree.o KVstore.o stopwords.o
InvertedIndex.o:
	$(CC) -c source/InvertedIndex.cpp
CBtreeFunc.o:
	$(CC) -c source/CBtreeFunc.cpp
Btree.o:
	$(CC) -c source/Btree.cpp
KVstore.o:
	$(CC) -c source/KVstore.cpp
stopwords.o:
	$(CC) -c source/stopwords.cpp
clean:
	@(rm -rf *.o searchQuery buildIndex ./index)
