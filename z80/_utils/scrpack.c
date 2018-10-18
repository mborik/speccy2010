#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#define BYTE uint8_t

size_t packBlock(BYTE *dest, BYTE *src, int len)
{
	BYTE *psrc = src;
	BYTE *maxsrc = src + len;
	BYTE *pdest = dest;
	BYTE *maxdest = dest + len - 1;
	BYTE x, cnt = 0;

	// detect empty block
	x = *src;
	while (++psrc < maxsrc) {
		if (x != *psrc)
		break;
	}

	if (psrc == maxsrc) {
		*dest = x; // pack failed: empty block
		return 1;
	}

	// compression
	psrc = src;
	while (psrc < maxsrc && pdest < maxdest) {
		x = *psrc++;
		if (maxsrc - psrc >= 2 && x == *psrc && x == *(psrc + 1)) {
			// found 3 repeating bytes
			if (cnt > 0) {
				*(pdest - cnt) = (BYTE)((cnt - 1) | 0x80); // count
				pdest++;
				cnt = 0;
			}

			// finding of repeating section
			if (pdest < maxdest) {
				psrc += 2;

				// max 130 (= 3 + 127)
				while (psrc < maxsrc && x == *psrc && cnt < 127) {
					psrc++;
					cnt++;
				}

				*pdest++ = cnt; // count
				*pdest++ = x; // byte
				cnt = 0;
			}
		}
		else {
			*(++pdest) = x; // unpackable byte
			if (++cnt == 0x80) { // max 128
				*(pdest - cnt) = 0xFF;
				pdest++;
				cnt = 0;
			}
		}
	}

	if (cnt > 0 && pdest < maxdest + 1) {
		*(pdest - cnt) = (BYTE)((cnt - 1) | 0x80);
		pdest++;
	}

	// packed block length
	int packedLen = pdest - dest;
	if (packedLen < len && psrc == maxsrc)
		return packedLen;

	// compression unsuccessful - dest will be src
	memcpy(dest, src, len);
	return len;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
		exit(1);

	char filePath[256];
	strcpy(filePath, argv[1]);

	FILE *f = fopen(filePath, "rb");
	fseek(f, 0, SEEK_END);
	size_t inputSize = ftell(f);

	BYTE *src = (BYTE *) malloc(inputSize);
	BYTE *dst = (BYTE *) malloc(inputSize);

	rewind(f);
	fread(src, sizeof(BYTE), inputSize, f);
	fclose(f);

	size_t packed = packBlock(dst, src, inputSize);
	free(src);

	strcat(filePath, ".bin");
	f = fopen(filePath, "wb");
	fwrite(dst, sizeof(BYTE), packed, f);

	free(dst);
	fclose(f);
}
