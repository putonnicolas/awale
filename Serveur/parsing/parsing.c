#include "parsing.h"

// ---------------------------------------------------------------- //
 void extract_props(const char *src, ParsedMessage *msg)
{
  if (!src || !msg || *src == '\0')
    return;

  char *copy = strdup(src);
  if (!copy)
    return;

  char *token = strtok(copy, " ");
  if (!token)
  {
    free(copy);
    return;
  }

  msg->command = strdup(token);
  msg->argc = 0;
  msg->argv = NULL;

  // Get the other arguments
  while ((token = strtok(NULL, " ")) != NULL)
  {
    char **tmp = realloc(msg->argv, sizeof(char *) * (msg->argc + 1));
    if (!tmp)
    {
      // If realloc failed, we free everything
      for (int i = 0; i < msg->argc; i++)
        free(msg->argv[i]);
      free(msg->argv);
      free(msg->command);
      free(copy);
      return;
    }
    msg->argv = tmp;
    msg->argv[msg->argc] = strdup(token);
    msg->argc++;
  }

  free(copy);
}
