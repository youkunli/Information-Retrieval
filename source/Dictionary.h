#ifndef DICTIONARY_H_
#define DICTIONARY_H_

#include<iostream>
#include<string.h>
#include "InvertedIndex.h"

using namespace std;

class record{
public:
	int doc_freq;
	mPosting* posting;
	char * term;
};

class Dictionary{
private:
	record* recordPool;
public:

	bool exist(string _term){

	}

	mPosting* getPosting(string _term){

	}
};

#endif /* DICTIONARY_H_ */
