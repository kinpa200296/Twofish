#ifndef Twofish_h
#define Twofish_h

#define BLOCK_SIZE 256

#define KEY_SIZE_INVALID -1
#define KEY_SIZE_SMALL 128
#define KEY_SIZE_MEDIUM 192
#define KEY_SIZE_LARGE 256

#define RESULT_SUCCESS 0

#define to_bytes(a) a/8

typedef unsigned char byte;
typedef unsigned short int sword;
typedef unsigned int word;

int encrypt(byte *data, size_t block_count, byte *key, size_t key_size);
int decrypt(byte *data, size_t block_count, byte *key, size_t key_size);

#endif
