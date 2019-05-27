#ifndef INVERTEDINDEX_H_
#define INVERTEDINDEX_H_

#include<iostream>
#include<fstream>
#include<string>
#include<sys/types.h>
#include<dirent.h>
#include<sys/stat.h>
#include<stack>
#include<unistd.h>
#include<boost/algorithm/string.hpp>
#include<boost/unordered_map.hpp>
#include<boost/property_tree/ptree.hpp>
#include<boost/property_tree/xml_parser.hpp>
#include<boost/foreach.hpp>
#include<vector>
#include<string.h>
#include<sstream>
#include<string>
#include<queue>
#include<map>
#include<algorithm>
#include "Btree.h"
#include "KVstore.h"
#include "stopwords.h"


using namespace std;
using namespace boost::algorithm;

extern ofstream _debug;

typedef vector<string>::iterator itV;

class ID_Score{
public:
	int id;
	double score;

	ID_Score(int _id, double _score){
		id = _id;
		score = _score;
	}
};

class ScoreCMP{
public:
	bool operator()(ID_Score& _is1, ID_Score& _is2)const{
		return _is1.score > _is2.score;
	}
};

class TopK{
public:
	priority_queue<ID_Score, vector<ID_Score>, ScoreCMP > Qscore;
	static const int K = 100;
public:
	bool push(ID_Score& _is){
		if(K > (int)Qscore.size()){
			Qscore.push(_is);
			return true;
		}else{
			const ID_Score _tmp_is = Qscore.top();
			if(_tmp_is.score < _is.score){
				Qscore.pop();
				Qscore.push(_is);
			}
		}
		return false;
	}
};

class IDlist{
private:
	typedef unsigned char uchar;
	static const bool debug_mode = false;
	uchar* var;
	int len;
	int i_size;

	void log(string _str){
		if(debug_mode){
			cout << _str << endl;
		}
	}

	int calSpaceLen(int _id){
		if(_id == 0) return 1;
		int _len = 0;
		while(_id > 0){
			_len++;
			_id = _id >> 7;
		}
		return _len;
	}

	int calSize(){
		int _count = 0;
		for(int i = 0; i < len; i++){
			if(var[i]>=128){
				_count++;
			}
		}
		return _count;
	}

	int get_at_len(int _len){
		int n = 0;
		for(int i = _len; i < this->len; i++){
			if(this->var[i] < 128){
				n = (n << 7) + this->var[i];
			}
			else{
				n = (n << 7) + this->var[i]-128;
				break;
			}
		}
		return n;
	}

	bool encode(const vector<int>& _id_vec, uchar* _var){
		uchar* tmp_buf = NULL;
		
		int tmp_len = 20;
		tmp_buf = new uchar[tmp_len];
		memset(tmp_buf, 0, tmp_len*sizeof(uchar));
		
		int v_len = 0;
		int i_len = 0;
		vector<int>::const_iterator vcit;
		for(vcit = _id_vec.begin(); vcit != _id_vec.end(); vcit++){
			int _id = *vcit;
			i_len = 0;
			while(true){
				tmp_buf[i_len] = _id % 128;
				if(_id < 128) break;
				_id = _id >> 7;
				i_len++;
			}
			tmp_buf[0] += 128;
			for(int i = i_len; i >= 0; i--){
				_var[v_len] = tmp_buf[i];
				v_len++;
			}
		}
		if(v_len == len){
			return true;
		}
		log("bug encode");
		exit(0);
		return false;
	}

	bool decode(vector<int>& _id_vec, uchar* _var, int _v_len)const{
		/*
		 * here should not exist _id_vec.clear()
		 * see call of decode()
		 */
		int n = 0;
		for(int i = 0; i < _v_len; i++){
			if(_var[i] < 128){
				n = (n << 7) + _var[i];
			}
			else{
				n = (n << 7) + _var[i]-128;
				_id_vec.push_back(n);
				n = 0;
			}
		}
		return true;
	}

public:
	IDlist(){
		var = NULL;
		i_size = 0;
		len = 0;
	}

	IDlist(FILE* _fp){
		int _len = 0;
		fread(&_len, sizeof(int), 1, _fp);
		if(_len == 0){
			this->var = NULL;
			this->len = 0;
			this->i_size = 0;
		}
		else{
			unsigned char* tmp = new unsigned char[_len];
			fread(tmp, sizeof(char), _len, _fp);
			this ->var = tmp;
			this ->len = _len;
			this->i_size = this ->calSize();
		}
	}

	IDlist(const IDlist* _list){
		this->len = _list->var_len();
		if(this->len == 0){
			this->var = NULL;
			this->i_size = 0;
		}
		else{
			this ->var = new uchar[this->len];
			memcpy(this->var, _list->getvar(), this->len*(sizeof(char)));
			this->i_size = _list->size();
		}
	}

	~IDlist(){
		delete[] this->var;
	}

	IDlist(const vector<int>& _id_vec){ //压缩 VB编码
		this->len = 0;
		vector<int>::const_iterator vcit;
		for(vcit = _id_vec.begin(); vcit != _id_vec.end(); vcit++){
			this->len += this->calSpaceLen(*vcit);
		}
		this->i_size = (int)_id_vec.size();
		if(_id_vec.empty()){
			this->var = NULL;
			this->i_size = 0;
		}
		else{
			this->var = new uchar[this->len];
			memset(this->var, 0, (this->len)*sizeof(uchar));
			this ->encode(_id_vec, this->var);
		}
		if(debug_mode){
			this ->print();
		}
	}

	void merge_not_sort(const IDlist* _list){
		int _new_len = this->len + _list->var_len();
		uchar* _new_var = new uchar[_new_len];
		memcpy(_new_var, this->var, this->len*sizeof(char));
		memcpy((_new_var+this->len), _list->getvar(), _list->var_len()*sizeof(char));
		this->len += _list->var_len();
		delete[] this->var;
		this->var = _new_var;
	}

	int write(FILE * _fp){
		int _len = this->len;
		fwrite(&_len, sizeof(int), 1, _fp);
		fwrite(this->var, sizeof(char), _len, _fp);
		return _len+sizeof(int);
	}

	int size()const{
		return i_size;
	}

	const unsigned char* getvar()const{
		return var;
	}

	bstr* tobstr(){
		return new bstr((char*)this->var, this->len);
	}

	void merge_sort(const IDlist* _list){
		vector<int> _id_vec;
		this ->getVec(_id_vec);
		_list->getVec(_id_vec);
		sort(_id_vec.begin(), _id_vec.end());
		this->len += _list->var_len();
		delete[] this->var;
		this->var = new uchar[this->len];
		this->encode(_id_vec, this->var);
		this->i_size += _list->size();
	}

	int var_len()const{
		return len;
	}

	int operator[](int _i){
		if(_i >= this->size()){
			log("bug op_i");
			exit(0);
		}
		return this->get(_i);
	}

	int get(int _i){
		int _i_tmp = 0;
		int _len = 0;
		while(_i_tmp < _i && _len < this->len){
			if(this->var[_len] >= 128){
				_i_tmp++;
			}
			_len++;
		}
		return this->get_at_len(_len);
	}

	bool exist(int _id){
		int n = 0;
		for(int i = 0; i < this->len; i++){
			if(this->var[i] < 128){
				n = (n << 7) + this->var[i];
			}
			else{
				n = (n << 7) + this->var[i]-128;
				if(n == _id) return true;
				n = 0;
			}
		}
		return false;
	}

	void iprint(){
		for(int i = 0; i < i_size; i++){
			cout << (this->get(i)) << " ";
		}
		cout << endl;
	}

	void print(){
		for(int i = 0; i < len; i++){
			cout << (var[i]-0) << " ";
		}
		cout << endl;
	}

	void getVec(vector<int>& _id_vec)const{
		//here should not have _id_vec.clear()
		this ->decode(_id_vec, this->var, this->len);
	}
};


class binary_vectorPosting{
private:
	IDlist* posting;
	bool isNot;
	bool isAndResult;
public:
	binary_vectorPosting(){
		isNot = false;
		isAndResult = false;
		posting = NULL;
	}

	binary_vectorPosting(IDlist * _list, bool _and, bool _not){
		posting = _list;
		isAndResult = _and;
		isNot = _not;
	}

	void set(IDlist * _list, bool _and, bool _not){
		posting = _list;
		isAndResult = _and;
		isNot = _not;
	}

	const IDlist* getlist()const{
		return posting;
	}

	int get(int i){
		return (*posting)[i];
	}

	int length(){
		return posting->size();
	}
};

class news {
public:
	//newsitem
	string title;
	string headline;
	string cont;
	news(string _xml_path){
		using boost::property_tree::ptree; //boost 读xml
		ptree _pt;
		ptree _root;
		try{
			read_xml(_xml_path, _pt);
			_root = _pt.get_child("newsitem");
			title = (_root.get_child("title")).data();
			headline = (_root.get_child("headline")).data();
			{
				cont = "";
				ptree _text = _root.get_child("text");
				for(ptree::iterator itr = _text.begin(); itr != _text.end(); itr++){
					cont += (itr->second).data()+"\n";
				}
			}
		}
		catch (std::exception& e){
			cout << "exception: " << _xml_path << endl;
		}
	}
};
// docID frequent positionVec
class Post {
private:
	IDlist posList;
public:
	int id;
	vector<int> pos;
	int frequent;
	Post* next;

	void getLine(stringstream& _buf){
		_buf << id << " " << frequent;
		for(int i = 0; i < (int)pos.size(); i++){
			_buf << " " << pos[i];
		}
	}

	int getTF(){  //term frequency ^
		return (int)pos.size();
	}

	int getid(){
		return id;
	}

	Post(const string& _str){
		stringstream _buf;
		_buf << _str;
		_buf >> id;
		_buf >> frequent;
		int _tmp = -1;
		pos.clear();
		for(int i = 0; i < frequent; i++){
			_buf >> _tmp;
			pos.push_back(_tmp);
		}
		next = NULL;
	}

	Post(int _id, int _fre = 1){
		id = _id;
		frequent = _fre;
		next = NULL;
	}

	Post(Post& a, Post& b){ //merge
		id = a.id;
		int i = 0, j = 0;
		while(i < a.pos.size() && j < b.pos.size()){
			if(a.pos[i] < b.pos[j]){
				pos.push_back(a.pos[i]);
				i++;
			}
			else if(a.pos[i] == b.pos[j]){
				pos.push_back(a.pos[i]);
				i++;
				j++;
			}
			else{
				pos.push_back(b.pos[j]);
				j++;
			}
		}
		while(i < a.pos.size()){
			pos.push_back(a.pos[i]);
			i++;
		}
		while(j < b.pos.size()){
			pos.push_back(b.pos[j]);
			j++;
		}
		frequent = pos.size();
	}
};
// linked table of Post(doc_posList) ->Post ->Post...
class Posting {
private:
	Post* head;
public:
	Posting(){
		head = NULL;
	}
	Posting(vector<string>::const_iterator _b, vector<string>::const_iterator _e){
		if(_b != _e){
			head = new Post(*_b);
			_b++;
		}
		Post* _cur = head;
		while(_b != _e){
			_cur->next = new Post(*_b);
			_b++;
			_cur = _cur->next;
		}
	}
	int getAllIDs(vector<int>& _id_vec){
		Post* _cur = head;
		while(_cur != NULL){
			_id_vec.push_back(_cur->getid());
			_cur = _cur->next;
		}
		return (int)_id_vec.size();
	}
	int getAllIDs_TFs(vector<int>& _id_v, vector<int>& _tf_v){
		Post* _cur = head;
		while(_cur != NULL){
			_id_v.push_back(_cur->getid()); //doc_id
			_tf_v.push_back(_cur->getTF()); //in this doc tf 
			_cur = _cur->next;
		}
		return (int)_id_v.size();
	}
	void merge(Posting* _post){
		/*
		 * when merging, there will never be two Posting share the same docID
		 * for all posList of one doc have been collected together
		 */

		Post* next1 = head;
		Post* next2 = _post->get();
		Post* cur = NULL;
		if(next1->id < next2->id){
			head = next1;
			next1 = next1->next;
		}
		else{
			head = next2;
			next2 = next2->next;
		}
		cur = head;
		while(next1 != NULL && next2 != NULL){
			if(next1->id < next2->id){
				cur ->next = next1;
				next1 = next1->next;
				cur = cur ->next;
			}
			else{
				cur ->next = next2;
				next2 = next2 ->next;
				cur = cur ->next;
			}
		}
		cur->next = (next1 != NULL ? next1 : next2);
	}
	void getLine(stringstream& _buf){//no \n
		if(head != NULL){
			head ->getLine(_buf);
		}
		Post* _cur = head->next;
		while(_cur != NULL){
			_buf << "\t";
			_cur ->getLine(_buf);
			_cur = _cur->next;
		}
	}
	~Posting(){
		Post* _ptmp = NULL;
		Post* _pcur = head;
		while(_pcur != NULL){
			_ptmp = _pcur ->next;
			delete _pcur;
			_pcur = _ptmp;
		}
	}
	bool find(int des_id){
		Post* now = head;
		while(now != NULL){
			if(now->id == des_id)
				return true;
			else if(now->id < des_id)
				now = now->next;
			else
				return false;
		}
		return false;
	}
	bool find_and_addone(int des_id, int pos_p){
		Post* now = head;
		while(now != NULL){
			if(now->id == des_id){
				now->frequent++;
				now->pos.push_back(pos_p);
				return true;
			}
			else if(now->id < des_id)
				now = now->next;
			else
				return false;
		}
		return false;
	}
	void add_post(int des_id, int pos_p){
		Post* now = head;
		Post* pre = head;
		Post* temp = new Post(des_id);
		temp->pos.push_back(pos_p);
		while(now != NULL){
			if(now->id < des_id){
				pre = now;
				now = now->next;
			}
			else{
				temp->next = now;
				pre->next = temp;
				return;
			}
		}
		if(pre != NULL)
			pre->next = temp;
		else
			head = temp;
	}
	Post* get(){
		return head;
	}
};

class binaryPosting{
private:
	IDlist* docid_list; //doc_id
	IDlist* freq_list; //in this doc tf
public:
	binaryPosting(){
		docid_list = NULL;
		freq_list = NULL;
	}
	binaryPosting(const vector<int>& _id_vec){
		docid_list = new IDlist(_id_vec);
	}
	binaryPosting(const vector<int>& _id_vec, const vector<int>& _tf_vec){
		docid_list = new IDlist(_id_vec);
		freq_list = new IDlist(_tf_vec);
	}
	binaryPosting(FILE* _fp){
		docid_list = new IDlist(_fp);
		freq_list = new IDlist(_fp);
	}
	void calScore(map<int, double>& doc2score, int _docNum){
		double _tmp = (_docNum + 0.0)/(docid_list->size()+0.0);
		double _idf = log(_tmp)/ (log(2.0));
		vector<int> vid, vtf;
		docid_list->getVec(vid);
		freq_list->getVec(vtf);
		for(int i = 0; i < (int)vid.size(); i++){
			int _id = vid[i];
			int _tf = vtf[i];
			doc2score[_id] += _idf*_tf;
		}
	}
	void merge_not_sort(binaryPosting* _post){
		/*
		 * when merging, there will never be two Posting share the same docID
		 * for all posList of one doc have been collected together
		 */
		(this->docid_list)->merge_not_sort(_post->get_docid_list());
		(this->freq_list)->merge_not_sort(_post->get_freq_list());
	}
	void merge_sort(binaryPosting* _post){
		(this->docid_list)->merge_sort(_post->get_docid_list());
		/* remain!!
		typedef vector<int> vi;
		vi vid1, vid2, tf1, tf2;
		vi vid, tf;

		docid_list->getVec(vid1);
		freq_list->getVec(tf1);
		_post->get_docid_list()->getVec(vid2);
		_post->get_freq_list()->getVec(tf2);

		vi::iterator iit1, iit2, tit1, tit2;

		iit1 = vid1.begin();
		iit2 = vid2.begin();
		tit1 = tf1.begin();
		tit2 = tf2.begin();

		while(iit1 != vid1.end() && iit2 != vid2.end()){
		}
		*/
	}
	const IDlist* get_docid_list(){
		return docid_list;
	}
	const IDlist* get_freq_list(){
		return freq_list;
	}
	int write(FILE* _of){
		int _sz1 = docid_list->write(_of);
		int _sz2 = freq_list->write(_of);
		return _sz1+_sz2;
	}
	~binaryPosting(){
		delete docid_list;
		delete freq_list;
	}
	void printVec(vector<int>& _vec){
		cout << "[" << _vec[0];
		for(int i = 1; i < (int)_vec.size(); i++){
			cout << ", " << _vec[i];
		}
		cout << "]" << endl;
	}
	void print(){
		vector<int> _id_vec;
		docid_list->getVec(_id_vec);
		printVec(_id_vec);
	}
	bool exist(int des_id){
		return docid_list->exist(des_id);
	}
};


class block_size{
public:
	block_size(string _bp, int _sz){
		block_path = _bp;
		size = _sz;
	}
	string block_path;
	int size;
	bool operator < (const block_size& _bs)const{
		//the smaller size , the higher priority
		int priori = 0 - size;
		int _bs_priori = 0 - _bs.size;
		return priori < _bs_priori;
	}
};

class binary_termPosting{
public:
	string term;
	binaryPosting* post;
	FILE* fp;           // term    IDList IDList
	binary_termPosting(string _t, binaryPosting* _p, FILE* _f){
		term = _t;
		post = _p;
		fp = _f;
	}
	binary_termPosting(FILE* _f){
		fp = _f;
		int _len = 0;
		fread(&_len, sizeof(int), 1, _f);
		char* tmp = new char[_len+1];
		fread(tmp, sizeof(char), _len, _f);
		tmp[_len]='\0';
		term = string(tmp);
		post = new binaryPosting(_f);
		delete[] tmp;
	}
	bool equal(const binary_termPosting* _tp){
		return term == _tp->term;
	}
	void merge_not_sort(binary_termPosting* _tp){
		post->merge_not_sort(_tp->post);
	}
	void merge_sort(binary_termPosting* _tp){
		post->merge_sort(_tp->post);
	}
	int write(FILE* _of){
		int tmp_len = term.length();
		fwrite(&tmp_len, sizeof(int), 1, _of);
		fwrite(term.c_str(), sizeof(char), term.length(), _of);
		int _len = sizeof(int)+term.length();
		_len += post->write(_of);
		return _len;
	}
	~binary_termPosting(){

	}
	void print(){
		cout << "term: " << term << endl;
		post->print();
		cout << "finish print" << endl;
	}
};

typedef binary_termPosting btP;

class termPosting{
public:
	string _term;
	Posting* _post;
	ifstream* _fin;
	termPosting(string _t, Posting* _p, ifstream* _f){
		_term = _t;
		_post = _p;
		_fin = _f;
	}
	termPosting(stringstream& _buf, ifstream* _f){
		_fin = _f;
		vector<string> s_post;
		string _line = _buf.str();
		split(s_post, _line, is_any_of("\t"));
		_term = s_post[0];
		vector<string>::iterator it_b, it_e;
		it_b = s_post.begin();
		it_b++;//after the term is the first post
		it_e = s_post.end();
		_post = new Posting(it_b, it_e);
	}
	bool equal(const termPosting* _tp){
		return _term == _tp->_term;
	}
	void merge(termPosting* _tp){
		_post->merge(_tp->_post);
	}
	void toLine(stringstream& _buf){
		_buf << _term << "\t";
		_post ->getLine(_buf);
	}
	int write(ofstream& _of){
		stringstream _buf;
		this ->toLine(_buf);
		const string _line = _buf.str();
		_of << _line << endl;
		return _line.length();
	}
	~termPosting(){

	}
};

class cmpTermPosting{
public:
	bool operator()(termPosting*& tp1, termPosting*& tp2)const{
		return tp1 ->_term > tp2 ->_term;
	}
};

class cmpbTermPosting{
public:
	bool operator()(binary_termPosting*& tp1, binary_termPosting*& tp2)const{
		return tp1 ->term > tp2 ->term;
	}
};

class SPIMIManager{
private:
	static const bool debug_mode = false;
	static const bool debug_2 = false;
	static const bool test_kv = false;
	FILE* fp_index;
public:
	KVstore* kv_store;
	binary_vectorPosting* getvp(string _term){
		binaryPosting* _bp = kv_store->getPosting(_term, fp_index);
		if(_bp == NULL){
			return new binary_vectorPosting(new IDlist(), false, false);
		}
		IDlist* _list = new IDlist(_bp->get_docid_list());
		return new binary_vectorPosting(_list, false, false);
	}
	binaryPosting * getbp(string _term){
		binaryPosting* _bp = kv_store->getPosting(_term, fp_index);
		if(_bp == NULL){
			return NULL;
		}
		return _bp;
	}
	string getdoc(int _id){
		return kv_store->getDocByID(_id);
	}
	int getdLen(int _did){
		return kv_store->getLenByDid(_did);
	}
	const vector<string> *files;
	int nextFile;
	int nMergeBlock;
	int nFilesOneBlock;
	int mergeTmpID;
	string indexpath;
	string inputpath;
	string blockpath;
	priority_queue<block_size> blocks;
	boost::unordered_map<string, vector<string> > bigram_dic;

	SPIMIManager();
	SPIMIManager(vector<string>* _files,string _indexpath, string _inputpath, string _blockpath, bool _is_build_mode);
	SPIMIManager(vector<string> * _files, bool _is_build_mode);

	~SPIMIManager();


	vector<string>* getNextBlock(){
		if(nextFile == (int)files ->size()){
			return NULL;
		}
		vector<string>* _s_vec = new vector<string>();
		for(int i = 0; i < nFilesOneBlock; i++){
			_s_vec ->push_back((*files)[nextFile]);
			nextFile++;
			if(nextFile >= (int)files ->size()){
				break;
			}
		}
		return _s_vec;
	}

	void VSM_buildIndex(){
		vector<string>* _s_vec = NULL;
		int _fid = 0;
		int _block_id = 0;
		string doc_id = indexpath + "/doc_id.dat";
		ofstream fout(doc_id.c_str());
		if(fout == NULL){
			cout << (indexpath + "/doc_id.dat open failed!\n");
			return;
		}
		while((_s_vec = this ->getNextBlock()) != NULL){
			map<string, Posting*> _dic;

			//build the block
			for(itV it = _s_vec->begin(); it != _s_vec->end(); it++){
				string _file_name = *it;
				string _file_path = inputpath+"/"+_file_name;
				
				if(_file_path.find("xml") != string::npos){
					this->buildPostFromXml(_file_path, _fid, _dic);
				}
				else{
					this->buildPostFromFile(_file_path, _fid, _dic);
				}
				//cout << "file: " << _file_path << endl;
				this->kv_store->setDocByID(_fid, _file_name);
				this->kv_store->setLenByDid(_fid, this->tmp_file_length);
				fout << _fid << "\t" << _file_name << endl;
				_fid++;
				
			}
			
			map<string, binaryPosting*> term2bP; // term  IDlist IDlist
			
			map<string, Posting*>::iterator itM = _dic.begin();
			vector<int> _id_vec, _tf_vec;
			for(; itM != _dic.end(); itM++){
				_id_vec.clear();
				_tf_vec.clear();
				itM->second->getAllIDs_TFs(_id_vec, _tf_vec);
				binaryPosting* _bp = new binaryPosting(_id_vec, _tf_vec);
				term2bP.insert(pair<string, binaryPosting*>(itM->first, _bp));
			}
			
			//output block
			this ->binary_writeBlock(_block_id, term2bP);
			_block_id++;
			
			 //delete
			delete _s_vec;
			_s_vec = NULL;
			map<string, Posting*>::iterator itSP = _dic.begin();
			for(; itSP != _dic.end(); itSP++){
				delete (itSP ->second);
			}
			map<string, binaryPosting*>::iterator itSbP = term2bP.begin();
			for(; itSbP != term2bP.end(); itSbP++){
				delete (itSbP ->second);
			}
		}//end while
		
		fout.close();
		
		this ->binary_mergeBlocks();
		system(("rm -rf " + blockpath).c_str());
		
	}

	int xml_ii;
	int tmp_file_length;
	
    void buildPostFromXml(string& _fpath, int _fid, map<string, Posting*>& _dic){
		set<string> token;
		news _ns(_fpath);
		stringstream _buf;
		_buf << _ns.title << endl;
		_buf << _ns.headline << endl;
		_buf << _ns.cont << endl;
		string line;
		int pos = 0;
		while(getline(_buf, line) != 0){
			if(line.length() < 2){
				continue;
			}
			vector<string> words;
			to_lower(line);
			split(words, line, is_any_of("-/,\" \t!'.?()\\"));
			for(int j = 0; j < (int)words.size(); j++){
				trim_if(words[j], is_punct());
				//words[j] = stemming(words[j]);
				if(stopword::isStopWord(words[j])){
						continue;
					}
				token.insert(words[j]);

				if(words[j].length() > 1){
					if(_dic.find(words[j]) != _dic.end()){
						if(!_dic[words[j]]->find_and_addone(_fid, pos)){
							_dic[words[j]]->add_post(_fid, pos);
						}
					}
					else{
						Posting* temp = new Posting();
						temp->add_post(_fid, pos);
						_dic[words[j]] = temp;
					}
					pos++;
				}
			}
		}//end while
		tmp_file_length = (int)token.size();
		xml_ii++;
	}
	//
	void buildPostFromFile(string& _fpath, int _fid, map<string, Posting*>& _dic){
		set<string> token;
		ifstream fin(_fpath.c_str());
		string line;
		int pos = 0;
		while(getline(fin, line) != 0){
			vector<string> words;
			to_lower(line);
			split(words, line, is_any_of("-/, \t!'.?()\\"));
			for(int j = 0; j < words.size(); j++){
				trim_if(words[j], is_punct());
				//words[j] = stemming(words[j]);
				if(stopword::isStopWord(words[j])){
						continue;
					}
				if(words[j].length() > 1){
					token.insert(words[j]);
					if(_dic.find(words[j]) != _dic.end()){
						if(!_dic[words[j]]->find_and_addone(_fid, pos)){
							_dic[words[j]]->add_post(_fid, pos);
						}
					}
					else{
						Posting* temp = new Posting();
						temp->add_post(_fid, pos);
						_dic[words[j]] = temp;
					}
					pos++;
				}
			}
		}//end while
		fin.close();
		tmp_file_length = (int)token.size();
	}
	//add into priority
	void binary_writeBlock(int _bid, map<string, binaryPosting*> _dic){
		string blockfilePath = this->getBlockPath(_bid);
		FILE* _fp = fopen(blockfilePath.c_str(), "wb");
		if(_fp == NULL){
			cout << (blockfilePath + " open failed!\n") << endl;
			return;
		}
		typedef map<string, binaryPosting*> ms2p;
		int _write_size = 0;
		for(ms2p::iterator iter = _dic.begin(); iter != _dic.end(); iter++){
			btP _btp(iter->first, iter->second, NULL); //binary_termPosting
			_write_size += _btp.write(_fp);
		}
		fclose(_fp);
		blocks.push(block_size(blockfilePath, _write_size)); //p queue
	}

	bstr* getNext_bstr(FILE* _fp){
		if(feof(_fp)){
			return NULL;
		}
		return new bstr(_fp);
	}

	btP* getNextbPost(FILE* _fp){
		if(feof(_fp)){
			return NULL;
		}
		btP* _btp = new btP(_fp);
		return _btp;
	}

	termPosting* getNextPost(ifstream* _fin){
		string _line;
		getline(*_fin, _line);
		if(_line.length() < 2){
			return NULL;
		}
		stringstream _buf;
		_buf << _line;
		return new termPosting(_buf, _fin);
	}

	void binary_mergeBlocks();

	string getIndexPath(){
		return indexpath+"/index.dat";
	}
	bool isIndexFile(string _merge_path){
		return _merge_path == (indexpath+"/index.dat");
	}

	vector<block_size>* nextMergeBlocks(int _n){

		if(blocks.empty()){
			return NULL;
		}
		vector<block_size>* _bs_vec = new vector<block_size>();
		while(_n > 0 && !blocks.empty()){
			_bs_vec->push_back(blocks.top());
			blocks.pop();
			_n--;
		}

		return _bs_vec;
	}

	string nextMergePath(){
		stringstream _ss;
		_ss << blockpath << "/merge" << mergeTmpID;
		mergeTmpID++;
		return _ss.str();
	}

	void debug(string _str){
		if(debug_mode || debug_2){
			_debug << _str << endl;
			_debug.flush();
		}
	}

	int binary_mergeGroupBlocks(vector<block_size> & _bs_vec, string _new_block_path);


	void bPOP(priority_queue<btP*, vector<btP*>, cmpbTermPosting >& _q){
		btP* _top = _q.top();
		_q.pop();
		//read next Post from the relative Block after every pop!
		btP* _tmp = this->getNextbPost(_top->fp);
		if(_tmp != NULL){
			_q.push(_tmp);//the new push will never equal the top term
		}
	}


	string getBlockPath(int _bid){
		stringstream _ss;
		_ss << blockpath << "/" << "block" << _bid;
		return _ss.str();
	}
};

class WordPair{
public:
	string s;
	int f;
	int dis;
	double sim;
};

extern bool sortByFreqFunc(const Post& a, const Post& b);
extern bool sortByIdFunc(const Post& a, const Post& b);
extern bool sortWordByDis(const WordPair& a, const WordPair& b);
extern bool sortWordBySim(const WordPair& a, const WordPair& b);
extern bool sortWord(const WordPair& a, const WordPair& b);


class VectorPosting {
private:
	vector<Post> posting;
	bool isNot;
	bool isAndResult;
public:
	VectorPosting(){
		isNot = false;
		isAndResult = false;
		posting.clear();
	}
	Post& get(int i){
		return posting[i];
	}
	int length(){
		return posting.size();
	}
	void push_back(Post post){
		posting.push_back(post);
	}
	vector<Post>& get_posting(){
		return posting;
	}
	void display(){
		for(int i = 0; i < length(); i++)
			cout << posting[i].id << " " << posting[i].frequent << endl;
		if(length() == 0)
			cout << "NONE\n";
		else
			cout << endl;
	}
};


class InvertedIndex{
private:
	boost::unordered_map<string, Posting*> dic;
	boost::unordered_map<string, VectorPosting> order_dic;
	boost::unordered_map<int, string> doc_id;
	boost::unordered_map<int, string> term_id;
	boost::unordered_map<string, vector<string> > bigram_dic;
	VectorPosting full_posting;
	string indexpath;
	string inputpath;
	SPIMIManager *spimi;
	static const bool ii_debug1 = false;
	static const bool ii_debug2 = true;
	static const int sk = 44;
	static const int rcv = 806791;
	//static const int rcv = 322;

	int editDistance(string& a, string& b){
		int dis[30][30];
		if(a.length() > 29 || b.length() > 29)
			return 3;
		for(int i = 0; i < 30; i++)
			for(int j = 0; j < 30; j++)
				dis[i][j] = 0;
		for(int i = 0; i <= a.length(); i++)
			dis[i][0] = i;
		for(int i = 0; i <= b.length(); i++)
			dis[0][i] = i;
		for(int i = 1; i <= a.length(); i++)
			for(int j = 1; j <= b.length(); j++){
				dis[i][j] = dis[i-1][j-1];
				if(a[i-1] != b[j-1])
					dis[i][j]++;
				if(dis[i][j-1] < dis[i][j])
					dis[i][j] = dis[i][j-1] + 1;
				if(dis[i-1][j] < dis[i][j])
					dis[i][j] = dis[i-1][j] + 1;
			}
		return dis[a.length()][b.length()];
	}
	double similarity(string a, string b){
		vector<char> va, vb;
		for(int i = 0; i < a.length(); i++)
			va.push_back(a[i]);
		for(int i = 0; i < b.length(); i++)
			vb.push_back(b[i]);
		sort(va.begin(), va.end());
		sort(vb.begin(), vb.end());
		int ll = 0, rr = 0;
		double same = 0;
		while(ll < va.size() && rr < vb.size()){
			if(va[ll] == vb[rr]){
				same = same + 1;
				ll++; rr++;
			}
			else if(va[ll] < vb[rr])
				ll++;
			else
				rr++;
		}
		if(a.length() + b.length() != 0)
			return same/(a.length()+b.length());
		return 0;
	}
	vector<string> candidate(string& word){
		vector<WordPair> v;
		string str = "$" + word + "$";
		string sub;
		boost::unordered_map<string, int> temp;
		for(int i = 0; i < str.length()-1; i++){
			sub = str.substr(i, 2);
			if(bigram_dic.find(sub) != bigram_dic.end()){
				vector<string> u = bigram_dic[sub];
				for(int i = 0; i < u.size(); i++){
					if(temp.find(u[i]) != temp.end())
						temp[u[i]] = temp[u[i]] + 1;
					else
						temp[u[i]] = 1;
				}
			}
		}
		//filter by bigram
		typedef boost::unordered_map<string, int> uumap;
		int f = 1;
		if(word.length() == 4)
			f = 2;
		if(word.length() > 4)
			f = 3;
		for(uumap::iterator iter = temp.begin(); iter != temp.end(); iter++){
			//cout << i << endl;
			WordPair t;
			t.s = iter->first;
			t.f = iter->second;
			if(t.f >= f){
				t.dis = editDistance(t.s, word);
				if(t.dis < 3){
					t.sim = similarity(t.s, word);
					v.push_back(t);
					if(v.size() > 300)
						break;
				}
			}
		}
		sort(v.begin(), v.end(), sortWord);
		vector<string> ans;
		for(int i = 0; i < v.size(); i++){
			if(i == 3 || v[i].dis > 2)
				break;
			ans.push_back(v[i].s);
		}

		return ans;
	}
	void correct(string w){
		vector<string> v = candidate(w);
		cout << "For " + w + " do you mean:";
		for(int i = 0; i < v.size(); i++)
			cout << " " + v[i];
		cout << endl;
	}

public:
	InvertedIndex(string dirname){
		dic.clear();
		indexpath = dirname;
	}
	InvertedIndex(string dirname, bool _is_search){
		indexpath = dirname;
		spimi = new SPIMIManager();
	}
	~InvertedIndex(){

	}

	VectorPosting& getFullPosting(){
		return full_posting;
	}

	vector<int> fullvec;
	binary_vectorPosting* getFullbPosting(){

		if(fullvec.empty()){
			for(int i = 0; i < rcv; i++){
				fullvec.push_back(i);
			}
		}
		return new binary_vectorPosting(new IDlist(fullvec), false, false);
	}

	static const bool debug_mode = false;
	static const bool debug_iv2 = true;
	void readIndex(string path) {
		string line;
		ifstream fin;

		doc_id.clear();
		full_posting = *(new VectorPosting());
		fin.open((path+"/doc_id.dat").c_str());
		if(fin == NULL){
			cout << (path+"/doc_id.dat" + " open failed!\n");
			return;
		}
		while(getline(fin, line) != 0){
			if(line.length() < 2)
				break;
			vector<string> words;
			split(words, line, is_any_of("\t"));
			int id = atoi(words[0].c_str());
			doc_id[id] = words[1];
			Post* t = new Post(id, 0);
			full_posting.push_back(*t);
		}
		fin.close();

		order_dic.clear();
		fin.open((path+"/index.dat").c_str());
		if(fin == NULL){
			cout << (path+"/index.dat" + " open failed!\n");
			return;
		}
		int _ii = 0;
		while(getline(fin, line) != 0){
			vector<string> words;
			split(words, line, is_any_of(" \t"));
			VectorPosting* temp = new VectorPosting();
			order_dic[words[0]] = *temp;
			for(int j = 1; j < words.size(); j++){
				int id = atoi(words[j].c_str());
				j++;
				int fre = atoi(words[j].c_str());
				Post* t = new Post(id, fre);
				for(int i = 0; i < fre; i++){
					j++;
					t->pos.push_back(atoi(words[j].c_str()));
				}
				order_dic[words[0]].push_back(*t);
				if(debug_mode && _ii == 17){
					stringstream _buf;
					t->getLine(_buf);
					cout << "[" <<words[0] << "]\t[" << _buf.str() << "]" << endl;
				}
			}

			_ii++;
		}
		fin.close();

		cout << "Read index over.\n";
	}

	VectorPosting& find(string key) {
		if(order_dic.find(key) != order_dic.end()){
			return order_dic[key];
		}
		else
			return *(new VectorPosting());
	}

	binary_vectorPosting * bfind(string _term){
		return spimi->getvp(_term);
	}

	binaryPosting* bfind_bp(string _term){
		return spimi->getbp(_term);
	}


	void SPIMI_binarybuild(vector<string>& inFileName){
		string blockpath = "block"; //files    "index"   source
		spimi = new SPIMIManager(&inFileName, indexpath, inputpath, blockpath, true);
		system(("rm -rf "+indexpath).c_str());
		system(("rm -rf "+blockpath).c_str());
		checkDir(indexpath);
		checkDir(blockpath);
		spimi->VSM_buildIndex();

		delete spimi;
	}

	void checkDir(string _path){
		if(opendir(_path.c_str()) == NULL)
			mkdir(_path.c_str(), 0755);
		else
			cout << _path << " exits!\n";
	}

	vector<string>& getAllFiles(string path, vector<string>& container=*(new vector<string>())) { // 遍历目录所有文件
		vector<string> &ret = container;
		inputpath = path;
		struct dirent* ent = NULL;
		DIR *pDir;
		pDir = opendir(path.c_str());
		if (pDir == NULL){
			return ret;
		}
		while((ent = readdir(pDir)) != NULL){
			if(ent->d_type == 8){
				string filename(ent->d_name);
				ret.push_back(filename);
			}
		}
		return ret;
	}

	void bgetDoc(binary_vectorPosting* posting){ // 文档id对应到文档名
		cout << "There are " << posting->length() << " document(s) fit the query.\n";
		//posting.sortByFreq();
		for(int i = 0; i < posting->length(); i++){
			int id = posting->get(i);
			string _doc = spimi->getdoc(id);
			cout << i+1 <<"\t[" << id << ", " << _doc << "]" << endl;
		}
		cout << endl;
	}

	void vectorSpaceSearch(string query){
		to_lower(query);
		vector<string> tokens;
		split(tokens, query, is_any_of("\t "));
		map<int, double> doc2score;
		for(int i = 0; i < (int)tokens.size(); i++){
			if(stopword::isStopWord(tokens[i])){
						continue;
					}
			binaryPosting* _bp = this->bfind_bp(tokens[i]);
			if(_bp != NULL){
				_bp->calScore(doc2score, InvertedIndex::rcv);
			}
		}

		map<int, double>::iterator mit = doc2score.begin();
		TopK topK;
		for(; mit != doc2score.end(); mit++){
			int _len = this->spimi->getdLen(mit->first);
			if(_len <= 0) {
				continue;
			}
			mit->second /= (_len+0.0);
			ID_Score _is(mit->first, mit->second);
			topK.push(_is);
		}
		priority_queue<ID_Score, vector<ID_Score>, ScoreCMP > qScore = topK.Qscore;
		
		int _count = qScore.size();
		system("rm -rf /Users/youkunli/a/result");
		checkDir("/Users/youkunli/a/result");
		int _flag = 0;
		if(!qScore.empty())
			system("open /Users/youkunli/a/result");
		while(!qScore.empty()){
			const ID_Score _is = qScore.top();
			string _doc = spimi->getdoc(_is.id);
			cout <<_count<< ": ["<<_doc<<", " << _is.id<<"] "<< _is.score << endl;
			system(("cp /Users/youkunli/Downloads/allrcv1xml/"+_doc+" /Users/youkunli/a/result/"+std::to_string(_count)+"_"+_doc).c_str());
			//system(("cp /Users/youkunli/a/Information-Retrival/19970301/"+_doc+" /Users/youkunli/a/result/"+std::to_string(_count)+"_"+_doc).c_str());
			_count--;
			qScore.pop();
		}
	}
};

#endif

