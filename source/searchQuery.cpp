#include<iostream>
#include "InvertedIndex.h"

using namespace std;

int main(int argc,char ** argv)
{
	string line;
	InvertedIndex* index = new InvertedIndex("index", true);
//	index->readIndex("index"); // not for binary
	while(true)
	{
		cout << "input query: ";
		getline(cin, line);
		if(line == "/q")
			break;
		index->vectorSpaceSearch(line);
	}
	delete index;
	return 0;
}
