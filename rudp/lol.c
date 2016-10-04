#include <stdio.h>
#define SIZE 10+1
int main(int argc, char const *argv[])
{
	
	char buf[SIZE] = "hahahahah";
	char expectation='0';
	expectation = (expectation == '0') ? '1': '0';
	printf("%c\n",expectation);
expectation = (expectation == '0') ? '1': '0';
	printf("%c\n",expectation);expectation = (expectation == '0') ? '1': '0';
	printf("%c\n",expectation);expectation = (expectation == '0') ? '1': '0';
	printf("%c\n",expectation);expectation = (expectation == '0') ? '1': '0';
	printf("%c\n",expectation);
	return 0;
}