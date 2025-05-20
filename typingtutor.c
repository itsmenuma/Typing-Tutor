//header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

//global declarations
#define max_para_length 200
#define max_file_line_length 200
#define max_attempts 10
#define max_leaderboard_entries 100
#define max_paragraphs 100
#define max_username_len 49

// Define named constants for difficulty levels
#define EASY_SPEED 5
#define MEDIUM_SPEED 8
#define HARD_SPEED 12
#define EASY_MEDIUM_SPEED 8
#define MEDIUM_HARD_SPEED 12
#define HARD_MAX_SPEED 16

// Macro for checking file operations
#define CHECK_FILE_OP(file, msg) do { \
    if (!(file)) {                    \
        perror(msg);                  \
        exit(EXIT_FAILURE);           \
    }                                \
} while (0)

//structure to store user profile
typedef struct {
    char username[max_username_len + 1];
    double bestSpeed;
    double bestAccuracy;
    double totalSpeed;
    double totalAccuracy;
    int totalAttempts;
} UserProfile;

//structure to store difficulty
typedef struct {
    int easy;
    int medium;
    int hard;
} Difficulty;

//structure to store typing statistics
typedef struct {
    double typingSpeed;
    double accuracy;
    int wrongChars;
    char paragraph[max_para_length];
} TypingStats;

// Structure for leaderboard entry (included because leaderboard code references it)
typedef struct {
    char username[max_username_len + 1];
    double typingSpeed;
    double wordsPerMinute;
    double accuracy;
    char difficulty[10];
} LeaderboardEntry;

// Globals for paragraph caching
char* cachedParagraphs[max_paragraphs];
int cachedParagraphCount = 0;

// Function prototypes
void loadParagraphs(const char* filename);
void freeCachedParagraphs(void);
char* getRandomParagraphCached(void);
void sanitizeUsername(char* username, size_t size);
void loadUserProfile(UserProfile* profile);
void updateUserProfile(UserProfile* profile, TypingStats* currentAttempt);
void displayUserSummary(const UserProfile* profile);
int promptDifficulty(Difficulty* difficulty);
int collectUserInput(char* inputText, size_t maxLen);
void printTypingStats(double elapsedTime, const char* input, const char* correctText, TypingStats* stats);
void displayPreviousAttempts(TypingStats attempts[], int numAttempts);

int main() {
    srand((unsigned int)time(NULL));
    UserProfile user = {0};
    TypingStats attempts[max_attempts] = {0};
    Difficulty difficulty = {0};

    loadUserProfile(&user);

    if (!promptDifficulty(&difficulty)) {
        printf("Failed to select difficulty. Exiting.\n");
        return 1;
    }

    loadParagraphs("paragraphs.txt");

    int numAttempts = 0;
    char tryAgain;

    do {
        TypingStats currentStats = {0};
        char* paragraph = getRandomParagraphCached();
        if (!paragraph) {
            fprintf(stderr, "No paragraph available.\n");
            break;
        }

        printf("\nType the following paragraph:\n\n%s\n", paragraph);
        printf("\nPress Enter when you finish typing.\n\n");

        clock_t start = clock();
        char inputText[max_para_length] = {0};

        if (!collectUserInput(inputText, max_para_length)) {
            printf("Input error. Please try again.\n");
            continue;
        }
        clock_t end = clock();

        double elapsedTime = (double)(end - start) / CLOCKS_PER_SEC;

        printTypingStats(elapsedTime, inputText, paragraph, &currentStats);

        attempts[numAttempts++] = currentStats;
        updateUserProfile(&user, &currentStats);

        printf("\nTyping Speed: %.2f cpm\n", currentStats.typingSpeed);
        printf("Accuracy: %.2f%%\n", currentStats.accuracy);
        printf("Wrong Characters: %d\n", currentStats.wrongChars);

        if (numAttempts < max_attempts) {
            printf("\nDo you want to try again? (y/n): ");
            int res = scanf(" %c", &tryAgain);
            while (getchar() != '\n'); // flush input buffer

            if (res != 1 || (tolower(tryAgain) != 'y' && tolower(tryAgain) != 'n')) {
                printf("Invalid input. Exiting.\n");
                break;
            }
        } else {
            printf("\nYou've reached the maximum number of attempts.\n");
            break;
        }

    } while (tolower(tryAgain) == 'y');

    displayUserSummary(&user);
    displayPreviousAttempts(attempts, numAttempts);

    freeCachedParagraphs();

    return 0;
}

// Load paragraphs once into cache
void loadParagraphs(const char* filename) {
    FILE* file = fopen(filename, "r");
    CHECK_FILE_OP(file, "Error opening paragraphs file");

    cachedParagraphCount = 0;
    char line[max_file_line_length];

    while (fgets(line, sizeof(line), file) && cachedParagraphCount < max_paragraphs) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
        if (strlen(line) > 0) {
            cachedParagraphs[cachedParagraphCount] = strdup(line);
            if (!cachedParagraphs[cachedParagraphCount]) {
                fprintf(stderr, "Memory allocation failure\n");
                exit(EXIT_FAILURE);
            }
            cachedParagraphCount++;
        }
    }

    fclose(file);

    if (cachedParagraphCount == 0) {
        fprintf(stderr, "No paragraphs found in file.\n");
        exit(EXIT_FAILURE);
    }
}

// Free cached paragraphs on exit
void freeCachedParagraphs(void) {
    for (int i = 0; i < cachedParagraphCount; i++) {
        free(cachedParagraphs[i]);
        cachedParagraphs[i] = NULL;
    }
    cachedParagraphCount = 0;
}

// Return a random paragraph from cached paragraphs
char* getRandomParagraphCached(void) {
    if (cachedParagraphCount == 0) return NULL;
    int index = rand() % cachedParagraphCount;
    return cachedParagraphs[index];
}

// Sanitize username input: remove newline and limit length
void sanitizeUsername(char* username, size_t size) {
    size_t len = strnlen(username, size);
    if (len > 0 && username[len - 1] == '\n') {
        username[len - 1] = '\0';
    }
    // Remove any non-alphanumeric chars (optional, simple sanitization)
    for (size_t i = 0; i < strlen(username); i++) {
        if (!isalnum(username[i]) && username[i] != '_' && username[i] != '-') {
            username[i] = '_';
        }
    }
}

// Load user profile or initialize if not found
void loadUserProfile(UserProfile* profile) {
    printf("Enter your username (max %d chars): ", max_username_len);
    if (!fgets(profile->username, sizeof(profile->username), stdin)) {
        fprintf(stderr, "Input error.\n");
        exit(EXIT_FAILURE);
    }
    sanitizeUsername(profile->username, sizeof(profile->username));

    char filename[100];
    snprintf(filename, sizeof(filename), "%s_profile.txt", profile->username);

    FILE* f = fopen(filename, "r");
    if (f != NULL) {
        if (fscanf(f, "%lf %lf %lf %lf %d",
                   &profile->bestSpeed, &profile->bestAccuracy,
                   &profile->totalSpeed, &profile->totalAccuracy,
                   &profile->totalAttempts) != 5) {
            // Reset if read fails
            profile->bestSpeed = 0;
            profile->bestAccuracy = 0;
            profile->totalSpeed = 0;
            profile->totalAccuracy = 0;
            profile->totalAttempts = 0;
        }
        fclose(f);
    } else {
        profile->bestSpeed = 0;
        profile->bestAccuracy = 0;
        profile->totalSpeed = 0;
        profile->totalAccuracy = 0;
        profile->totalAttempts = 0;
    }
}

// Update user profile file
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
    if (f != NULL) {
        fprintf(f, "%.2lf %.2lf %.2lf %.2lf %d\n",
                profile->bestSpeed, profile->bestAccuracy,
                profile->totalSpeed, profile->totalAccuracy,
                profile->totalAttempts);
        fclose(f);
    } else {
        perror("Error saving user profile");
    }
}

// Display user summary
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

// Prompt difficulty, return 1 if success else 0
int promptDifficulty(Difficulty* difficulty) {
    int choice;
    printf("Select difficulty level:\n1. Easy\n2. Medium\n3. Hard\n");

    while (1) {
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            while (getchar() != '\n'); // flush input buffer
            continue;
        }
        while (getchar() != '\n'); // flush newline
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
            return 0;
    }
    return 1;
}

// Collect user input safely, return 1 if success else 0
int collectUserInput(char* inputText, size_t maxLen) {
    size_t idx = 0;
    int ch;
    while (idx < maxLen - 1) {
        ch = getchar();
        if (ch == EOF) return 0;
        if (ch == '\n') break;
        inputText[idx++] = (char)ch;
    }
    inputText[idx] = '\0';
    return 1;
}

// Calculate and print typing stats
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
    strncpy(stats->paragraph, correctText, max_para_length);
}

// Display previous attempts summary
void displayPreviousAttempts(TypingStats attempts[], int numAttempts) {
    printf("\nPrevious Attempts:\n");
    printf("-------------------------------------------------\n");
    printf("| Attempt |  CPM  | Accuracy (%%) | Wrong Chars |\n");
    printf("-------------------------------------------------\n");

    for (int i = 0; i < numAttempts; i++) {
        printf("|   %2d    | %.2f |    %.2f%%    |     %2d      |\n",
               i + 1, attempts[i].typingSpeed, attempts[i].accuracy, attempts[i].wrongChars);
    }

    printf("-------------------------------------------------\n");
}
