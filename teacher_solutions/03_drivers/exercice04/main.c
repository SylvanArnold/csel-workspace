#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const char* text =
    "\n"
    "bonjour le monde\n"
    "ce mois d'octobre est plutot humide...\n"
    "ce n'est qu'un petit texte de test...\n";

static const char* text2 =
    "\n"
    "et voici un complement au premier text..\n"
    "ce n'est qu'un deuxieme petit texte de test...\n";

static const char* blabla =
    "blabla blabla blabla blabla blabla blabla blabla\n";

int main(int argc, char* argv[])
{
    if (argc <= 1) return 0;

    int fdw = open(argv[1], O_RDWR);
    write(fdw, argv[1], strlen(argv[1]));
    write(fdw, text, strlen(text));
    write(fdw, text2, strlen(text2));

    int s;
    do {
        s = write(fdw, blabla, strlen(blabla));
    } while (s >= 0);
    close(fdw);

    int fdr = open(argv[1], O_RDONLY);
    while (1) {
        char buff[100];
        ssize_t sz = read(fdr, buff, sizeof(buff) - 1);
        if (sz <= 0) break;
        buff[sizeof(buff) - 1] = 0;
        printf("%s", buff);
    }
    close(fdr);

    return 0;
}
