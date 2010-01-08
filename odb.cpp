#include <stdio.h>
#include <sys/timeb.h>
#include <list>
#include <vector>
#include <deque>

#include "odb.h"

using namespace std;

int main (int argc, char* argv[])
{
	struct timeb *start, *end;
	double duration;
	unsigned long i;
	struct odb_bank *bank;

	// list<char> s;
	// vector<char> s;
	deque<char> s;

	start = (struct timeb*)malloc(sizeof(struct timeb));
	end = (struct timeb*)malloc(sizeof(struct timeb));
	bank = odb_bank_init(1, 100000);

	ftime(start);

	for (i = 0 ; i < 1000000000 ; i++)
		//s.push_back(i);
		odb_bank_add(bank, (char*)(&i));

	ftime(end);

	duration = (end->time - start->time) + 0.001 * (end->millitm - start->millitm);

	printf("Bank contains %lu items, with positions currently at %lu and %lu.\nOperation completed in %fs.\n", bank->data_count, bank->posA, bank->posB, duration);

	return 0;
}