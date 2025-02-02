#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

// Variable atomique pour gérer l'accusé de réception (ack)
volatile sig_atomic_t ack = 0;

// Gestionnaire de signal pour l'accusé de réception
void handle_ack(int sig) {
    (void)sig; // Ignorer le paramètre de signal non utilisé
    ack = 1;   // Mettre à jour l'accusé de réception
}

// Fonction pour envoyer un bit au serveur
void send_bit(int pid, int bit) {
    ack = 0; // Réinitialiser l'accusé de réception
    // Envoyer SIGUSR1 pour un bit 1, SIGUSR2 pour un bit 0
    if (kill(pid, bit ? SIGUSR1 : SIGUSR2) == -1) {
        perror("kill"); // Gestion d'erreur si l'envoi échoue
        exit(EXIT_FAILURE);
    }

    int delay = 1000; // Délai d'attente pour l'accusé de réception
    while (!ack && delay > 0) {
        usleep(1); // Attendre 1 microseconde
        delay--;
    }

    if (delay <= 0) {
        fprintf(stderr, "No ack received\n"); // Aucun accusé de réception reçu
    }
}

// Fonction pour envoyer un octet (8 bits) au serveur
void send_byte(int pid, unsigned char byte) {
    for (int bit = 7; bit >= 0; bit--) {
        send_bit(pid, byte & (1 << bit)); // Envoyer chaque bit de l'octet
    }
}

// Fonction pour envoyer un message complet au serveur
void send_msg(int pid, const char *msg) {
    const unsigned char *m = (const unsigned char *)msg; // Convertir le message en unsigned char
    size_t len = strlen(msg); // Longueur du message
    size_t i = 0;

    while (i < len) {
        unsigned char byte = m[i]; // Obtenir chaque octet du message
        send_byte(pid, byte); // Envoyer l'octet au serveur
        i++;
    }

    send_byte(pid, '\n'); // Envoyer un caractère de nouvelle ligne pour terminer le message
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <pid> <message>\n", argv[0]); // Vérifier les arguments
        exit(EXIT_FAILURE);
    }

    signal(SIGUSR1, handle_ack); // Configurer le gestionnaire de signal pour SIGUSR1

    int pid = atoi(argv[1]); // Convertir le PID en entier
    const char *msg = argv[2]; // Récupérer le message à envoyer

    printf("Sending to %d.\n", pid); // Afficher le PID du serveur
    send_msg(pid, msg); // Envoyer le message au serveur
    printf("Done.\n"); // Confirmer l'envoi

    return 0;
}