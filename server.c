#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define MAX_MSG 4096 // Taille maximale du message
#define TS_LEN 12    // Longueur de l'horodatage

// Structure pour stocker les informations du message en cours de réception
typedef struct {
    unsigned char msg[MAX_MSG]; // Buffer pour stocker le message
    int idx;                    // Index actuel dans le buffer
    unsigned char byte;         // Octet en cours de construction
    int bit_cnt;                // Compteur de bits reçus pour l'octet
    pid_t client_pid;           // PID du client qui envoie le message
} MsgInfo;

// Variable volatile pour stocker les informations du message en cours
volatile MsgInfo current_msg = {.idx = 0, .byte = 0, .bit_cnt = 0, .client_pid = 0};

// Fonction pour obtenir l'horodatage actuel
void get_time(char *ts) {
    time_t now;
    struct tm *tm_info;
    time(&now); // Obtenir l'heure actuelle
    tm_info = localtime(&now); // Convertir en temps local
    strftime(ts, TS_LEN, "[%H:%M:%S]", tm_info); // Formater l'horodatage
}

// Fonction pour enregistrer le message dans un fichier log
void log_msg(const unsigned char *msg, pid_t client_pid) {
    FILE *log = fopen("conversations.log", "a"); // Ouvrir le fichier log en mode ajout
    if (log == NULL) {
        perror("fopen"); // Gestion d'erreur si l'ouverture échoue
        return;
    }
    char ts[TS_LEN];
    get_time(ts); // Obtenir l'horodatage
    fprintf(log, "%s [From %d] %s\n", ts, client_pid, msg); // Écrire le message dans le log
    fclose(log); // Fermer le fichier log
}

// Gestionnaire de signal pour traiter les bits reçus
void handle_signal(int sig, siginfo_t *info, void *ctx) {
    (void)ctx; // Ignorer le contexte non utilisé

    // Si c'est le début d'un nouveau message, enregistrer le PID du client
    if (current_msg.idx == 0) {
        ((volatile MsgInfo *)&current_msg)->client_pid = info->si_pid;
    }
    // Ignorer les signaux d'autres clients
    else if (current_msg.client_pid != info->si_pid) {
        return;
    }

    // Déterminer si le bit reçu est 1 (SIGUSR1) ou 0 (SIGUSR2)
    int bit = (sig == SIGUSR1) ? 1 : 0;
    // Construire l'octet en décalant et en ajoutant le nouveau bit
    ((volatile MsgInfo *)&current_msg)->byte = (current_msg.byte << 1) | bit;
    ((volatile MsgInfo *)&current_msg)->bit_cnt++;

    // Envoyer un accusé de réception au client
    kill(info->si_pid, SIGUSR1);

    // Si un octet complet (8 bits) a été reçu
    if (current_msg.bit_cnt == 8) {
        if (current_msg.byte == '\n') { // Si c'est la fin du message
            ((volatile MsgInfo *)&current_msg)->msg[current_msg.idx] = '\0'; // Terminer la chaîne

            // Copier le message et le PID pour l'affichage et le log
            char temp_msg[MAX_MSG];
            pid_t temp_pid = current_msg.client_pid;
            memcpy(temp_msg, (void*)current_msg.msg, current_msg.idx + 1);

            char ts[TS_LEN];
            get_time(ts); // Obtenir l'horodatage

            // Afficher le message reçu
            printf("%s [From %d] %s\n", ts, temp_pid, temp_msg);
            // Enregistrer le message dans le log
            log_msg((unsigned char *)temp_msg, temp_pid);

            // Réinitialiser le buffer pour le prochain message
            memset((void*)&current_msg.msg, 0, MAX_MSG);
            ((volatile MsgInfo *)&current_msg)->idx = 0;
        } else {
            // Ajouter l'octet au buffer si il y a de la place
            if (current_msg.idx < MAX_MSG - 1) {
                ((volatile MsgInfo *)&current_msg)->msg[current_msg.idx] = current_msg.byte;
                ((volatile MsgInfo *)&current_msg)->idx++;
            }
        }
        // Réinitialiser l'octet et le compteur de bits
        ((volatile MsgInfo *)&current_msg)->byte = 0;
        ((volatile MsgInfo *)&current_msg)->bit_cnt = 0;
    }
}

int main() {
    printf("Miniteams starting...\n");
    printf("My PID is %d\n", getpid()); // Afficher le PID du serveur

    // Configurer les gestionnaires de signaux pour SIGUSR1 et SIGUSR2
    struct sigaction sa;
    sa.sa_sigaction = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(SIGUSR1, &sa, NULL) == -1 || sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("sigaction"); // Gestion d'erreur si la configuration échoue
        exit(EXIT_FAILURE);
    }

    printf("Waiting for messages\n");
    while (1) {
        pause(); // Attendre indéfiniment des signaux
    }
    return 0;
}