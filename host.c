#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>

typedef struct player{
	int id;
	int money;
	int wintimes;
	int rank;
}Player;

typedef struct bid{
	int index;
	int sum;
}Bid;

int main(int argc, char* argv[])
{
	/* Open FIFO */
	char filename[50] = "host";
	char fifo[10] = ".FIFO";
	strcat(filename, argv[1]); // file : host + hostid
	
	char readfile[100];
	char writefile[4][100];
	char alpha[4][5] = {"_A", "_B", "_C", "_D"};
	
	// form read file
	strcpy(readfile, filename);
	strcat(readfile, fifo);

	// form write file
	for(int i = 0; i < 4; i++)
	{
		strcpy(writefile[i], filename);
		strcat(writefile[i], alpha[i]);
		strcat(writefile[i], fifo);
	}
	


	/* Set up FIFO */

	for(int i = 0; i < 4; i++)
	{
		unlink(writefile[i]);
		mkfifo(writefile[i], 0777);
	}
	unlink(readfile);
	mkfifo(readfile, 0777);

	/* Fork Player */
	
	char random_key[4][10] = {"1111", "2222", "3333", "4444"};
	char player_index[4][5] = {"A", "B", "C", "D"};
	pid_t pid[4];
	FILE *wfp[4]; int fd;
	for(int i = 0; i < 4; i++)
	{
		
		if((pid[i] = fork()) == 0)
			execl("player", "player", argv[1], player_index[i], random_key[i], (char*) 0);
		else
		{
			fd = open(writefile[i], O_WRONLY);
			wfp[i] = fdopen(fd, "w");
		}
	}

	int time = 0;
	fd = open(readfile, O_RDONLY);
	FILE *rfp = fdopen(fd, "r");
	
	/* Start Match */
	while(1)
	{
	
		Player player[4];
		for(int i = 0; i < 4; i++)
		{	
			scanf("%d", &player[i].id);
			if(player[i].id == -1)
				exit(0);
			player[i].money = 0;
			player[i].wintimes = 0;
		}
		
		if(time > 0)
		{
			for(int i = 0; i < 4; i++)
			{
				if((pid[i] = fork()) == 0)
					execl("player", "player", argv[1], player_index[i], random_key[i], (char*) 0);
			
				else
				{
					fd = open(writefile[i], O_WRONLY);
					wfp[i] = fdopen(fd, "w");		// truncate
				}
			}
		}

		for(int game = 0; game < 10; game++)
		{
			// add money
			for(int i = 0; i < 4; i++)
				player[i].money += 1000;
		
			// tell player how much each other have
			char announce[100];
			sprintf(announce, "%d %d %d %d\n", player[0].money, player[1].money, player[2].money, player[3].money);
			for(int i = 0; i < 4; i++)
			{
				fputs(announce, wfp[i]);
				fflush(wfp[i]);
				fsync(fileno(wfp[i]));
			}
			
			// process each bid from players
			char buffer[4][100];
			Bid bid[4];
			
			for(int i = 0; i < 4; i++)
			{
				fgets(buffer[i], 100, rfp);
				char abcd[5];
				char key[10];
				int moneybid; int index = -1;
				sscanf(buffer[i], "%s %s %d", abcd, key, &moneybid);
				
				// match up index
				for(int j = 0; j < 4; j++)
				{
					if(strcmp(abcd, player_index[j]) == 0)
						index = j;
				}
				assert(index != -1);
				assert(strcmp(key, random_key[index]) == 0);
				
				bid[i].index = index;
				bid[i].sum = moneybid;
			}
			// find out who wins ( suppose somebody wins )
			// kick out duplicated bid
			int nondup[4] = {1, 1, 1, 1};
			for(int i = 0; i < 4; i++)
				for(int j = 0; j < 4; j++)
				{
					if(i == j) continue;
					if(bid[i].sum == bid[j].sum)
					{
						nondup[i] = 0;
						nondup[j] = 0;
					}
				}

			// who's the biggest
			int max = -1; int maxind = -1;
			for(int i = 0; i < 4; i++)
			{
				if(nondup[i] == 0) continue;
				if(bid[i].sum > max)
				{
					max = bid[i].sum;
					maxind = i;
				}
			}
			if(max == -1) continue;
			player[bid[maxind].index].wintimes++;
		}
		
		for(int i = 0; i < 4; i++)
			wait(NULL);
		// rank up players
		int rk = 1;	int out[4] = {0};
		while(rk <= 4)
		{
			int max = -1; 
			int count = 0;
			for(int i = 0; i < 4; i++)
			{
				if(out[i] == 1) continue;
				if(player[i].wintimes > max)
					max = player[i].wintimes;					
			}
			for(int i = 0; i < 4; i++)
				if(player[i].wintimes == max)
				{
					player[i].rank = rk;
					out[i] = 1;
					count++;
				}
			rk += count;
		}
		
		// printout to stdout
		for(int i = 0; i < 4; i++)
			printf("%d %d\n", player[i].id, player[i].rank);

		fflush(stdout);
		fsync(fileno(stdout));
		time++;
	}
}
