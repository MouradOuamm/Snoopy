#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Définition des structures

struct Level {
    char **map; // La carte du niveau
    int width; // La largeur du niveau
    int height; // La hauteur du niveau

    // Position de Snoopy
    int snoopy_x;
    int snoopy_y;

    // Position de la balle
    int ball_x;
    int ball_y;

    // Position des oiseaux
    int birds_x[4];
    int birds_y[4];

    // Position des blocs
    struct Block **blocks;
    int nb_blocks;

    // Temps restant
    int time_left;

    // Nombre de vies restantes
    int lives;

    // Score courant
    int score;
};

// Définition des types de blocs

enum BlockType {
    BLOCK_EMPTY,
    BLOCK_PUSHABLE,
    BLOCK_BREAKABLE,
    BLOCK_TRAPPED
};

// Définition de la structure d'un bloc

struct Block {
    // Type du bloc
    enum BlockType type;

    // Position du bloc
    int x;
    int y;
};

// Définition des directions

const int directions[4][2] = {
    {-1, 0}, // Haut
    {1, 0}, // Bas
    {0, -1}, // Gauche
    {0, 1} // Droite
};

// Fonction pour charger un niveau à partir d'un fichier texte
struct Level *load_level(const char *filename) {

    // Ouvre le fichier
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return NULL;
    }

    // Lit la largeur et la hauteur du niveau
    int width, height;
    fscanf(file, "%d %d", &width, &height);

    // Alloue la mémoire pour la carte du niveau
    char **map = malloc(sizeof(char *) * height);
    for (int i = 0; i < height; i++) {
        map[i] = malloc(sizeof(char) * width);
    }

    // Lit la carte du niveau
    for (int i = 0; i < height; i++) {
        fgets(map[i], width + 1, file);
    }

    // Ferme le fichier
    fclose(file);

    // Crée un nouvel objet de niveau
    struct Level *level = malloc(sizeof(struct Level));
    level->map = map;
    level->width = width;
    level->height = height;

    // Initialise les autres données du niveau
    level->snoopy_x = 0;
    level->snoopy_y = 0;
    level->ball_x = 0;
    level->ball_y = 0;
    for (int i = 0; i < 4; i++) {
        level->birds_x[i] = -1;
        level->birds_y[i] = -1;
    }
    level->time_left = 120;
    level->lives = 3;
    level->score = 0;

    // Initialise la position des blocs
    level->nb_blocks = 0;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (map[i][j] != ' ') {
                level->blocks[level->nb_blocks] = malloc(sizeof(struct Block));
                level->blocks[level->nb_blocks]->type = (map[i][j] == 'P') ? BLOCK_PUSHABLE : BLOCK_BREAKABLE;
                level->blocks[level->nb_blocks]->x = j;
                level->blocks[level->nb_blocks]->y = i;
                level->nb_blocks++;
            }
        }
    }

    return level;
}

// Fonction pour libérer la mémoire allouée à un niveau
void free_level(struct Level *level) {

    for (int i = 0; i < level->height; i++) {
        free(level->map[i]);
    }
    free(level->map);

    for (int i = 0; i < level->nb_blocks; i++) {
        free(level->blocks[i]);
    }
    free(level->blocks);

    free(level);
}

// Fonction pour vérifier si une direction est valide
int is_valid_direction(int direction) {
    return direction >= 0 && direction < 4;
}
enum MoveSnoopyError move_snoopy(struct Level *level, int direction) {
    if (!is_valid_direction(direction)) {
        return MOVE_SNOOPY_INVALID_DIRECTION;
    }
    // Obtient la nouvelle position de Snoopy
    int new_x = level->snoopy_x + directions[direction][0];
    int new_y = level->snoopy_y + directions[direction][1];

    // Vérifie qu'il n'y a pas de collision
    if (level->map[new_y][new_x] != ' ') {
        return MOVE_SNOOPY_COLLISION;
    }

    // Vérifie que Snoopy ne sort pas du niveau
    if (new_x < 0 || new_x >= level->width || new_y < 0 || new_y >= level->height) {
        return MOVE_SNOOPY_OUT_OF_BOUNDS;
    }

    // Met à jour la position de Snoopy
    level->snoopy_x = new_x;
    level->snoopy_y = new_y;

    return MOVE_SNOOPY_OK;
}

// Fonction pour déplacer la balle
void move_ball(struct Level *level) {

    // Obtient la nouvelle position de la balle
    int new_x = level->ball_x + directions[level->ball_direction][0];
    int new_y = level->ball_y + directions[level->ball_direction][1];

    // Vérifie qu'il n'y a pas de collision
    if (level->map[new_y][new_x] != ' ') {

        // Si la balle touche un bord, elle rebondit
        if (new_x < 0 || new_x >= level->width || new_y < 0 || new_y >= level->height) {
            level->ball_direction = (level->ball_direction + 2) % 4;
        } else {

            // Si la balle touche un bloc, elle peut le détruire
            if (level->blocks[new_y][new_x].type == BLOCK_BREAKABLE) {
                level->blocks[new_y][new_x].type = BLOCK_EMPTY;
            } else if (level->blocks[new_y][new_x].type == BLOCK_TRAPPED) {
                level->lives--;
                if (level->lives <= 0) {
                    // Game over
                    return;
                }
            }

            // Si la balle touche un oiseau, le niveau est terminé
            for (int i = 0; i < 4; i++) {
                if (level->birds_x[i] == new_x && level->birds_y[i] == new_y) {
                    level->score += 100;
                    level->birds_x[i] = -1;
                    level->birds_y[i] = -1;

                    // Si tous les oiseaux sont récupérés, le niveau est terminé
                    if (is_level_completed(level)) {
                        level->time_left = 0;
                    }
                    break;
                }
            }
        }
    }

    // Met à jour la position de la balle
    level->ball_x = new_x;
    level->ball_y = new_y;
}

// Fonction pour vérifier si le niveau est terminé
int is_level_completed(struct Level *level) {

    for (int i = 0; i < 4; i++) {
        if (level->birds_x[i] != -1) {
            return 0;
        }
    }


    return 1;
}

// Fonction pour afficher le menu
void show_menu() {

    printf("** La revanche de Snoopy **\n\n");
    printf("1. Nouveau jeu\n");
    printf("2. Charger un jeu\n");
    printf("3. Quitter\n\n");
    printf("Votre choix : ");
}

// Fonction pour afficher le niveau à l'écran
void display_level(struct Level *level) {

    for (int i = 0; i < level->height; i++) {
        for (int j = 0; j < level->width; j++) {
            switch (level->map[i][j]) {
                case ' ':
                    printf(" ");
                    break;
                case 'P':
                    printf("P");
                    break;
                case 'B':
                    printf("B");
                    break;
                case 'S':
                    printf("S");
                    break;
                case '*':
                    printf("*");
                    break;
            }
        }
        printf("\n");
    }

    // Affiche la position de Snoopy
    printf("Snoopy : (%d, %d)\n", level->snoopy_x, level->snoopy_y);

    // Affiche le temps restant et le score
    printf("Temps restant : %d | Score : %d\n", level->time_left, level->score);
}

// Fonction principale
int main() {

    // Initialise le générateur de nombres aléatoires
    srand(time(NULL));

    // Affiche le menu
    show_menu();

    // Tant que le joueur ne quitte pas le jeu
    while (1) {

        // Charge le niveau actuel
        struct Level *level;
        int choice;
        scanf("%d", &choice);
        switch (choice) {
            case 1:
                level = load_level("levels/level1.txt");
                break;
            case 2:
                printf("Entrez le nom du fichier à charger : ");
                char filename[255];
                scanf("%s", filename);
                level = load_level(filename);
                break;
            case 3:
                return 0;
            default:
                printf("Choix invalide\n");
                continue;
        }

        // Initialise le timer
        level->time_left = 120;

        // Joue le niveau
        while (level->time_left > 0 && level->lives > 0) {

            // Affiche le niveau à l'écran
            display_level(level);

            // Attend une entrée du joueur
            int key = getchar();

            // Déplace Snoopy
            if (key == 'z') {
                move_snoopy(level, MOVE_SNOOPY_UP);
            } else if (key == 'q') {
                move_snoopy(level, MOVE_SNOOPY_DOWN);
            } else if (key == 's') {
                move_snoopy(level, MOVE_SNOOPY_LEFT);
            } else if (key == 'd') {
                move_snoopy(level, MOVE_SNOOPY_RIGHT);
            }

            // Déplace la balle
            move_ball(level);

            // Décompte le temps restant
            level->time_left--;

            // Vérifie si le niveau est terminé
            if (is_level_completed(level)) {
                printf("Félicitations ! Vous avez terminé le niveau !\n");
                break;
            }
        }

        // Affiche le message de game over si nécessaire
        if (level->lives <= 0) {
            printf("Game over ! Vous avez perdu toutes vos vies.\n");
        }

        // Libère la mémoire du niveau
        free_level(level);

        // Demande au joueur s'il souhaite continuer
        char choice;
        printf("Voulez-vous continuer (o/n) ? ");
        scanf("%c", &choice);
        if (choice != 'o') {
            break;
        }
    }

    return 0;
}

