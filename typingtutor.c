// header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <sys/time.h>

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

// Error handling macro
#define CHECK_FILE_OP(f, msg) do { if (!(f)) { perror(msg); exit(EXIT_FAILURE); } } while (0)

// structure to store user profile
typedef struct {
    char username[50];
    double bestSpeed;
    double bestAccuracy;
    double totalSpeed;
    double totalAccuracy;
    int totalAttempts;
} UserProfile;

// structure to store difficulty
typedef struct {
    int easy;
    int medium;
    int hard;
} Difficulty;

// structure to store typing statistics
typedef struct {
    double typingSpeed;    // Characters per minute (CPM)
    double wordsPerMinute; // Words per minute (WPM)
    double accuracy;
    int wrongChars;
    char paragraph[max_para_length];
    int caseInsensitive;
} TypingStats;

// structure to store leaderboard entries
typedef struct {
    char username[50];
    double typingSpeed;    // CPM
    double wordsPerMinute; // WPM
    double accuracy;
    char difficulty[10]; // Difficulty level (Easy, Medium, Hard)
} LeaderboardEntry;

// structure to cache paragraphs
typedef struct {
    char **paragraphs;
    int count;
} ParagraphCache;

// Function prototypes
void loadParagraphs(FILE *file, ParagraphCache *cache);
void freeParagraphCache(ParagraphCache *cache);
char *getRandomParagraph(ParagraphCache *cache);
void sanitizeUsername(char *username, size_t size);
void loadUserProfile(UserProfile *profile);
void updateUserProfile(UserProfile *profile, TypingStats *currentAttempt);
void displayUserSummary(UserProfile *profile);
void printTypingStats(double elapsedTime, const char *input, const char *correctText, Difficulty difficulty, TypingStats *stats);
void displayPreviousAttempts(TypingStats attempts[], int numAttempts);
void promptDifficulty(Difficulty *difficulty, char *difficultyLevel);
void loadLeaderboard(LeaderboardEntry leaderboard[], int *numEntries);
void saveLeaderboard(LeaderboardEntry leaderboard[], int numEntries);
void updateLeaderboard(UserProfile *profile, TypingStats *currentAttempt, const char *difficulty);
void displayLeaderboard(const char *difficulty);
void collectUserInput(char *input, size_t inputSize, double *elapsedTime);
int isValidInput(const char *input);
void processAttempts(ParagraphCache *cache);

// Load paragraphs into cache
void loadParagraphs(FILE *file, ParagraphCache *cache) {
    char line[max_file_line_length];
    cache->count = 0;
    cache->paragraphs = NULL;

    while (fgets(line, sizeof(line), file) != NULL) {
        if (line[0] != '\n') cache->count++;
    }

    cache->paragraphs = malloc(cache->count * sizeof(char *));
    CHECK_FILE_OP(cache->paragraphs, "Memory allocation error for paragraph cache");

    fseek(file, 0, SEEK_SET);
    int index = 0;
    while (fgets(line, sizeof(line), file) != NULL) {
        if (line[0] != '\n') {
            size_t len = strlen(line);
            if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
            cache->paragraphs[index] = strdup(line);
            CHECK_FILE_OP(cache->paragraphs[index], "Memory allocation error for paragraph");
            index++;
        }
    }
}

// Free paragraph cache
void freeParagraphCache(ParagraphCache *cache) {
    for (int i = 0; i < cache->count; i++) {
        free(cache->paragraphs[i]);
        cache->paragraphs[i] = NULL; // Prevent accidental reuse
    }
    free(cache->paragraphs);
    cache->paragraphs = NULL; // Prevent accidental reuse
}

// Get random paragraph from cache
char *getRandomParagraph(ParagraphCache *cache) {
    if (cache->count == 0) {
        fprintf(stderr, "Error: No paragraphs available.\n");
        exit(EXIT_FAILURE);
    }
    return cache->paragraphs[rand() % cache->count];
}

// Sanitize username
void sanitizeUsername(char *username, size_t size) {
    for (size_t i = 0; i < strlen(username) && i < size - 1; i++) {
        if (!isalnum(username[i]) && username[i] != '-' && username[i] != '_') {
            username[i] = '_';
        }
    }
    username[size - 1] = '\0';
}

// Load user profile
void loadUserProfile(UserProfile *profile) {
    printf("Enter your username: ");
    CHECK_FILE_OP(fgets(profile->username, sizeof(profile->username), stdin), "Error reading username");
    profile->username[strcspn(profile->username, "\n")] = '\0';
    if (strlen(profile->username) == 0) {
        strcpy(profile->username, "default");
    }
    sanitizeUsername(profile->username, sizeof(profile->username));

    char filename[100];
    snprintf(filename, sizeof(filename), "%s_profile.txt", profile->username);
    FILE *f = fopen(filename, "r");
    if (f && fscanf(f, "%lf %lf %lf %lf %d", &profile->bestSpeed, &profile->bestAccuracy,
                    &profile->totalSpeed, &profile->totalAccuracy, &profile->totalAttempts) == 5) {
        fclose(f);
    } else {
        if (f) fclose(f);
        profile->bestSpeed = profile->bestAccuracy = profile->totalSpeed = profile->totalAccuracy = 0;
        profile->totalAttempts = 0;
    }
}

// Update user profile
void updateUserProfile(UserProfile *profile, TypingStats *currentAttempt) {
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
    if (f) {
        fprintf(f, "%.2lf %.2lf %.2lf %.2lf %d", profile->bestSpeed, profile->bestAccuracy,
                profile->totalSpeed, profile->totalAccuracy, profile->totalAttempts);
        fclose(f);
    } else {
        fprintf(stderr, "Error saving user profile to '%s'\n", filename);
    }
}

// Display user summary
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
    printf("--------------------------------------------------------\n");
}

// Calculate typing statistics
void printTypingStats(double elapsedTime, const char *input, const char *correctText, Difficulty difficulty, TypingStats *stats) {
    int correctCount = 0, wrongCount = 0;
    int minLen = strlen(correctText) < strlen(input) ? strlen(correctText) : strlen(input);

    for (int i = 0; i < minLen; i++) {
        char c1 = correctText[i];
        char c2 = input[i];
        if (stats->caseInsensitive) {
            c1 = tolower(c1);
            c2 = tolower(c2);
        }
        if (c1 == c2) {
            correctCount++;
        } else {
            wrongCount++;
        }
    }

    int totalCharacters = minLen;
    double accuracy = (double)correctCount / totalCharacters * 100;

    if (elapsedTime < 0.01) elapsedTime = 0.01;

    double cpm = (totalCharacters / elapsedTime) * 60.0;
    double wpm = cpm / 5.0;

    stats->typingSpeed = cpm;
    stats->wordsPerMinute = wpm;
    stats->accuracy = accuracy;
    stats->wrongChars = wrongCount;
    strncpy(stats->paragraph, correctText, max_para_length);
}

// Display previous attempts
void displayPreviousAttempts(TypingStats attempts[], int numAttempts) {
    printf("\nPrevious Attempts:\n");
    printf("---------------------------------------------------------------------\n");
    printf("| Attempt |  CPM  |  WPM  | Accuracy (%%) | Wrong Chars |\n");
    printf("---------------------------------------------------------------------\n");
    for (int i = 0; i < numAttempts; i++) {
        printf("|   %2d    | %6.2f | %6.2f |    %6.2f    |     %3d     |\n",
               i + 1, attempts[i].typingSpeed, attempts[i].wordsPerMinute,
               attempts[i].accuracy, attempts[i].wrongChars);
    }
    printf("---------------------------------------------------------------------\n");
}

// Prompt for difficulty
void promptDifficulty(Difficulty *difficulty, char *difficultyLevel) {
    int choice;
    printf("Select difficulty level:\n1. Easy\n2. Medium\n3. Hard\n");
    while (1) {
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1 || choice < 1 || choice > 3) {
            printf("Invalid input. Please enter a number between 1 and 3.\n");
            while (getchar() != '\n');
            continue;
        }
        while (getchar() != '\n');
        break;
    }

    switch (choice) {
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
    }
}

// Load leaderboard
void loadLeaderboard(LeaderboardEntry leaderboard[], int *numEntries) {
    FILE *file = fopen("leaderboard.txt", "r");
    if (!file) {
        *numEntries = 0;
        return;
    }

    *numEntries = 0;
    while (fscanf(file, "%s %lf %lf %lf %s", leaderboard[*numEntries].username,
                  &leaderboard[*numEntries].typingSpeed,
                  &leaderboard[*numEntries].wordsPerMinute,
                  &leaderboard[*numEntries].accuracy,
                  leaderboard[*numEntries].difficulty) == 5) {
        (*numEntries)++;
        if (*numEntries >= max_leaderboard_entries) break;
    }
    fclose(file);
}

// Save leaderboard
void saveLeaderboard(LeaderboardEntry leaderboard[], int numEntries) {
    FILE *file = fopen("leaderboard.txt", "w");
    if (!file) {
        perror("Error saving leaderboard");
        return;
    }

    for (int i = 0; i < numEntries; i++) {
        fprintf(file, "%s %.2f %.2f %.2f %s\n", leaderboard[i].username,
                leaderboard[i].typingSpeed, leaderboard[i].wordsPerMinute,
                leaderboard[i].accuracy, leaderboard[i].difficulty);
    }
    fclose(file);
}

// Update leaderboard
void updateLeaderboard(UserProfile *profile, TypingStats *currentAttempt, const char *difficulty) {
    LeaderboardEntry leaderboard[max_leaderboard_entries];
    int numEntries;
    loadLeaderboard(leaderboard, &numEntries);

    LeaderboardEntry newEntry;
    strncpy(newEntry.username, profile->username, sizeof(newEntry.username));
    newEntry.typingSpeed = currentAttempt->typingSpeed;
    newEntry.wordsPerMinute = currentAttempt->wordsPerMinute;
    newEntry.accuracy = currentAttempt->accuracy;
    strncpy(newEntry.difficulty, difficulty, sizeof(newEntry.difficulty));

    if (numEntries < max_leaderboard_entries) {
        leaderboard[numEntries++] = newEntry;
    } else {
        int worstIndex = 0;
        for (int i = 1; i < numEntries; i++) {
            if (leaderboard[i].typingSpeed < leaderboard[worstIndex].typingSpeed &&
                strcmp(leaderboard[i].difficulty, difficulty) == 0) {
                worstIndex = i;
            }
        }
        if (newEntry.typingSpeed > leaderboard[worstIndex].typingSpeed) {
            leaderboard[worstIndex] = newEntry;
        }
    }

    for (int i = 0; i < numEntries - 1; i++) {
        for (int j = i + 1; j < numEntries; j++) {
            if (strcmp(leaderboard[i].difficulty, leaderboard[j].difficulty) == 0 &&
                leaderboard[i].typingSpeed < leaderboard[j].typingSpeed) {
                LeaderboardEntry temp = leaderboard[i];
                leaderboard[i] = leaderboard[j];
                leaderboard[j] = temp;
            }
        }
    }
    saveLeaderboard(leaderboard, numEntries);
}

// Display leaderboard
void displayLeaderboard(const char *difficulty) {
    LeaderboardEntry leaderboard[max_leaderboard_entries];
    int numEntries;
    loadLeaderboard(leaderboard, &numEntries);

    printf("\nLeaderboard for %s Difficulty:\n", difficulty);
    printf("-------------------------------------------------------------\n");
    printf("| Rank | Username       | CPM    | WPM    | Accuracy (%%) |\n");
    printf("-------------------------------------------------------------\n");

    int rank = 1;
    for (int i = 0; i < numEntries; i++) {
        if (strcmp(leaderboard[i].difficulty, difficulty) == 0) {
            printf("| %4d | %-14s | %6.2f | %6.2f | %10.2f |\n",
                   rank++, leaderboard[i].username,
                   leaderboard[i].typingSpeed, leaderboard[i].wordsPerMinute,
                   leaderboard[i].accuracy);
            if (rank > 10) break;
        }
    }
    if (rank == 1) {
        printf("|      No entries for this difficulty level yet          |\n");
    }
    printf("-------------------------------------------------------------\n");
}

// Collect user input
void collectUserInput(char *input, size_t inputSize, double *elapsedTime) {
    struct timeval startTime, endTime;
    gettimeofday(&startTime, NULL);
    printf("Your input: \n");
    fflush(stdout);
    CHECK_FILE_OP(fgets(input, inputSize, stdin), "Error reading input");
    gettimeofday(&endTime, NULL);
    *elapsedTime = (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec) / 1000000.0;
}

// Validate input
int isValidInput(const char *input) {
    if (strlen(input) == 0) return 0;
    int isWhitespaceOnly = 1;
    for (size_t i = 0; i < strlen(input); i++) {
        if (!isspace((unsigned char)input[i])) {
            isWhitespaceOnly = 0;
            break;
        }
    }
    return !isWhitespaceOnly;
}

// Process typing attempts
void processAttempts(ParagraphCache *cache) {
    printf("Welcome to Typing Tutor!\n");
    UserProfile profile;
    loadUserProfile(&profile);

    char input[max_para_length];
    Difficulty difficulty;
    char difficultyLevel[10];
    TypingStats attempts[max_attempts];
    int numAttempts = 0;
    int caseChoice;

    promptDifficulty(&difficulty, difficultyLevel);

    while (numAttempts < max_attempts) {
        char *currentPara = getRandomParagraph(cache);
        printf("Enable case-insensitive typing? (1-YES, 0-NO): ");
        if (scanf("%d", &caseChoice) != 1 || (caseChoice != 0 && caseChoice != 1)) {
            printf("Invalid input. Please enter 0 or 1.\n");
            while (getchar() != '\n');
            continue;
        }
        while (getchar() != '\n');

        printf("\nType the following paragraph:\n%s\n", currentPara);
        double elapsedTime;
        collectUserInput(input, sizeof(input), &elapsedTime);

        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') input[len - 1] = '\0';

        if (!isValidInput(input)) {
            printf("Input cannot be empty or contain only whitespace. Please try again.\n");
            continue;
        }

        TypingStats currentAttempt = { .caseInsensitive = caseChoice };
        printTypingStats(elapsedTime, input, currentPara, difficulty, &currentAttempt);
        attempts[numAttempts++] = currentAttempt;

        updateUserProfile(&profile, &currentAttempt);
        updateLeaderboard(&profile, &currentAttempt, difficultyLevel);

        printf("\nTyping Stats for Current Attempt:\n");
        printf("--------------------------------------------------------\n");
        printf("Characters Per Minute (CPM): %.2f\n", currentAttempt.typingSpeed);
        printf("Words Per Minute (WPM): %.2f\n", currentAttempt.wordsPerMinute);
        printf("Accuracy: %.2f%%\n", currentAttempt.accuracy);
        printf("Wrong Characters: %d\n", currentAttempt.wrongChars);
        printf("Time taken: %.2f seconds\n", elapsedTime);
        printf("--------------------------------------------------------\n");

        printf("\nDo you want to continue? (y/n): ");
        char choice[3];
        CHECK_FILE_OP(fgets(choice, sizeof(choice), stdin), "Error reading choice");
        if (tolower(choice[0]) != 'y') {
            displayPreviousAttempts(attempts, numAttempts);
            displayUserSummary(&profile);

            printf("\nWould you like to see the leaderboard for %s difficulty? (y/n): ", difficultyLevel);
            CHECK_FILE_OP(fgets(choice, sizeof(choice), stdin), "Error reading choice");
            if (tolower(choice[0]) == 'y') {
                displayLeaderboard(difficultyLevel);
            }

            printf("\nThanks for using Typing Tutor!\n");
            break;
        }
    }
}

// Main function
int main() {
    srand((unsigned int)time(NULL));
    FILE *file = fopen("paragraphs.txt", "r");
    if (!file) {
        perror("Error opening file 'paragraphs.txt'");
        return 1;
    }

    ParagraphCache cache;
    loadParagraphs(file, &cache);
    fclose(file);

    processAttempts(&cache);

    freeParagraphCache(&cache);
    return 0;
}