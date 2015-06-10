#include <stdio.h>

void main()
{
    int i;
    double x = 1.0;
    for (i = 0; i < 10000000; ++i) {
        x += x / 2;
	x /= 2;
    }
    printf("%lf\n", x);
}
