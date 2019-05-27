#include<iostream>
#include<vector>
#include "InvertedIndex.h"

using namespace std;

int main(int argc,char ** argv){
	char path[5000];
	//cout << "argc = " << argc << endl;
	if(argc < 2){
		cout << argv[0]  << " input_directory\n";
		return -1;
	}
	sscanf(argv[1], "%s", path);
	vector<string> files;
	string spath(path);
	InvertedIndex* index = new InvertedIndex("index");
	index->getAllFiles(spath, files);
	cout << "There are " << files.size() << " files..." << endl;
	cout << "There are " << (int)sqrt(files.size()) << " blocks" << endl;
	index->SPIMI_binarybuild(files);
	
	delete index;
	return 0;
}
