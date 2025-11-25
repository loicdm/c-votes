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
  int thresholdW;
  int thresholdR;
  int voteCount;
  int blanks;
  bool blankWin;
  const Candidate *restrict winner;
  CandidateDynArray rejects;
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
    fprintf(stream, "[%s][%zu] : %d/%d\n", c->name, c->id, c->scoreFor,
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
bool InitElection(Election *e, const char *restrict name,
                  unsigned int thresholdW, unsigned int thresholdR) {
  e->name = strdup(name);
  if (!InitCandidateDynArray(&(e->candidates))) {
    free(e);
    return false;
  }
  if (!InitCandidateDynArray(&(e->rejects))) {
    FreeCandidateDynArray(&(e->candidates));
    free(e);
    return false;
  }
  e->thresholdW = thresholdW;
  e->thresholdR = thresholdR;
  e->blanks = 0;
  e->blankWin = false;
  e->voteCount = 0;
  // printf("Election initialisée : [%s]\n", name);
  return true;
}

void FreeElectionContent(Election *e) {
  FreeCandidateDynArray(&(e->candidates));
  FreeCandidateDynArray(&(e->rejects));
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

bool FindWinner(Election *e) {
  Candidate *winner = NULL;
  bool result = false;
  CandidateDynArray *candidates = &(e->candidates);
  if (candidates->count > 0) {
    winner = &(candidates->array[0]);
    for (size_t i = 0; i < candidates->count; i++) {
      Candidate *c = &(candidates->array[i]);
      if (c->scoreFor > winner->scoreFor)
        winner = c;
    }
  }
  if ((100 * winner->scoreFor) >= e->voteCount * e->thresholdW &&
      (100 * winner->scoreAgainst) <= e->voteCount * e->thresholdR) {
    e->winner = winner;
    result = true;
  }

  if (e->blanks && (100 * e->blanks) >= e->voteCount * e->thresholdW) {
    if (winner && winner->scoreFor < e->blanks) {
      winner = NULL;
      result = false;
      e->blankWin = true;
    }
  }
  return result;
}

void DumpElection(Election *e, FILE *stream) {
  DumpCandidateDynArray(&(e->candidates), stream);
  fprintf(stream, "Nombre de votes blancs : %d\n", e->blanks);
  fprintf(stream, "Total des votes : %d\n", e->voteCount);
}

bool FindReject(Election *e) {
  bool result = false;
  CandidateDynArray *candidates = &(e->candidates);
  for (size_t i = 0; i < candidates->count; i++) {
    Candidate *c = &(candidates->array[i]);
    if ((100 * c->scoreAgainst) > e->voteCount * e->thresholdR) {
      Candidate looser;
      InitCandidate(&looser, c->name, c->id, c->scoreFor, c->scoreAgainst);
      PushCandidateToDynArray(&(e->rejects), looser);
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
        printf("%d vote(s) comptabilisé pour le candidat [%s] ID [%zu] score "
               "[%d]\n",
               count, c->name, c->id, count);
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
  if (argc < 5) {
    ShowHelp(stderr, argv);
    return EXIT_FAILURE;
  }
  int thresholdW = atoi(argv[3]);
  int thresholdR = atoi(argv[4]);

  if (thresholdR > 100 || thresholdR < 0 || thresholdW > 100 ||
      thresholdW < 0) {
    fprintf(stderr, "Les seuils doivent être entre 0 et 100\n");
    return EXIT_FAILURE;
  }

  Election e;
  if (!InitElection(&e, argv[1], thresholdW, thresholdR)) {
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
    if (e.blankWin) {
      printf("Le vote blanc l'emporte avec %d vote(s)\n", e.blanks);
    }
  } else {
    printf("Le gagnant est [%s] avec %d votes\n", e.winner->name,
           e.winner->scoreFor);
  }
  if (FindReject(&e)) {
    printf("Les candidats rejetés sont : \n");
    DumpCandidateDynArray(&(e.rejects), stdout);
  }

  FreeElectionContent(&e);
  return EXIT_SUCCESS;
}
