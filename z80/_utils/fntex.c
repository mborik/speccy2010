#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#define BYTE  uint8_t
#define WORD  uint16_t
#define DWORD uint32_t
//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
#pragma pack(push, 1)
	typedef struct FNT_HEADER {
		WORD  version;
		DWORD filesize;
		char  copyright[60];
		WORD  type;
		WORD  pointsize;
		WORD  vertres;
		WORD  hortres;
		WORD  ascent;
		WORD  internal_leading;
		WORD  external_leading;
		BYTE  italic;
		BYTE  underline;
		BYTE  strikeout;
		WORD  weight;
		BYTE  charset;
		WORD  width;
		WORD  height;
		BYTE  pitchfamily;
		WORD  avgwidth;
		WORD  maxwidth;
		BYTE  firstchar;
		BYTE  lastchar;
		BYTE  defchar;
		BYTE  breakchar;
		WORD  widthbytes;
		DWORD deviceoffset;
		DWORD faceoffset;
		DWORD bitspointer;
		DWORD bitsoffset;
		BYTE  mbz1;
		DWORD flags;
		WORD  aspace;
		WORD  bspace;
		WORD  cspace;
		DWORD coloroffset;
	} FNT_HEADER;
#pragma pack(pop)

	if (argc < 2)
		exit(1);

	char filePath[256];
	strcpy(filePath, argv[1]);

	size_t len = 0;
	BYTE *fontData = NULL;
	FILE *f = fopen(filePath, "rb");
	FNT_HEADER *fnt = (FNT_HEADER *) malloc(sizeof(FNT_HEADER));

	if (fread(fnt, sizeof(FNT_HEADER), 1, f) == 1 &&
		fnt->version == 0x300 && fnt->type == 0 &&
		fnt->italic == 0 && fnt->underline == 0 &&
		fnt->strikeout == 0 && fnt->width <= 8 &&
		fnt->avgwidth == fnt->width && fnt->maxwidth == fnt->width &&
		fnt->flags == 0x11 && fnt->coloroffset == 0) {

		len = fnt->height * ((fnt->lastchar - fnt->firstchar) + 1);
		fontData = (BYTE *) malloc(len);

		fseek(f, fnt->bitsoffset, SEEK_SET);
		if (fread(fontData, sizeof(BYTE), len, f) < len) {
			printf("Possibly corrupted font resource file\n");
			free(fnt);
			fclose(f);
			exit(2);
		}
	}

	free(fnt);
	fclose(f);

	if (fontData == NULL) {
		printf("Can't load font resource file\n");
		exit(1);
	}

	for (DWORD i = 0; i < len; i++)
		fontData[i] >>= 2;

	strcat(filePath, ".bin");
	f = fopen(filePath, "wb");
	fwrite(fontData, sizeof(BYTE), len, f);

	free(fontData);
	fclose(f);
}
