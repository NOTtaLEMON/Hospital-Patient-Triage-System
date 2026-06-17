/*
 * ============================================================
 *  Smart Hospital Priority and Routing System
 *  Course  : Principles of Programming using C (CS222IA)
 *  College : RV College of Engineering, Bengaluru
 *  Part    : B – PBL Mini Project
 * ============================================================
 *
 *  WHAT THIS PROGRAM DOES:
 *  1. Accepts patient vitals (name, age, temperature, BP, pulse, symptom)
 *  2. Calculates a composite risk score from the vitals
 *  3. Classifies the patient: EMERGENCY / URGENT / NORMAL
 *  4. Routes the patient to the correct department
 *  5. Logs all records with timestamps to patients.txt
 *  6. Displays ALL patients (current + past sessions) sorted by priority
 *  7. Delete a particular patient record OR clear all records
 * ============================================================
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#define MAX_PATIENTS  200
#define LOG_FILE      "patients.txt"


#define RED    ""
#define YELLOW ""
#define GREEN  ""
#define CYAN   ""
#define RESET  ""


struct Patient {
    int    id;
    char   name[50];
    int    age;
    float  temperature;
    int    bp;
    int    pulse;
    char   symptom[100];
    int    riskScore;
    char   priority[20];
    char   department[50];
    char   timestamp[30];
};

struct Patient patients[MAX_PATIENTS];
int numPatients = 0;


int maxID = 0;



void getCurrentTimestamp(char *buffer) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, 30, "%Y-%m-%d %H:%M:%S", t);  
}



int calculateRiskScore(float temp, int bp, int pulse) {
    int score = 0;

    if      (temp >= 39.5) score += 40;
    else if (temp >= 38.0) score += 20;
    else if (temp <= 35.0) score += 30;

    if      (bp >= 180)    score += 40;
    else if (bp >= 140)    score += 20;
    else if (bp <= 90)     score += 30;

    if      (pulse >= 120) score += 30;
    else if (pulse <= 50)  score += 30;

    return score;
}


void classifyPriority(struct Patient *p) {
    if      (p->riskScore >= 70) strcpy(p->priority, "EMERGENCY");
    else if (p->riskScore >= 35) strcpy(p->priority, "URGENT");
    else                         strcpy(p->priority, "NORMAL");
}


void routeDepartment(struct Patient *p) {
    char sym[100];
    strcpy(sym, p->symptom);                /* strcpy: copy to safe buffer */

    int i;
    for (i = 0; sym[i]; i++)               /* manual lowercase            */
        if (sym[i] >= 'A' && sym[i] <= 'Z')
            sym[i] += 32;

    /* strstr: returns non-NULL if keyword found anywhere in sym */
    if (strstr(sym, "chest") || strstr(sym, "heart"))
        strcpy(p->department, "Cardiology");

    else if (strstr(sym, "seizure") || strstr(sym, "dizziness") ||
             strstr(sym, "headache") || strstr(sym, "neuro"))
        strcpy(p->department, "Neurology");

    else if (strstr(sym, "fracture") || strstr(sym, "bone") ||
             strstr(sym, "joint")    || strstr(sym, "sprain"))
        strcpy(p->department, "Orthopedics");

    else if (strstr(sym, "fever") || strstr(sym, "vomit") ||
             strstr(sym, "diarrhoea") || strstr(sym, "abdomen"))
        strcpy(p->department, "General Medicine");

    else if (strstr(sym, "burn") || strstr(sym, "wound") ||
             strstr(sym, "injury"))
        strcpy(p->department, "Emergency Surgery");

    else
        strcpy(p->department, "General OPD");
}


/* ============================================================
 *  FUNCTION: logToFile
 *  Appends one record to patients.txt.
 *  "a" mode = append, existing records are never erased.
 * ============================================================ */
void logToFile(struct Patient p) {
    FILE *fp = fopen(LOG_FILE, "a");
    if (fp == NULL) {
        printf(RED "Error: Could not open log file.\n" RESET);
        return;
    }
    fprintf(fp,
        "[%s] ID:%d | Name:%-20s | Age:%3d | Temp:%5.1f | BP:%4d | "
        "Pulse:%3d | Score:%3d | Priority:%-10s | Dept:%s\n",
        p.timestamp, p.id, p.name, p.age,
        p.temperature, p.bp, p.pulse,
        p.riskScore, p.priority, p.department
    );
    fclose(fp);
}


/* ============================================================
 *  FUNCTION: loadFromFile
 *  Reads patients.txt at startup and parses every line back
 *  into the patients[] array using sscanf().
 *
 *  WHY THIS FIXES OPTION 2:
 *  Before this function existed, patients[] only had patients
 *  added in the CURRENT session. Now we load past records
 *  into the same array first, so displayAllPatients() shows
 *  everyone — past and present.
 *
 *  sscanf(line, format, &fields...)
 *    Like scanf() but reads from a string instead of keyboard.
 *    Format must match exactly how fprintf() wrote it.
 *
 *  After sscanf, strlen() + a trim loop removes trailing spaces
 *  that the fixed-width format (%-20s) pads into the fields.
 * ============================================================ */
void loadFromFile() {
    FILE *fp = fopen(LOG_FILE, "r");
    if (fp == NULL) return;   /* no file yet — first ever run */

    char line[400];
    numPatients = 0;

    while (fgets(line, sizeof(line), fp) != NULL && numPatients < MAX_PATIENTS) {
        struct Patient p;

        int parsed = sscanf(line,
            "[%[^]]] ID:%d | Name:%20[^|] | Age:%d | Temp:%f | BP:%d | "
            "Pulse:%d | Score:%d | Priority:%10[^|] | Dept:%49[^\n]",
            p.timestamp, &p.id, p.name, &p.age,
            &p.temperature, &p.bp, &p.pulse,
            &p.riskScore, p.priority, p.department
        );

        if (parsed == 10) {
            /* Trim trailing spaces from name (left by %-20 padding) */
            /* strlen() returns length, we walk backwards until non-space */
            int len = (int)strlen(p.name) - 1;
            while (len >= 0 && p.name[len] == ' ') { p.name[len] = '\0'; len--; }

            /* Same trim for priority field */
            len = (int)strlen(p.priority) - 1;
            while (len >= 0 && p.priority[len] == ' ') { p.priority[len] = '\0'; len--; }

            strcpy(p.symptom, "(from log)");   /* symptom not stored in file */

            /* Track the highest ID seen so far across all loaded records.
             * New patients will get maxID+1, avoiding any collision even
             * if previous records were deleted and gaps exist in IDs.     */
            if (p.id > maxID) maxID = p.id;

            patients[numPatients] = p;
            numPatients++;
        }
    }
    fclose(fp);
}


/* ============================================================
 *  FUNCTION: rewriteLogFile
 *  Overwrites patients.txt from scratch using patients[].
 *  Called after every delete so file stays in sync with memory.
 *
 *  "w" mode = write / overwrite — erases file content first,
 *  then we write all remaining patients back into it.
 * ============================================================ */
void rewriteLogFile() {
    FILE *fp = fopen(LOG_FILE, "w");   /* "w" truncates file to empty */
    if (fp == NULL) {
        printf(RED "Error: Could not rewrite log file.\n" RESET);
        return;
    }
    int i;
    for (i = 0; i < numPatients; i++) {
        fprintf(fp,
            "[%s] ID:%d | Name:%-20s | Age:%3d | Temp:%5.1f | BP:%4d | "
            "Pulse:%3d | Score:%3d | Priority:%-10s | Dept:%s\n",
            patients[i].timestamp, patients[i].id, patients[i].name,
            patients[i].age, patients[i].temperature, patients[i].bp,
            patients[i].pulse, patients[i].riskScore,
            patients[i].priority, patients[i].department
        );
    }
    fclose(fp);
}


/* ============================================================
 *  FUNCTION: bubbleSortByScore
 *  Sorts patients[] descending by riskScore (highest first).
 *  Same Bubble Sort from Lab Program 1, applied to structs.
 *  Swap condition: left.score < right.score (we want HIGH first)
 * ============================================================ */
void bubbleSortByScore() {
    int i, j;
    struct Patient temp;
    for (i = 0; i < numPatients - 1; i++) {
        for (j = 0; j < numPatients - i - 1; j++) {
            if (patients[j].riskScore < patients[j + 1].riskScore) {
                temp            = patients[j];
                patients[j]     = patients[j + 1];
                patients[j + 1] = temp;
            }
        }
    }
}


/* ============================================================
 *  FUNCTION: addPatient
 *  Complete intake pipeline: read -> score -> classify ->
 *  route -> timestamp -> store -> display -> log to file.
 *
 *  fgets() used for name/symptom (scanf stops at spaces).
 *  strcspn(buf, "\n") strips the newline fgets leaves behind.
 * ============================================================ */
void addPatient() {
    if (numPatients >= MAX_PATIENTS) {
        printf(RED "Patient capacity full (%d).\n" RESET, MAX_PATIENTS);
        return;
    }

    struct Patient p;

    /* Use maxID+1 so the ID is always unique across sessions.
     * numPatients+1 would collide when records were deleted before. */
    maxID++;
    p.id = maxID;

    printf(CYAN "\n--- New Patient Registration ---\n" RESET);

    printf("Enter Patient Name    : ");
    getchar();
    fgets(p.name, 50, stdin);
    p.name[strcspn(p.name, "\n")] = '\0';      /* strcspn strips '\n' */

    printf("Enter Age             : ");
    scanf("%d", &p.age);

    printf("Enter Temperature (C) : ");
    scanf("%f", &p.temperature);

    printf("Enter Blood Pressure  : ");
    scanf("%d", &p.bp);

    printf("Enter Pulse (bpm)     : ");
    scanf("%d", &p.pulse);

    printf("Enter Chief Symptom   : ");
    getchar();
    fgets(p.symptom, 100, stdin);
    p.symptom[strcspn(p.symptom, "\n")] = '\0'; /* strcspn strips '\n' */

    p.riskScore = calculateRiskScore(p.temperature, p.bp, p.pulse);
    classifyPriority(&p);
    routeDepartment(&p);
    getCurrentTimestamp(p.timestamp);

    patients[numPatients] = p;
    numPatients++;

    printf("\n+--------------------------------------+\n");
    printf("  Patient ID   : %d\n",  p.id);
    printf("  Name         : %s\n",  p.name);
    printf("  Risk Score   : %d\n",  p.riskScore);

    /* strcmp compares priority string — cannot use == on strings */
    if      (strcmp(p.priority, "EMERGENCY") == 0)
        printf("  Priority     : " RED    "%s\n" RESET, p.priority);
    else if (strcmp(p.priority, "URGENT") == 0)
        printf("  Priority     : " YELLOW "%s\n" RESET, p.priority);
    else
        printf("  Priority     : " GREEN  "%s\n" RESET, p.priority);

    printf("  Department   : %s\n",  p.department);
    printf("  Registered   : %s\n",  p.timestamp);
    printf("+--------------------------------------+\n");

    logToFile(p);
    printf(GREEN "Record saved to %s\n" RESET, LOG_FILE);
}


/* ============================================================
 *  FUNCTION: displayAllPatients
 *  Shows ALL patients (loaded from file + this session) sorted
 *  by risk score. Works across sessions because loadFromFile()
 *  at startup already filled patients[] with past records.
 *
 *  strcmp() for colour selection (can't use == on strings).
 * ============================================================ */
void displayAllPatients() {
    if (numPatients == 0) {
        printf(YELLOW "No patient records found.\n" RESET);
        return;
    }

    bubbleSortByScore();

    printf(CYAN "\n==== All Patients | Priority Sorted | Most Critical First ====\n" RESET);
    printf("%-4s %-22s %-4s %-6s %-6s %-5s %-10s %-22s %-20s\n",
           "ID", "Name", "Age", "Score", "Temp", "BP",
           "Priority", "Department", "Registered");
    printf("--------------------------------------------------------------------------------------------\n");

    int i;
    for (i = 0; i < numPatients; i++) {
        const char *col = GREEN;
        if      (strcmp(patients[i].priority, "EMERGENCY") == 0) col = RED;
        else if (strcmp(patients[i].priority, "URGENT")    == 0) col = YELLOW;

        printf("%s%-4d %-22s %-4d %-6d %-6.1f %-5d %-10s %-22s %-20s\n" RESET,
               col,
               patients[i].id,    patients[i].name,
               patients[i].age,   patients[i].riskScore,
               patients[i].temperature, patients[i].bp,
               patients[i].priority,    patients[i].department,
               patients[i].timestamp);
    }
    printf("--------------------------------------------------------------------------------------------\n");
    printf("Total records: %d\n", numPatients);
}


/* ============================================================
 *  FUNCTION: viewLogFile
 *  Raw view of patients.txt — prints every stored line as-is.
 *  fgets() reads line by line until EOF (end of file).
 * ============================================================ */
void viewLogFile() {
    FILE *fp = fopen(LOG_FILE, "r");
    if (fp == NULL) {
        printf(YELLOW "No log file found. Register a patient first.\n" RESET);
        return;
    }
    printf(CYAN "\n==== Raw Log File: %s ====\n" RESET, LOG_FILE);
    char line[400];
    while (fgets(line, sizeof(line), fp) != NULL)
        printf("%s", line);
    fclose(fp);
}


/* ============================================================
 *  FUNCTION: searchPatientByName
 *  Linear search across all patients[] (past + current).
 *  Converts both query and patient name to lowercase, then
 *  uses strstr() for partial case-insensitive matching.
 *  strcspn() used to strip newline from fgets input.
 * ============================================================ */
void searchPatientByName() {
    if (numPatients == 0) {
        printf(YELLOW "No patient records found.\n" RESET);
        return;
    }

    char query[50];
    printf("Enter name to search: ");
    getchar();
    fgets(query, 50, stdin);
    query[strcspn(query, "\n")] = '\0';   /* strcspn strips newline */

    int k;
    for (k = 0; query[k]; k++)
        if (query[k] >= 'A' && query[k] <= 'Z') query[k] += 32;

    int found = 0, i;
    for (i = 0; i < numPatients; i++) {
        char nameLower[50];
        strcpy(nameLower, patients[i].name);   /* strcpy to safe copy */
        for (k = 0; nameLower[k]; k++)
            if (nameLower[k] >= 'A' && nameLower[k] <= 'Z') nameLower[k] += 32;

        if (strstr(nameLower, query) != NULL) {  /* strstr partial match */
            printf("\n-- Match Found ---------------------\n");
            printf("  ID         : %d\n",      patients[i].id);
            printf("  Name       : %s\n",       patients[i].name);
            printf("  Age        : %d\n",       patients[i].age);
            printf("  Temp       : %.1f C\n",   patients[i].temperature);
            printf("  BP         : %d mmHg\n",  patients[i].bp);
            printf("  Pulse      : %d bpm\n",   patients[i].pulse);
            printf("  Risk Score : %d\n",       patients[i].riskScore);
            printf("  Priority   : %s\n",       patients[i].priority);
            printf("  Department : %s\n",       patients[i].department);
            printf("  Registered : %s\n",       patients[i].timestamp);
            found = 1;
        }
    }

    if (!found)
        printf(RED "No patient found matching \"%s\".\n" RESET, query);
}


/* ============================================================
 *  FUNCTION: deletePatientByID
 *  Deletes ONE patient from both in-memory array and file.
 *
 *  BUG THAT WAS HERE (now fixed):
 *  The old code called displayAllPatients() which internally
 *  calls bubbleSortByScore(). That reorders patients[] by score.
 *  The 'found' index was searched BEFORE the sort, so after the
 *  sort it pointed to the wrong patient entirely — causing wrong
 *  deletions or an infinite-looking loop when confirm never read
 *  cleanly because scanf("%c") was consuming a leftover newline.
 *
 *  FIX 1 — ID search happens AFTER display/sort:
 *  We display first, then ask for the ID, then search. By then
 *  the array is already sorted and the search finds the correct
 *  index in the sorted order.
 *
 *  FIX 2 — confirmation uses fgets instead of scanf("%c"):
 *  scanf("%c") after another scanf leaves a '\n' in the buffer
 *  which gets consumed as the answer, skipping the prompt.
 *  fgets reads the whole line cleanly every time.
 *
 *  HOW THE SHIFT WORKS:
 *    Before (delete index 1):  [A][B][C][D]
 *    After shift loop:         [A][C][D][D]
 *    After numPatients--:      [A][C][D]
 * ============================================================ */
void deletePatientByID() {
    if (numPatients == 0) {
        printf(YELLOW "No records to delete.\n" RESET);
        return;
    }

    /* Display first — this also sorts the array by score */
    displayAllPatients();

    int targetID;
    printf("\nEnter Patient ID to delete: ");
    scanf("%d", &targetID);

    /* Search for the ID in the (now sorted) array */
    int found = -1, i;
    for (i = 0; i < numPatients; i++) {
        if (patients[i].id == targetID) { found = i; break; }
    }

    if (found == -1) {
        printf(RED "No patient with ID %d found.\n" RESET, targetID);
        return;
    }

    /* Confirmation — fgets reads the whole line, no leftover '\n' issue */
    printf(YELLOW "Delete %s (ID %d, %s)? Type y to confirm: " RESET,
           patients[found].name, targetID, patients[found].priority);

    char confirmBuf[8];
    /* consume the '\n' left by the scanf("%d") above */
    while (getchar() != '\n');
    fgets(confirmBuf, sizeof(confirmBuf), stdin);
    confirmBuf[strcspn(confirmBuf, "\n")] = '\0';

    if (confirmBuf[0] != 'y' && confirmBuf[0] != 'Y') {
        printf("Delete cancelled.\n");
        return;
    }

    /* Shift every element after 'found' one step left */
    for (i = found; i < numPatients - 1; i++)
        patients[i] = patients[i + 1];

    numPatients--;

    rewriteLogFile();
    printf(GREEN "Patient ID %d deleted successfully.\n" RESET, targetID);
}


/* ============================================================
 *  FUNCTION: deleteAllRecords
 *  Clears every patient from memory AND empties the log file.
 *
 *  strcmp(confirm, "YES") == 0  — checks exact string match
 *  strcspn used to strip newline from the confirmation input
 *  fopen("w") on an existing file truncates it to zero bytes
 * ============================================================ */
void deleteAllRecords() {
    if (numPatients == 0) {
        printf(YELLOW "No records to clear.\n" RESET);
        return;
    }

    printf(RED "WARNING: This permanently deletes ALL %d records.\n" RESET, numPatients);
    printf("Type YES to confirm: ");
    char confirm[10];
    /* consume leftover '\n' from menu scanf before fgets */
    while (getchar() != '\n');
    fgets(confirm, 10, stdin);
    confirm[strcspn(confirm, "\n")] = '\0';   /* strcspn strips newline */

    if (strcmp(confirm, "YES") != 0) {        /* strcmp: exact match check */
        printf("Clear cancelled.\n");
        return;
    }

    numPatients = 0;
    maxID = 0;

    FILE *fp = fopen(LOG_FILE, "w");  /* "w" empties the file instantly */
    if (fp) fclose(fp);

    printf(GREEN "All records cleared successfully.\n" RESET);
}


/* ============================================================
 *  FUNCTION: deleteMenu
 *  Sub-menu for delete options.
 *  switch-case routes to the correct delete function.
 * ============================================================ */
void deleteMenu() {
    printf(CYAN "\n-- Delete Options -------------------\n" RESET);
    printf("  1. Delete a particular patient (by ID)\n");
    printf("  2. Clear ALL records\n");
    printf("  0. Back\n");
    printf("Enter choice: ");

    int sub;
    scanf("%d", &sub);

    switch (sub) {
        case 1: deletePatientByID(); break;
        case 2: deleteAllRecords();  break;
        case 0: break;
        default: printf(RED "Invalid option.\n" RESET);
    }
}


/* ============================================================
 *  FUNCTION: displayMenu
 *  Prints the main menu. Option 5 = Delete, Option 6 = Exit.
 * ============================================================ */
void displayMenu() {
    printf(CYAN "\n+==========================================+\n");
    printf("|   Smart Hospital Priority & Routing      |\n");
    printf("+==========================================+\n" RESET);
    printf("  1. Register New Patient\n");
    printf("  2. View All Patients (Priority Sorted)\n");
    printf("  3. Search Patient by Name\n");
    printf("  4. View Raw Log File\n");
    printf("  5. Delete Records\n");
    printf("  6. Exit\n");
    printf(CYAN "+==========================================+\n" RESET);
    printf("Enter choice: ");
}


/* ============================================================
 *  MAIN FUNCTION
 *  1. loadFromFile() — pulls ALL past records into patients[]
 *  2. do-while loop keeps menu alive until choice == 6
 *  3. switch-case dispatches to the right function
 * ============================================================ */
int main() {
    printf(GREEN "\n  Welcome to Smart Hospital Priority & Routing System\n" RESET);
    printf("  RV College of Engineering | CS222IA PBL\n");

    loadFromFile();   /* load past records BEFORE showing menu */

    if (numPatients > 0)
        printf(CYAN "  Loaded %d existing record(s) from %s\n" RESET,
               numPatients, LOG_FILE);
    else
        printf(YELLOW "  No existing records. Starting fresh.\n" RESET);

    int choice;
    do {
        displayMenu();
        scanf("%d", &choice);

        switch (choice) {
            case 1: addPatient();          break;
            case 2: displayAllPatients();  break;
            case 3: searchPatientByName(); break;
            case 4: viewLogFile();         break;
            case 5: deleteMenu();          break;
            case 6:
                printf(GREEN "\nExiting. Log saved to %s.\n\n" RESET, LOG_FILE);
                break;
            default:
                printf(RED "Invalid choice. Enter 1-6.\n" RESET);
        }

    } while (choice != 6);

    return 0;
}
