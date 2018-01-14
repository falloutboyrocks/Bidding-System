#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <string.h>
#include <assert.h>


/* problem */

// pipe
// hostie

typedef struct host{
	int listenfd;
	int writefd;
}Hostie;

typedef struct player{
	int rank;
	int score;
}Player;


int main(int argc, char *argv[])
{
	int host_num = atoi(argv[1]);
	int player_num = atoi(argv[2]);

	int scoreboard[player_num];
	for(int i = 0; i < player_num; i++) scoreboard[i] = 0;
	
	/* 0 for listen, 1 for speak */
	int ptc_fd[host_num][2];
	int ctp_fd[host_num][2];
	Hostie Host[host_num];
	

	
	// build up pipe
	for(int i = 0; i < host_num; i++)
	{
		pipe(ptc_fd[i]);
		pipe(ctp_fd[i]);
		Host[i].listenfd = ctp_fd[i][0];
		Host[i].writefd = ptc_fd[i][1];
		//printf("fd = %d %d %d %d\n", ptc_fd[i][0], ptc_fd[i][1], ctp_fd[i][0], ctp_fd[i][1]);
	}

	// build host
	pid_t pid[host_num];
	for(int i = 0; i < host_num; i++)
	{
		if((pid[i] = fork()) == 0)
		{
			/* host i */
			
			// set up pipes
			// close non-using pipes
			for(int j = 0; j < host_num; j++)
			{
				if(j == i) continue;
				else
				{
					close(ptc_fd[j][0]);
					close(ptc_fd[j][1]);
					close(ctp_fd[j][0]);
					close(ctp_fd[j][1]);
				}
			}
			
			// set up host listener
			close(ptc_fd[i][1]);
			dup2(ptc_fd[i][0], STDIN_FILENO);
			close(ptc_fd[i][0]);
			// set up host speaker
			close(ctp_fd[i][0]);
			dup2(ctp_fd[i][1], STDOUT_FILENO);
			close(ctp_fd[i][1]);


			/* now host use stdio to r/w to pipe */


			// run host
			char host_id[10];
			sprintf(host_id, "%d", i + 1);
			execl("host", "host", host_id, (char *) 0);	
		}
	}
	
	// Biddin' System
	// close none use pipe of bidding system
	for(int i = 0; i < host_num; i++)
	{
		close(ptc_fd[i][0]);
		close(ctp_fd[i][1]);
	}

	// Disttribute players using IO multiplexing
	
	fd_set master_set, read_set;
	for(int i = 0; i < 100; i++) FD_CLR(i, &master_set);
	for(int i = 0; i < host_num; i++)
		FD_SET(Host[i].listenfd, &master_set);

	int host_status[host_num];
	for(int i = 0; i < host_num; i++) host_status[i] = 1; /* 0 = not ready for new match, 1 = ready */

	for(int i = 0; i < player_num; i++)
		for(int j = i + 1; j < player_num; j++)
			for(int k = j + 1; k < player_num; k++)
				for(int l = k + 1; l < player_num; l++)
				{
					for(int checker = 0; checker < host_num; checker++)
						if(host_status[checker] == 1)
						{
							// write
							char buffer[100];
							sprintf(buffer, "%d %d %d %d\n", i + 1, j + 1, k + 1, l + 1);
							FILE *fp = fdopen(Host[checker].writefd, "w");
							assert(fp != NULL);
							fputs(buffer, fp);
							fflush(fp);
							fsync(fileno(fp));
							// set status
							host_status[checker] = 0;
							break;
						}

					for(int a = 0; a < host_num; a++)
						if(host_status[a]) continue;

					// else just have to wait for somebody to ready
					/* check out which match is over */
					memcpy(&read_set, &master_set, sizeof(master_set));
					select(100, &read_set, NULL, NULL, NULL);
						
					for(int fd = 0; fd < host_num; fd++)
					{
						if(FD_ISSET(Host[fd].listenfd, &read_set))
						{
							// handle read
							char buffer[4][50];
							int id; int rank;
							FILE* fp = fdopen(Host[fd].listenfd, "r");
							assert(fp != NULL);
							for(int time = 0; time < 4; time++)
							{
								fgets(buffer[time], 50, fp);
								sscanf(buffer[time], "%d %d", &id, &rank);	
								scoreboard[id - 1] += (4 - rank);
							}
							
							// set status
							host_status[fd] = 1;
						}
					}		
				}
	
	// wait for all process done
	for(int i = 0; i < host_num; i++)
		if(host_status[i] == 0)
		{
			char buffer[4][50];
			FILE *fp = fdopen(Host[i].listenfd, "r");
			assert(fp != NULL);
			int rank; int id;
			for(int time = 0; time < 4; time++)
			{
				fgets(buffer[time], 50, fp);
				sscanf(buffer[time], "%d %d", &id, &rank);
				scoreboard[id - 1] += (4 - rank);
			}
		}

	// send killer message
	char kill[30] = "-1 -1 -1 -1\n";
	for(int i = 0; i < host_num; i++)
	{
		FILE *fp = fdopen(Host[i].writefd, "w");
		assert(fp != NULL);
		fputs(kill, fp);
		fflush(fp);
		fsync(fileno(fp));
	}
		
	// wait
	int stat;
	for(int i = 0; i < host_num; i++)
		wait(&stat);
	
	// print outcome
	Player player[player_num];
	for(int i = 0; i < player_num; i++)
		player[i].score = scoreboard[i];

	int rank = 1; int out[player_num];
	for(int i = 0; i < player_num; i++) out[i] = 0;
	while(rank <= player_num)
	{
		int max = -1;
		int count = 0;
		for(int i = 0; i < player_num; i++)
		{
			if(out[i]) continue;
			if(player[i].score > max) max = player[i].score;
		}
		
		assert(max != -1);
		for(int i = 0; i < player_num; i++)
			if(max == player[i].score)
			{
				count++;
				out[i] = 1;
				player[i].rank = rank;
			}
		rank += count;
	}

	for(int i = 0; i < player_num; i++)
		printf("%d %d\n", i + 1, player[i].rank);

	return 0;
}
