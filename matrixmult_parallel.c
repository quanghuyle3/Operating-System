/**
 * @author Romuz Abdulhamidov
 * romuz.abdulhamidov@sjsu.edu
 * CS 149, San Jose State University
 * Assignment 5
 * Date Created: 11/06/2023
 * Last Modified: 11/11/2023
 *
 * Description: This module implements matrix multiplication on two matrices, but does so
 *		in parallel. A child process is spawned for each row, and the resulting matrix is
 *		appended to the R matrix. The program waits for more text files to be entered, replaces
 *		the first text file given, computes the product of the two matrices, and appends it to
 *		R again. It does so in a loop until the parent process stops passing it files. The R
 *		matrix reallocates more memory each time a file is being passed.
 * Constraints:
 *	A is a 8x8 matrix
 *	W is a 8x8 matrix
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_SIZE 511
#define FILENAME_SIZE 127
#define ROWS 8
#define COLS 8
#define FILES_LIMIT 31

 /**
  * populateMatrix() takes a filepath and a matrix of integers,
  * reads from the file, and populates the matrix.
  */
int populateMatrix(char* fp, int matrix[ROWS][COLS]) {
	FILE* f = fopen(fp, "r");
	if (f == NULL) {
		return 1;
	}

	char buffer[BUFFER_SIZE];

	// row
	int r = 0;
	// iterate file line by line
	while (fgets(buffer, sizeof(buffer), f) != NULL && r < ROWS) {
		// c tracks matrix column
		int c = 0;

		// Temporary value is used to convert a character numbers to an int
		int temp = 0;
		// isTemp is a bool to track temp value. It is false if the previous character is not a number
		bool isTemp = false;

		// Iterate a buffer (line) character by character
		for (int i = 0; i < strlen(buffer) && c < COLS; i++) {
			// if character is digit, keep track with temp
			if (buffer[i] >= '0' && buffer[i] <= '9') {
				temp *= 10;
				temp += buffer[i] - '0';
				isTemp = true;
			}
			// if char is not digit, but temp was tracked, then temp is the int to use in matrix
			else if (isTemp) {
				matrix[r][c] = temp;
				c++;

				// Reset temporary value and boolean
				isTemp = false;
				temp = 0;
			}
		}

		// if last char in a line was a digit
		if (isTemp) {
			matrix[r][c] = temp;
			c++;
		}
		r++;
	}

	fclose(f);

	return 0;
}

/**
 * printMatrix() takes a matrix and prints it to stdout.
 */
void printMatrix(int matrix[ROWS][COLS]) {
	for (int i = 0; i < ROWS; i++) {
		for (int j = 0; j < COLS; j++) {
			int num = matrix[i][j];
			if (num < 10) {
				printf(" %d ", num);
			}
			else {
				printf("%d ", num);
			}
		}
		printf("\n");
	}
}

/**
 * clearMatrix() takes a matrix and sets all of its elements to 0.
 */
void clearMatrix(int matrix[ROWS][COLS]) {
	for (int i = 0; i < ROWS; i++) {
		for (int j = 0; j < COLS; j++) {
			matrix[i][j] = 0;
		}
	}
}

void freeMatrix(int** matrix, int rows) {
	for (int i = 0; i < rows; i++) {
		free(matrix[i]);
	}
	free(matrix);
}

int main(int argc, char* argv[]) {
	// number of arguments must be 2
	if (argc != 3) {
		fprintf(stderr, "error: must have exactly 1 parameters. \n");
		return 1;
	}

	// Declare variable to store matrices
	int a[ROWS][COLS] = { 0 };
	int w[ROWS][COLS] = { 0 };

	// Get the filepaths for A & W
	char* fpa = argv[1];
	char* fpw = argv[2];

	// Populate matrices
	if (populateMatrix(fpa, a) != 0) {
		fprintf(stderr, "error: cannot open a file %s\n", fpa);
		return 1;
	};
	if (populateMatrix(fpw, w) != 0) {
		fprintf(stderr, "error: cannot open a file %s\n", fpw);
		return 1;
	};

	printf("%s = [ \n", argv[1]);
	printMatrix(a);
	printf("]\n");

	printf("%s = [ \n", argv[2]);
	printMatrix(w);
	printf("]\n");

	// Flush stdout before forking
	fflush(stdout);

	// Allocate memory for matrix
	int resRows = ROWS;
	int** res = (int**)malloc(ROWS * sizeof(int*));
	if (res == NULL) {
		fprintf(stderr, "error: cannot allocate memory for R matrix");
		return 1;
	}

	for (int i = 0; i < ROWS; i++) {
		res[i] = (int*)malloc(COLS * sizeof(int));
		// Free memory if there is error
		if (res[i] == NULL) {
			freeMatrix(res, i);
			fprintf(stderr, "error: cannot allocate memory for rows of R matrix");
			return 1;
		}
	}
	
	/* Compute A * W */
	// Set up pipes for each row
	int pipes[ROWS][2];
	for (int i = 0; i < ROWS; i++) {
		if (pipe(pipes[i]) == -1) {
			freeMatrix(res, ROWS);
			fprintf(stderr, "Error piping\n");
			return 1;
		}
	}

	// Create Forks
	for (int i = 0; i < ROWS; i++) {
		int pid = fork();
		if (pid == -1) {
			fprintf(stderr, "Error forking");
			freeMatrix(res, ROWS);
			return 1;
		}

		// If child process
		if (pid == 0) {
			// Close read end of the pipe
			close(pipes[i][0]);

			int result[COLS] = { 0 };

			// Multiply rows
			for (int j = 0; j < COLS; j++) {
				result[j] = 0;
				for (int k = 0; k < ROWS; k++) {
					result[j] += a[i][k] * w[k][j];
				}
			}

			write(pipes[i][1], &result, sizeof(result));

			// Close write end of the pipe and exit child process
			close(pipes[i][1]);

			return 0;
		}
	}

	/* In Parent Process */
	// Wait for processes to complete
	for (int i = 0; i < ROWS; i++) {
		wait(NULL);
	}

	int r = 0;
	for (; r < ROWS; r++) {
		// Close write end of the pipe
		close(pipes[r][1]);

		int result[ROWS] = { 0 };
		read(pipes[r][0], &result, sizeof(result));

		// Close read end of the pipe
		close(pipes[r][0]);

		// Populate matrix row
		for (int j = 0; j < COLS; j++) {
			res[r][j] = result[j];
		}
	}

	char name[FILENAME_SIZE];
	while (read(0, name, sizeof(name)) > 0) {
		// break out of loop if it is END_OF_INPUT
		if (strcmp(name, "END_OF_INPUT") == 0) {
			break;
		}
		clearMatrix(a);
		if (populateMatrix(name, a) != 0) {
			freeMatrix(res, resRows);
			fprintf(stderr, "error: cannot open a file %s\n", name);
			return 1;
		};

		// Create pipes
		int pipes[ROWS][2];
		for (int i = 0; i < ROWS; i++) {
			if (pipe(pipes[i]) == -1) {
				freeMatrix(res, resRows);
				fprintf(stderr, "Error piping\n");
				return 1;
			}
		}

		// Create Forks
		for (int i = 0; i < ROWS; i++) {
			int pid = fork();
			if (pid == -1) {
				fprintf(stderr, "Error forking");
				freeMatrix(res, ROWS);
				return 1;
			}

			// If child process
			if (pid == 0) {
				// Close read end of the pipe
				close(pipes[i][0]);

				int result[COLS] = { 0 };

				// Multiply rows
				for (int j = 0; j < COLS; j++) {
					result[j] = 0;
					for (int k = 0; k < ROWS; k++) {
						result[j] += a[i][k] * w[k][j];
					}
				}

				write(pipes[i][1], &result, sizeof(result));

				// Close write end of the pipe and exit child process
				close(pipes[i][1]);

				return 0;
			}
		}

		/* In Parent Process */
		// Reallocate more space for the matrix
		resRows += ROWS;
		int** temp = (int**)realloc(res, resRows * sizeof(int*));
		if (temp == NULL) {
			fprintf(stderr, "error: cannot reallocate memory for matrix R");
			freeMatrix(res, resRows - ROWS);
			return 1;
		}
		res = temp;

		for (int i = resRows - ROWS; i < resRows; i++) {
			res[i] = (int*)malloc(COLS * sizeof(int));
			// Free memory if there is error
			if (res[i] == NULL) {
				freeMatrix(res, i);
				fprintf(stderr, "error: cannot allocate memory for rows of R matrix");
				return 1;
			}
		}

		// wait for children
		for (int i = 0; i < ROWS; i++) {
			wait(NULL);
		}

		for (; r < resRows; r++) {
			// Close write end of the pipe
			close(pipes[r % ROWS][1]);

			int result[ROWS] = { 0 };
			read(pipes[r % ROWS][0], &result, sizeof(result));

			// Close read end of the pipe
			close(pipes[r % ROWS][0]);

			// Populate matrix row
			for (int j = 0; j < COLS; j++) {
				res[r][j] = result[j];
			}
		}
	}

	printf("R = [ \n");
	for (int i = 0; i < resRows; i++) {
		for (int j = 0; j < COLS; j++) {
			printf(" %d ", res[i][j]);
		}
		printf("\n");
	}
	printf("]\n");

	freeMatrix(res, resRows);

	fflush(stdout);
	fflush(stderr);

	return 0;
}