#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <sys/time.h>
#include <strings.h>
#include <math.h>

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__WINDOWS__)
    #include <conio.h>
    #include <windows.h>
    #define IS_WINDOWS 1
#else
    #include <termios.h>
    #include <unistd.h>
    #define IS_WINDOWS 0
#endif

#if IS_WINDOWS
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#ifndef ENABLE_VIRTUAL_TERMINAL_INPUT
#define ENABLE_VIRTUAL_TERMINAL_INPUT 0x0200
#endif
#endif

#define max_para_length 200
#define max_file_line_length 200
#define max_attempts 10
#define max_leaderboard_entries 100

#define EASY_SPEED 5
#define EASY_MEDIUM_SPEED 8
#define MEDIUM_HARD_SPEED 12
#define HARD_SPEED 12
#define HARD_MAX_SPEED 16

#define CHECK_FILE_OP(f, msg) do { if (!(f)) { perror(msg); exit(EXIT_FAILURE); } } while (0)

#define ANSI_RESET "\033[0m"
#define ANSI_RED "\033[1;31m"
#define ANSI_GREEN "\033[1;32m"
#define ANSI_YELLOW "\033[1;33m"
#define ANSI_WHITE "\033[1;37m"
#define ANSI_BG_RED "\033[41m"
#define ANSI_BG_GREEN "\033[42m"
#define ANSI_CLEAR_SCREEN "\033[2J"
#define ANSI_CURSOR_HOME "\033[H"
#define ANSI_CURSOR_UP "\033[A"

typedef struct {
    char username[50];
    double bestSpeed;
    double bestAccuracy;
    double totalSpeed;
    double totalAccuracy;
    int totalAttempts;
} UserProfile;

typedef struct {
    int easy;
    int medium;
    int hard;
} Difficulty;

typedef struct {
    double typingSpeed;   
    double wordsPerMinute; 
    double accuracy;
    int wrongChars;
    char paragraph[max_para_length];
    int caseInsensitive;
} TypingStats;

typedef struct {
    char username[50];
    double typingSpeed;   
    double wordsPerMinute; 
    double accuracy;
    char difficulty[20]; 
} LeaderboardEntry;

typedef struct {
    char **paragraphs;
    int count;
} ParagraphCache;

// Function declarations (unchanged)
void loadParagraphs(FILE *file, ParagraphCache *cache);
void freeParagraphCache(ParagraphCache *cache);
char *getRandomParagraph(ParagraphCache *cache);
void sanitizeUsername(char *username, size_t size);
void loadUserProfile(UserProfile *profile);
void updateUserProfile(UserProfile *profile, TypingStats *currentAttempt);
void printTypingStats(double elapsedTime, const char *input, const char *correctText, Difficulty difficulty, TypingStats *stats);
void loadLeaderboard(LeaderboardEntry leaderboard[], int *numEntries);
void saveLeaderboard(LeaderboardEntry leaderboard[], int numEntries);
void updateLeaderboard(UserProfile *profile, TypingStats *currentAttempt, const char *difficulty);
void displayLeaderboard(const char *difficulty);
int min3(int a, int b, int c);
int levenshtein(const char *s1, const char *s2, int caseInsensitive);
void toLowerStr(char *dst, const char *src);
void trim_newline(char *str);
void promptDifficulty(Difficulty *difficulty, char *difficultyLevel);
void displayPreviousAttempts(TypingStats attempts[], int numAttempts);
void loadParagraphsForDifficulty(FILE *file, ParagraphCache *cache, const char *difficultyLevel);
void displayUserSummary(UserProfile *profile);
void collectUserInput(char *input, size_t inputSize, double *elapsedTime);
int isValidInput(const char *input);
void processAttempts(ParagraphCache *cache);
char getRealTimeChar();
void clearScreen();
void enableWindowsColorSupport();
void initializeRealtimeMode();
void displayRealtimeTyping(const char* targetText, const char* userInput, int currentPos, int wrongChars, double elapsedTime);
void collectUserInputRealtime(const char* targetText, char *input, size_t inputSize, double *elapsedTime, TypingStats *stats);
int promptTypingMode();

// Function implementations (unchanged except for main)
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

void save_progress(int wpm, int accuracy) {
    FILE *file = fopen("progress.txt", "a");
    if (file) {
        time_t now = time(0);
        struct tm *t = localtime(&now);
        fprintf(file, "%d,%d,%04d-%02d-%02d\n", 
                wpm, accuracy, 
                t->tm_year+1900, t->tm_mon+1, t->tm_mday);
        fclose(file);
    }
}

// Load paragraphs for specific difficulty into cache
void loadParagraphsForDifficulty(FILE *file, ParagraphCache *cache, const char *difficultyLevel)
{
    char line[max_file_line_length];
    int inSection = 0;
    int count = 0;
    char marker[16];
    snprintf(marker, sizeof(marker), "#%s", difficultyLevel);

    while (fgets(line, sizeof(line), file)) {
        trim_newline(line);
        if (line[0] == '#') {
            inSection = (strcasecmp(line, marker) == 0);
            continue;
        }
        if (inSection && strlen(line) > 0 && line[0] != '#') {
            count++;
        }
    }

    if (count == 0) {
        cache->paragraphs = NULL;
        cache->count = 0;
        return;
    }

    cache->paragraphs = malloc(count * sizeof(char *));
    cache->count = count;

    fseek(file, 0, SEEK_SET);
    inSection = 0;
    int index = 0;
    while (fgets(line, sizeof(line), file)) {
        trim_newline(line);
        if (line[0] == '#') {
            inSection = (strcasecmp(line, marker) == 0);
            continue;
        }
        if (inSection && strlen(line) > 0 && line[0] != '#') {
            cache->paragraphs[index++] = strdup(line);
            if (index >= count) break;
        }
    }
}

void freeParagraphCache(ParagraphCache *cache) {
    for (int i = 0; i < cache->count; i++) {
        free(cache->paragraphs[i]);
        cache->paragraphs[i] = NULL;
    }
    free(cache->paragraphs);
    cache->paragraphs = NULL;
}

char *getRandomParagraph(ParagraphCache *cache) {
    if (cache->count == 0) {
        fprintf(stderr, "Error: No paragraphs available.\n");
        exit(EXIT_FAILURE);
    }
    return cache->paragraphs[rand() % cache->count];
}

void sanitizeUsername(char *username, size_t size) {
    for (size_t i = 0; i < strlen(username) && i < size - 1; i++) {
        if (!isalnum(username[i]) && username[i] != '-' && username[i] != '_') {
            username[i] = '_';
        }
    }
    username[size - 1] = '\0';
}

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

void printTypingStats(double elapsedTime, const char *input, const char *correctText, Difficulty difficulty, TypingStats *stats) {
    int dist = levenshtein(correctText, input, stats->caseInsensitive);
    int len = strlen(correctText);
    double accuracy = ((double)(len - dist) / len) * 100.0;
    if (accuracy < 0) accuracy = 0;

    if (elapsedTime < 0.01) elapsedTime = 0.01;

    double cpm = (strlen(input) / elapsedTime) * 60.0;
    double wpm = cpm / 5.0;

    stats->typingSpeed = cpm;
    stats->wordsPerMinute = wpm;
    stats->accuracy = accuracy;
    stats->wrongChars = dist;
    strncpy(stats->paragraph, correctText, max_para_length - 1);
    stats->paragraph[max_para_length - 1] = '\0';
}

int min3(int a, int b, int c) {
    if (a <= b && a <= c) return a;
    else if (b <= c) return b;
    else return c;
}

int levenshtein(const char *s1, const char *s2, int caseInsensitive) {
    int len1 = strlen(s1);
    int len2 = strlen(s2);
    int i, j;

    int **dp = malloc((len1 + 1) * sizeof(int *));
    for (i = 0; i <= len1; i++)
        dp[i] = malloc((len2 + 1) * sizeof(int));

    for (i = 0; i <= len1; i++) dp[i][0] = i;
    for (j = 0; j <= len2; j++) dp[0][j] = j;

    for (i = 1; i <= len1; i++) {
        for (j = 1; j <= len2; j++) {
            char c1 = s1[i - 1];
            char c2 = s2[j - 1];
            if (caseInsensitive) {
                c1 = tolower(c1);
                c2 = tolower(c2);
            }
            if (c1 == c2)
                dp[i][j] = dp[i - 1][j - 1];
            else
                dp[i][j] = 1 + min3(dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]);
        }
    }

    int distance = dp[len1][len2];
    for (i = 0; i <= len1; i++) free(dp[i]);
    free(dp);

    return distance;
}

void loadLeaderboard(LeaderboardEntry leaderboard[], int *numEntries) {
    FILE *file = fopen("leaderboard.txt", "r");
    if (!file) {
        *numEntries = 0;
        return;
    }

    *numEntries = 0;
    while (fscanf(file, "%49s %lf %lf %lf %9s",
                  leaderboard[*numEntries].username,
                  &leaderboard[*numEntries].typingSpeed,
                  &leaderboard[*numEntries].wordsPerMinute,
                  &leaderboard[*numEntries].accuracy,
                  leaderboard[*numEntries].difficulty) == 5) {
        (*numEntries)++;
        if (*numEntries >= max_leaderboard_entries) break;
    }
    fclose(file);
}

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

void updateLeaderboard(UserProfile *profile, TypingStats *currentAttempt, const char *difficulty) {
    LeaderboardEntry leaderboard[max_leaderboard_entries];
    int numEntries;
    int replaced = 0;
    loadLeaderboard(leaderboard, &numEntries);

    LeaderboardEntry newEntry;
    strncpy(newEntry.username, profile->username, sizeof(newEntry.username) - 1);
    newEntry.username[sizeof(newEntry.username) - 1] = '\0';
    newEntry.typingSpeed = currentAttempt->typingSpeed;
    newEntry.wordsPerMinute = currentAttempt->wordsPerMinute;
    newEntry.accuracy = currentAttempt->accuracy;
    strncpy(newEntry.difficulty, difficulty, sizeof(newEntry.difficulty) - 1);
    newEntry.difficulty[sizeof(newEntry.difficulty) - 1] = '\0';

    for (int i = 0; i < numEntries; i++) {
        if (strcmp(leaderboard[i].username, newEntry.username) == 0 &&
            strcmp(leaderboard[i].difficulty, newEntry.difficulty) == 0 &&
            newEntry.typingSpeed > leaderboard[i].typingSpeed) {
            leaderboard[i] = newEntry;
            replaced = 1;
            break;
        }
    }

    if (!replaced) {
        if (numEntries < max_leaderboard_entries) {
            leaderboard[numEntries++] = newEntry;
        } else {
            int worstIndex = -1;
            double worstScore = 1e9;
            for (int i = 0; i < numEntries; i++) {
                if (strcmp(leaderboard[i].difficulty, newEntry.difficulty) == 0 &&
                    leaderboard[i].typingSpeed < worstScore) {
                    worstScore = leaderboard[i].typingSpeed;
                    worstIndex = i;
                }
            }
            if (worstIndex != -1 && newEntry.typingSpeed > leaderboard[worstIndex].typingSpeed) {
                leaderboard[worstIndex] = newEntry;
            }
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

void displayLeaderboard(const char *difficulty) {
    LeaderboardEntry leaderboard[max_leaderboard_entries];
    int numEntries;
    loadLeaderboard(leaderboard, &numEntries);

    printf("\nLeaderboard for %s Difficulty:\n", difficulty);
    printf("-------------------------------------------------------------\n");
    printf("| Rank | Username       | CPM    | WPM    | Accuracy (%%) |\n");
    printf("-------------------------------------------------------------\n");

    int rank = 1;
    int shown = 0;
    for (int i = 0; i < numEntries && rank <= 10; i++) {
        if (strcmp(leaderboard[i].difficulty, difficulty) == 0) {
            printf("| %4d | %-14s | %6.2f | %6.2f | %10.2f |\n",
                   rank, leaderboard[i].username,
                   leaderboard[i].typingSpeed, leaderboard[i].wordsPerMinute,
                   leaderboard[i].accuracy);
            rank++;
            shown++;
        }
    }
    if (shown == 0) {
        printf("|      No entries for this difficulty level yet          |\n");
    }
    printf("-------------------------------------------------------------\n");
}

void collectUserInput(char *input, size_t inputSize, double *elapsedTime) {
    struct timeval startTime, endTime;
    gettimeofday(&startTime, NULL);
    printf("Your input: \n");
    fflush(stdout);
    CHECK_FILE_OP(fgets(input, inputSize, stdin), "Error reading input");
    gettimeofday(&endTime, NULL);
    *elapsedTime = (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec) / 1000000.0;
}

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

void promptDifficulty(Difficulty *difficulty, char *difficultyLevel) {
    int choice;
    printf("Select difficulty:\n");
    printf("1. Easy\n2. Medium\n3. Hard\n");
    printf("Enter your choice (1-3): ");
    while (scanf("%d", &choice) != 1 || choice < 1 || choice > 3) {
        printf("Invalid input. Please enter 1, 2, or 3: ");
        while (getchar() != '\n');
    }
    while (getchar() != '\n');

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

void displayPreviousAttempts(TypingStats attempts[], int numAttempts) {
    printf("\nPrevious Attempts:\n");
    printf("--------------------------------------------------------\n");
    printf("| Attempt | CPM    | WPM    | Accuracy (%%) | Wrong Chars |\n");
    printf("--------------------------------------------------------\n");
    for (int i = 0; i < numAttempts; i++) {
        printf("| %7d | %6.2f | %6.2f | %11.2f | %11d |\n",
               i + 1,
               attempts[i].typingSpeed,
               attempts[i].wordsPerMinute,
               attempts[i].accuracy,
               attempts[i].wrongChars);
    }
    printf("--------------------------------------------------------\n");
}

char getRealTimeChar() {
#if IS_WINDOWS
    return _getch();
#else
    struct termios old, new;
    char ch;
    
    if (tcgetattr(STDIN_FILENO, &old) != 0) {
        return getchar();
    }
    
    new = old;
    new.c_lflag &= ~(ICANON | ECHO);
    new.c_cc[VMIN] = 1;
    new.c_cc[VTIME] = 0;
    
    if (tcsetattr(STDIN_FILENO, TCSANOW, &new) != 0) {
        return getchar();
    }
    
    if (read(STDIN_FILENO, &ch, 1) != 1) {
        ch = 0;
    }
    
    tcsetattr(STDIN_FILENO, TCSANOW, &old);
    
    return ch;
#endif
}

void clearScreen() {
#if IS_WINDOWS
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD count;
    DWORD cellCount;
    COORD homeCoords = {0, 0};
    
    if (hConsole == INVALID_HANDLE_VALUE) {
        printf("\033[2J\033[H");
        fflush(stdout);
        return;
    }
    
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        printf("\033[2J\033[H");
        fflush(stdout);
        return;
    }
    
    cellCount = csbi.dwSize.X * csbi.dwSize.Y;
    
    if (!FillConsoleOutputCharacter(hConsole, (TCHAR)' ', cellCount, homeCoords, &count)) {
        printf("\033[2J\033[H");
        fflush(stdout);
        return;
    }
    
    if (!FillConsoleOutputAttribute(hConsole, csbi.wAttributes, cellCount, homeCoords, &count)) {
        printf("\033[2J\033[H");
        fflush(stdout);
        return;
    }
    
    SetConsoleCursorPosition(hConsole, homeCoords);
#else
    printf("\033[2J\033[H");
    fflush(stdout);
#endif
}

void enableWindowsColorSupport() {
#if IS_WINDOWS
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
#endif
}

void initializeRealtimeMode() {
    enableWindowsColorSupport();
    clearScreen();
    printf(ANSI_GREEN "Real-Time Typing Mode Initialized!\n" ANSI_RESET);
    printf(ANSI_WHITE "Platform: ");
#if IS_WINDOWS
    printf("Windows\n");
#else
    printf("Unix/Linux/macOS\n");
#endif
    printf("Color support: Enabled\n" ANSI_RESET);
    printf("\nPress any key to continue...\n");
    getRealTimeChar();
}

void displayRealtimeTyping(const char* targetText, const char* userInput, int currentPos, int wrongChars, double elapsedTime) {
    int targetLen = strlen(targetText);
    int inputLen = strlen(userInput);
    
    clearScreen();
    
    printf(ANSI_GREEN "=== Real-Time Typing Mode ===\n" ANSI_RESET);
    printf(ANSI_WHITE "Target Text:\n" ANSI_RESET);
    
    for (int i = 0; i < targetLen; i++) {
        if (i < inputLen) {
            if (userInput[i] == targetText[i]) {
                printf(ANSI_GREEN "%c" ANSI_RESET, targetText[i]);
            } else {
                printf(ANSI_BG_RED ANSI_WHITE "%c" ANSI_RESET, targetText[i]);
            }
        } else if (i == currentPos) {
            printf(ANSI_YELLOW "%c" ANSI_RESET, targetText[i]);
        } else {
            printf(ANSI_WHITE "%c" ANSI_RESET, targetText[i]);
        }
    }
    
    printf("\n\n" ANSI_WHITE "Your Input:\n" ANSI_RESET);
    
    for (int i = 0; i < inputLen; i++) {
        if (i < targetLen) {
            if (userInput[i] == targetText[i]) {
                printf(ANSI_GREEN "%c" ANSI_RESET, userInput[i]);
            } else {
                printf(ANSI_RED "%c" ANSI_RESET, userInput[i]);
            }
        } else {
            printf(ANSI_RED "%c" ANSI_RESET, userInput[i]);
        }
    }
    
    printf(ANSI_YELLOW "_" ANSI_RESET);
    
    printf("\n\n" ANSI_WHITE "Progress: %d/%d characters | Errors: %d | Time: %.1fs\n" ANSI_RESET, 
           currentPos, targetLen, wrongChars, elapsedTime);
    
    if (currentPos > 0) {
        double currentCPM = (currentPos / elapsedTime) * 60.0;
        double currentWPM = currentCPM / 5.0;
        printf("Current Speed: %.1f CPM (%.1f WPM)\n", currentCPM, currentWPM);
    }
    
    printf("\n" ANSI_YELLOW "Controls: ESC=quit | Backspace=correct | Any key=type" ANSI_RESET "\n");
    
    fflush(stdout);
}

void collectUserInputRealtime(const char* targetText, char *input, size_t inputSize, double *elapsedTime, TypingStats *stats) {
    struct timeval startTime, currentTime;
    gettimeofday(&startTime, NULL);
    
    int targetLen = strlen(targetText);
    int currentPos = 0;
    int wrongChars = 0;
    char ch;
    
    memset(input, 0, inputSize);
    
    initializeRealtimeMode();
    
    while (currentPos < targetLen) {
        gettimeofday(&currentTime, NULL);
        *elapsedTime = (currentTime.tv_sec - startTime.tv_sec) + 
                       (currentTime.tv_usec - startTime.tv_usec) / 1000000.0;
        
        displayRealtimeTyping(targetText, input, currentPos, wrongChars, *elapsedTime);
        
        ch = getRealTimeChar();
        
        if (ch == 27) {
            printf(ANSI_RED "\nTest cancelled by user.\n" ANSI_RESET);
            input[0] = '\0';
            return;
        } else if (ch == 8 || ch == 127) {
            if (currentPos > 0) {
                currentPos--;
                input[currentPos] = '\0';
                wrongChars = 0;
                for (int i = 0; i < currentPos; i++) {
                    if (input[i] != targetText[i]) {
                        wrongChars++;
                    }
                }
            }
        } else if (ch >= 32 && ch <= 126 && currentPos < (int)(inputSize - 1)) {
            input[currentPos] = ch;
            input[currentPos + 1] = '\0';
            
            if (ch != targetText[currentPos]) {
                wrongChars++;
            }
            
            currentPos++;
        }
        
        if (currentPos >= targetLen) {
            input[targetLen] = '\0';
            break;
        }
    }
    
    gettimeofday(&currentTime, NULL);
    *elapsedTime = (currentTime.tv_sec - startTime.tv_sec) + 
                   (currentTime.tv_usec - startTime.tv_usec) / 1000000.0;
    
    clearScreen();
    displayRealtimeTyping(targetText, input, currentPos, wrongChars, *elapsedTime);
    
    printf(ANSI_GREEN "\n=== Test Completed! ===\n" ANSI_RESET);
    printf("Press any key to continue...\n");
    getRealTimeChar();
    
    stats->wrongChars = wrongChars;
}

int promptTypingMode() {
    int choice;
    printf("\nSelect typing mode:\n");
    printf("1. Classic Mode (type entire paragraph, then see results)\n");
    printf("2. Real-Time Mode (see errors highlighted as you type)\n");
    printf("Enter your choice (1-2): ");
    
    while (scanf("%d", &choice) != 1 || choice < 1 || choice > 2) {
        printf("Invalid input. Please enter 1 or 2: ");
        while (getchar() != '\n');
    }
    while (getchar() != '\n');
    
    return choice;
}

void toLowerStr(char *dst, const char *src) {
    while (*src) {
        *dst++ = tolower((unsigned char)*src++);
    }
    *dst = '\0';
}

void trim_newline(char *str) {
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[--len] = '\0';
    }
}

void processAttempts(ParagraphCache *cache) {
    printf("Welcome to Typing Tutor!\n");
    UserProfile profile;
    loadUserProfile(&profile);

    char input[max_para_length];
    Difficulty difficulty;
    char difficultyLevel[20];
    TypingStats attempts[max_attempts];
    int numAttempts = 0;
    int caseChoice;
    int typingMode;

    promptDifficulty(&difficulty, difficultyLevel);
    typingMode = promptTypingMode();

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
        TypingStats currentAttempt = {.caseInsensitive = caseChoice};
        
        if (typingMode == 2) {
            collectUserInputRealtime(currentPara, input, sizeof(input), &elapsedTime, &currentAttempt);
            if (strlen(input) == 0) {
                printf("Attempt cancelled. Try again.\n");
                continue;
            }
        } else {
            collectUserInput(input, sizeof(input), &elapsedTime);
        }

        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n')
            input[len - 1] = '\0';

        if (!isValidInput(input)) {
            printf("Input cannot be empty or contain only whitespace. Please try again.\n");
            continue;
        }

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

int main(int argc, char *argv[]) {
    srand((unsigned int)time(NULL));
    ParagraphCache cache = {0};

    if (argc == 3 && strcmp(argv[1], "--get-paragraph") == 0) {
        const char *difficultyLevel = argv[2];
        FILE *file = fopen("paragraphs.txt", "r");
        if (!file) {
            fprintf(stderr, "Error: Could not open paragraphs.txt\n");
            return 1;
        }
        loadParagraphsForDifficulty(file, &cache, difficultyLevel);
        fclose(file);
        if (cache.count == 0) {
            fprintf(stderr, "No paragraphs found for difficulty: %s\n", difficultyLevel);
            return 1;
        }
        char *para = getRandomParagraph(&cache);
        printf("Random Paragraph:\n%s\n", para);
        freeParagraphCache(&cache);
        return 0;
    }

    if (argc >= 2 && strcmp(argv[1], "--get-leaderboard") == 0) {
        const char *difficulty = (argc >= 3) ? argv[2] : "Easy";
        const char *currentUser = (argc >= 4) ? argv[3] : NULL;
        double userCPM = (argc >= 5) ? atof(argv[4]) : -1;
        double userWPM = (argc >= 6) ? atof(argv[5]) : -1;
        double userAccuracy = (argc >= 7) ? atof(argv[6]) : -1;

        LeaderboardEntry leaderboard[max_leaderboard_entries];
        int numEntries;
        loadLeaderboard(leaderboard, &numEntries);

        printf("\nLeaderboard for %s Difficulty:\n", difficulty);
        printf("-------------------------------------------------------------\n");
        printf("| Rank | Username       | CPM    | WPM    | Accuracy (%%) |\n");
        printf("-------------------------------------------------------------\n");

        int rank = 1;
        int shown = 0;
        for (int i = 0; i < numEntries && rank <= 10; i++) {
            if (strcmp(leaderboard[i].difficulty, difficulty) == 0) {
                printf("| %4d | %-14s | %6.2f | %6.2f | %10.2f |\n",
                       rank, leaderboard[i].username,
                       leaderboard[i].typingSpeed, leaderboard[i].wordsPerMinute,
                       leaderboard[i].accuracy);
                rank++;
                shown++;
            }
        }

        if (shown == 0) {
            printf("|      No entries for this difficulty level yet          |\n");
        }
        printf("-------------------------------------------------------------\n");

        if (currentUser && userCPM > 0 && userWPM > 0 && userAccuracy > 0) {
            int userRank = 1;
            for (int i = 0; i < numEntries; i++) {
                if (strcmp(leaderboard[i].difficulty, difficulty) == 0) {
                    if (
                        strcmp(leaderboard[i].username, currentUser) == 0 &&
                        fabs(leaderboard[i].typingSpeed - userCPM) < 0.01 &&
                        fabs(leaderboard[i].wordsPerMinute - userWPM) < 0.01 &&
                        fabs(leaderboard[i].accuracy - userAccuracy) < 0.01
                    ) {
                        if (userRank > 10) {
                            printf("\nYour Result:\n");
                            printf("| %4d | %-14s | %6.2f | %6.2f | %10.2f |\n",
                                   userRank,
                                   leaderboard[i].username,
                                   leaderboard[i].typingSpeed,
                                   leaderboard[i].wordsPerMinute,
                                   leaderboard[i].accuracy);
                        }
                        break;
                    }
                    userRank++;
                }
            }
        }
        return 0;
    }

    if (argc == 1) {
        FILE *file = fopen("paragraphs.txt", "r");
        if (!file) {
            file = fopen("paragraphs.txt", "w");
            CHECK_FILE_OP(file, "Creating paragraphs.txt failed");
            fprintf(file, "One day after a heavy meal. It was sleeping under a tree.\n");
            fprintf(file, "After a while, there came a mouse and it started to play on the lion.\n");
            fprintf(file, "Suddenly the lion got up with anger and looked for those who disturbed its nice sleep.\n");
            fprintf(file, "Then it saw a small mouse standing trembling with fear.\n");
            fprintf(file, "The lion jumped on it and started to kill it. The mouse requested the lion to forgive it.\n");
            fprintf(file, "The lion felt pity and left it. The mouse ran away.\n");
            fprintf(file, "On another day, the lion was caught in a net by a hunter. The mouse came there and cut the net.\n");
            fprintf(file, "Thus it escaped. There after, the mouse and the lion became friends.\n");
            fprintf(file, "They lived happily in the forest afterwards.\n");
            fclose(file);
            file = fopen("paragraphs.txt", "r");
            CHECK_FILE_OP(file, "Error opening paragraphs.txt");
        }
        loadParagraphs(file, &cache);
        fclose(file);
        processAttempts(&cache);
        freeParagraphCache(&cache);
        return 0;
    }

    if (argc < 7) {
        printf("Usage: %s <username> <difficulty> <caseInsensitive> <elapsedTime> <userInput> <paragraph>\n", argv[0]);
        return 1;
    }

    const char *username = argv[1];
    const char *difficultyLevel = argv[2];
    int caseInsensitive = atoi(argv[3]);
    double elapsedTime = atof(argv[4]);
    const char *userInput = argv[5];
    const char *para = argv[6];

    printf("Random Paragraph:\n%s\n", para);

    Difficulty difficulty;
    if (strcmp(difficultyLevel, "Easy") == 0)
        difficulty = (Difficulty){EASY_SPEED, EASY_MEDIUM_SPEED, MEDIUM_HARD_SPEED};
    else if (strcmp(difficultyLevel, "Medium") == 0)
        difficulty = (Difficulty){EASY_MEDIUM_SPEED, MEDIUM_HARD_SPEED, HARD_MAX_SPEED};
    else
        difficulty = (Difficulty){MEDIUM_HARD_SPEED, HARD_MAX_SPEED, HARD_SPEED + 4};

    TypingStats stats = {.caseInsensitive = caseInsensitive};
    char userInputCopy[max_para_length], paraCopy[max_para_length];
    if (caseInsensitive) {
        toLowerStr(userInputCopy, userInput);
        toLowerStr(paraCopy, para);
        printTypingStats(elapsedTime, userInputCopy, paraCopy, difficulty, &stats);
    } else {
        printTypingStats(elapsedTime, userInput, para, difficulty, &stats);
    }

    printf("\nTyping Stats:\n");
    printf("CPM: %.2f\n", stats.typingSpeed);
    printf("WPM: %.2f\n", stats.wordsPerMinute);
    printf("Accuracy: %.2f%%\n", stats.accuracy);
    printf("Wrong Characters: %d\n", stats.wrongChars);

    if (stats.typingSpeed >= difficulty.hard) {
        printf("Performance: Excellent! You passed the Hard threshold.\n");
    } else if (stats.typingSpeed >= difficulty.medium) {
        printf("Performance: Good! You passed the Medium threshold.\n");
    } else if (stats.typingSpeed >= difficulty.easy) {
        printf("Performance: Fair! You passed the Easy threshold.\n");
    } else {
        printf("Performance: Needs Improvement. Try to type faster!\n");
    }

    FILE *f = fopen("leaderboard.txt", "a");
    if (f) {
        fprintf(f, "%s %.2f %.2f %.2f %s\n", username, stats.typingSpeed, stats.wordsPerMinute, stats.accuracy, difficultyLevel);
        fclose(f);
    }

    UserProfile profile;
    strncpy(profile.username, username, sizeof(profile.username));
    profile.bestSpeed = stats.typingSpeed;
    profile.bestAccuracy = stats.accuracy;
    profile.totalSpeed = stats.typingSpeed;
    profile.totalAccuracy = stats.accuracy;
    profile.totalAttempts = 1;
    updateLeaderboard(&profile, &stats, difficultyLevel);

    return 0;
}