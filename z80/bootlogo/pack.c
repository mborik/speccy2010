#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#define BYTE  uint8_t

int packBlock(BYTE *dest, BYTE *src, int len)
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

void displayMemory(BYTE *address, int length)
{
	BYTE ch;
	int i = 0;
	printf("\t");
	while (length-- > 0) {
		printf("0x%02X, ", *address++);
		if (!(++i % 8) || (length == 0 && i % 8)) {
			if (length > 0) {
				printf("\n\t");
			}
		}
	}

	puts("");
}

int main()
{
	BYTE src[6912], dst[6912];

	FILE *f = fopen("speccy2010.logo.scr", "rb");
	fread(src, sizeof(BYTE), 6912, f);
	fclose(f);

	int total = packBlock(dst, src, 6912);

	puts("#ifndef SHELL_LOGO_H_INCLUDED");
	puts("#define SHELL_LOGO_H_INCLUDED\n");

	printf("const unsigned char logoPacked[%d] = {\n", total);
	displayMemory(dst, total);

	puts("};\n");
	puts("#endif");
}
