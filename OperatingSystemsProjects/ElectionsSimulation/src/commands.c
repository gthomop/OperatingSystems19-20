/***** commmands.c *****/
#include "commands.h"
#include "votersbloom.h"
#include "votersrbt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void lbf(VotersBloomFilter *voters_bloom, char *key) {
  if (votersbloom_in(voters_bloom, key)) {
    fprintf(stdout, "# KEY %s POSSIBLY-IN REGISTRY.\n", key);
  } else {
    fprintf(stdout, "- KEY %s NOT-IN-BF.\n", key);
  }

  return;
}

void lrb(VotersRBT *voters_tree, char *key) {
  char result = (votersrbt_find(voters_tree, key) == NULL) ? 0 : 1;
  if (result) {
    fprintf(stdout, "# KEY %s FOUND-IN-RBT.\n", key);
  } else {
    fprintf(stdout, "- KEY %s NOT-IN-RBT.\n", key);
  }

  return;
}

void ins(VotersRBT *voters_tree, VotersBloomFilter *voters_bloom,
         PostalCodesRBT *postalcodes, int *count, char *record) {
  int post = 0;
  Voter *v = voter_init(record, &post);

  if (!(votersbloom_in(voters_bloom, v->id_num)) &&
      !(votersrbt_insert(voters_tree, v))) {
    postalcodes_insert(postalcodes, post, v);

    (*count)++;

    votersbloom_insert_update(voters_bloom, v->id_num, *count, voters_tree);

    fprintf(stdout, "# REC-WITH KEY %s INSERTED\n", v->id_num);

    return;
  }

  fprintf(stdout, "- REC-WITH KEY %s EXISTS\n", v->id_num);
  voter_free(v);

  return;
}

void find(VotersRBT *voters_tree, VotersBloomFilter *voters_bloom, char *key) {
  Voter *found = NULL;

  if (votersbloom_in(voters_bloom, key) &&
      (found = votersrbt_find(voters_tree, key)) != NULL) {
    fprintf(stdout, "# REC-IS: %s %s %s %d %d\n", found->id_num, found->name,
            found->surname, found->post->code, found->age);

    return;
  }

  fprintf(stdout, "- REC-WITH KEY %s NOT-IN-STRUCTS\n", key);

  return;
}

void delete (VotersRBT *voters_tree, VotersBloomFilter *voters_bloom,
             PostalCodesRBT *postalCodes, char *key, int *count,
             int *havevoted) {
  if (!votersbloom_in(voters_bloom, key)) {
    fprintf(stdout, "- KEY %s NOT-IN-STRUCTS\n", key);
    return;
  }
  Voter *to_delete = votersrbt_find(voters_tree, key);
  if (to_delete == NULL) {
    fprintf(stdout, "- KEY %s NOT-IN-STRUCTS\n", key);
    return;
  }

  if (to_delete->hasvoted)
    (*havevoted)--;

  (*count)--;

  postalcodes_remove_voter(postalCodes, to_delete);
  votersrbt_delete(voters_tree, key);
  votersbloom_delete_update(voters_bloom, *count, voters_tree);

  fprintf(stdout, "# DELETED KEY %s FROM-STRUCTS\n", key);

  return;
}

void vote(VotersRBT *voters_tree, VotersBloomFilter *voters_bloom, char *key,
          int *havevoted) {
  if (!votersbloom_in(voters_bloom, key)) {
    fprintf(stdout, "- REC-WITH KEY %s NOT-IN-STRUCTS\n", key);
    return;
  }

  Voter *found = votersrbt_find(voters_tree, key);
  if (found == NULL) {
    fprintf(stdout, "- REC-WITH KEY %s NOT-IN-STRUCTS\n", key);
    return;
  }

  if (found->hasvoted == YES) {
    fprintf(stdout, "- REC-WITH KEY %s ALREADY-VOTED\n", key);
    return;
  }

  found->hasvoted = YES;

  found->post->havevoted++;

  (*havevoted)++;

  fprintf(stdout, "# REC-WITH KEY %s SET-VOTED\n", key);

  return;
}

void load(VotersRBT *voters_tree, VotersBloomFilter *voters_bloom,
          char *fileofkeys, int *havevoted) {
  FILE *idsFile = fopen(fileofkeys, "r");
  if (idsFile == NULL) {
    perror("load fopen()");
    return;
  }

  char *line = NULL, *id_num = NULL, *str = NULL;
  size_t len = 0;

  while ((getline(&line, &len, idsFile)) != -1) {
    str = line;
    id_num = strsep(&str, " \n\t");
    vote(voters_tree, voters_bloom, id_num, havevoted);

    free(line);
    line = NULL;
    len = 0;
  }

  free(line);

  fclose(idsFile);

  return;
}

void voted(PostalCodesRBT *postalCodes, char *codestr, int havevoted) {
  if (codestr == NULL || codestr[0] == 0) {
    fprintf(stdout, "# NUMBER %d\n", havevoted);

    return;
  }

  int code = (int)strtol(codestr, NULL, 10);

  int ret = postalcodes_get_votes(postalCodes, code);

  if (ret == -1) {
    fprintf(stdout, "- NO-VOTERS-WITH POSTCODE %d\n", code);
  } else {
    fprintf(stdout, "# IN POSTCODE %d VOTERS-ARE %d\n", code, ret);
  }

  return;
}

void votedperpc(PostalCodesRBT *postalCodes) {
  ConnList *list_of_pcs = rbt_get_all_nodes(postalCodes);

  ListNode *curr = list_of_pcs->start_of_list;

  if (curr == NULL) {
    fprintf(stdout, "- NO-RECORD\n");
    return;
  }

  double rate = 0.0;

  while (curr->next != NULL) {
    rate = ((double)(((PostalCode *)curr->data)->havevoted) /
            (double)(((PostalCode *)curr->data)->voters_number)) *
           100;
    fprintf(stdout, "# IN POSTCODE %d VOTERS-ARE %.2f%%\n",
            ((PostalCode *)curr->data)->code, rate);
    curr = curr->next;
  }

  rate = ((double)(((PostalCode *)curr->data)->havevoted) /
          ((double)((PostalCode *)curr->data)->voters_number)) *
         100;
  fprintf(stdout, "# IN POSTCODE %d VOTERS-ARE %.2f%%\n",
          ((PostalCode *)curr->data)->code, rate);

  connlist_free(list_of_pcs);
  return;
}