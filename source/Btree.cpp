#include "Btree.h"

Btree::Btree(const string& _store_path, const string& _file_name, const char* _mode){
	mode = string(_mode);
	storePath = _store_path;
	fileName = _file_name;
	btree = NULL;
	string _btree_path = _store_path + "/" + fileName;
	if(strcmp(_mode, "r") == 0){//read-only, btree must exist
		openRead(_btree_path);
	}
	else if(strcmp(_mode, "rw") == 0){//read and write, btree must exist
		openReadWrite(_btree_path);
	}
	else if(strcmp(_mode, "w") == 0){//build a new btree, delete if exist
		openWrite(_btree_path);
	}
	else{
		cout << "[" << _mode << "] is error mode when open: " << _btree_path << endl;
	}
	{
		btree ->log("new Btree\n");
	}
}
Btree::~Btree(){
	delete btree;
	btree = NULL;
}
bool Btree::insert(const bstr& _key, const bstr& _val){
	mleafdata _mdata;
	_mdata.set_sKey_sValue(_key.str, _key.len, _val.str, _val.len);
	return btree ->Insert(_mdata);
}
bool Btree::insert(const char* _key, int _klen, const char* _val, int _vlen){
	mleafdata _mdata;
	_mdata.set_sKey_sValue(_key, _klen, _val, _vlen);
	return btree ->Insert(_mdata);
}
bool Btree::search(const char* _key, int _klen, char*& _val, int & _vlen)const{
	KeyType _k;
	mleafdata _mdata;
	_k.set_sKey(_key, _klen);
	{
		btree ->log("before search\n");
	}
	bool _search = btree ->Search(_k, _mdata);
	{
		btree ->log("after search\n");
	}
	if(!_search){
		btree ->log("not find");
		_val = NULL;
		_vlen = 0;
		return false;
	}
	{
		btree ->log("got search and before copy\n");
	}
	_vlen = _mdata.pVal->lenTerm[0];
	_val = new char[_vlen+1];
	memcpy(_val, _mdata.pVal->Term[0], _vlen);
	_val[_vlen]='\0';
	{
		stringstream _ss;
		_ss << "len= " << _vlen << " val = " << _val << endl;
		btree ->log(_ss.str().c_str());
		btree ->log("before ret search\n");
	}
	return true;
}
bool Btree::remove(const bstr& _kstr){
	KeyType _key;
	_key.set_sKey(_kstr.cstr(), _kstr.len);
	return btree ->Delete(_key);
}
bool Btree::flush(){
	if(this ->btree == NULL){
		return false;
	}
	(this ->btree) ->StoreTree();
	return true;
}
bool Btree::release(){
	this ->flush();
	delete btree;
	btree = new BPlusTree((this->getBtreeFilePath()).c_str(), "rw");
	return true;
}
bool Btree::close(){
	btree ->log("before flush\n");
	this ->flush();
	btree ->log("after flush\n");
	delete btree;
	btree = NULL;
	return true;
}

//private
bool Btree::deleteifExistPath(const string& _path){
	return true;
}
bool Btree::openRead(const string& _btree_path){
	{
		if(btree != NULL){
			cout << "err in openRead btree [" + _btree_path + "]: btree is not NULL" << endl;
			exit(0);
		}
	}
	this ->btree = new BPlusTree((_btree_path).c_str(), "open");
	return true;
}
bool Btree::openWrite(const string& _btree_path){
	this ->btree = new BPlusTree((_btree_path).c_str(), "build");
	return true;
}
bool Btree::openReadWrite(const string& _btree_path){
	{
		if(btree != NULL){
			cout << "err in openReadWrite btree [" + _btree_path + "]: btree is not NULL" << endl;
			exit(0);
		}
	}
	this ->btree = new BPlusTree((_btree_path).c_str(), "open");
	return true;
}

