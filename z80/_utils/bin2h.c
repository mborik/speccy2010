#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	if (argc < 4)
		exit(1);

	FILE *f = fopen(argv[1], "rb");
	fseek(f, 0, SEEK_END);
	size_t contentSize = ftell(f);

	uint8_t *content = (uint8_t *) malloc(contentSize);

	rewind(f);
	fread(content, sizeof(uint8_t), contentSize, f);
	fclose(f);

	//----
	printf("#ifndef %s\n", argv[3]);
	printf("#define %s\n\n", argv[3]);

	printf("const unsigned char %s[%u] = {\n", argv[2], contentSize);

	int i = 0;

	printf("\t");
	while (contentSize-- > 0) {
		printf("0x%02X, ", *content++);
		if (!(++i % 8) || (contentSize == 0 && i % 8)) {
			if (contentSize > 0) {
				printf("\n\t");
			}
		}
	}

	puts("\n};\n");
	puts("#endif");
}
