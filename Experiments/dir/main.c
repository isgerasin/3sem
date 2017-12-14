#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>


int main(int argc, char const *argv[])
{
		if (argc != 2)
	{
		fprintf(stderr, "Usage %s <dir>\n", argv[0]);
		return -1;
	}
	DIR* dir = opendir(argv[1]);
	if (dir == NULL)
	{
		fprintf(stderr, "Bad path\n");
		return -1;
	}

	size_t countFile = 0;
	size_t countJostkiiFile = 0;
	struct dirent* nowDirent = NULL;
	printf("Directories in %s :\n", argv[1]);
	while( nowDirent = readdir(dir) )
	{
		if( nowDirent->d_type == DT_DIR )
		{
			printf("\\%s\n", nowDirent->d_name);
			continue;
		}
		if ( nowDirent->d_type == DT_REG )
			countJostkiiFile++;
		if ( nowDirent->d_type == DT_FIFO || nowDirent->d_type == DT_LNK || nowDirent->d_type == DT_REG)
			countFile++;
	}
	printf("Number of files in directory : %lu\n", countFile);
	printf("Number of jestkii files in directory : %lu\n", countFile - countJostkiiFile);

	if ( closedir(dir) )
	{
	fprintf(stderr, "Can't close DIR\n");
	return -1;
	}
	return 0;
}

