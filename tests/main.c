#include <stdio.h>

int x = 10;

int pow2(int x) {
	return x * x;
}

int pow3(int x) {
	return x * x * x;
}

int pow4(int x) {
	return x * x * x * x;
}

int main(void) {

	printf("%d\n", pow2(x));
	printf("%d\n", pow3(x));
	printf("%d\n", pow4(x));

	return 0;
}
