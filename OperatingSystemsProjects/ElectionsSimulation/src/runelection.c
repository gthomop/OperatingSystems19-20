/****** runelection.c ******/

#include "commands.h"
#include "postalcodes.h"
#include "run.h"
#include "votersbloom.h"
#include "votersrbt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

char *read_key(char *str);
void free_on_exit(VotersRBT *voters_tree, VotersBloomFilter *voters_bloom,
                  PostalCodesRBT *postalCodes);

int main(int argc, char **argv) {
  fprintf(stdout, "********* %s *********\n", argv[0]);
  CmdParams params = run(argc, argv);

  if (params.rightUsage == 0) {
    return 1;
  }

  char *registry;
  int numofupdates;

  if ((registry = realpath(params.params[0], NULL)) ==
      NULL) { // if registry does not exist, exit
    perror("registry - realpath()");
    free(registry);
    freeCmdParams(&params);

    return 1;
  }

  numofupdates = (int)strtol(params.params[2], NULL, 10);

  FILE *regf = NULL;
  VotersRBT *voters_tree = votersrbt_init();
  PostalCodesRBT *postalcodes = postalcodes_init();

  int count = 0;

  regf = fopen(registry, "r");

  if (regf == NULL) {
    perror("Registry fopen()");
    free_on_exit(voters_tree, NULL, postalcodes);
    free(registry);
    freeCmdParams(&params);

    return 1;
  }

  char *line = NULL;
  int post = 0;
  size_t emptyline = 0;

  while ((getline(&line, &emptyline, regf)) != -1) {
    Voter *v = voter_init(line, &post);

    if (votersrbt_insert(voters_tree, v)) {
      fprintf(stderr,
              "2A voter with id number %s already exists in the registry.\n",
              v->id_num);
      voter_free(v);

      free(line);
      line = NULL;
      emptyline = 0;

      continue;
    }

    postalcodes_insert(postalcodes, post, v);

    count++;
    free(line);
    line = NULL;
    emptyline = 0;
  }

  free(line);

  fprintf(stdout, "Finised reading from file\n");

  /** Finished reading from file **/

  fclose(regf);
  free(registry);

  /** Open the output file **/
  FILE *outf = fopen(params.params[1], "w");

  if (outf == NULL) {
    perror("Outfile fopen()");
    free_on_exit(voters_tree, NULL, postalcodes);
    freeCmdParams(&params);

    return 1;
  }

  freeCmdParams(&params);

  /** Basic check **/
  if (count == 0) {
    fprintf(stderr,
            "Error: Could not read any registration. The program exits.\n");
    free_on_exit(voters_tree, NULL, postalcodes);

    return 1;
  }

  /** Initialize the Bloom Filter **/
  VotersBloomFilter *voters_bloom = votersbloom_init(count, numofupdates);
  votersbloom_fill(voters_bloom, voters_tree);

  /** Start reading commands **/
  line = NULL;          /** getline() buffer **/
  char *command = NULL; /** tokenized string **/
  char *str =
      NULL; /** pointer to the location in the buffer we are currently at **/
  char *cmdargu = NULL; /** the argument that follows almost all commands **/
  ssize_t len = 0;      /** keeps getline()'s return code **/
  size_t sz = 0; /** just getline()'s requirement for the size of the buffer **/
  int havevoted =
      0; /** tracks how many voters currently in the register have voted **/

  while (1) {
    do {
      if (line != NULL) {
        free(line);
        line = NULL;
      }
      fprintf(stdout, "Give command:\n");
      sz = 0;
      len = getline(&line, &sz, stdin);
      if (len < 1) {
        free(line);
        line = NULL;
        fprintf(stderr, "Error: Failed to read a command.\n");
        continue;
      }

      line[len - 1] = 0; /** the \n character is set to 0 **/
      len--;
      str = line; /** in order to free(line), we use another variable to do the
                     strsep() **/
      command = strsep(&str, " ");
    } while (command[0] == 0); /** skips possible " " inputs as commmands **/

    if (!(strcmp(command, "exit"))) {
      fprintf(stdout, "%s exits.\n", argv[0]);
      free(line);
      break;
    } else if (!(strcmp(command, "lbf"))) {
      cmdargu = read_key(str);
      if (cmdargu == NULL)
        continue;

      lbf(voters_bloom, cmdargu);
      continue;
    } else if (!(strcmp(command, "lrb"))) {
      cmdargu = read_key(str);
      if (cmdargu == NULL)
        continue;

      lrb(voters_tree, cmdargu);

      continue;
    } else if (!(strcmp(command, "ins"))) {
      ins(voters_tree, voters_bloom, postalcodes, &count, str);

      continue;
    } else if (!(strcmp(command, "find"))) {
      cmdargu = read_key(str);
      if (cmdargu == NULL)
        continue;

      find(voters_tree, voters_bloom, cmdargu);

      continue;

    } else if (!(strcmp(command, "delete"))) {
      cmdargu = read_key(str);
      if (cmdargu == NULL)
        continue;

      delete (voters_tree, voters_bloom, postalcodes, cmdargu, &count,
              &havevoted);

      continue;
    } else if (!(strcmp(command, "vote"))) {
      cmdargu = read_key(str);
      if (cmdargu == NULL)
        continue;

      vote(voters_tree, voters_bloom, cmdargu, &havevoted);

      continue;
    } else if (!(strcmp(command, "load"))) {
      cmdargu = NULL;
      cmdargu = strsep(&str, " ");

      if (cmdargu == NULL || cmdargu[0] == 0) {
        fprintf(stderr, "- Error: Could not read a filename.\n");
        continue;
      }

      load(voters_tree, voters_bloom, cmdargu, &havevoted);
      continue;
    } else if (!(strcmp(command, "voted"))) {
      cmdargu = strsep(&str, " ");
      voted(postalcodes, cmdargu, havevoted);

      continue;
    } else if (!(strcmp(command, "votedperpc"))) {
      votedperpc(postalcodes);

      continue;
    } else {
      fprintf(stderr, "Wrong command, try:\nlbf key (lookup bloom-filter)\nlrb "
                      "key(lookup red-black tree)\nins record(insert record to "
                      "red-black tree)\nfind key\ndelete key\nvote key\nload "
                      "fileofkeys\nvoted\nvoted postcode\nvotedperpc\nexit\n");
      continue;
    }
  }

  ConnList *list_of_tree = rbt_get_all_nodes(voters_tree);

  ListNode *curr = list_of_tree->start_of_list;

  if (curr != NULL) {
    while (curr->next != NULL) {
      fprintf(outf, "%s %s %s %d %c %d\n", ((Voter *)curr->data)->id_num,
              ((Voter *)curr->data)->surname, ((Voter *)curr->data)->name,
              ((Voter *)curr->data)->age, ((Voter *)curr->data)->sex,
              ((Voter *)curr->data)->post->code);
      curr = curr->next;
    }
    fprintf(outf, "%s %s %s %d %c %d\n", ((Voter *)curr->data)->id_num,
            ((Voter *)curr->data)->surname, ((Voter *)curr->data)->name,
            ((Voter *)curr->data)->age, ((Voter *)curr->data)->sex,
            ((Voter *)curr->data)->post->code);
  }
  connlist_free(list_of_tree);
  free_on_exit(voters_tree, voters_bloom, postalcodes);
  fclose(outf);

  return 0;
}

/** This procedure makes sure that the commands that require a key
 * do have a key written afterwards\
 */
char *read_key(char *str) {
  char *key = NULL;
  key = strsep(&str, " ");

  if (key == NULL || key[0] == 0) {
    fprintf(stderr, "- Error: Could not read a key.\n");
    return NULL;
  }

  return key;
}

void free_on_exit(VotersRBT *voters_tree, VotersBloomFilter *voters_bloom,
                  PostalCodesRBT *postalCodes) {
  postalcodes_free(postalCodes);
  if (voters_bloom != NULL) {
    votersbloom_free(voters_bloom);
  }
  votersrbt_free(voters_tree);

  return;
}