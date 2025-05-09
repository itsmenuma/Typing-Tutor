#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAX_PARA_LENGTH 500
#define MAX_LINE_LENGTH 500
#define MAX_ATTEMPTS 10
#define HISTORY_FILE "history.txt"

typedef struct {
    int easy;
    int medium;
    int hard;
    char label[10];
} Difficulty;

typedef struct {
    double typingSpeed;
    double accuracy;
    int wrongChars;
    char paragraph[MAX_PARA_LENGTH];
    char timestamp[30];
} TypingStats;

void trimNewline(char* str) {
    size_t len = strlen(str);
    if (len && str[len - 1] == '\n') str[len - 1] = '\0';
}

char* getRandomParagraph(FILE* file) {
    char line[MAX_LINE_LENGTH];
    char* paragraphs[100];
    int count = 0;

    while (fgets(line, sizeof(line), file)) {
        trimNewline(line);
        if (strlen(line) > 0) {
            paragraphs[count] = strdup(line);
            count++;
        }
    }

    if (count == 0) {
        fprintf(stderr, "No paragraphs found.\n");
        exit(EXIT_FAILURE);
    }

    int idx = rand() % count;
    char* selected = strdup(paragraphs[idx]);

    for (int i = 0; i < count; i++) free(paragraphs[i]);
    return selected;
}

void printTypingStats(double elapsedTime, const char* input, const char* correctText, TypingStats* stats) {
    int correctCount = 0, wrongCount = 0;
    int minLen = strlen(correctText) < strlen(input) ? strlen(correctText) : strlen(input);

    for (int i = 0; i < minLen; i++) {
        if (correctText[i] == input[i]) correctCount++;
        else wrongCount++;
    }

    int totalCharacters = minLen;
    double accuracy = (double)correctCount / totalCharacters * 100;
    double typingSpeed = (totalCharacters / 5.0) / (elapsedTime / 60.0);

    stats->typingSpeed = typingSpeed;
    stats->accuracy = accuracy;
    stats->wrongChars = wrongCount;
    strncpy(stats->paragraph, correctText, MAX_PARA_LENGTH);

    time_t now = time(NULL);
    strftime(stats->timestamp, sizeof(stats->timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
}

void saveAttemptToFile(const TypingStats* stat) {
    FILE* file = fopen(HISTORY_FILE, "a");
    if (!file) {
        perror("Unable to save history");
        return;
    }

    fprintf(file, "%s | Speed: %.2f CPM | Accuracy: %.2f%% | Wrong: %d\n",
            stat->timestamp, stat->typingSpeed, stat->accuracy, stat->wrongChars);
    fclose(file);
}

void showHistory() {
    FILE* file = fopen(HISTORY_FILE, "r");
    if (!file) {
        printf("No previous history found.\n");
        return;
    }

    printf("\n--- Previous Attempts History ---\n");
    char line[300];
    while (fgets(line, sizeof(line), file)) {
        printf("%s", line);
    }
    fclose(file);
    printf("---------------------------------\n");
}

void promptDifficulty(Difficulty* difficulty) {
    int choice;
    printf("Select difficulty level:\n1. Easy\n2. Medium\n3. Hard\n");

    while (1) {
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Try again.\n");
            while (getchar() != '\n');
            continue;
        }
        while (getchar() != '\n');
        if (choice >= 1 && choice <= 3) break;
        printf("Please enter 1, 2, or 3.\n");
    }

    switch (choice) {
        case 1: *difficulty = (Difficulty){5, 8, 12, "Easy"}; break;
        case 2: *difficulty = (Difficulty){8, 12, 16, "Medium"}; break;
        case 3: *difficulty = (Difficulty){12, 16, 20, "Hard"}; break;
    }
}

void processAttempts(FILE* file) {
    srand((unsigned int)time(NULL));
    char input[MAX_PARA_LENGTH];
    Difficulty difficulty;
    TypingStats attempt;

    promptDifficulty(&difficulty);

    while (1) {
        char* paragraph = getRandomParagraph(file);
        printf("\nType the following paragraph:\n%s\n", paragraph);

        printf("Your input:\n");
        clock_t start = clock();
        if (!fgets(input, sizeof(input), stdin)) {
            perror("Error reading input");
            exit(EXIT_FAILURE);
        }
        clock_t end = clock();

        double timeTaken = (double)(end - start) / CLOCKS_PER_SEC;
        trimNewline(input);

        printTypingStats(timeTaken, input, paragraph, &attempt);

        printf("\n--- Typing Statistics ---\n");
        printf("Time: %s\n", attempt.timestamp);
        printf("Speed: %.2f CPM\n", attempt.typingSpeed);
        printf("Accuracy: %.2f%%\n", attempt.accuracy);
        printf("Wrong Characters: %d\n", attempt.wrongChars);
        printf("--------------------------\n");

        saveAttemptToFile(&attempt);

        free(paragraph);

        printf("\nDo you want to continue? (y/n): ");
        char choice[5];
        if (!fgets(choice, sizeof(choice), stdin)) break;
        if (tolower(choice[0]) != 'y') break;
    }

    fclose(file);
    showHistory();
}

int main() {
    FILE* file = fopen("paragraphs.txt", "r");
    if (!file) {
        perror("Cannot open 'paragraphs.txt'");
        return 1;
    }

    processAttempts(file);
    return 0;
}
