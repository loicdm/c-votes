#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DYN_ARRAY_INIT_CAP 16

struct SCandidate {
  const char *restrict name;
  size_t id;
  int scoreFor;
  int scoreAgainst;
};
typedef struct SCandidate Candidate;

struct SCandidateDynArray {
  Candidate *array;
  size_t count;
  size_t capacity;
};
typedef struct SCandidateDynArray CandidateDynArray;

struct SElection {
  const char *restrict name;
  CandidateDynArray candidates;
  int voteCount;
  int blanks;
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
  fprintf(stream, "[Nom][id] - POUR/CONTRE :\n");
  for (size_t i = 0; i < cda->count; i++) {
    Candidate *c = &(cda->array[i]);
    fprintf(stream, "[%s][%zu] - %d/%d\n", c->name, c->id, c->scoreFor,
            c->scoreAgainst);
  }
}
bool InitCandidate(Candidate *c, const char *restrict name, const size_t id,
                   int scoreFor, int scoreAgainst) {
  c->name = strdup(name);
  if (!c->name)
    return false;
  c->id = id;
  c->scoreFor = scoreFor;
  c->scoreAgainst = scoreAgainst;
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
bool InitElection(Election *e, const char *restrict name) {
  e->name = strdup(name);
  if (!InitCandidateDynArray(&(e->candidates))) {
    return false;
  }

  e->blanks = 0;
  e->voteCount = 0;
  return true;
}

void FreeElectionContent(Election *e) {
  FreeCandidateDynArray(&(e->candidates));
  free((void *)e->name);
}

bool AddCandidateToElection(Election *e, const Candidate c) {
  if (!PushCandidateToDynArray(&(e->candidates), c))
    return false;
  // printf("[%s][%zu] ajouté à l'élection [%s]\n", c.name, c.id, e->name);
  return true;
}

void VoteForCandidate(Candidate *candidate, int count) {
  if (count >= 0) {
    candidate->scoreFor += count;
  } else {
    candidate->scoreAgainst += -(count);
  }
}

void DumpElection(Election *e, FILE *stream) {
  DumpCandidateDynArray(&(e->candidates), stream);
  fprintf(stream, "Nombre de votes blancs : %d\n", e->blanks);
  fprintf(stream, "Total des votes : %d\n", e->voteCount);
}

bool GetElectionDataFromFile(Election *e, const char *restrict path) {
  FILE *fp = fopen(path, "r");
  if (fp == NULL) {
    perror("Erreur lors de l'ouverture du fichier");
    return false;
  }

  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  size_t lineCount = 0;
  // Process des candidats
  if ((read = getline(&line, &len, fp)) != -1) {
    size_t id = 0;
    if (line[read - 1] == '\n')
      line[read - 1] = '\0';
    char *token = strtok(line, ",");
    while (token) {
      Candidate c;
      if (!InitCandidate(&c, token, id, 0, 0)) {
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
      lineCount++;
    }
  }
  // process des votes
  while ((read = getline(&line, &len, fp)) != -1) {
    size_t id = 0;
    if (line[read - 1] == '\n')
      line[read - 1] = '\0';
    char *token = strtok(line, ",");
    bool all_zeros = true;
    bool well_formed = true;
    while (token) {
      if (id >= e->candidates.count) {
        fprintf(stderr, "Ligne %zu mal formée\n", lineCount);
        well_formed = false;
        break;
      }
      int count = atoi(token);
      if (count != 0) {
        Candidate *c = GetCandidateFromDynArray(&(e->candidates), id);
        VoteForCandidate(c, count);
        if (count > 0) {
          printf("%d vote(s) comptabilisé POUR le candidat [%s] ID [%zu] score "
                 "%d/%d\n",
                 count, c->name, c->id, c->scoreFor, c->scoreAgainst);
        } else {
          printf(
              "%d vote(s) comptabilisé CONTRE le candidat [%s] ID [%zu] score "
              "%d/%d\n",
              -count, c->name, c->id, c->scoreFor, c->scoreAgainst);
        }

        all_zeros = false;
      }
      token = strtok(NULL, ",");
      id++;
    }
    if (well_formed) {
      if (all_zeros) {
        printf("Vote blanc comptabilisé\n");
        e->blanks++;
      }
      e->voteCount++;
    }
    lineCount++;
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
      "Usage : %s nom_election fichier_election seuil_victoire seuil_rejet\n",
      argv[0]);
}

int main(int argc, char **argv) {
  if (argc < 3) {
    ShowHelp(stderr, argv);
    return EXIT_FAILURE;
  }

  Election e;
  if (!InitElection(&e, argv[1])) {
    fprintf(stderr, "Impossible d'initialiser l'élection\n");
    return EXIT_FAILURE;
  }
  if (!GetElectionDataFromFile(&e, argv[2])) {
    fprintf(stderr, "Impossible de remplir l'élection avec le fichier : %s\n",
            argv[2]);
    return EXIT_FAILURE;
  }
  printf("Résumé de l'élection :\n");

  DumpElection(&e, stdout);

  FreeElectionContent(&e);
  return EXIT_SUCCESS;
}
