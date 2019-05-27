#ifndef KVSTORE_H_
#define KVSTORE_H_

#include<iostream>
#include<string.h>
#include "Btree.h"

using namespace std;

class binaryPosting;

class KVstore{
private:
	static const bool debug_mode = false;
	static const bool test = false;

	Btree* id2doc;
	Btree* did2len;
	Btree* term2offset;
	void flush(Btree* _btree);
	bool setValueByKey(Btree* _btree, const char* _key, int _klen, const char* _val, int _vlen);
	bool getValueByKey(Btree* _btree, const char* _key, int _klen, char*& _val, int& _vlen);
public:
	KVstore(bool isCreateMode);
	~KVstore();
	bool setOffsetByTerm(string _term, long long int _offset);
	long long int getOffsetByTerm(string _term);
	int getLenByDid(int _did);
	bool setLenByDid(int _did, int _len);
	string getDocByID(int _doc_id);
	bool setDocByID(int _id, string _doc);
	binaryPosting* getPosting(string _term, FILE* _fp);
	void flush();
};

#endif /* KVSTORE_H_ */