#include <stdio.h>

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

	printf("%d\n", pow2(10));
	printf("%d\n", pow3(10));
	printf("%d\n", pow4(10));

	return 0;
}
