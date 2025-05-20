#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define max_para_length 200
#define max_file_line_length 200
#define max_attempts 10

#define EASY_SPEED 5
#define MEDIUM_SPEED 8
#define HARD_SPEED 12
#define EASY_MEDIUM_SPEED 8
#define MEDIUM_HARD_SPEED 12
#define HARD_MAX_SPEED 16

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
} TypingStats;

// Load user profile from file if exists
void loadUserProfile(UserProfile* profile) {
    printf("Enter your username: ");
    fgets(profile->username, sizeof(profile->username), stdin);
    size_t len = strlen(profile->username);
    if (len > 0 && profile->username[len - 1] == '\n') {
        profile->username[len - 1] = '\0';
    }

    char filename[100];
    snprintf(filename, sizeof(filename), "%s_profile.txt", profile->username);

    FILE* f = fopen(filename, "r");
    if (f != NULL) {
        if (fscanf(f, "%lf %lf %lf %lf %d",
                   &profile->bestSpeed, &profile->bestAccuracy,
                   &profile->totalSpeed, &profile->totalAccuracy,
                   &profile->totalAttempts) != 5) {
            // File content invalid, reset stats
            profile->bestSpeed = 0;
            profile->bestAccuracy = 0;
            profile->totalSpeed = 0;
            profile->totalAccuracy = 0;
            profile->totalAttempts = 0;
        }
        fclose(f);
    } else {
        // New user, initialize stats
        profile->bestSpeed = 0;
        profile->bestAccuracy = 0;
        profile->totalSpeed = 0;
        profile->totalAccuracy = 0;
        profile->totalAttempts = 0;
    }
}

// Update and save user profile after each attempt
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

// Display user summary statistics
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

// Read paragraphs from file and pick a random one
char* getRandomParagraph(FILE* file) {
    char* paragraphs[100];
    int count = 0;
    char line[max_file_line_length];

    while (fgets(line, sizeof(line), file) != NULL && count < 100) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        if (strlen(line) > 0) {
            paragraphs[count++] = strdup(line);
        }
    }

    if (count == 0) {
        fprintf(stderr, "No paragraphs found in the file.\n");
        exit(EXIT_FAILURE);
    }

    int randomIndex = rand() % count;
    char* result = strdup(paragraphs[randomIndex]);

    for (int i = 0; i < count; i++) {
        free(paragraphs[i]);
    }

    return result;
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
    double accuracy = (double)correctCount / totalCharacters * 100;

    if (elapsedTime < 3) {
        printf("\n### Please do not paste or hit enter immediately ###\n\n");
        elapsedTime = 3;
    }

    double typingSpeed = (totalCharacters / 5.0) / (elapsedTime / 60.0); // CPM approx
    double wpm = typingSpeed / 1.0; // same as CPM here

    stats->typingSpeed = typingSpeed;
    stats->wordsPerMinute = wpm;
    stats->accuracy = accuracy;
    stats->wrongChars = wrongCount;
    strncpy(stats->paragraph, correctText, max_para_length);
    stats->paragraph[max_para_length - 1] = '\0';  // Safety
}

// Display all previous attempts
void displayPreviousAttempts(TypingStats attempts[], int numAttempts) {
    printf("\nPrevious Attempts:\n");
    printf("--------------------------------------------------------------\n");
    printf("| Attempt |  CPM  | Accuracy (%%) | Wrong Characters |\n");
    printf("--------------------------------------------------------------\n");

    for (int i = 0; i < numAttempts; i++) {
        printf("|   %2d    | %.2f |    %.2f%%    |        %2d       |\n",
               i + 1, attempts[i].typingSpeed, attempts[i].accuracy, attempts[i].wrongChars);
    }

    printf("--------------------------------------------------------------\n");
}

// Prompt user for difficulty level (placeholder for future use)
void promptDifficulty(Difficulty* difficulty) {
    int choice;
    printf("Select difficulty level:\n1. Easy\n2. Medium\n3. Hard\n");

    while (1) {
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            while (getchar() != '\n'); // flush input buffer
            continue;
        }
        while (getchar() != '\n'); // flush input buffer

        if (choice >= 1 && choice <= 3) break;
        printf("Invalid choice. Please enter 1, 2, or 3.\n");
    }

    // Assign speeds based on choice (can be extended)
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
    }
}

int main() {
    srand((unsigned int)time(NULL));

    UserProfile user;
    TypingStats attempts[max_attempts];
    Difficulty difficulty;

    loadUserProfile(&user);
    promptDifficulty(&difficulty);

    int numAttempts = 0;
    char tryAgain;

    do {
        FILE* paraFile = fopen("paragraphs.txt", "r");
        if (!paraFile) {
            perror("Error opening paragraphs file");
            return 1;
        }

        TypingStats currentStats;

        char* paragraph = getRandomParagraph(paraFile);
        fclose(paraFile);

        printf("\nType the following paragraph:\n\n%s\n", paragraph);
        printf("\nPress Enter when you finish typing.\n\n");

        char inputText[max_para_length];
        int idx = 0;
        char ch;
        clock_t start = clock();

        while (idx < max_para_length - 1) {
            ch = getchar();
            if (ch == '\n') break;
            inputText[idx++] = ch;
        }
        inputText[idx] = '\0';

        clock_t end = clock();
        double elapsedTime = (double)(end - start) / CLOCKS_PER_SEC;

        printTypingStats(elapsedTime, inputText, paragraph, &currentStats);
        attempts[numAttempts++] = currentStats;
        updateUserProfile(&user, &currentStats);

        free(paragraph);

        printf("\nTyping Speed: %.2f cpm\n", currentStats.typingSpeed);
        printf("Accuracy: %.2f%%\n", currentStats.accuracy);
        printf("Wrong Characters: %d\n", currentStats.wrongChars);

        if (numAttempts < max_attempts) {
            printf("\nDo you want to try again? (y/n): ");
            scanf(" %c", &tryAgain);
            while (getchar() != '\n'); // flush newline leftover
        } else {
            printf("\nYou've reached the maximum number of attempts.\n");
            tryAgain = 'n';
        }

    } while (tolower(tryAgain) == 'y');

    displayUserSummary(&user);
    displayPreviousAttempts(attempts, numAttempts);

    return 0;
}
