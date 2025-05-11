// header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
// global declarations
#define max_para_length 200
#define max_file_line_length 200
#define max_attempts 10
// structure to store difficulty
typedef struct
{
    int easy;
    int medium;
    int hard;
} Difficulty;
// structure to store typing statistics
typedef struct
{
    double typingSpeed;
    double accuracy;
    int wrongChars;
    char paragraph[max_para_length];
} TypingStats;
// function to generate a random paragraph
char *getRandomParagraph(FILE *file)
{
    char line[max_file_line_length];
    int numParas = 0;

    while (fgets(line, sizeof(line), file) != NULL) // while this is true read from the file till u reach new line character
    {
        if (line[0] != '\n')
        {
            numParas++; // counts the no of paragraphs in the file
        }
    }
    // case when the file is empty
    if (numParas == 0)
    {
        perror("Error: No paragraphs found in the file.\n");
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_SET); // This function resets the file position to the beigning of the file

    int randomIndex = rand() % numParas; // A random index is generated

    for (int i = 0; i < randomIndex; i++) // the paragrphs are read again from the start to the randomly generated index
    {
        if (fgets(line, sizeof(line), file) == NULL) // if the randomly generated index does not contain any paragraphs
        {
            perror("Error reading paragraph from file.\n");
            exit(EXIT_FAILURE);
        }
    }

    size_t len = strlen(line);            // length of the randomly generated para is stored in this
    if (len > 0 && line[len - 1] == '\n') // last line of the string is appended with '\0'.or new line character is removed
    {
        line[len - 1] = '\0';
    }

    char *paragraph = strdup(line); // The paragraph is duplicated using strdup to allocate memory dynamically.
    if (paragraph == NULL)          // If memory could not be allocated dynamically
    {
        perror("Memory allocation error");
        exit(EXIT_FAILURE);
    }

    return paragraph; // return the randomly generated para
}
// function to calculate the typing statistics
void printTypingStats(double elapsedTime, const char *input, const char *correctText, Difficulty difficulty, TypingStats *stats)
{
    int correctCount = 0, wrongCount = 0;                                                   // initially no of wrong and right counts are initilized to 0
    int minLen = strlen(correctText) < strlen(input) ? strlen(correctText) : strlen(input); // this finds the minimum length of the input and correct striing
    // checking and updating correct and wrong counts
    for (int i = 0; i < minLen; i++)
    {
        if (correctText[i] == input[i])
        {
            correctCount++;
        }
        else
        {
            wrongCount++;
        }
    }

    int totalCharacters = minLen;                                   // total no of chacaters is stored in this variavble
    double accuracy = (double)correctCount / totalCharacters * 100; // accuracy is calculated

    double typingSpeed = (totalCharacters / 5.0) / (elapsedTime / 60.0); // assuming each letter has a min of 5 characters...calculating the typing speed per min hence (/60)
    // stastical values are updated in the structure and stored
    stats->typingSpeed = typingSpeed;
    stats->accuracy = accuracy;
    stats->wrongChars = wrongCount;
    strncpy(stats->paragraph, correctText, max_para_length);
}
// display function for previous attempts
void displayPreviousAttempts(TypingStats attempts[], int numAttempts)
{
    printf("\nPrevious Attempts:\n");
    printf("--------------------------------------------------------\n");
    printf("| Attempt | Typing Speed (cpm) | Accuracy | Wrong Chars |\n");
    printf("--------------------------------------------------------\n");

    for (int i = 0; i < numAttempts; i++)
    {
        printf("|   %2d    |        %.2f         |  %.2f   |     %2d      |\n", i + 1, attempts[i].typingSpeed, attempts[i].accuracy, attempts[i].wrongChars);
    }

    printf("--------------------------------------------------------\n");
}
// function to prompt difficulty
void promptDifficulty(Difficulty *difficulty)
{
    int choice;
    // ask the user for choice
    printf("Select difficulty level:\n1. Easy\n2. Medium\n3. Hard\n");

    while (1)
    {
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1) // if a character is entered instead of no
        {
            perror("Invalid input. Please enter a number.\n");
            while (getchar() != '\n')
                ;
            continue;
        }

        while (getchar() != '\n')
            ;

        if (choice >= 1 && choice <= 3) // if th e choice is btween this range come out of the function
        {
            break;
        }
        else
        {
            perror("Invalid choice. Please enter a number between 1 and 3.\n");
        }
    }

    switch (choice)
    {
    case 1:
        *difficulty = (Difficulty){5, 8, 12};
        break; // based on the choice difficulty level is decided
    case 2:
        *difficulty = (Difficulty){8, 12, 16};
        break; // These values represent the speed requirements for each difficulty level.
    case 3:
        *difficulty = (Difficulty){12, 16, 20};
        break;
    default:
        printf("Invalid choice. Using default difficulty level (Easy).\n"); // default is easy
        *difficulty = (Difficulty){5, 8, 12};
    }
}
// fucntion to store process attempts
void processAttempts(FILE *file)
{

    srand((unsigned int)time(NULL)); // seed value is set to unsigned int time

    printf("Welcome to Typing Tutor!\n"); // welcome message
    // local variable declarations
    char input[max_para_length];
    Difficulty difficulty;
    TypingStats attempts[max_attempts];
    int numAttempts = 0; // initially no of attempts is set to 0

    promptDifficulty(&difficulty); // this function lets the user chose the difficulty level

    while (1)
    {
        char *currentPara = getRandomParagraph(file); // a random para is stored in current para variable

        printf("\nType the following paragraph:\n%s\n", currentPara); // the random para is displayed

        clock_t startTime = clock(); // time is started
        printf("Your input: \n");
        fflush(stdout); // fflush() is typically used for output stream only. Its purpose is to clear the output buffer and move the buffered data to console

        if (fgets(input, sizeof(input), stdin) == NULL) // error readiing the input
        {
            perror("Error reading input"); // The C library function void perror(const char *str) prints a descriptive error message to stderr.
            exit(EXIT_FAILURE);
        }

        clock_t endTime = clock();                                           // time stops
        double elapsedTime = (double)(endTime - startTime) / CLOCKS_PER_SEC; // elapsed time is calculated per sec

        // Prevent unrealistically short elapsed time
        if (elapsedTime < 1.0) {
            printf("\n⚠️  Typing too fast or pasted input detected. Please type the paragraph manually.\n");
            printf("This attempt will not be recorded.\n");
            free(currentPara);
            continue;
        }

        // Check if input is empty
        if (strlen(input) == 0) {
            printf("\n⚠️  No input detected. Please type something.\n");
            free(currentPara);
            continue;
        }


        size_t len = strlen(input); // length of  the inputed para is stored in this
        if (len > 0 && input[len - 1] == '\n')
        {
            input[len - 1] = '\0'; // string is appended with '\0' and new line character is removed
        }

        TypingStats currentAttempt;
        printTypingStats(elapsedTime, input, currentPara, difficulty, &currentAttempt); // function to calculate typing stats is called

        printf("\nTyping Stats for Current Attempt:\n");
        printf("--------------------------------------------------------\n");
        printf("Typing Speed: %.2f characters per minute\n", currentAttempt.typingSpeed);
        printf("Accuracy: %.2f%%\n", currentAttempt.accuracy);
        printf("Wrong Characters: %d\n", currentAttempt.wrongChars);
        printf("--------------------------------------------------------\n");

        // Evaluate performance based on difficulty
        if (currentAttempt.typingSpeed >=    difficulty.hard) {
            printf("Performance Level: 🟢 Excellent (Hard)\n");
        } else if (currentAttempt.typingSpeed >= difficulty.medium) {
            printf("Performance Level: 🟡 Good (Medium)\n");
        } else if (currentAttempt.typingSpeed >= difficulty.easy) {
            printf("Performance Level: 🟠 Basic (Easy)\n");
        } else {
            printf("Performance Level: 🔴 Below Easy - Keep practicing!\n");
    }


        attempts[numAttempts++] = currentAttempt; // last 10 attempts are stored here

        if (numAttempts >= max_attempts)
        {
            displayPreviousAttempts(attempts, numAttempts);
            printf("\nMaximum attempts reached. Exiting...\n");
            break;
        }

        printf("\nDo you want to continue? (y/n): "); // asks the user if we wants to continue
        char choice[3];
        if (fgets(choice, sizeof(choice), stdin) == NULL)
        {
            perror("Error reading choice");
            exit(EXIT_FAILURE);
        }

        if (tolower(choice[0]) != 'y') // If the arguments passed to the tolower() function is other than an uppercase alphabet, it returns the same character that is passed to the function.
        {
            displayPreviousAttempts(attempts, numAttempts);
            printf("\nThanks for using Typing Tutor!\n");
            break;
        }
    }

    fclose(file); // file is closed
}
// main function
int main()
{
    FILE *file = fopen("paragraphs.txt", "r"); // the name of the file is stored in this file variable
    if (file == NULL)
    {
        perror("Error opening file 'paragraphs.txt'");
        return 1;
    }

    processAttempts(file);

    return 0;
}
