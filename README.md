# IMDB Movie Database (C)

A menu-driven console application written in C that parses an IMDB movie dataset and lets the user run several queries on it. The program reads movie information (title, budget, year, director, genre, rating, etc.) from `movie.txt` and exposes the data through a simple interactive menu.

## Data Model

The program uses two main structs:

- **`Budget`** — budget, year, title, title type (Feature Film / TV Series), Top 250 flag
- **`Name`** — list of genres, list of directors, "1001 Must See" flag, rating, score, URL

## Menu Options

1. List the budget array
2. List the name array
3. List unique genres
4. List movies through the years (ascending)
5. List movies by score
6. Show full information for a single movie (case-insensitive search)
7. Frequency of genres
8. Exit

## Build & Run

```bash
gcc -o movies odev.c
./movies
```

The program expects `movie.txt` to be in the same directory as the executable.

## Data Format

`movie.txt` is a semicolon-separated (`;`) file with the following columns:

```
budget;title;title type;directors;rating;runtime;year;genres;votes;top250;1001 must see;url
```

## Files

- `odev.c` — Source code
- `movie.txt` — IMDB movie dataset
