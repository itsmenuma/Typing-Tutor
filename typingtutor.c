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

// Define named constants for difficulty levels
#define EASY_SPEED 5
#define MEDIUM_SPEED 8
#define HARD_SPEED 12
#define EASY_MEDIUM_SPEED 8
#define MEDIUM_HARD_SPEED 12
#define HARD_MAX_SPEED 16

// structure to store user profile
typedef struct {
    char username[50];
    double bestSpeed;
    double bestAccuracy;
    double totalSpeed;
    double totalAccuracy;
    int totalAttempts;
    int currentStreak; // New field to track the current streak
    int bestStreak;    // New field to track the best streak
} UserProfile;

// structure to store difficulty
typedef struct {
    int easy;
    int medium;
    int hard;
} Difficulty;

// structure to store typing statistics
typedef struct {
    double typingSpeed;
    double accuracy;
    int wrongChars;
    char paragraph[max_para_length];
    int caseInsensitive;
} TypingStats;

// function to create user profile
void loadUserProfile(UserProfile *profile) {
    printf("Enter your username: ");
    if (fgets(profile->username, sizeof(profile->username), stdin) != NULL) {
        size_t len = strlen(profile->username);
        if (len > 0 && profile->username[len - 1] == '\n') {
            profile->username[len - 1] = '\0';
        }
    } else {
        perror("Error reading username");
        exit(EXIT_FAILURE);
    }

    char filename[100];
    snprintf(filename, sizeof(filename), "%s_profile.txt", profile->username);
    FILE *f = fopen(filename, "r");

    if (f != NULL) {
        if (fscanf(f, "%lf %lf %lf %lf %d %d %d", &profile->bestSpeed, &profile->bestAccuracy,
                   &profile->totalSpeed, &profile->totalAccuracy, &profile->totalAttempts,
                   &profile->currentStreak, &profile->bestStreak) != 7) {
            // Handle potential file format issues
            profile->bestSpeed = 0;
            profile->bestAccuracy = 0;
            profile->totalSpeed = 0;
            profile->totalAccuracy = 0;
            profile->totalAttempts = 0;
            profile->currentStreak = 0;
            profile->bestStreak = 0;
        }
        fclose(f);
    } else {
        profile->bestSpeed = 0;
        profile->bestAccuracy = 0;
        profile->totalSpeed = 0;
        profile->totalAccuracy = 0;
        profile->totalAttempts = 0;
        profile->currentStreak = 0;
        profile->bestStreak = 0;
    }
}

// function to Update and Save user Profile After Each Attempt
void updateUserProfile(UserProfile *profile, TypingStats *currentAttempt) {
    if (currentAttempt->typingSpeed > profile->bestSpeed) {
        profile->bestSpeed = currentAttempt->typingSpeed;
    }
    if (currentAttempt->accuracy > profile->bestAccuracy) {
        profile->bestAccuracy = currentAttempt->accuracy;
    }

    profile->totalSpeed += currentAttempt->typingSpeed;
    profile->totalAccuracy += currentAttempt->accuracy;
    profile->totalAttempts++;

    if (profile->currentStreak > profile->bestStreak) {
        profile->bestStreak = profile->currentStreak;
    }

    char filename[100];
    snprintf(filename, sizeof(filename), "%s_profile.txt", profile->username);
    FILE *f = fopen(filename, "w");

    if (f != NULL) {
        fprintf(f, "%.2lf %.2lf %.2lf %.2lf %d %d %d\n", profile->bestSpeed, profile->bestAccuracy,
                profile->totalSpeed, profile->totalAccuracy, profile->totalAttempts,
                profile->currentStreak, profile->bestStreak);
        fclose(f);
    } else {
        fprintf(stderr, "Error saving user profile to '%s'\n", filename);
    }
}

// function to display user summary
void displayUserSummary(UserProfile *profile) {
    printf("\nUser Summary for %s:\n", profile->username);
    printf("--------------------------------------------------------\n");
    printf("Best Typing Speed: %.2f cpm\n", profile->bestSpeed);
    printf("Best Accuracy: %.2f%%\n", profile->bestAccuracy);
    if (profile->totalAttempts > 0) {
        printf("Average Typing Speed: %.2f cpm\n", profile->totalSpeed / profile->totalAttempts);
        printf("Average Accuracy: %.2f%%\n", profile->totalAccuracy / profile->totalAttempts);
    }
    printf("Total Attempts: %d\n", profile->totalAttempts);
    printf("Current Streak: %d\n", profile->currentStreak);
    printf("Best Streak: %d\n", profile->bestStreak);
    printf("--------------------------------------------------------\n");
}

// function to generate a random paragraph
char *getRandomParagraph(FILE *file) {
    char line[max_file_line_length];
    int numParas = 0;

    // Count the number of paragraphs
    fseek(file, 0, SEEK_SET);
    while (fgets(line, sizeof(line), file) != NULL) {
        if (line[0] != '\n' && line[0] != '\r') { // Handle both \n and \r for different systems
            numParas++;
        }
    }

    if (numParas == 0) {
        fprintf(stderr, "Error: No paragraphs found in the file or file is empty.\n");
        exit(EXIT_FAILURE);
    }

    // Generate a random index
    fseek(file, 0, SEEK_SET);
    int randomIndex = rand() % numParas;
    int currentParaIndex = 0;

    // Read to the randomly selected paragraph
    while (fgets(line, sizeof(line), file) != NULL) {
        if (line[0] != '\n' && line[0] != '\r') {
            if (currentParaIndex == randomIndex) {
                size_t len = strlen(line);
                if (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
                    line[len - 1] = '\0';
                }
                char *paragraph = strdup(line);
                if (paragraph == NULL) {
                    perror("Memory allocation error");
                    exit(EXIT_FAILURE);
                }
                return paragraph;
            }
            currentParaIndex++;
        }
    }

    // This should ideally not be reached
    fprintf(stderr, "Error: Could not retrieve paragraph.\n");
    exit(EXIT_FAILURE);
}

// function to calculate the typing statistics
void printTypingStats(double elapsedTime, const char *input, const char *correctText, Difficulty difficulty, TypingStats *stats) {
    int correctCount = 0, wrongCount = 0;
    int minLen = strlen(correctText) < strlen(input) ? strlen(correctText) : strlen(input);
    char highlightedInput[max_para_length * 5]; // Increased size to accommodate ANSI codes
    int highlightedIndex = 0;

    for (int i = 0; i < minLen; i++) {
        char c1 = correctText[i];
        char c2 = input[i];

        if (stats->caseInsensitive) {
            c1 = tolower(c1);
            c2 = tolower(c2);
        }

        if (c1 == c2) {
            highlightedIndex += snprintf(highlightedInput + highlightedIndex, sizeof(highlightedInput) - highlightedIndex, "%c", input[i]);
            correctCount++;
        } else {
            highlightedIndex += snprintf(highlightedInput + highlightedIndex, sizeof(highlightedInput) - highlightedIndex, "\x1B[31m%c\x1B[0m", input[i]); // Red color for error
            wrongCount++;
        }
    }

    // Highlight extra characters in input
    if (strlen(input) > strlen(correctText)) {
        for (size_t i = strlen(correctText); i < strlen(input); i++) {
            highlightedIndex += snprintf(highlightedInput + highlightedIndex, sizeof(highlightedInput) - highlightedIndex, "\x1B[31m%c\x1B[0m", input[i]); // Red color for extra input
            wrongCount++;
        }
    }
    highlightedInput[highlightedIndex] = '\0'; // Null-terminate the highlighted input

    int totalCharacters = strlen(correctText); // Accuracy based on the correct paragraph length
    double accuracy = totalCharacters > 0 ? (double)correctCount / totalCharacters * 100 : 0;

    if (elapsedTime < 0.1) { // More sensitive to prevent instant submission
        printf("\n");
        printf("### Please DO NOT paste text or press enter immediately ###\n");
        printf("\n");
        elapsedTime = 0.1; // Avoid division by zero or very small numbers
    }

    double typingSpeed = (totalCharacters / 5.0) / (elapsedTime / 60.0);
    stats->typingSpeed = typingSpeed;
    stats->accuracy = accuracy;
    stats->wrongChars = wrongCount;
    strncpy(stats->paragraph, correctText, max_para_length);

    printf("\nYour input: %s\n", highlightedInput); // Display the highlighted input
}

// display function for previous attempts
void displayPreviousAttempts(TypingStats attempts[], int numAttempts) {
    printf("\nPrevious Attempts:\n");
    printf("--------------------------------------------------------\n");
    printf("| Attempt | Typing Speed (cpm) | Accuracy | Wrong Chars |\n");
    printf("--------------------------------------------------------\n");

    for (int i = 0; i < numAttempts; i++) {
        printf("|    %2d   |         %.2f         |  %.2f  |     %2d    |\n", i + 1, attempts[i].typingSpeed, attempts[i].accuracy, attempts[i].wrongChars);
    }

    printf("--------------------------------------------------------\n");
}

// function to prompt difficulty
void promptDifficulty(Difficulty *difficulty) {
    int choice;
    // ask the user for choice

    printf("Select difficulty level:\n1. Easy\n2. Medium\n3. Hard\n");

    while (1) {
        printf("Enter your choice: ");

        if (scanf("%d", &choice) != 1) // if a character is entered instead of a number
        {
            fprintf(stderr, "Invalid input. Please enter a number.\n");
            while (getchar() != '\n');
            continue;
        }

        while (getchar() != '\n');

        if (choice >= 1 && choice <= 3) // if the choice is within this range, break out of the loop
        {
            break;
        } else {
            fprintf(stderr, "Invalid choice. Please enter a number between 1 and 3.\n");
        }
    }

    switch (choice) {
    case 1:
        *difficulty = (Difficulty){EASY_SPEED, EASY_MEDIUM_SPEED, MEDIUM_HARD_SPEED};
        break;
    case 2:
        *difficulty = (Difficulty){EASY_MEDIUM_SPEED, MEDIUM_HARD_SPEED, HARD_MAX_SPEED};
        break;
    case 3:
        *difficulty = (Difficulty){MEDIUM_HARD_SPEED, HARD_MAX_SPEED, HARD_SPEED + 4};
        break;
    default:
        printf("Invalid choice. Using default difficulty level (Easy).\n");
        *difficulty = (Difficulty){EASY_SPEED, EASY_MEDIUM_SPEED, MEDIUM_HARD_SPEED};
    }
}

// function to process attempts
void processAttempts(FILE *file) {
    srand((unsigned int)time(NULL)); // seed value is set to unsigned int time

    printf("Welcome to Typing Tutor!\n"); // welcome message
    UserProfile profile;                 // Initialize user profile and stats
    loadUserProfile(&profile);          //  Load user profile from file
    // local variable declarations

    char input[max_para_length];
    Difficulty difficulty;
    TypingStats attempts[max_attempts];
    int numAttempts = 0; // initially no of attempts is set to 0
    int caseChoice;

    promptDifficulty(&difficulty); // this function lets the user choose the difficulty level

    while (1) {
        char *currentPara = getRandomParagraph(file); // a random para is stored in current para variable

        printf("Enable case-insensitive typing ? (1- YES, 0-NO) ");
        if (scanf("%d", &caseChoice) != 1) {
            fprintf(stderr, "Invalid input. Assuming case-sensitive (0).\n");
            caseChoice = 0;
            while (getchar() != '\n');
        } else {
            while (getchar() != '\n'); // Clear buffer
        }

        printf("\nType the following paragraph:\n%s\n", currentPara); // the random para is displayed

        clock_t startTime = clock(); // time is started
        printf("Your input: \n");
        fflush(stdout); // fflush() is typically used for output stream only. Its purpose is to clear the output buffer and move the buffered data to console

        if (fgets(input, sizeof(input), stdin) == NULL) // error reading the input
        {
            perror("Error reading input"); // The C library function void perror(const char *str) prints a descriptive error message to stderr.
            exit(EXIT_FAILURE);
        }

        clock_t endTime = clock();                                           // time stops
        double elapsedTime = (double)(endTime - startTime) / CLOCKS_PER_SEC; // elapsed time is calculated per sec

        // Prevent unrealistically short elapsed time
        if (elapsedTime < 0.1) { // More sensitive to prevent instant submission
            printf("\n⚠️  Typing too fast or pasted input detected. Please type the paragraph manually.\n");
            printf("This attempt will not be recorded.\n");
            free(currentPara);
            continue;
        }

        // Check if input is empty or only contains newline
        input[strcspn(input, "\n")] = 0; // Remove trailing newline
        if (strlen(input) == 0) {
            printf("\n⚠️  No input detected. Please type something.\n");
            free(currentPara);
            continue;
        }

        // Check if input contains only whitespace
        int isWhitespaceOnly = 1;
        for (size_t i = 0; i < strlen(input); i++) {
            if (!isspace((unsigned char)input[i])) {
                isWhitespaceOnly = 0;
                break;
            }
        }

        if (isWhitespaceOnly) {
            printf("Input cannot contain only whitespace. Please try again.\n");
            free(currentPara);
            continue;
        }

        TypingStats currentAttempt;
        currentAttempt.caseInsensitive = caseChoice;
        printTypingStats(elapsedTime, input, currentPara, difficulty, &currentAttempt); // function to calculate typing stats is called
        updateUserProfile(&profile, &currentAttempt);                                  // Update user profile

        // Increment streak after a successful attempt (every attempt for now)
        profile.currentStreak++;
        updateUserProfile(&profile, &currentAttempt); // Save the updated streak

        printf("\nTyping Stats for Current Attempt:\n");
        printf("--------------------------------------------------------\n");
        printf("Typing Speed: %.2f characters per minute\n", currentAttempt.typingSpeed);
        printf("Accuracy: %.2f%%\n", currentAttempt.accuracy);
        printf("Wrong Characters: %d\n", currentAttempt.wrongChars);
        printf("Current Streak: %d\n", profile.currentStreak);
        printf("--------------------------------------------------------\n");

        attempts[numAttempts++] = currentAttempt; // Store the current attempt

        if (numAttempts >= max_attempts) {
            displayPreviousAttempts(attempts, numAttempts);
            printf("\nMaximum attempts reached. Exiting...\n");
            break;
        }

        printf("\nDo you want to continue? (y/n): "); // asks the user if they want to continue

        char choice[3];
        if (fgets(choice, sizeof(choice), stdin) == NULL) {
            perror("Error reading choice");
            exit(EXIT_FAILURE);
        }
        choice[strcspn(choice, "\n")] = 0; // Remove trailing newline

        if (tolower(choice[0]) != 'y') // If the arguments passed to the tolower() function is other than an uppercase alphabet, it returns the same character that is passed to the function.
        {
            displayPreviousAttempts(attempts, numAttempts);
            displayUserSummary(&profile); //  Display summary at the end
            printf("\nThanks for using Typing Tutor!\n");
            break;
        }

        free(currentPara); // free the allocated memory for the paragraph
    }

    fclose(file); // file is closed
}

// main function
int main() {
    // Seed randomness once at the start of the program
    srand((unsigned int)time(NULL));
    // open the file in read mode
    FILE *file = fopen("paragraphs.txt", "r");
    if (file == NULL) {
        perror("Error opening file 'paragraphs.txt'");
        return 1;
    }

    processAttempts(file);

    return 0;
}