# Définir le compilateur et les options de compilation
CC = gcc
CFLAGS = -Wall -Wextra -Werror

# Définir les cibles
TARGETS = client server

# Règle par défaut pour compiler toutes les cibles
all: $(TARGETS)

# Règle pour compiler le client
client: client.c
	$(CC) $(CFLAGS) -o client client.c

# Règle pour compiler le serveur
server: server.c
	$(CC) $(CFLAGS) -o server server.c

# Règle pour nettoyer les fichiers compilés
clean:
	rm -f $(TARGETS)

# Règle pour nettoyer complètement le projet
fclean: clean

# Règle pour recompiler complètement le projet
re: fclean all

# Définir les cibles comme des cibles phony
.PHONY: all clean fclean re
