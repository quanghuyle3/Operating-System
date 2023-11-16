/**
 * @author Romuz Abdulhamidov
 * romuz.abdulhamidov@sjsu.edu
 * CS 149, San Jose State University
 * Assignment 5
 * Date Created: 11/06/2023
 * Last Modified: 11/11/2023
 *
 * Description: This module runs matrixmult_parallel of multiple matrices.
 *		It takes two or more matrix filepaths, and runs matrix multiplication on pairs
 *		of first filepath and all the rest filepaths. On the console input, the program
 *		takes more filepaths and passes them as input to the children processes.
 * Constraints:
 *	All matrices are 8x8
 *	At each input, only one filepath is passed
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#define FILENAME_SIZE 127

/**
 * fileExists() takes a filename and checks if it exists or not.
 * If file doesn't exit, it returns 0.
 */
int fileExists(char* fp) {
	FILE* f = fopen(fp, "r");
	if (f != NULL) {
		fclose(f);
		return 1;
	}
	return 0;
}

int main(int argc, char* argv[]) {
	// if less than 2 arguments are given, exit the program
	if (argc < 3) {
		fprintf(stderr, "error: must have at least 2 parameters.\n");
		return 1;
	}

	double start = clock();
	int main_out = dup(1);

	// Set up Parent-To-Child pipes
	const int pipeSize = argc - 2;
	int ptoc[pipeSize][2];
	for (int i = 0; i < pipeSize; i++) {
		if (pipe(ptoc[i]) == -1) {
			fprintf(stderr, "error piping!");
			return 1;
		}
	}

	// Initialize A text file
	char a[FILENAME_SIZE];
	strncpy(a, argv[1], sizeof(a));
	a[sizeof(a) - 1] = '\0';

	// Create a process for each W.txt
	for (int i = 0; i < pipeSize; i++) {
		char w[FILENAME_SIZE];
		strncpy(w, argv[i + 2], sizeof(w));
		w[sizeof(w) - 1] = '\0';

		int child_pid = fork();
		if (child_pid == -1) {
			fprintf(stderr, "error forking!");
			return 1;
		}

		if (child_pid == 0) {
			/* In Child Process */
			close(ptoc[i][1]);

			char out[FILENAME_SIZE];
			char err[FILENAME_SIZE];

			sprintf(out, "%d.out", getpid());
			sprintf(err, "%d.err", getpid());

			// Create file descriptors
			int fdout = open(out, O_RDWR | O_CREAT | O_APPEND, 0777);
			int fderr = open(err, O_RDWR | O_CREAT | O_APPEND, 0777);

			// Redirect stdout and stderr to PID.out and PID.err
			dup2(ptoc[i][0], 0);
			dup2(fdout, 1);
			dup2(fderr, 2);

			printf("Starting command %d: child %d pid of parent %d\n", i + 1, getpid(), getppid());

			// Flush stdout to output file
			fflush(stdout);

			if (execlp("./matrixmult_parallel", "./matrixmult_parallel", a, w, NULL) == -1) {
				fprintf(stderr, "execlp error\n");
				return 1;
			}
		}
	}

	/* In Parent Process */

	// Close read ends of Parent-To-Child pipes
	for (int i = 0; i < pipeSize; i++) {
		close(ptoc[i][0]);
	}

	// Capture consequent A.txt from STDIN
	while (fgets(a, sizeof(a), stdin) != NULL) {
		int n = strlen(a);
		a[n - 1] = '\0';

		// Write A filename to children
		for (int i = 0; i < pipeSize; i++) {
			write(ptoc[i][1], a, sizeof(a));
		}

		if (!fileExists(a)) {
			break;
		};
	}

	// Close Parent-to-Child write pipes
	const char endPipeStr[FILENAME_SIZE] = "END_OF_INPUT";
	for (int i = 0; i < pipeSize; i++) {
		write(ptoc[i][1], endPipeStr, sizeof(endPipeStr));
	}
	// Close Parent-to-Child write pipes
	for (int i = 0; i < pipeSize; i++) {
		close(ptoc[i][1]);
	}

	// Wait for child processes to complete
	for (int i = 0; i < pipeSize; i++) {
		// Get child process ID and exit code
		int child_status;
		int child_pid = wait(&child_status);

		// Open respective child output file
		char out[FILENAME_SIZE];
		sprintf(out, "%d.out", child_pid);
		int fdout = open(out, O_RDWR | O_CREAT | O_APPEND, 0777);
		dup2(fdout, 1);

		printf("Finished command %d: child %d pid of parent %d\n", i + 1, child_pid, getpid());
		printf("Exited with exitcode = %d\n", child_status);

		// Flush stdout to output file and close fdout
		fflush(stdout);
		close(fdout);
	}

	// Switch back to default stdout
	dup2(main_out, 1);

	double end = clock();
	printf("Program completed in: %.2lf milliseconds\n", (end - start) / (CLOCKS_PER_SEC / 1000.0));

	return 0;
}