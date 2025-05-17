// header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <sys/time.h> // To include gettimeofday()

// global declarations
#define max_para_length 200
#define max_file_line_length 200
#define max_attempts 10
#define max_leaderboard_entries 100

// Define named constants for difficulty levels
#define EASY_SPEED 5
#define MEDIUM_SPEED 8
#define HARD_SPEED 12
#define EASY_MEDIUM_SPEED 8
#define MEDIUM_HARD_SPEED 12
#define HARD_MAX_SPEED 16

// structure to store user profile
typedef struct
{
    char username[50];
    double bestSpeed;
    double bestAccuracy;
    double totalSpeed;
    double totalAccuracy;
    int totalAttempts;
} UserProfile;

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
    double typingSpeed;    // Characters per minute (CPM)
    double wordsPerMinute; // Words per minute (WPM)
    double accuracy;
    int wrongChars;
    char paragraph[max_para_length];
    int caseInsensitive;
} TypingStats;

// structure to store leaderboard entries
typedef struct
{
    char username[50];
    double typingSpeed;    // CPM
    double wordsPerMinute; // WPM
    double accuracy;
    char difficulty[10]; // Difficulty level (Easy, Medium, Hard)
} LeaderboardEntry;

// Function prototypes for leaderboard-related functions
void loadLeaderboard(LeaderboardEntry leaderboard[], int *numEntries);
void saveLeaderboard(LeaderboardEntry leaderboard[], int numEntries);
void updateLeaderboard(UserProfile *profile, TypingStats *currentAttempt, const char *difficulty);
void displayLeaderboard(const char *difficulty);

// Load user profile from file or initialize if not found
void loadUserProfile(UserProfile *profile)
{
    printf("Enter your username: ");
    fgets(profile->username, sizeof(profile->username), stdin);
    profile->username[strcspn(profile->username, "\n")] = '\0'; // Remove newline

    char filename[100];
    snprintf(filename, sizeof(filename), "%s_profile.txt", profile->username);
    FILE *f = fopen(filename, "r");

    if (f && fscanf(f, "%lf %lf %lf %lf %d", &profile->bestSpeed, &profile->bestAccuracy,
                    &profile->totalSpeed, &profile->totalAccuracy, &profile->totalAttempts) == 5)
    {
        fclose(f);
    }
    else
    {
        if (f)
            fclose(f);
        profile->bestSpeed = profile->bestAccuracy = profile->totalSpeed = profile->totalAccuracy = 0;
        profile->totalAttempts = 0;
    }
}

// Update profile with current attempt and save to file
void updateUserProfile(UserProfile *profile, TypingStats *currentAttempt)
{
    if (currentAttempt->typingSpeed > profile->bestSpeed)
        profile->bestSpeed = currentAttempt->typingSpeed;
    if (currentAttempt->accuracy > profile->bestAccuracy)
        profile->bestAccuracy = currentAttempt->accuracy;

    profile->totalSpeed += currentAttempt->typingSpeed;
    profile->totalAccuracy += currentAttempt->accuracy;
    profile->totalAttempts++;

    char filename[100];
    snprintf(filename, sizeof(filename), "%s_profile.txt", profile->username);
    FILE *f = fopen(filename, "w");
    if (f)
    {
        fprintf(f, "%.2lf %.2lf %.2lf %.2lf %d", profile->bestSpeed, profile->bestAccuracy,
                profile->totalSpeed, profile->totalAccuracy, profile->totalAttempts);
        fclose(f);
    }
    else
    {
        fprintf(stderr, "Error saving user profile to '%s'\n", filename);
    }
}

// Display user profile summary
void displayUserSummary(UserProfile *profile)
{
    printf("\nUser Summary for %s:\n", profile->username);
    printf("--------------------------------------------------------\n");
    printf("Best Typing Speed: %.2f cpm\n", profile->bestSpeed);
    printf("Best Accuracy: %.2f%%\n", profile->bestAccuracy);
    if (profile->totalAttempts > 0)
    {
        printf("Average Typing Speed: %.2f cpm\n", profile->totalSpeed / profile->totalAttempts);
        printf("Average Accuracy: %.2f%%\n", profile->totalAccuracy / profile->totalAttempts);
    }
    printf("Total Attempts: %d\n", profile->totalAttempts);
    printf("--------------------------------------------------------\n");
}

// function to fetch the random paragraph from paragraph.txt
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
    int correctCount = 0, wrongCount = 0;
    int minLen = strlen(correctText) < strlen(input) ? strlen(correctText) : strlen(input);

    // checking and updating correct and wrong counts
    for (int i = 0; i < minLen; i++)
    {
        char c1 = correctText[i];
        char c2 = input[i];

        if (stats->caseInsensitive)
        {
            c1 = tolower(c1);
            c2 = tolower(c2);
        }

        if (c1 == c2)
        {
            correctCount++;
        }
        else
        {
            wrongCount++;
        }
    }

    int totalCharacters = minLen;
    double accuracy = (double)correctCount / totalCharacters * 100;

    // Prevent division by zero
    if (elapsedTime < 0.01)
    {
        elapsedTime = 0.01; // Set minimum time to prevent division by zero
    }

    // Standard formulas for typing measurements:
    // CPM = (Characters / Elapsed Time in seconds) * 60
    double cpm = (totalCharacters / elapsedTime) * 60.0;

    // WPM = (Characters / 5) / (Elapsed Time in minutes)
    // OR: WPM = CPM / 5
    double wpm = cpm / 5.0;

    // Store both metrics
    stats->typingSpeed = cpm;    // CPM
    stats->wordsPerMinute = wpm; // WPM
    stats->accuracy = accuracy;
    stats->wrongChars = wrongCount;
    strncpy(stats->paragraph, correctText, max_para_length);
}

// display function for previous attempts
void displayPreviousAttempts(TypingStats attempts[], int numAttempts)
{
    printf("\nPrevious Attempts:\n");
    printf("---------------------------------------------------------------------\n");
    printf("| Attempt |  CPM  |  WPM  | Accuracy (%%) | Wrong Chars |\n");
    printf("---------------------------------------------------------------------\n");

    for (int i = 0; i < numAttempts; i++)
    {
        printf("|   %2d    | %6.2f | %6.2f |    %6.2f    |     %3d     |\n",
               i + 1,
               attempts[i].typingSpeed,    // CPM
               attempts[i].wordsPerMinute, // WPM
               attempts[i].accuracy,
               attempts[i].wrongChars);
    }

    printf("---------------------------------------------------------------------\n");
}

// function to prompt difficulty
void promptDifficulty(Difficulty *difficulty, char *difficultyLevel)
{
    int choice;
    // ask the user for choice

    printf("Select difficulty level:\n1. Easy\n2. Medium\n3. Hard\n");

    while (1)
    {
        printf("Enter your choice: ");

        if (scanf("%d", &choice) != 1) // if a character is entered instead of a number
        {
            perror("Invalid input. Please enter a number.\n");
            while (getchar() != '\n')
                ;
            continue;
        }

        while (getchar() != '\n')
            ;

        if (choice >= 1 && choice <= 3) // if the choice is within this range, break out of the loop
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
        *difficulty = (Difficulty){EASY_SPEED, EASY_MEDIUM_SPEED, MEDIUM_HARD_SPEED};
        strcpy(difficultyLevel, "Easy");
        break;
    case 2:
        *difficulty = (Difficulty){EASY_MEDIUM_SPEED, MEDIUM_HARD_SPEED, HARD_MAX_SPEED};
        strcpy(difficultyLevel, "Medium");
        break;
    case 3:
        *difficulty = (Difficulty){MEDIUM_HARD_SPEED, HARD_MAX_SPEED, HARD_SPEED + 4};
        strcpy(difficultyLevel, "Hard");
        break;
    default:
        printf("Invalid choice. Using default difficulty level (Easy).\n");
        *difficulty = (Difficulty){EASY_SPEED, EASY_MEDIUM_SPEED, MEDIUM_HARD_SPEED};
        strcpy(difficultyLevel, "Easy");
    }
}

// Load leaderboard from file
void loadLeaderboard(LeaderboardEntry leaderboard[], int *numEntries)
{
    FILE *file = fopen("leaderboard.txt", "r");
    if (!file)
    {
        *numEntries = 0; // If the file doesn't exist, start with an empty leaderboard
        return;
    }

    *numEntries = 0;
    while (fscanf(file, "%s %lf %lf %lf %s", leaderboard[*numEntries].username,
                  &leaderboard[*numEntries].typingSpeed,
                  &leaderboard[*numEntries].wordsPerMinute,
                  &leaderboard[*numEntries].accuracy,
                  leaderboard[*numEntries].difficulty) == 5)
    {
        (*numEntries)++;
        if (*numEntries >= max_leaderboard_entries)
            break;
    }

    fclose(file);
}

// Save leaderboard to file
void saveLeaderboard(LeaderboardEntry leaderboard[], int numEntries)
{
    FILE *file = fopen("leaderboard.txt", "w");
    if (!file)
    {
        perror("Error saving leaderboard");
        return;
    }

    for (int i = 0; i < numEntries; i++)
    {
        fprintf(file, "%s %.2f %.2f %.2f %s\n", leaderboard[i].username,
                leaderboard[i].typingSpeed,
                leaderboard[i].wordsPerMinute,
                leaderboard[i].accuracy,
                leaderboard[i].difficulty);
    }

    fclose(file);
}

// Update leaderboard with current attempt
void updateLeaderboard(UserProfile *profile, TypingStats *currentAttempt, const char *difficulty)
{
    LeaderboardEntry leaderboard[max_leaderboard_entries];
    int numEntries;

    // Load the existing leaderboard
    loadLeaderboard(leaderboard, &numEntries);

    // Create a new entry for the current attempt
    LeaderboardEntry newEntry;
    strncpy(newEntry.username, profile->username, sizeof(newEntry.username));
    newEntry.typingSpeed = currentAttempt->typingSpeed;
    newEntry.wordsPerMinute = currentAttempt->wordsPerMinute;
    newEntry.accuracy = currentAttempt->accuracy;
    strncpy(newEntry.difficulty, difficulty, sizeof(newEntry.difficulty));

    // Add the new entry to the leaderboard
    if (numEntries < max_leaderboard_entries)
    {
        leaderboard[numEntries++] = newEntry;
    }
    else
    {
        // If leaderboard is full, replace the worst entry if the current one is better
        int worstIndex = 0;
        for (int i = 1; i < numEntries; i++)
        {
            if (leaderboard[i].typingSpeed < leaderboard[worstIndex].typingSpeed &&
                strcmp(leaderboard[i].difficulty, difficulty) == 0)
            {
                worstIndex = i;
            }
        }

        if (newEntry.typingSpeed > leaderboard[worstIndex].typingSpeed ||
            numEntries < max_leaderboard_entries)
        {
            leaderboard[worstIndex] = newEntry;
        }
    }

    // Sort the leaderboard by typing speed (descending order) for each difficulty
    for (int i = 0; i < numEntries - 1; i++)
    {
        for (int j = i + 1; j < numEntries; j++)
        {
            if (strcmp(leaderboard[i].difficulty, leaderboard[j].difficulty) == 0 &&
                leaderboard[i].typingSpeed < leaderboard[j].typingSpeed)
            {
                LeaderboardEntry temp = leaderboard[i];
                leaderboard[i] = leaderboard[j];
                leaderboard[j] = temp;
            }
        }
    }

    // Save the updated leaderboard
    saveLeaderboard(leaderboard, numEntries);
}

// Display leaderboard for a specific difficulty
void displayLeaderboard(const char *difficulty)
{
    LeaderboardEntry leaderboard[max_leaderboard_entries];
    int numEntries;

    // Load the leaderboard
    loadLeaderboard(leaderboard, &numEntries);

    printf("\nLeaderboard for %s Difficulty:\n", difficulty);
    printf("-------------------------------------------------------------\n");
    printf("| Rank | Username       | CPM    | WPM    | Accuracy (%%) |\n");
    printf("-------------------------------------------------------------\n");

    int rank = 1;
    for (int i = 0; i < numEntries; i++)
    {
        if (strcmp(leaderboard[i].difficulty, difficulty) == 0)
        {
            printf("| %4d | %-14s | %6.2f | %6.2f | %10.2f |\n",
                   rank++, leaderboard[i].username,
                   leaderboard[i].typingSpeed,
                   leaderboard[i].wordsPerMinute,
                   leaderboard[i].accuracy);

            if (rank > 10)
                break; // Only show top 10 entries
        }
    }

    if (rank == 1)
    {
        printf("|      No entries for this difficulty level yet          |\n");
    }

    printf("-------------------------------------------------------------\n");
}

// function to process attempts
void processAttempts(FILE *file)
{
    srand((unsigned int)time(NULL));

    printf("Welcome to Typing Tutor!\n");
    UserProfile profile;
    loadUserProfile(&profile);

    char input[max_para_length];
    Difficulty difficulty;
    char difficultyLevel[10]; // To store the difficulty level as a string
    TypingStats attempts[max_attempts];
    int numAttempts = 0;
    int caseChoice;

    promptDifficulty(&difficulty, difficultyLevel);

    while (1)
    {
        char *currentPara = getRandomParagraph(file);

        printf("Enable case-insensitive typing ? (1- YES, 0-NO) ");
        scanf("%d", &caseChoice);
        while (getchar() != '\n')
            ; // Clear buffer

        printf("\nType the following paragraph:\n%s\n", currentPara);

        // Use gettimeofday for accurate wall-clock time
        struct timeval startTime, endTime;
        gettimeofday(&startTime, NULL);

        printf("Your input: \n");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            perror("Error reading input");
            exit(EXIT_FAILURE);
        }

        gettimeofday(&endTime, NULL);

        // Calculate elapsed time in seconds with microsecond precision
        double elapsedTime = (endTime.tv_sec - startTime.tv_sec) +
                             (endTime.tv_usec - startTime.tv_usec) / 1000000.0;

        // Check if input is empty
        if (strlen(input) == 0)
        {
            printf("\n⚠️  No input detected. Please type something.\n");
            free(currentPara);
            continue;
        }

        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n')
        {
            input[len - 1] = '\0';
        }

        // Check if input is empty or contains only whitespace
        int isWhitespaceOnly = 1;
        for (size_t i = 0; i < strlen(input); i++)
        {
            if (!isspace((unsigned char)input[i]))
            {
                isWhitespaceOnly = 0;
                break;
            }
        }

        if (strlen(input) == 0 || isWhitespaceOnly)
        {
            printf("Input cannot be empty or contain only whitespace. Please try again.\n");
            free(currentPara);
            continue;
        }

        TypingStats currentAttempt;
        currentAttempt.caseInsensitive = caseChoice;
        printTypingStats(elapsedTime, input, currentPara, difficulty, &currentAttempt);

        // Store the attempt
        attempts[numAttempts++] = currentAttempt;

        // Update user profile
        updateUserProfile(&profile, &currentAttempt);

        // Update leaderboard
        updateLeaderboard(&profile, &currentAttempt, difficultyLevel);

        printf("\nTyping Stats for Current Attempt:\n");
        printf("--------------------------------------------------------\n");
        printf("Characters Per Minute (CPM): %.2f\n", currentAttempt.typingSpeed);
        printf("Words Per Minute (WPM): %.2f\n", currentAttempt.wordsPerMinute);
        printf("Accuracy: %.2f%%\n", currentAttempt.accuracy);
        printf("Wrong Characters: %d\n", currentAttempt.wrongChars);
        printf("Time taken: %.2f seconds\n", elapsedTime);
        printf("--------------------------------------------------------\n");

        if (numAttempts >= max_attempts)
        {
            displayPreviousAttempts(attempts, numAttempts);
            printf("\nMaximum attempts reached. Exiting...\n");
            break;
        }

        printf("\nDo you want to continue? (y/n): ");

        char choice[3];
        if (fgets(choice, sizeof(choice), stdin) == NULL)
        {
            perror("Error reading choice");
            exit(EXIT_FAILURE);
        }

        if (tolower(choice[0]) != 'y')
        {
            displayPreviousAttempts(attempts, numAttempts);
            displayUserSummary(&profile);

            // Display leaderboard
            printf("\nWould you like to see the leaderboard for %s difficulty? (y/n): ", difficultyLevel);
            if (fgets(choice, sizeof(choice), stdin) && tolower(choice[0]) == 'y')
            {
                displayLeaderboard(difficultyLevel);
            }

            printf("\nThanks for using Typing Tutor!\n");
            break;
        }

        free(currentPara);
    }

    fclose(file);
}

// main function
int main()
{
    // Seed randomness once at the start of the program
    srand((unsigned int)time(NULL));
    // open the file in read mode
    FILE *file = fopen("paragraphs.txt", "r"); // the name of the file is stored in this file variable
    if (file == NULL)
    {
        perror("Error opening file 'paragraphs.txt'");
        return 1;
    }

    processAttempts(file);

    return 0;
}
