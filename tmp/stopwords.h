/*
 * stopwords.h
 *
 *  Created on: 2014-4-11
 *      Author: liyouhuan
 */

#ifndef STOPWORDS_H_
#define STOPWORDS_H_
#include<string>
#include<string.h>
#include<set>
#include<iostream>
using namespace std;

class stopword{
private:
	static set<string> swSet;
public:
	static bool isStopWord(const string& _str);

	static void initial();

};


#endif /* STOPWORDS_H_ */
