#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

#define ELECTION_INIT_CAP 16

struct s_candidate {
  const char *restrict name;
  size_t id;
  size_t score;
};
typedef struct s_candidate Candidate;

struct s_election {
  const char *restrict name;
  Candidate *candidates;
  size_t count;
  size_t capacity;
  const Candidate *restrict winner;
};
typedef struct s_election Election;

bool init_candidate(Candidate *c, const char *restrict name, const size_t id) {
  c->name = strdup(name);
  if (!c->name)
    return false;
  c->id = id;
  c->score = 0;
  printf("Candidat [%s] enregistré avec l'ID %zu\n", name, id);
  return true;
}

void free_candidate(Candidate *c) { free((void *)c->name); }

bool init_election(Election *e, const char *restrict name) {
  e->name = strdup(name);
  e->candidates = malloc(sizeof(struct s_candidate) * ELECTION_INIT_CAP);
  if (!e->candidates) {
    free(e);
    return false;
  }
  e->capacity = ELECTION_INIT_CAP;
  e->count = 0;
  printf("Election initialisée : %s\n", name);
  return true;
}

void free_election_content(Election *e) {
  for (size_t i = 0; i < e->count; ++i) {
    free_candidate(&(e->candidates[i]));
  }
  free((void *)e->name);
  free(e->candidates);
}

bool add_candidate_to_election(Election *e, const Candidate c) {
  if (e->count == e->capacity) {
    e->capacity *= 2;
    e->candidates =
        realloc(e->candidates, e->capacity * sizeof(struct s_candidate));
    if (!e->candidates)
      return false;
  }
  e->candidates[e->count++] = c;
  printf("%s (%zu) ajouté à l'élection %s\n", c.name, c.id, e->name);
  return true;
}

void vote_for_candidate(Candidate *candidate, size_t count) {
  candidate->score += count;
  printf("%zu vote(s) comptabilisé pour le candidat %s (%zu)\n", count,
         candidate->name, candidate->id);
}

Candidate *decide_election_v1(Election *e) {
  Candidate *winner = NULL;
  if (e->count > 0) {
    winner = &(e->candidates[0]);
    for (size_t i = 0; i < e->count; ++i) {
      if (e->candidates[i].score > winner->score)
        winner = &(e->candidates[i]);
    }
    e->winner = winner;
  }
  return winner;
}

bool fill_election_from_file(Election *e, const char *restrict path) {
  FILE *fp = fopen(path, "r");
  if (fp == NULL) {
    perror("Error opening file");
    return false;
  }

  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  // Process des candidats
  if ((read = getline(&line, &len, fp)) != -1) {
    size_t id = 0;
    if (line[read - 1] == '\n')
      line[read - 1] = '\0';
    char *token = strtok(line, ",");
    while (token) {
      Candidate c;
      if (!init_candidate(&c, token, id++)) {
        fclose(fp);
        free(line);
        return false;
      }
      if (!add_candidate_to_election(e, c)) {
        free_candidate(&c);
        fclose(fp);
        free(line);
        return false;
      }
      token = strtok(NULL, ",");
    }
  }
  // process des votes
  while ((read = getline(&line, &len, fp)) != -1) {
    size_t id = 0;
    if (line[read - 1] == '\n')
      line[read - 1] = '\0';
    char *token = strtok(line, ",");
    while (token) {
      size_t count = atoi(token);
      if (count > 0) {
        Candidate *c = &(e->candidates[id]);
        vote_for_candidate(c, count);
      }
      token = strtok(NULL, ",");
      ++id;
    }
  }
  // Free the memory allocated by getline and close the file
  free(line);
  fclose(fp);
  printf(
      "Données de l'élection [%s] importées avec le contenu du fichier : %s\n",
      e->name, path);
  return true;
}

void show_help(FILE *s, char **argv) {
  fprintf(s, "Usage : %s nom_election fichier_election\n", argv[0]);
}

int main(int argc, char **argv) {
  if (argc < 3) {
    show_help(stderr, argv);
    return EXIT_FAILURE;
  }

  Election e;
  if (!init_election(&e, argv[1])) {
    fprintf(stderr, "Could not initialize election\n");
    return EXIT_FAILURE;
  }
  if (!fill_election_from_file(&e, argv[2])) {
    fprintf(stderr, "Could not fill election from file\n");
    return EXIT_FAILURE;
  }
  Candidate *winner = decide_election_v1(&e);
  if (!winner) {
    printf("Aucun Gagnant\n");
  }
  printf("Le gagnant est [%s] avec %zu votes\n", winner->name, winner->score);
  free_election_content(&e);
  return EXIT_SUCCESS;
}
