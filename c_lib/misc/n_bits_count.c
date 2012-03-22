/*
 * Write an efficient C program to count the number of bits set in an
 * unsigned integer.
 * i/p     o/p
 * ====    ===
 * 0(00)    0
 * 1(01)    1
 * 2(10)    1
 * 3(11)    2
 * .....  ...
 */
void main(int argc, char* argv[])
{
	int mask,val,x,count=0;

	val = atoi(argv[1]);
	printf("val = %x\n",val);

	for(x=0,mask=1; x<sizeof(int)*8;x++, mask=mask<<1){
		if(val & mask)
			count++;
	}
	printf("count = %d\n",count);

	count =0;
	for(x=0; x<sizeof(int)*8; x++){
		if(val&0x1)
			count++;
		val =val >>1;
	}
	printf("count = %d\n",count);
}
