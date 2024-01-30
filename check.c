#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <stdlib.h>
#include <locale.h>
#include <ctype.h>
#include "./libs/miniz.h"


// gcc check.c ./libs/miniz.c -o check.exe -Wall
const char *documents[] = {".docx", ".doc", NULL};

const char *sourceFiles[] = {".c", ".java", ".asm",
                              ".sh", ".ps", ".bat", NULL};

const char *fileExtensions[] = {".txt", ".docx", ".doc", ".h", ".c", ".cpp", ".cs", ".java",
                           ".html", ".css", ".js", ".xml", ".json", ".yaml", ".asm", ".exe",
                           ".sh", ".ps", ".bat", ".r", ".py", ".php", ".sql", NULL};


int hasPolishLetters(const char *text) {
    int len = strlen(text);
    wchar_t *long_text = (wchar_t *)malloc((len + 1) * sizeof(wchar_t));
    mbstowcs(long_text, text, len + 1);

    for (int i = 0; long_text[i] != L'\0'; i++) {
        wchar_t polishLetters[] = L"ąĄćĆęĘłŁńŃóÓśŚżŻźŹ";
        if (wcschr(polishLetters, long_text[i]) != NULL) {
            free(long_text);
            return 1;
        }
    }

    free(long_text);
    return 0; 
}

int main(int argc, char *argv[]) {
    
    setlocale(LC_ALL, "pl_PL.UTF-8");

    char* imie = argv[2];
    char* nazwisko = argv[1];


    int imieLen = strlen(imie);
    int nazwiskoLen = strlen(nazwisko);
    printf("%s\n", imie);
    printf("%s\n", nazwisko);


    char rozszerzenie[5] = "";

    int i;
    int j = 0;
    for (i = imieLen - 4; i < imieLen; i++) {
        rozszerzenie[j] = imie[i];
        j++;
    }


    if (argc == 2) {
        printf("brak spacji.\n");
        return 1;
    }

    if (argc == 1) {
        printf("zla nazwa archiwum.\n");
        return 1;
    }

    if (argc > 3) {
        printf("za duzo argumentow.\n");
        return 1;
    }

    if (hasPolishLetters(nazwisko) ) {
        printf("polskie znaki w nazwisku.\n");
        return 1;
    }
    if (hasPolishLetters(imie) ) {
        printf("polskie znaki w imieniu.\n");
        return 1;
    }


    if (strcmp(".zip", rozszerzenie) != 0) {
        printf("wymagane archiwum .zip.\n");
        return 1;
    }

    if (nazwisko[0] < 65 || nazwisko[0] > 90) {
        printf("nazwisko z malej litery.\n");
        return 1;
    }

    if (imie[0] < 65 || imie[0] > 90) {
        printf("imie z malej litery.\n");
        return 1;
    }

    for (i = 1; i < nazwiskoLen; i++) {
        if (nazwisko[i] >= 65 && nazwisko[i] <= 90) {
            printf("za duzo duzych liter w nazwisku.\n");
            return 1;
        }
    }

    for (i = 1; i < imieLen; i++) {
        if (imie[i] >= 65 && imie[i] <= 90) {
            printf("za duzo duzych liter w imieniu.\n");
            return 1;
        }
    }
    
    int imieBezZipLen = imieLen - 4 + 1;

    char imieBezZip[50] = "";
    strncpy(imieBezZip, imie, imieBezZipLen - 1);
    
    const char slownik[] = "names.txt";

    FILE *fp = fopen(slownik, "r");

    if (fp == NULL) {
        perror(slownik);
        printf("slownik nie jest dostepny.\n");
        return 1;
    }

    char line[1200 + 1];
    int wystapienia = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {

        line[strlen(line) - 1] = '\0';

        if (strcmp(imieBezZip, line) == 0) {
            wystapienia = 1;
            break;
        }
    }

    if (wystapienia == 0) {
        printf("imienia %s nie ma w slowniku.\n", imieBezZip);
        fclose(fp);
        return 1;
    }

    fclose(fp);

    char archiveName[50] = "";
    strcat(archiveName, nazwisko);
    strcat(archiveName, " ");
    strcat(archiveName, imie);

    char searchedCatalog[50] = "";
    strcat(searchedCatalog, nazwisko);
    strcat(searchedCatalog, " ");
    strcat(searchedCatalog, imieBezZip);
    strcat(searchedCatalog, "/");

   const char projectsFile[] = "projects.txt";

    FILE *projectsFilePointer = fopen(projectsFile, "r");

    if (projectsFilePointer == NULL) {
        perror(projectsFile);
        return 1;
    }

    int numberOfProjects = 0;

    while (fgets(line, sizeof(line), projectsFilePointer) != NULL) {
        numberOfProjects++;
    }

    rewind(projectsFilePointer);

    char (*filesToSearch)[50] = malloc(numberOfProjects * sizeof(*filesToSearch));
    char (*projectNames)[50] = malloc(numberOfProjects * sizeof(*projectNames));
    int (*occurrences)[2] = malloc(numberOfProjects * sizeof(*occurrences));

    int projectIndex = 0;

    while (fgets(line, sizeof(line), projectsFilePointer) != NULL) {
        line[strlen(line) - 1] = '\0';

        char temp[50] = "";
        strcat(temp, searchedCatalog);
        strcat(temp, line);
        strcpy(filesToSearch[projectIndex], temp);

        strcpy(projectNames[projectIndex], line);

        projectIndex++;
    }

    fclose(projectsFilePointer);

    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));

    if (mz_zip_reader_init_file(&zip, archiveName, 0) == 0) {
        printf("nie ma archiwum %s.\n", archiveName);
        free(filesToSearch);
        free(projectNames);
        free(occurrences);
        return 1;
    }

    int numberOfFiles = mz_zip_reader_get_num_files(&zip);

    int hasRequiredCatalog = 0, isCorrect = 0;

    for (int i = 0; i < numberOfFiles; i++) {
        mz_zip_archive_file_stat stats;
        
        if (mz_zip_reader_file_stat(&zip, i, &stats) == 0) {
            printf("blad pobierania informacji o pliku.\n");
            free(filesToSearch);
            free(projectNames);
            free(occurrences);
            return 1;
        }

        if ((strcmp(stats.m_filename, searchedCatalog) == 0) && stats.m_is_directory) {
            hasRequiredCatalog = 1;
        }

        for (int j = 0; j < numberOfProjects; j++) {
            for (int k = 0; k < 100; k++) {
                if (sourceFiles[k] == NULL) break;

                if (!stats.m_is_directory && (strstr(stats.m_filename, filesToSearch[j]) != NULL) && (strstr(stats.m_filename, sourceFiles[k]) != NULL)) {
                    occurrences[j][0] = 1;
                }
            }

            for (int k = 0; k < 100; k++) {
                if (documents[k] == NULL) break;

                if (!stats.m_is_directory && (strstr(stats.m_filename, filesToSearch[j]) != NULL) && (strstr(stats.m_filename, documents[k]) != NULL)) {
                    occurrences[j][1] = 1;
                }
            }
        }

    }

    mz_zip_reader_end(&zip);

    searchedCatalog[strlen(searchedCatalog) - 1] = '\0';

    for (int i = 0; i < numberOfProjects; i++)  {

        if (occurrences[i][0] && occurrences[i][1]) {
            isCorrect = 1;
            break;
        }
    }

    

    if (isCorrect && hasRequiredCatalog) {

        printf("archiwum poprawne, zawiera pliki:\n");

        mz_zip_reader_init_file(&zip, archiveName, 0);

        for (int i = 0; i < numberOfFiles; i++) {
            mz_zip_archive_file_stat stats;
            
            if (mz_zip_reader_file_stat(&zip, i, &stats) == 0) {
                printf("Blad podczas pobierania informacji o pliku.\n");
                free(filesToSearch);
                free(projectNames);
                free(occurrences);
                return 1;
            }

            if (!stats.m_is_directory) {
                printf("\t%s", stats.m_filename);

                int known = 0;
                int withoutExtension = 0;

                for (int j = 0; j < 100; j++) {
                    if (fileExtensions[j] == NULL) break;
                    if (strstr(stats.m_filename, fileExtensions[j]) != NULL) {
                        known = 1;
                        break;
                    } else {
                        if (strstr(stats.m_filename, ".") == NULL) withoutExtension = 1;
                    }
                }

                if (withoutExtension) printf(" brak rozszerzenia\n");
                else if (!known) printf(" nieznane rozszerzenie\n");
                else printf("\n");
            }
        }

        mz_zip_reader_end(&zip);
    }
    else if (hasRequiredCatalog && !isCorrect) {

        int correctFiles = 0;
        int startedProjectIndex = 0;

        for (int i = 0; i < numberOfProjects; i++) {
            if (occurrences[i][0]) {
                correctFiles++;
                startedProjectIndex = i;
            }

            if (occurrences[i][1]) {
                correctFiles++;
                startedProjectIndex = i;
            }
        }

        if (correctFiles == 0) {
            printf("brak pliku zrodlowego.\n");
            printf("brak dokumentacji.\n");
        }
        else {
            if (occurrences[startedProjectIndex][0] == 0) printf("brak pliku zrodlowego.\n");
            if (occurrences[startedProjectIndex][1] == 0) printf("brak pliku z dokumentacja.\n");
        }
    }
    else if (!hasRequiredCatalog) printf("brakuje katalogu %s.\n", searchedCatalog);

    free(filesToSearch);
    free(projectNames);
    free(occurrences);

    return 0;

}
