#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
	/* Generate filename */
	char filename[100] = "host";
	strcat(filename, argv[1]);
	
	char fifo[10] = ".FIFO";
	char readfile[200]; char writefile[200];
	strcpy(readfile, filename);
	strcat(readfile, fifo);
	strcpy(writefile, filename);
	strcat(writefile, "_");
	strcat(writefile, argv[2]);
	strcat(writefile, fifo);

	FILE* rfp = fopen(writefile, "r");
	FILE* wfp = fopen(readfile, "w");
	// find codename
	int code = -1;
	char alpha[4][5] = {"A", "B", "C", "D"};
	for(int i = 0; i < 4; i++)
		if(strcmp(alpha[i], argv[2]) == 0)
			code = i;
	assert(code != -1);

	for(int game = 0; game < 10; game++)
	{
		char rbuf[100];
		char wbuf[100];
		fgets(rbuf, 100, rfp);
		int bid[4];
		for(int i = 0; i < 4; i++)
			sscanf(rbuf, "%d", &bid[i]);
		if(game == 0 || game == 1) // should bid whatever the player had
			sprintf(wbuf, "%s %s 0\n", argv[2], argv[3]);
		else
			sprintf(wbuf, "%s %s 1250\n", argv[2], argv[3]);
		fputs(wbuf, wfp);
		fflush(wfp);
		fsync(fileno(wfp));
	}
}
