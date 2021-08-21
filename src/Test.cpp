#include <iostream>
#include <stdio.h>

using namespace std;

#define AD "sdgae"

#define CD(ab) (AD)+string("abdcq") +"'" #ab "'"



int main()
{
	string s = string(getenv("flag"));
	if(!s.empty())
	cout<<"Success:"<<s<<endl;
}