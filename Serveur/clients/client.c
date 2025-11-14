#include "client.h"

 void load_user_data(Client *client)
{
  FILE *f = fopen(USER_DB_FILE, "r");
  if (!f)
  {
    perror("Erreur ouverture %s en lecture");
    client->wins = 0;
    strncpy(client->bio, "pas de bio", sizeof(client->bio) - 1);
    client->bio[sizeof(client->bio) - 1] = '\0';
    return;
  }

  char line[BUF_SIZE];
  char found = 0;

  while (fgets(line, sizeof(line), f))
  {
    char name[BUF_SIZE], bio[BUF_SIZE];
    unsigned int wins;

    bio[0] = '\0';
    if (sscanf(line, "%255[^;];%u;%255[^\n]", name, &wins, bio) >= 2)
    {
      if (strcmp(name, client->name) == 0)
      {
        client->wins = wins;
        strncpy(client->bio, bio, sizeof(client->bio) - 1);
        client->bio[sizeof(client->bio) - 1] = '\0';
        found = 1;
        break;
      }
    }
  }

  fclose(f);

  if (!found)
  {
    client->wins = 0;
    strncpy(client->bio, "pas de bio", sizeof(client->bio) - 1);
    client->bio[sizeof(client->bio) - 1] = '\0';
  }
}

// --------------------------------------------------------------//
// Update a user's bio in users.txt
 void update_user_bio(Client *client, char *newBio)
{
  FILE *f = fopen(USER_DB_FILE, "r");
  if (!f)
  {
    perror("Erreur ouverture users.txt en lecture");
    return;
  }

  // Load all lines in memory
  char lines[256][BUF_SIZE];
  int count = 0;

  while (fgets(lines[count], sizeof(lines[count]), f) && count < 256)
  {
    count++;
  }
  fclose(f);

  // Rewrite file with updated bio
  f = fopen(USER_DB_FILE, "w");
  if (!f)
  {
    perror("Erreur ouverture %s en écriture");
    return;
  }

  for (int i = 0; i < count; i++)
  {
    char name[BUF_SIZE], bio[BUF_SIZE] = {0};
    int wins = 0;

    if (sscanf(lines[i], "%[^;];%d;%[^\n]", name, &wins, bio) >= 2)
    {
      if (strcmp(name, client->name) == 0)
      {
        // Replace bio with the new one
        fprintf(f, "%s;%d;%s\n", name, wins, newBio);
        strncpy(client->bio, newBio, sizeof(client->bio) - 1);
      }
      else
      {
        // Keep existing data unchanged
        fprintf(f, "%s;%d;%s\n", name, wins, bio);
      }
    }
  }

  fclose(f);
}

// ------------------------------------------------------------
// Increment the user's win count in users.txt by +1
 void increment_user_win_count(Client *client)
{
  FILE *f = fopen(USER_DB_FILE, "r");
  if (!f)
  {
    perror("Erreur ouverture users.txt en lecture");
    return;
  }

  // Load all lines in memory
  char lines[256][BUF_SIZE];
  int count = 0;

  while (fgets(lines[count], sizeof(lines[count]), f) && count < 256)
  {
    count++;
  }
  fclose(f);

  // Rewrite file with updated wins
  f = fopen(USER_DB_FILE, "w");
  if (!f)
  {
    perror("Erreur ouverture users.txt en écriture");
    return;
  }

  for (int i = 0; i < count; i++)
  {
    char name[BUF_SIZE], bio[BUF_SIZE] = {0};
    int wins = 0;

    // Parse each line
    if (sscanf(lines[i], "%[^;];%d;%[^\n]", name, &wins, bio) >= 2)
    {
      if (strcmp(name, client->name) == 0)
      {
        wins += 1;
        client->wins = wins;
      }
      fprintf(f, "%s;%d;%s\n", name, wins, bio);
    }
  }

  fclose(f);
}

// ----------------------------------------------------------------------------------//
 char check_existing_user(Client *client)
{
  FILE *f = fopen(USER_DB_FILE, "a+");
  if (f != NULL)
  {
    char line[BUF_SIZE];
    char found = 0;

    rewind(f);
    while (fgets(line, sizeof(line), f))
    {
      line[strcspn(line, "\r\n")] = '\0';
      if (strncmp(line, client->name, strlen(client->name)) == 0 &&
          line[strlen(client->name)] == ';')
      {
        found = 1;
        break;
      }
    }

    if (!found)
    {
      fprintf(f, "%s;%d;%s\n", client->name, 0, "pas de bio");
      fflush(f);
    }

    fclose(f);

    return found;
  }
  else
  {
    perror("Erreur ouverture users.txt");
    return 0;
  }
}

// ----------------------------------------------------------------------------------//
void chat(Client *client, Client **otherClients,
          ParsedMessage *props)
{
  char message[2048] = {0};

  for (int i = 0; i < props->argc; i++)
  {
    strcat(message, props->argv[i]);
    strcat(message, " ");
  }

  send_message_to_all_clients(otherClients, *client, MAX_CLIENTS, message, 0);
}