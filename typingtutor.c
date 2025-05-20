// header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

// global declarations
#define MAX_PARA_LENGTH 200
#define MAX_FILE_LINE_LENGTH 200
#define MAX_ATTEMPTS 10
#define MAX_LEADERBOARD_ENTRIES 100
#define MAX_PARAGRAPHS 100

#define EASY_SPEED 5
#define MEDIUM_SPEED 8
#define HARD_SPEED 12
#define EASY_MEDIUM_SPEED 8
#define MEDIUM_HARD_SPEED 12
#define HARD_MAX_SPEED 16

// macro for safe file open
#define CHECK_FILE_OP(file, filename, mode) do { \
    file = fopen(filename, mode); \
    if (!file) { \
        perror("File operation error"); \
        exit(EXIT_FAILURE); \
    } \
} while(0)

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
    double typingSpeed;
    double accuracy;
    int wrongChars;
    char paragraph[MAX_PARA_LENGTH];
} TypingStats;

// structure for leaderboard entries
typedef struct {
    char username[50];
    double typingSpeed;
    double wordsPerMinute;
    double accuracy;
    char difficulty[10];
} LeaderboardEntry;

// Global paragraph cache
static char *paragraphCache[MAX_PARAGRAPHS];
static int paragraphCount = 0;

// Forward declarations
void loadUserProfile(UserProfile* profile);
void updateUserProfile(UserProfile* profile, TypingStats* currentAttempt);
void displayUserSummary(const UserProfile* profile);
void loadParagraphsCache(const char* filename);
char* getRandomParagraphCached(void);
void freeParagraphCache(void);
void printTypingStats(double elapsedTime, const char* input, const char* correctText, TypingStats* stats);
void displayPreviousAttempts(TypingStats attempts[], int numAttempts);
void promptDifficulty(Difficulty *difficulty);
int collectUserInput(char* input, int maxLength);
int isValidUsername(const char* username);
void sanitizeUsername(char* username);
void loadLeaderboard(LeaderboardEntry leaderboard[], int* numEntries);
void saveLeaderboard(LeaderboardEntry leaderboard[], int numEntries);
void updateLeaderboard(UserProfile *profile, TypingStats *currentAttempt, const char *difficulty);
void displayLeaderboard(const char *difficulty);

int main() {
    srand((unsigned int)time(NULL));

    UserProfile user = {0};
    TypingStats attempts[MAX_ATTEMPTS] = {0};
    Difficulty difficulty = {0};

    loadUserProfile(&user);
    promptDifficulty(&difficulty);

    // Load all paragraphs into cache once
    loadParagraphsCache("paragraphs.txt");
    if (paragraphCount == 0) {
        fprintf(stderr, "No paragraphs found in paragraphs.txt\n");
        return EXIT_FAILURE;
    }

    int numAttempts = 0;
    char tryAgain;

    do {
        TypingStats currentStats = {0};
        char* paragraph = getRandomParagraphCached();

        printf("\nType the following paragraph:\n\n%s\n", paragraph);
        printf("\nPress Enter when you finish typing.\n\n");

        char inputText[MAX_PARA_LENGTH] = {0};
        clock_t start = clock();

        if (!collectUserInput(inputText, MAX_PARA_LENGTH)) {
            printf("Invalid input detected. Please try again.\n");
            continue;  // Do not count this attempt
        }

        clock_t end = clock();
        double elapsedTime = (double)(end - start) / CLOCKS_PER_SEC;

        printTypingStats(elapsedTime, inputText, paragraph, &currentStats);
        attempts[numAttempts++] = currentStats;
        updateUserProfile(&user, &currentStats);
        updateLeaderboard(&user, &currentStats, (difficulty.easy == EASY_SPEED) ? "Easy" :
                                                      (difficulty.medium == MEDIUM_HARD_SPEED) ? "Medium" : "Hard");

        printf("\nTyping Speed: %.2f cpm\n", currentStats.typingSpeed);
        printf("Accuracy: %.2f%%\n", currentStats.accuracy);
        printf("Wrong Characters: %d\n", currentStats.wrongChars);

        if (numAttempts < MAX_ATTEMPTS) {
            printf("\nDo you want to try again? (y/n): ");
            scanf(" %c", &tryAgain);
            while (getchar() != '\n');  // flush stdin
        } else {
            printf("\nYou've reached the maximum number of attempts.\n");
            tryAgain = 'n';
        }

    } while (tolower(tryAgain) == 'y');

    displayUserSummary(&user);
    displayPreviousAttempts(attempts, numAttempts);

    freeParagraphCache();

    return 0;
}

// Function implementations

void loadUserProfile(UserProfile* profile) {
    char usernameInput[50];
    printf("Enter your username (max 49 chars, no spaces or special chars): ");
    fgets(usernameInput, sizeof(usernameInput), stdin);
    size_t len = strlen(usernameInput);
    if (len > 0 && usernameInput[len - 1] == '\n')
        usernameInput[len - 1] = '\0';

    sanitizeUsername(usernameInput);

    if (!isValidUsername(usernameInput)) {
        printf("Invalid username. Using default 'guest'.\n");
        strcpy(usernameInput, "guest");
    }
    strncpy(profile->username, usernameInput, sizeof(profile->username));

    char filename[100];
    snprintf(filename, sizeof(filename), "%s_profile.txt", profile->username);

    FILE* f = fopen(filename, "r");
    if (f) {
        if (fscanf(f, "%lf %lf %lf %lf %d",
                   &profile->bestSpeed, &profile->bestAccuracy,
                   &profile->totalSpeed, &profile->totalAccuracy,
                   &profile->totalAttempts) != 5) {
            profile->bestSpeed = 0;
            profile->bestAccuracy = 0;
            profile->totalSpeed = 0;
            profile->totalAccuracy = 0;
            profile->totalAttempts = 0;
        }
        fclose(f);
    } else {
        profile->bestSpeed = profile->bestAccuracy = profile->totalSpeed = profile->totalAccuracy = 0;
        profile->totalAttempts = 0;
    }
}

void updateUserProfile(UserProfile* profile, TypingStats* currentAttempt) {
    if (currentAttempt->typingSpeed > profile->bestSpeed) {
        profile->bestSpeed = currentAttempt->typingSpeed;
    }
    if (currentAttempt->accuracy > profile->bestAccuracy) {
        profile->bestAccuracy = currentAttempt->accuracy;
    }

    profile->totalSpeed += currentAttempt->typingSpeed;
    profile->totalAccuracy += currentAttempt->accuracy;
    profile->totalAttempts++;

    char filename[100];
    snprintf(filename, sizeof(filename), "%s_profile.txt", profile->username);

    FILE* f = fopen(filename, "w");
    if (f) {
        fprintf(f, "%.2lf %.2lf %.2lf %.2lf %d\n",
                profile->bestSpeed, profile->bestAccuracy,
                profile->totalSpeed, profile->totalAccuracy,
                profile->totalAttempts);
        fclose(f);
    } else {
        perror("Error saving user profile");
    }
}

void displayUserSummary(const UserProfile* profile) {
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

void loadParagraphsCache(const char* filename) {
    FILE* file;
    CHECK_FILE_OP(file, filename, "r");

    char line[MAX_FILE_LINE_LENGTH];
    paragraphCount = 0;

    while (fgets(line, sizeof(line), file) && paragraphCount < MAX_PARAGRAPHS) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
            line[len - 1] = '\0';

        if (strlen(line) > 0) {
            paragraphCache[paragraphCount] = strdup(line);
            if (!paragraphCache[paragraphCount]) {
                fprintf(stderr, "Memory allocation failed\n");
                exit(EXIT_FAILURE);
            }
            paragraphCount++;
        }
    }
    fclose(file);
}

char* getRandomParagraphCached(void) {
    if (paragraphCount == 0) return NULL;
    int randomIndex = rand() % paragraphCount;
    return paragraphCache[randomIndex];
}

void freeParagraphCache(void) {
    for (int i = 0; i < paragraphCount; i++) {
        free(paragraphCache[i]);
        paragraphCache[i] = NULL;
    }
    paragraphCount = 0;
}

void printTypingStats(double elapsedTime, const char* input, const char* correctText, TypingStats* stats) {
    int correctCount = 0, wrongCount = 0;
    int minLen = (int)(strlen(correctText) < strlen(input) ? strlen(correctText) : strlen(input));

    for (int i = 0; i < minLen; i++) {
        if (correctText[i] == input[i])
            correctCount++;
        else
            wrongCount++;
    }

    int totalCharacters = minLen > 0 ? minLen : 1;
    double accuracy = (double)correctCount / totalCharacters * 100.0;

    if (elapsedTime < 3.0) {
        printf("\n### Please do not paste or hit enter immediately ###\n\n");
        elapsedTime = 3.0;
    }

    double typingSpeed = (totalCharacters / 5.0) / (elapsedTime / 60.0);

    stats->typingSpeed = typingSpeed;
    stats->accuracy = accuracy;
    stats->wrongChars = wrongCount;
    strncpy(stats->paragraph, correctText, MAX_PARA_LENGTH);
    stats->paragraph[MAX_PARA_LENGTH - 1] = '\0';
}

void displayPreviousAttempts(TypingStats attempts[], int numAttempts) {
    printf("\nPrevious Attempts:\n");
    printf("-------------------------------------------------------------\n");
    printf("| Attempt |   CPM   | Accuracy (%%) | Wrong Chars |\n");
    printf("-------------------------------------------------------------\n");

    for (int i = 0; i < numAttempts; i++) {
        printf("|   %2d    | %7.2f |     %6.2f    |     %3d     |\n",
               i + 1, attempts[i].typingSpeed, attempts[i].accuracy, attempts[i].wrongChars);
    }
    printf("-------------------------------------------------------------\n");
}

void promptDifficulty(Difficulty *difficulty) {
    int choice;
    printf("Select difficulty level:\n1. Easy\n2. Medium\n3. Hard\n");

    while (1) {
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            while (getchar() != '\n');  // flush input
            continue;
        }
        while (getchar() != '\n');  // flush input

        if (choice >= 1 && choice <= 3) break;
        printf("Invalid choice. Please enter a number between 1 and 3.\n");
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
            *difficulty = (Difficulty){EASY_SPEED, EASY_MEDIUM_SPEED, MEDIUM_HARD_SPEED};
            break;
    }
}

int collectUserInput(char* input, int maxLength) {
    // Read user input safely from stdin
    if (fgets(input, maxLength, stdin) == NULL) {
        return 0;
    }

    size_t len = strlen(input);
    if (len > 0 && input[len - 1] == '\n') {
        input[len - 1] = '\0';
    }

    // Validate input: only printable characters or spaces allowed
    for (size_t i = 0; i < strlen(input); i++) {
        if (!isprint((unsigned char)input[i]) && input[i] != '\t' && input[i] != ' ') {
            return 0;
        }
    }

    return 1;
}

int isValidUsername(const char* username) {
    if (strlen(username) == 0) return 0;
    for (size_t i = 0; i < strlen(username); i++) {
        if (!isalnum((unsigned char)username[i]) && username[i] != '_' && username[i] != '-') {
            return 0;
        }
    }
    return 1;
}

void sanitizeUsername(char* username) {
    // Remove leading/trailing whitespace and limit length
    size_t len = strlen(username);

    // Trim trailing whitespace
    while (len > 0 && isspace((unsigned char)username[len - 1])) {
        username[len - 1] = '\0';
        len--;
    }

    // Trim leading whitespace
    size_t start = 0;
    while (username[start] && isspace((unsigned char)username[start])) {
        start++;
    }
    if (start > 0) {
        memmove(username, username + start, len - start + 1);
    }

    // Truncate if longer than max
    if (strlen(username) >= 49) {
        username[49] = '\0';
    }
}

void loadLeaderboard(LeaderboardEntry leaderboard[], int* numEntries) {
    FILE* file = fopen("leaderboard.txt", "r");
    *numEntries = 0;

    if (!file) return;  // No leaderboard yet

    while (*numEntries < MAX_LEADERBOARD_ENTRIES &&
           fscanf(file, "%49s %lf %lf %lf %9s",
                  leaderboard[*numEntries].username,
                  &leaderboard[*numEntries].typingSpeed,
                  &leaderboard[*numEntries].wordsPerMinute,
                  &leaderboard[*numEntries].accuracy,
                  leaderboard[*numEntries].difficulty) == 5) {
        (*numEntries)++;
    }
    fclose(file);
}

void saveLeaderboard(LeaderboardEntry leaderboard[], int numEntries) {
    FILE* file = fopen("leaderboard.txt", "w");
    if (!file) {
        perror("Error saving leaderboard");
        return;
    }

    for (int i = 0; i < numEntries; i++) {
        fprintf(file, "%s %.2f %.2f %.2f %s\n",
                leaderboard[i].username,
                leaderboard[i].typingSpeed,
                leaderboard[i].wordsPerMinute,
                leaderboard[i].accuracy,
                leaderboard[i].difficulty);
    }
    fclose(file);
}

void updateLeaderboard(UserProfile *profile, TypingStats *currentAttempt, const char *difficulty) {
    LeaderboardEntry leaderboard[MAX_LEADERBOARD_ENTRIES];
    int numEntries = 0;

    loadLeaderboard(leaderboard, &numEntries);

    LeaderboardEntry newEntry;
    strncpy(newEntry.username, profile->username, sizeof(newEntry.username));
    newEntry.typingSpeed = currentAttempt->typingSpeed;
    newEntry.wordsPerMinute = currentAttempt->typingSpeed / 5.0;  // Approximate WPM
    newEntry.accuracy = currentAttempt->accuracy;
    strncpy(newEntry.difficulty, difficulty, sizeof(newEntry.difficulty));

    if (numEntries < MAX_LEADERBOARD_ENTRIES) {
        leaderboard[numEntries++] = newEntry;
    } else {
        // Replace worst score if current is better
        int worstIndex = 0;
        for (int i = 1; i < numEntries; i++) {
            if (leaderboard[i].typingSpeed < leaderboard[worstIndex].typingSpeed) {
                worstIndex = i;
            }
        }
        if (newEntry.typingSpeed > leaderboard[worstIndex].typingSpeed) {
            leaderboard[worstIndex] = newEntry;
        }
    }

    // Sort leaderboard by typing speed descending
    for (int i = 0; i < numEntries - 1; i++) {
        for (int j = i + 1; j < numEntries; j++) {
            if (leaderboard[j].typingSpeed > leaderboard[i].typingSpeed) {
                LeaderboardEntry temp = leaderboard[i];
                leaderboard[i] = leaderboard[j];
                leaderboard[j] = temp;
            }
        }
    }

    saveLeaderboard(leaderboard, numEntries);
}

void displayLeaderboard(const char *difficulty) {
    LeaderboardEntry leaderboard[MAX_LEADERBOARD_ENTRIES];
    int numEntries = 0;
    loadLeaderboard(leaderboard, &numEntries);

    printf("\nLeaderboard (%s difficulty):\n", difficulty);
    printf("-------------------------------------------------------------\n");
    printf("| Rank | Username       | CPM     | WPM     | Accuracy(%%) |\n");
    printf("-------------------------------------------------------------\n");

    int rank = 1;
    for (int i = 0; i < numEntries && rank <= 10; i++) {
        if (strcmp(leaderboard[i].difficulty, difficulty) == 0) {
            printf("| %4d | %-14s | %7.2f | %7.2f | %11.2f |\n",
                   rank,
                   leaderboard[i].username,
                   leaderboard[i].typingSpeed,
                   leaderboard[i].wordsPerMinute,
                   leaderboard[i].accuracy);
            rank++;
        }
    }
    if (rank == 1) {
        printf("|                  No entries for this difficulty             |\n");
    }
    printf("-------------------------------------------------------------\n");
}
