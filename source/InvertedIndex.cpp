#include"InvertedIndex.h"
#include"KVstore.h"
#include "stopwords.h"
#include<string.h>
#include<iostream>
#include<sstream>

using namespace std;

SPIMIManager::SPIMIManager(){
	nextFile = 0;
	indexpath = "index";
	inputpath = "input";
	blockpath = "block";
	mergeTmpID = 0;
	_debug.open("debug");
	xml_ii = 0;
	string _index_file = this->getIndexPath();
	fp_index = fopen(_index_file.c_str(), "rb");
	if(!fp_index){
		cout << "fp_index bug" << endl;
		exit(0);
	}
	kv_store = new KVstore(false);
}

SPIMIManager::SPIMIManager(vector<string>* _files,
		string _indexpath, string _inputpath, string _blockpath, bool _is_build_mode){
	files = _files;
	nextFile = 0;
	nFilesOneBlock = (int)sqrt((int)_files->size());
	indexpath = _indexpath;
	inputpath = _inputpath;
	blockpath = _blockpath;
	mergeTmpID = 0;
	_debug.open("debug");
	xml_ii = 0;
	if(!_is_build_mode){
		string _index_file = this->getIndexPath();
		fp_index = fopen(_index_file.c_str(), "rb");
		if(!fp_index){
			cout << "fp_index bug" << endl;
			exit(0);
		}
	}
	kv_store = new KVstore(_is_build_mode);
}

SPIMIManager::SPIMIManager(vector<string> * _files, bool _is_build_mode){
		files = _files;
		nextFile = 0;
		nFilesOneBlock = (int)sqrt((int)_files->size());
		indexpath = "index";
		inputpath = "input";
		blockpath = "block";
		mergeTmpID = 0;
		_debug.open("debug");
		xml_ii = 0;
		if(!_is_build_mode){
			string _index_file = this->getIndexPath();
			fp_index = fopen(_index_file.c_str(), "rb");
			if(!fp_index){
				cout << "fp_index bug" << endl;
				exit(0);
			}
		}
		else
		{
			fp_index = NULL;
		}
		kv_store = new KVstore(_is_build_mode);
}

SPIMIManager::~SPIMIManager(){
	if(debug_2){
		stringstream _tmp;
		_tmp << "begin destruct spimi" << endl;
		debug(_tmp.str());
	}
	delete kv_store;
	if(fp_index != NULL)
	{
		fclose(fp_index);
	}
	if(debug_2){
		stringstream _tmp;
		_tmp << "finish destruct spimi" << endl;
		debug(_tmp.str());
	}
	_debug.close();
}

void SPIMIManager::binary_mergeBlocks(){
	this ->mergeTmpID = 0;
	nMergeBlock = (int)sqrt(blocks.size());
	{
		cout << "binary: there are " << blocks.size() << " blocks" << endl;
		cout << "each merge " << nMergeBlock << " blocks" << endl;
	}
	while(blocks.size() > 1){
		vector<block_size>* _bsv = this->nextMergeBlocks(nMergeBlock);
		string _next_mergepath = this->nextMergePath();
		{
			if(blocks.empty()){
				_next_mergepath = this ->getIndexPath();
			}
		}
		int _new_block_size = binary_mergeGroupBlocks(*_bsv, _next_mergepath);

		{//add new block into priority_queue: blocks
			blocks.push(block_size(_next_mergepath, _new_block_size));
		}
		{
			delete _bsv;
		}
	}
	kv_store->flush();
	if(debug_2){
		stringstream _tmp;
		_tmp << "finish binary_mergeBlocks" << endl;
		debug(_tmp.str());
	}
}

int SPIMIManager::binary_mergeGroupBlocks(vector<block_size> & _bs_vec, string _new_block_path){
	//assume that vec size is two
	if(debug_2){
		stringstream _tmp;
		_tmp << "IN binary_mergeGB " << _new_block_path << endl;
		debug(_tmp.str());
	}
	priority_queue<btP*, vector<btP*>, cmpbTermPosting > tPQueue;
	vector<block_size>::iterator itV;
	const int _bs_sz = (int)_bs_vec.size();
	FILE* ifp_block[_bs_sz];
	FILE* ofp_block;
	{//open
		ofp_block = fopen(_new_block_path.c_str(), "wb");
		if(!ofp_block){
			cout << "failed open " << _new_block_path << endl;
			exit(0);
		}
		for(int i = 0; i < _bs_sz; i ++){
			ifp_block[i] = fopen(_bs_vec[i].block_path.c_str(), "rb");
			if(!ifp_block){
				cout << "failed open " << _bs_vec[i].block_path.c_str() << endl;
				exit(0);
			}
		}
	}
	int _new_block_size = 0;
	/*
	 *
	 */
	{//merge
		for(int i = 0; i < _bs_sz; i ++){
			tPQueue.push(this->getNextbPost(ifp_block[i]));
		}
		if(debug_2){
			stringstream _tmp;
			_tmp << "finish init " << _new_block_path << endl;
			debug(_tmp.str());
		}
		while(! tPQueue.empty()){
			btP* _top = tPQueue.top();
			this->bPOP(tPQueue);
			if(! tPQueue.empty())// be careful!!!
			{
				if(debug_2){
					stringstream _tmp;
					_tmp << "top term: " << _top->term << endl;
					debug(_tmp.str());
				}
				if(_top->term == "lear"){
					if(debug_2){
						stringstream _tmp;
						_tmp << "come the lear " << endl;
						debug(_tmp.str());
					}
				}
				while(_top->equal(tPQueue.top())){//merge the posting of the same term
					btP* _tmp_top = tPQueue.top();
					this->bPOP(tPQueue);
					if(debug_2){
						stringstream _tmp;
						_tmp << "[" << _top->term << ", " << _tmp_top->term << "]" << endl;
						debug(_tmp.str());
					}
					if(this->isIndexFile(_new_block_path))
					{
//						_top->merge_sort(_tmp_top);
						// only in vector space model, here is not_sort
						_top->merge_not_sort(_tmp_top);
					}
					else
					{
						_top->merge_not_sort(_tmp_top);
					}
					if(tPQueue.empty()){
						break;
					}
				}
			}
			if (this->isIndexFile(_new_block_path)) {
				long long int _offset = ftell(ofp_block);
				(this->kv_store)->setOffsetByTerm(_top->term, _offset);
				_new_block_size += _top->post->write(ofp_block);
			}
			else
			{
				_new_block_size = _top->write(ofp_block);
			}
		}//end while empty

	}
	{//clear
		fclose(ofp_block);
		for(int i = 0; i < _bs_sz; i ++){
			fclose(ifp_block[i]);
		}
		for(int i = 0; i < _bs_vec.size(); i ++){
			system(("rm -rf " + _bs_vec[i].block_path).c_str());
		}
		if(debug_2){
			stringstream _tmp;
			_tmp << "finish block: " << _new_block_path << endl;
			debug(_tmp.str());
		}
	}
	return _new_block_size;
}

ofstream _debug;

bool sortByFreqFunc(const Post& a, const Post& b)
{
	return a.frequent > b.frequent;
}
bool sortByIdFunc(const Post& a, const Post& b)
{
	return a.id < b.id;
}
bool sortWordByDis(const WordPair& a, const WordPair& b)
{
	return a.dis < b.dis;
}
bool sortWordBySim(const WordPair& a, const WordPair& b)
{
	return a.sim > b.sim;
}
bool sortWord(const WordPair& a, const WordPair& b)
{
	if(a.dis != b.dis)
		return a.dis < b.dis;
	return a.sim > b.sim;
}

void SPIMIManager::buildPostFromXml(string& _fpath, int _fid, map<string, Posting*>& _dic){
		set<string> token;
		news _ns(_fpath);
		stringstream _buf;
		_buf << _ns.title << endl;
		_buf << _ns.headline << endl;
		_buf << _ns.cont << endl;
		string line;
		int pos = 0;
		while(getline(_buf, line) != 0)
		{
			if(line.length() < 2) {
				continue;
			}
			vector<string> words;
			to_lower(line);
			split(words, line, is_any_of("-/,\" \t!'.?()\\"));
			for(int j = 0; j < (int)words.size(); j ++)
			{
				trim_if(words[j], is_punct());
				{
					//words[j] = stemming(words[j]);
					if(stopword::isStopWord(words[j])){
						continue;
					}
					token.insert(words[j]);
				}
				if(words[j].length() > 1)
				{
					if(_dic.find(words[j]) != _dic.end())
					{
						if(! _dic[words[j]]->find_and_addone(_fid, pos))
						{
							_dic[words[j]]->add_post(_fid, pos);
						}
					}
					else
					{
						Posting* temp = new Posting();
						temp->add_post(_fid, pos);
						_dic[words[j]] = temp;
					}
					pos ++;
				}
			}
		}//end while
		tmp_file_length = (int)token.size();
		xml_ii ++;
	}
