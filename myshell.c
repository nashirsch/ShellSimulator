#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h> 
#include <fcntl.h>

#define MAXLINE 514

//prints out standard error message
void errorout(){
	char error_message[30] = "An error has occurred\n";
	write(STDOUT_FILENO, error_message, strlen(error_message));
}

//runs cd according to arguments
void cd(char *c, int i){
	if(i == 1)
	{
		if(chdir(getenv("HOME")) != 0){
			errorout();
		}
	}
	else
		if(chdir(c) != 0){
			errorout();
		}
}

//runs print system call
void myPrint(char *msg)
{
    write(STDOUT_FILENO, msg, strlen(msg));
}

//manually create new process for non-built in function
void notbuilt(char** argv, int i, int r, char* redi){
	int status;
	int fd;
	int count;
	char buf[5000];
	pid_t ret = getpid();
	if((ret = fork()) == 0)
	{
		if(r == 1)
		{
			fd = creat(redi, 0666);
			dup2(fd, STDOUT_FILENO);
			close(fd);
		}
		if(r == 2)
		{
			fd = open(redi, O_RDWR);
			count = read(fd, buf, 5000);
			dup2(fd, STDOUT_FILENO);
		}
		if(execvp(argv[0], argv) < 0){
			errorout();
		}
		if(r == 2)
		{
			write(fd, buf, count);
			close(fd);
		}
		exit(0);
	}
	else
		waitpid(ret, &status, 0);
	return;
}

//process a given command string, tokenizes into args
void cmdProcessor(char* cmd){
	int i = 0;
	int n = 0;
	int r = 0;
	int error = 0;
	char *ind;
	char *tok = cmd;
	char *dirCheck;
	char *new = strdup(cmd);
	char pwd[512];
	char *argv[50];
	char *redi[50];
	FILE *file;
	struct stat st;

	//tokenizes arguments in command before possible redirection argument
	int d = 1;
	while((*tok != '\0') && (*tok != '>')){
		if((d == 1) && ((*tok != ' ') && (*tok != '\t')))
		{
			argv[i] = tok;
			i++;
		}

		d = 0;

		if((*tok == ' ') || (*tok == '\t'))
		{
			d = 1;
			*tok = '\0';
		}

		tok++;
	}

	if(*tok == '>')
	{
		*tok = '\0';
	}

	//check if there is redirection
	if((ind = strchr(new, '>')) != NULL){
		r = 1;
		if(*(ind+1) == '+'){
			r = 2;
		}
	}

	//check if redirection is properly specified
	if(r > 0)
	{
		d = 1;
		tok = ind + 1;
		if(r == 2)
			tok++;

		while(*tok && ((*tok == ' ')||(*cmd == '\t')||(*tok == '\n')||(*tok == ';')))
			tok++;
		
		while(*tok != '\0'){
			if((d == 1) && ((*tok != ' ') && (*tok != '\t') && (*tok != '\n') && (*tok != ';')))
			{
				redi[n] = tok;
				n++;
			}

			d = 0;

			if((*tok == ' ') || (*tok == '\t') || (*tok == '\n') || (*tok == ';'))
			{
				d = 1;
				*tok = '\0';
			}

			tok++;
		}

		//classify r and error according to redirection specification
		if(n != 1){
			error = 1;
		}
		else{
			file = fopen(redi[0], "r");
			if((r == 1) && (file != NULL))
				error = 1;
			if((r == 2) && (file == NULL))
				r = 1;
		}

		if(error != 1){
			dirCheck = strdup(redi[0]);
			char *pos = strrchr(dirCheck, '/');
			if (pos != NULL) {
   				*pos = '\0';
				if(stat(dirCheck, &st) < 0){
					error = 1;
				}
			}
		}
	}

	argv[i] = NULL;

	if(argv[0] == NULL){
		if(r != 0){
			errorout();
			return;
		}
		else
			return;
	}

	if(error > 0)
	{
		errorout();
		return;
	}
	else if(0 == strcmp(argv[0], "exit")){
		if (i != 1){
			errorout();
			return;
		}
		else if (r != 0){
			errorout();
			return;
		}
		else
			exit(0);
	}
	else if(0 == strcmp(argv[0], "cd")){
		if(i > 2){
			errorout();
			return;
		}
		else if(r != 0){
			errorout();
			return;
		}
		else
			cd(argv[1], i);
	}
	else if(0 == strcmp(argv[0], "pwd")){
		if (r != 0){
			errorout();
			return;
		}
		else if(i > 1){
			errorout();
			return;
		}
		else{
			getcwd(pwd, 512);
			strcat(pwd, "\n");
			myPrint(pwd);
		}
	}
	else
		notbuilt(argv, i, r, redi[0]);
	
	return;
}

//process a given line, tokenizes into commands
void lineProcessor(char* line, int sig){
	if(strlen(line) >= MAXLINE){
		myPrint(line);
		errorout();
		return;
	}

	char *c = ";\n";
	char *cmd;

	if(sig == 1){
		myPrint(line);
	}

	cmd = strtok(line, c);

	while(cmd != NULL){
		while(*cmd && ((*cmd == ' ')||(*cmd == '\t')))
			cmd++;
		cmdProcessor(cmd);

		cmd = strtok(NULL, c);
	}
	return;
}

//wrapper function for batch mode
void batchWrapper(char* fp){
	char cmd_buff[5000];
    char *pinput;

	FILE *file = fopen(fp, "r");
	if(file == NULL){
		errorout();
		return;
	}
	while(!feof(file)){
		pinput = fgets(cmd_buff, 5000, file);
		if (!pinput) {
            exit(0);
        }

        pinput = cmd_buff;
        while((*pinput == ' ') || (*pinput == '\t'))
        	pinput++;

        if((pinput[0] != '\n') && (pinput[0] != '\0')){
        	lineProcessor(cmd_buff, 1);
        }
	}
}


int main(int argc, char *argv[]) 
{
    char cmd_buff[5000];
    char *pinput;

    if(argc == 2)
    {
    	batchWrapper(argv[1]);
    	exit(0);
    }

    if((argc > 2) || (argc < 1))
    {
    	errorout();
    	exit(0);
    }

    while (1) {
        myPrint("myshell> ");
        pinput = fgets(cmd_buff, 5000, stdin);
        if (!pinput) {
            exit(0);
        }
        if(strcmp(cmd_buff,"")){
        	lineProcessor(cmd_buff, 0);
        }
    }
}








