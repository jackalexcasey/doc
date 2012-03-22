/*
 * Rotate left value by n bits
 */


void main(int argc, char* argv[])
{
	int rotate_bit,val;

	val = atoi(argv[1]);
	rotate_bit = atoi(argv[2]);

	(val<<rotate_bit) | (val>>(8-rotate_bit));

}
