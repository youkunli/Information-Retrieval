#include"KVstore.h"
#include"InvertedIndex.h"

KVstore::KVstore(bool isCreateMode){
	if(isCreateMode)
	{
		string _btree_path = "BTREE";
		system(("rm -rf "+_btree_path+"/").c_str());
		system(("mkdir "+_btree_path).c_str());
		term2id = new Btree(_btree_path, "term2id", "w");
		doc2id = new Btree(_btree_path, "doc2id", "w");
		id2doc = new Btree(_btree_path, "id2doc", "w");
		term2offset = new Btree(_btree_path, "term2offset", "w");
		did2len = new Btree(_btree_path, "did2len", "w");
	}
	else
	{
		string _btree_path = "BTREE";
		term2id = new Btree(_btree_path, "term2id", "r");
		doc2id = new Btree(_btree_path, "doc2id", "r");
		id2doc = new Btree(_btree_path, "id2doc", "r");
		term2offset = new Btree(_btree_path, "term2offset", "r");
		did2len = new Btree(_btree_path, "did2len", "r");
	}
}
KVstore::~KVstore(){
	delete term2id;
	delete doc2id;
	delete id2doc;
	delete term2offset;
	delete did2len;
}

int KVstore::getIDByTerm(string _term){
	char* tmp = NULL;
	int _len = 0;
	bool _get = this->getValueByKey(term2id,
			_term.c_str(), _term.length()*sizeof(char),
			tmp, _len);
	if(!_get) return -1;

	int _id = *(int*)tmp;
	delete[] tmp;
	return _id;
}
void KVstore::flush(Btree* _btree){
	_btree->flush();
}
void KVstore::flush(){
	this->flush(term2offset);
	this->flush(term2id);
	this->flush(doc2id);
	this->flush(id2doc);
	this->flush(did2len);
}
bool KVstore::setOffsetByTerm(string _term, long long int _offset){
	return this->setValueByKey(
			term2offset, _term.c_str(),	_term.length(),
			(char*)&_offset, sizeof(long long int)
			);

}
long long int KVstore::getOffsetByTerm(string _term){
	char* _tmp = NULL;
	int  _lli;
	bool _get = this->getValueByKey(
			term2offset, _term.c_str(),_term.length(),
			_tmp, _lli
			);

	{
		if(!_get) return -1;
	}
	long long int _ret = *((long long int*)_tmp);
	delete[] _tmp;
	return _ret;
}
int KVstore::getIDByDoc(string _doc){
	char* tmp = NULL;
	int _len = 0;
	bool _get = this->getValueByKey(doc2id, _doc.c_str(), _doc.length()*(sizeof(char)), tmp, _len);
	if(!_get) return -1;

	int _id = *((int*)tmp);
	delete[] tmp;
	return _id;
}
string KVstore::getDocByID(int _doc_id){
	char* tmp = NULL;
	int _len = 0;
	bool _get = this->getValueByKey(id2doc, (char*)&_doc_id, sizeof(int), tmp, _len);
	if(!_get){
		return NULL;
	}

	string _ret = string(tmp);
	delete[] tmp;
	return _ret;
}
int KVstore::getLenByDid(int _did){
	char* tmp = NULL;
	int _tmp_i = 0;
	bool _get = this->getValueByKey(did2len, (char*)&_did, sizeof(int), tmp, _tmp_i);
	if(!_get){
		return -1;
	}
	int _len = *(int*)tmp;
	delete[] tmp;
	return _len;
}
bool KVstore::setLenByDid(int _did, int _len){
	return this->setValueByKey(did2len, (char*)&_did, sizeof(int), (char*)&_len, sizeof(int));
}
bool KVstore::setIDByDoc(string _doc, int _id){
	return this->setValueByKey(doc2id, _doc.c_str(), _doc.length()*(sizeof(char)), (char*)&_id, sizeof(int));
}
bool KVstore::setDocByID(int _id, string _doc){
	return this->setValueByKey(id2doc, (char*)&_id, sizeof(int), _doc.c_str(), _doc.length()*sizeof(char));
}
binaryPosting* KVstore::getPosting(string _term, FILE* _fp){
	long long int _offset = this->getOffsetByTerm(_term);
	if(_offset == -1){
		return NULL;
	}
	fseek(_fp, _offset, SEEK_SET);
	binaryPosting* _bp = new binaryPosting(_fp);
	return _bp;
}
bool KVstore::setValueByKey(Btree* _btree, const char* _key, int _klen, const char* _val, int _vlen){
	return _btree->insert(_key, _klen, _val, _vlen);
}
bool KVstore::getValueByKey(Btree* _btree, const char* _key, int _klen, char*& _val, int& _vlen){
	bool _ret = _btree->search(_key, _klen, _val, _vlen);
	return _ret;
}
