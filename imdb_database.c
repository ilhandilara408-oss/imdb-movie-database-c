#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ===================== Basic structures ===================== */

struct Budget {
    int   budget;     /* numeric budget */
    int   year;       /* numeric year */
    char *title;      /* movie title */
    char *titletype;  /* e.g., Feature Film, TV Series */
    char *top250;     /* "Y" or empty */
};

struct Name {
    char  **genre;        /* comma-separated genres -> array */
    int     genreCount;
    char   *title;        /* same title text as Budget.title */
    char  **directors;    /* comma-separated directors -> array */
    int     directorCount;
    char   *mustsee;      /* from "1001 must see" column: "Y" or empty */
    double  rating;       /* from dataset */
    double  score;        /* allowed to choose; here we use rating */
    char   *url;          /* movie URL */
};

/* ===================== Small helpers (student style) ===================== */

/* strdup alternative (C89 safe) */
static char *my_strdup(const char *src) {
    size_t n;
    char *p;
    if (!src) src = "";
    n = strlen(src) + 1;
    p = (char*)malloc(n);
    if (!p) { perror("malloc"); exit(1); }
    memcpy(p, src, n);
    return p;
}

/* trim spaces and \r\n on both ends (very simple) */
static void trim_inplace(char *s) {
    char *start = s;
    char *end;
    if (!s) return;
    while (*start==' ' || *start=='\t' || *start=='\r' || *start=='\n') start++;
    if (start != s) memmove(s, start, strlen(start)+1);
    end = s + strlen(s);
    while (end > s && (end[-1]==' ' || end[-1]=='\t' || end[-1]=='\r' || end[-1]=='\n')) end--;
    *end = '\0';
}

/* very small case-insensitive strcmp (ASCII only, student-level) */
static int strcmp_nocase(const char *a, const char *b) {
    unsigned char ca, cb;
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    while (*a && *b) {
        ca = (unsigned char)tolower((unsigned char)*a);
        cb = (unsigned char)tolower((unsigned char)*b);
        if (ca != cb) return (int)ca - (int)cb;
        a++; b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

/* split by ';' and KEEP empty fields (so columns never shift).
   out[i] is malloc'ed; caller must free. Returns number of columns (<= max). */
static int split_semicolon(const char *line, char **out, int max) {
    int count = 0;
    const char *p = line;
    const char *start = line;

    while (*p && count < max) {
        if (*p == ';') {
            int len = (int)(p - start);
            char *s = (char*)malloc(len + 1);
            if (!s) { perror("malloc"); exit(1); }
            memcpy(s, start, len);
            s[len] = '\0';
            out[count++] = s;
            p++;
            start = p;
        } else {
            p++;
        }
    }
    if (count < max) {
        int len = (int)(p - start);
        char *s = (char*)malloc(len + 1);
        if (!s) { perror("malloc"); exit(1); }
        memcpy(s, start, len);
        s[len] = '\0';
        out[count++] = s;
    }
    return count;
}

/* split by a single char (like ','); KEEP empties; returns array + count */
static char **split_by_char(const char *text, char delim, int *outCount) {
    char **arr = NULL;
    int cap = 0, cnt = 0;
    const char *p = text;
    const char *start = text;

    if (!text || !*text) { *outCount = 0; return NULL; }

    for (;;) {
        if (*p == delim || *p == '\0') {
            int len = (int)(p - start);
            char *s = (char*)malloc(len + 1);
            if (!s) { perror("malloc"); exit(1); }
            memcpy(s, start, len);
            s[len] = '\0';
            trim_inplace(s);

            if (cnt == cap) {
                int ncap = cap ? cap * 2 : 4;
                char **tmp = (char**)realloc(arr, ncap * sizeof(char*));
                if (!tmp) { perror("realloc"); exit(1); }
                arr = tmp; cap = ncap;
            }
            arr[cnt++] = s;

            if (*p == '\0') break;
            p++;
            start = p;
        } else {
            p++;
        }
    }

    *outCount = cnt;
    return arr;
}

/* bubble sort for Budget: year DESC, if tie then budget DESC */
static void sort_budget(struct Budget *arr, int n) {
    int i, j;
    for (i = 0; i < n - 1; i++) {
        for (j = 0; j < n - i - 1; j++) {
            int move = 0;
            if (arr[j].year < arr[j+1].year) move = 1;
            else if (arr[j].year == arr[j+1].year && arr[j].budget < arr[j+1].budget) move = 1;
            if (move) {
                struct Budget t = arr[j];
                arr[j] = arr[j+1];
                arr[j+1] = t;
            }
        }
    }
}

/* ===================== Main ===================== */

int main(void) {
    FILE *fp;
    char line[8192];
    int count = 0;
    int i;

    fp = fopen("movie.txt", "r");
    if (!fp) {
        printf("Cannot open file: movie.txt\n");
        return 1;
    }

    /* 1) Count lines (skip header) */
    if (!fgets(line, sizeof(line), fp)) {
        printf("Empty file.\n");
        fclose(fp);
        return 1;
    }
    while (fgets(line, sizeof(line), fp)) count++;
    rewind(fp);

    /* 2) Allocate arrays once */
    {
        /* declare after rewind so older C compilers are happy */
    }
    {
        struct Budget *budgets = (struct Budget*)malloc(sizeof(struct Budget) * count);
        struct Name   *names   = (struct Name*)  malloc(sizeof(struct Name)   * count);

        if (!budgets || !names) { perror("malloc"); fclose(fp); return 1; }

        /* 3) Read and fill */
        fgets(line, sizeof(line), fp); /* skip header */
        i = 0;
        while (i < count && fgets(line, sizeof(line), fp)) {
            char *cols[16];
            int ncols, k;
            char *tmp;

            line[strcspn(line, "\n")] = '\0';
            ncols = split_semicolon(line, cols, 16);   /* keep empty fields */
            while (ncols < 12) cols[ncols++] = my_strdup("");

            for (k = 0; k < ncols; k++) trim_inplace(cols[k]);

            /* columns by header in your file:
               0 budget (int)
               1 title (string)
               2 title type (string)
               3 directors (comma list)
               4 rating (double)
               5 runtime (not used)
               6 year (int)
               7 genres (comma list)
               8 votes (optional)
               9 top250 (string)
               10 1001 must see (string)
               11 url (string)
            */

            /* Budget fields */
            budgets[i].budget    = atoi(cols[0]);
            budgets[i].title     = my_strdup(cols[1]);
            budgets[i].titletype = my_strdup(cols[2]);
            budgets[i].year      = atoi(cols[6]);
            budgets[i].top250    = my_strdup(cols[9]);

            /* Name fields */
            names[i].title = my_strdup(cols[1]);

            tmp = my_strdup(cols[3]);
            names[i].directors = split_by_char(tmp, ',', &names[i].directorCount);
            free(tmp);

            names[i].rating = atof(cols[4]);
            names[i].score  = names[i].rating; /* simple: score = rating */

            tmp = my_strdup(cols[7]);
            names[i].genre = split_by_char(tmp, ',', &names[i].genreCount);
            free(tmp);

            names[i].mustsee = my_strdup(cols[10]);
            names[i].url     = my_strdup(cols[11]);

            /* free the temporary split columns */
            for (k = 0; k < ncols; k++) free(cols[k]);

            i++;
        }
        fclose(fp);

        /* 4) Sort Budget array as required (year desc, then budget desc) */
        sort_budget(budgets, count);

        /* 5) Menu loop */
        for (;;) {
            int choice;
            printf("\n1. List of budget array\n");
            printf("2. List of name array\n");
            printf("3. List of genres (unique)\n");
            printf("4. List of the Movie Through the Years (ascending years)\n");
            printf("5. List of the Movie Through the Scores\n");
            printf("6. All Information of a Single Movie (case-insensitive)\n");
            printf("7. Frequency of the Genres\n");
            printf("8. Exit\n");
            printf("Choose: ");

            if (scanf("%d", &choice) != 1) {
                int c;
                while ((c = getchar()) != '\n' && c != EOF) ;
                printf("Invalid input!\n");
                continue;
            }
            {
                int c;
                while ((c = getchar()) != '\n' && c != EOF) ;
            }

            if (choice == 1) {
                int t;
                for (t = 0; t < count; t++) {
                    printf("%d | %d | %s | %s | %s\n",
                        budgets[t].budget,
                        budgets[t].year,
                        budgets[t].title ? budgets[t].title : "",
                        budgets[t].titletype ? budgets[t].titletype : "",
                        budgets[t].top250 ? budgets[t].top250 : "");
                }
            }
            else if (choice == 2) {
                int t, g, d;
                for (t = 0; t < count; t++) {
                    printf("Title: %s\n", names[t].title ? names[t].title : "");
                    printf("  Genres: ");
                    for (g = 0; g < names[t].genreCount; g++) {
                        printf("%s%s", names[t].genre[g], (g == names[t].genreCount-1) ? "" : ", ");
                    }
                    printf("\n  Directors: ");
                    for (d = 0; d < names[t].directorCount; d++) {
                        printf("%s%s", names[t].directors[d], (d == names[t].directorCount-1) ? "" : ", ");
                    }
                    printf("\n  MustSee: %s  Rating: %.1f  Score: %.1f\n",
                        names[t].mustsee ? names[t].mustsee : "",
                        names[t].rating, names[t].score);
                    printf("  URL: %s\n\n", names[t].url ? names[t].url : "");
                }
            }
            else if (choice == 3) {
                /* unique genre list (case-insensitive uniqueness) */
                char **uniq = NULL;
                int ucount = 0, ucap = 0;
                int t, g;
                for (t = 0; t < count; t++) {
                    for (g = 0; g < names[t].genreCount; g++) {
                        char *gen = names[t].genre[g];
                        int u, found = 0;
                        if (!gen || !*gen) continue;
                        for (u = 0; u < ucount; u++) {
                            if (strcmp_nocase(uniq[u], gen) == 0) { found = 1; break; }
                        }
                        if (!found) {
                            if (ucount == ucap) {
                                int ncap = ucap ? ucap * 2 : 16;
                                char **tmp = (char**)realloc(uniq, ncap * sizeof(char*));
                                if (!tmp) { perror("realloc"); exit(1); }
                                uniq = tmp; ucap = ncap;
                            }
                            uniq[ucount++] = my_strdup(gen);
                        }
                    }
                }
                for (i = 0; i < ucount; i++) {
                    printf("%s\n", uniq[i]);
                    free(uniq[i]);
                }
                free(uniq);
            }
            else if (choice == 4) {
                /* budgets is sorted by year DESC.
                   Print reversed to get ASC groups by year. */
                int t = count;
                int lastYear = -1234567;
                while (t-- > 0) {
                    if (budgets[t].year != lastYear) {
                        lastYear = budgets[t].year;
                        printf("\n%d:\n", budgets[t].year);
                    }
                    printf("  - %s\n", budgets[t].title ? budgets[t].title : "");
                }
            }
            else if (choice == 5) {
                /* sort a copy of names by score DESC (bubble sort) */
                struct Name *copy = (struct Name*)malloc(sizeof(struct Name) * count);
                int a, b;
                if (!copy) { perror("malloc"); exit(1); }
                for (a = 0; a < count; a++) copy[a] = names[a];
                for (a = 0; a < count - 1; a++) {
                    for (b = 0; b < count - a - 1; b++) {
                        if (copy[b].score < copy[b+1].score) {
                            struct Name tmp = copy[b];
                            copy[b] = copy[b+1];
                            copy[b+1] = tmp;
                        }
                    }
                }
                for (a = 0; a < count; a++) {
                    printf("%.1f - %s\n", copy[a].score, copy[a].title ? copy[a].title : "");
                }
                free(copy);
            }
            else if (choice == 6) {
                /* case-insensitive single movie info by title */
                char query[512];
                int foundIndex = -1;
                printf("Enter movie title: ");
                if (!fgets(query, sizeof(query), stdin)) continue;
                query[strcspn(query, "\n")] = '\0';

                for (i = 0; i < count; i++) {
                    if (names[i].title && strcmp_nocase(names[i].title, query) == 0) {
                        foundIndex = i;
                        break; /* show first match only */
                    }
                }

                if (foundIndex == -1) {
                    printf("Not found.\n");
                } else {
                    int bi = -1; /* matching Budget index (same title) */
                    for (i = 0; i < count; i++) {
                        if (budgets[i].title && strcmp_nocase(budgets[i].title, names[foundIndex].title) == 0) {
                            bi = i; break;
                        }
                    }
                    if (bi != -1) {
                        printf("Year:%d  Budget:%d  Type:%s  Top250:%s\n",
                            budgets[bi].year,
                            budgets[bi].budget,
                            budgets[bi].titletype ? budgets[bi].titletype : "",
                            budgets[bi].top250 ? budgets[bi].top250 : "");
                    }
                    {
                        int g, d;
                        printf("Genres: ");
                        for (g = 0; g < names[foundIndex].genreCount; g++) {
                            printf("%s%s", names[foundIndex].genre[g],
                                   (g == names[foundIndex].genreCount-1) ? "" : ", ");
                        }
                        printf("\nDirectors: ");
                        for (d = 0; d < names[foundIndex].directorCount; d++) {
                            printf("%s%s", names[foundIndex].directors[d],
                                   (d == names[foundIndex].directorCount-1) ? "" : ", ");
                        }
                        printf("\nMustSee:%s  Rating:%.1f  Score:%.1f\n",
                            names[foundIndex].mustsee ? names[foundIndex].mustsee : "",
                            names[foundIndex].rating, names[foundIndex].score);
                        printf("URL:%s\n", names[foundIndex].url ? names[foundIndex].url : "");
                    }
                }
            }
            else if (choice == 7) {
                /* simple frequency: linear search into a small dynamic list */
                struct Pair { char *genre; int count; };
                struct Pair *pairs = NULL;
                int size = 0, cap = 0;
                int t, g;
                for (t = 0; t < count; t++) {
                    for (g = 0; g < names[t].genreCount; g++) {
                        char *gen = names[t].genre[g];
                        int k, found = -1;
                        if (!gen || !*gen) continue;
                        for (k = 0; k < size; k++) {
                            if (strcmp_nocase(pairs[k].genre, gen) == 0) { found = k; break; }
                        }
                        if (found == -1) {
                            if (size == cap) {
                                int ncap = cap ? cap * 2 : 16;
                                struct Pair *tmp2 = (struct Pair*)realloc(pairs, ncap * sizeof(struct Pair));
                                if (!tmp2) { perror("realloc"); exit(1); }
                                pairs = tmp2; cap = ncap;
                            }
                            pairs[size].genre = my_strdup(gen);
                            pairs[size].count = 1;
                            size++;
                        } else {
                            pairs[found].count++;
                        }
                    }
                }
                for (i = 0; i < size; i++) {
                    printf("%s : %d\n", pairs[i].genre, pairs[i].count);
                    free(pairs[i].genre);
                }
                free(pairs);
            }
            else if (choice == 8) {
                printf("Exit.\n");
                break;
            }
            else {
                printf("Invalid choice!\n");
            }
        }

    
        return 0;
    }
}

