/*
 * Based ondemonstrably correct Random Number Generator, using
 * a prime modulus multiplicative linear congruential generator.
 * See Communciations of the ACM, October 1988, Volume 31, Number 10,
 * page 1195 for the algorithm (* Integer Version 2 *), and the
 * method for demonstrating correct implementation.
 * This integer implementation assumes that MAXINT is >= 2**31 - 1.
 *
 * Paul Jackson
 * 25 Oct 88
 */


hash(int seed)
{
	int m = 2147483647;	/* 2**31 - 1 */
	int a = 16807;	/* Alternatives: 48271 or 69621 */
	int q = 127773;	/* Alternatives: 44488 or 30845 */
	int r = 2836;	/* Alternatives:  3399 or 23902 */

	int lo, hi, test;

	hi = seed/q;
	lo = seed%q;
	test = a*lo - r*hi;
	if (test > 0)
		seed = test;
	else
		seed = test + m;
	return seed;
}
