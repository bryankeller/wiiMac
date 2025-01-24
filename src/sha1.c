/*
SHA-1 in C
By Steve Reid <steve@edmweb.com>
100% Public Domain

Test Vectors (from FIPS PUB 180-1)
"abc"
  A9993E36 4706816A BA3E2571 7850C26C 9CD0D89D
"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
  84983E44 1C3BD26E BAAE4AA1 F95129E5 E54670F1
A million repetitions of "a"
  34AA973C D4C4DAA4 F61EEB2B DBAD2731 6534016F
*/

#define SHA1HANDSOFF

#include "string.h"
#include "sha1.h"
#include "bootmii_ppc.h"
#include "hollywood.h"
#include "malloc.h"
#include "types.h"

//should be divisibly by four
#define BLOCKSIZE 32

#define SHA_CMD_FLAG_EXEC (1<<31)
#define SHA_CMD_FLAG_IRQ  (1<<30)
#define SHA_CMD_FLAG_ERR  (1<<29)
#define SHA_CMD_AREA_BLOCK ((1<<10) - 1)

typedef struct {
	unsigned long state[5];
	unsigned long count[2];
	unsigned char buffer[64];
} SHA1_CTX;

typedef u32 sha1[5];

static void SHA1Transform(unsigned long state[5], unsigned char buffer[64]);
static void SHA1Transforml(unsigned long state[5], unsigned char buffer[64], u32 len);
static void SHA1Init(SHA1_CTX* context);
static void SHA1Update(SHA1_CTX* context, unsigned char* data, unsigned int len);
static void SHA1Final(unsigned char digest[20], SHA1_CTX* context);

static void SHA1Transform(unsigned long state[5], unsigned char buffer[64]) {
	SHA1Transforml(state, buffer, 1);
}

/* Hash a single 512-bit block. This is the core of the algorithm. */

static void SHA1Transforml(unsigned long state[5], unsigned char buffer[64], u32 len)
{
	/* Copy context->state[] to working vars */
	write32(SHA_H0, state[0]);
	write32(SHA_H1, state[1]);
	write32(SHA_H2, state[2]);
	write32(SHA_H3, state[3]);
	write32(SHA_H4, state[4]);

	static u32 num_blocks;
	num_blocks = len;

	// assign block to local copy which is 64-byte aligned
	static u8 block[64*BLOCKSIZE] ALIGNED(64);
	// for further improvments!
	// u8 *block = memalign(64, 64*num_blocks);
	memcpy(block, buffer, 64*num_blocks);

	// royal flush :)
	sync_after_write(block, 64*num_blocks);

	// tell sha1 controller the block source address
	write32(SHA_SRC, virt_to_phys(block));
	
	// tell sha1 controller number of blocks
	if (num_blocks != 0)
		num_blocks--;
	write32(SHA_CMD, (read32(SHA_CMD) & ~(SHA_CMD_AREA_BLOCK)) | num_blocks);

	// fire up hashing and wait till its finished
	write32(SHA_CMD, read32(SHA_CMD) | SHA_CMD_FLAG_EXEC);
	while (read32(SHA_CMD) & SHA_CMD_FLAG_EXEC);

	// free the aligned data
	// free(block);

	/* Add the working vars back into context.state[] */
	state[0] = read32(SHA_H0);
	state[1] = read32(SHA_H1);
	state[2] = read32(SHA_H2);
	state[3] = read32(SHA_H3);
	state[4] = read32(SHA_H4);
}


/* SHA1Init - Initialize new context */

static void SHA1Init(SHA1_CTX* context)
{
	// reset sha-1 engine
	write32(SHA_CMD, read32(SHA_CMD) & ~(SHA_CMD_FLAG_EXEC));
	while ((read32(SHA_CMD) & SHA_CMD_FLAG_EXEC) != 0);

	/* SHA1 initialization constants */
	context->state[0] = 0x67452301;
	context->state[1] = 0xEFCDAB89;
	context->state[2] = 0x98BADCFE;
	context->state[3] = 0x10325476;
	context->state[4] = 0xC3D2E1F0;
	context->count[0] = context->count[1] = 0;
}


/* Run your data through this. */

static void SHA1Update(SHA1_CTX* context, unsigned char* data, unsigned int len)
{
	unsigned int i, j;

	j = (context->count[0] >> 3) & 63;
	if ((context->count[0] += len << 3) < (len << 3)) 
		context->count[1]++;
	context->count[1] += (len >> 29);
	if ((j + len) > 63) {
		memcpy(&context->buffer[j], data, (i = 64-j));
		SHA1Transform(context->state, context->buffer);
		// try bigger blocks at once
		for ( ; i + 63 + ((BLOCKSIZE-1)*64) < len; i += (64 + (BLOCKSIZE-1)*64)) {
			SHA1Transforml(context->state, &data[i], BLOCKSIZE);
		}
		for ( ; i + 63 + (((BLOCKSIZE/2)-1)*64) < len; i += (64 + ((BLOCKSIZE/2)-1)*64)) {
			SHA1Transforml(context->state, &data[i], BLOCKSIZE/2);
		}
		for ( ; i + 63 + (((BLOCKSIZE/4)-1)*64) < len; i += (64 + ((BLOCKSIZE/4)-1)*64)) {
			SHA1Transforml(context->state, &data[i], BLOCKSIZE/4);
		}
		for ( ; i + 63 < len; i += 64) {
			SHA1Transform(context->state, &data[i]);
		}
		j = 0;
	}
	else i = 0;
	memcpy(&context->buffer[j], &data[i], len - i);
}


/* Add padding and return the message digest. */

static void SHA1Final(unsigned char digest[20], SHA1_CTX* context)
{
	unsigned long i, j;
	unsigned char finalcount[8];

	for (i = 0; i < 8; i++) {
		finalcount[i] = (unsigned char)((context->count[(i >= 4 ? 0 : 1)]
		>> ((3-(i & 3)) * 8) ) & 255);  /* Endian independent */
	}

	SHA1Update(context, (unsigned char *)"\200", 1);
	while ((context->count[0] & 504) != 448) {
		SHA1Update(context, (unsigned char *)"\0", 1);
	}
	SHA1Update(context, finalcount, 8);  /* Should cause a SHA1Transform() */
	for (i = 0; i < 20; i++) {
		digest[i] = (unsigned char)
		((context->state[i>>2] >> ((3-(i & 3)) * 8) ) & 255);
	}
	/* Wipe variables */
	i = j = 0;
	memset(context->buffer, 0, 64);
	memset(context->state, 0, 20);
	memset(context->count, 0, 8);
	memset(&finalcount, 0, 8);
#ifdef SHA1HANDSOFF  /* make SHA1Transform overwrite it's own static vars */
	SHA1Transform(context->state, context->buffer);
#endif
}

void SHA1(unsigned char *ptr, unsigned int size, unsigned char *outbuf) 
{
	SHA1_CTX ctx;
  
	SHA1Init(&ctx);
	SHA1Update(&ctx, ptr, size);
	SHA1Final(outbuf, &ctx);
}

static void SHA1TestIt(const char *str, u32 len)
{
	u8 *in = malloc(len*sizeof(u8));
	sha1 out;
	strlcpy((char*)in, str, len+1);

	u32 diff = read32(HW_TIMER);
	SHA1(in, len, (u8*) out);
	diff = read32(HW_TIMER)-diff;

	printf("SHA-1 (in hardware) says (H0|H1|H2|H3|H4): %08X|%08X|%08X|%08X|%08X this was calculatet in %02d (hollywood-)cycles\n", out[0], out[1], out[2], out[3], out[4], diff);
}


void SHA1TestCases(void)
{
	// expected sha1 hash of string "lasasdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5" (len = 3811)
	// 33ad104c4fd0de2e90edd6f6103de8304043ab20
	SHA1TestIt("lasasdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5asdfasghasfghahadghadghadfghasdgfasdgfafghasdfgasdfasdfasdfasdfasdfahadhjaer5yq35635uyjzgzdghad56q346aw456w46aw5", 3811);
	// expected sha1 hash of string "asdf" (len = 4)
	// 3da541559918a808c2402bba5012f6c60b27661c
	SHA1TestIt("asdf", 4);

	// expected sha1 hash of string "asdfasdfjalsdfjklasdkjflaksdjflaksdjflkasjdfklajsdfkljasldkfjlaskdjflkajsdfakljsdfklasjddflkjalskdjfasldfjlasdkjflaskjdflaksdf" (len = 126)
	// 74e1e9e5489d08dc65108aff3a21756e8fc269d2
	SHA1TestIt("asdfasdfjalsdfjklasdkjflaksdjflaksdjflkasjdfklajsdfkljasldkfjlaskdjflkajsdfakljsdfklasjddflkjalskdjfasldfjlasdkjflaskjdflaksdf", 126);
}

