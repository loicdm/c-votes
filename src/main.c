#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

#define DYN_ARRAY_INIT_CAP 16

struct SCandidate {
  const char *restrict name;
  size_t id;
  int score;
};
typedef struct SCandidate Candidate;
;
struct SCandidateDynArray {
  Candidate *array;
  size_t count;
  size_t capacity;
};
typedef struct SCandidateDynArray CandidateDynArray;

struct SElection {
  const char *restrict name;
  CandidateDynArray candidates;
  int threshold_w;
  int threshold_l;
  const Candidate *restrict winner;
  CandidateDynArray loosers;
};
typedef struct SElection Election;

bool InitCandidateDynArray(CandidateDynArray *cda) {
  cda->array = malloc(sizeof(struct SCandidate) * DYN_ARRAY_INIT_CAP);
  if (!cda->array)
    return false;
  cda->capacity = DYN_ARRAY_INIT_CAP;
  cda->count = 0;
  return true;
}

bool PushCandidateToDynArray(CandidateDynArray *cda, Candidate c) {
  if (cda->count == cda->capacity) {
    cda->capacity *= 2;
    cda->array = realloc(cda->array, cda->capacity * sizeof(struct SCandidate));
    if (!cda->array)
      return false;
  }
  cda->array[cda->count++] = c;
  return true;
}

Candidate *GetCandidateFromDynArray(CandidateDynArray *cda, size_t index) {
  if (index < cda->count) {
    return &(cda->array[index]);
  } else {
    return NULL;
  }
}

void DumpCandidateDynArray(CandidateDynArray *cda, FILE *stream) {
  for (size_t i = 0; i < cda->count; i++) {
    Candidate *c = &(cda->array[i]);
    fprintf(stream, "[%s][%zu] : %d\n", c->name, c->id, c->score);
  }
}
bool InitCandidate(Candidate *c, const char *restrict name, const size_t id) {
  c->name = strdup(name);
  if (!c->name)
    return false;
  c->id = id;
  c->score = 0;
  // printf("Candidat [%s] enregistré avec l'ID [%zu]\n", name, id);
  return true;
}

void FreeCandidate(Candidate *c) { free((void *)c->name); }

void FreeCandidateDynArray(CandidateDynArray *cda) {
  for (size_t i = 0; i < cda->count; i++) {
    FreeCandidate(&(cda->array[i]));
  }
  free(cda->array);
}
bool InitElection(Election *e, const char *restrict name, int threshold_w,
                  int threshold_l) {
  e->name = strdup(name);
  if (!InitCandidateDynArray(&(e->candidates))) {
    free(e);
    return false;
  }
  if (!InitCandidateDynArray(&(e->loosers))) {
    FreeCandidateDynArray(&(e->candidates));
    free(e);
    return false;
  }
  e->threshold_w = threshold_w;
  e->threshold_l = threshold_l;
  // printf("Election initialisée : [%s]\n", name);
  return true;
}

void FreeElectionContent(Election *e) {
  FreeCandidateDynArray(&(e->candidates));
  FreeCandidateDynArray(&(e->loosers));
  free((void *)e->name);
}

bool AddCandidateToElection(Election *e, const Candidate c) {
  if (!PushCandidateToDynArray(&(e->candidates), c))
    return false;
  // printf("[%s][%zu] ajouté à l'élection [%s]\n", c.name, c.id, e->name);
  return true;
}

void VoteForCandidate(Candidate *candidate, int count) {
  candidate->score += count;
}

bool FindWinner(Election *e) {
  Candidate *winner = NULL;
  bool result = false;
  CandidateDynArray *candidates = &(e->candidates);
  if (candidates->count > 0) {
    winner = &(candidates->array[0]);
    for (size_t i = 0; i < candidates->count; i++) {
      Candidate *c = &(candidates->array[i]);
      if (c->score > winner->score)
        winner = c;
    }
  }
  if (winner->score >= e->threshold_w) {
    e->winner = winner;
    result = true;
  }
  return result;
}

void DumpElection(Election *e, FILE *stream) {
  DumpCandidateDynArray(&(e->candidates), stream);
}

bool FindLoosers(Election *e) {
  bool result = false;
  CandidateDynArray *candidates = &(e->candidates);
  for (size_t i = 0; i < candidates->count; i++) {
    Candidate *c = &(candidates->array[i]);
    if (c->score < e->threshold_l) {
      Candidate looser;
      InitCandidate(&looser, c->name, c->id);
      PushCandidateToDynArray(&(e->loosers), looser);
      result = true;
    }
  }

  return result;
}

bool FileElectionFromFile(Election *e, const char *restrict path) {
  FILE *fp = fopen(path, "r");
  if (fp == NULL) {
    perror("Erreur lors de l'ouverture du fichier");
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
      if (!InitCandidate(&c, token, id)) {
        fclose(fp);
        free(line);
        return false;
      }

      printf("Candidat [%s] initialisé avec l'ID [%zu]\n", token, id);
      if (!AddCandidateToElection(e, c)) {
        FreeCandidate(&c);
        fclose(fp);
        free(line);
        return false;
      }
      printf("[%s][%zu] ajouté à l'élection [%s]\n", c.name, c.id, e->name);
      id++;
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
      int count = atoi(token);
      if (count != 0) {
        Candidate *c = GetCandidateFromDynArray(&(e->candidates), id);
        VoteForCandidate(c, count);
        printf("%d vote(s) comptabilisé pour le candidat [%s] ID [%zu] score "
               "[%d]\n",
               count, c->name, c->id, c->score);
      }
      token = strtok(NULL, ",");
      id++;
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

void ShowHelp(FILE *s, char **argv) {
  fprintf(
      s,
      "Usage : %s nom_election fichier_election seuil_victoire seuil_defaite\n",
      argv[0]);
}

int main(int argc, char **argv) {
  if (argc < 5) {
    ShowHelp(stderr, argv);
    return EXIT_FAILURE;
  }

  Election e;
  if (!InitElection(&e, argv[1], atoi(argv[3]), atoi(argv[4]))) {
    fprintf(stderr, "Impossible d'initialiser l'élection\n");
    return EXIT_FAILURE;
  }
  if (!FileElectionFromFile(&e, argv[2])) {
    fprintf(stderr, "Impossible de remplir l'élection avec le fichier : %s\n",
            argv[2]);
    return EXIT_FAILURE;
  }
  printf("Résumé de l'élection :\n");

  DumpElection(&e, stdout);

  printf("--------------------------\n");
  printf("Résultats : \n");

  if (!FindWinner(&e)) {
    printf("Aucun Gagnant\n");
  } else {
    printf("Le gagnant est [%s] avec %d votes\n", e.winner->name,
           e.winner->score);
  }
  if (FindLoosers(&e)) {
    printf("Les perdants sont : \n");
    DumpCandidateDynArray(&(e.loosers), stdout);
  }

  FreeElectionContent(&e);
  return EXIT_SUCCESS;
}
