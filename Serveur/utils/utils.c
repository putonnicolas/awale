#include "utils.h"

// ---------------------------------------------------- //
 void help(Client *client)
{
  char message[2048];
  snprintf(message, sizeof(message),
           "\n--- Liste des commandes disponibles ---\n"
           "  list                  → Affiche la liste des joueurs connectés\n"
           "  challenge <nom>       → Défie un joueur en duel\n"
           "  deny                  → Refuse le dernier challenge reçu\n"
           "  play <index>          → Joue un coup (index du trou à jouer)\n"
           "  ff                    → Abandonne la partie en cours\n"
           "  watch <nom>           → Observe la partie d’un joueur\n"
           "  stopwatch             → Arrête d'observer la partie actuellement "
           "observée\n"
           "  say <message>         → Envoie un message à tout les joueurs "
           "connectés\n"
           "  bio <message>         → Met à jour votre bio\n"
           "  help                  → Affiche cette aide\n"
           "\n----------------------------------------\n");

  send_message_to_specific_client(client, message, 1);
}