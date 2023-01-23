#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define MAX_SIZE 4096

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "bad usage: %s\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    FILE* filePointer = fopen(argv[1], "w");
    if (filePointer == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    srand(time(NULL));
    int whichText = rand() % 3;
    char text[MAX_SIZE];
    switch (whichText) {
        case 0:
            strcpy(text,"\"I would not take this thing, if it lay by the highway.\nNot were Minas Tirith falling in ruin and I alone could save her, so, using the weapon of the Dark Lord for her good and my glory.\nNo, I do not wish for such triumphs, Frodo son of Drogo….\" \n\n\"For myself, I would see the White Tree flower again in the courts of the kings, and the Silver Crown return, and Minas Tirith in peace: Minas Anor again as of old, full of light, high and fair, beautiful as a queen among other queens: not a mistress of many slaves, nay, not even a kind mistress of willing slaves [Galadriel!].\nWar must be, while we defend our lives against a destroyer who would devour us all; but I do not love the bright sword for its sharpness, nor the arrow for its swiftness, nor the warrior for his glory.\nI love only that which they defend: the city of the Men of Numenor; and I would have loved her for her memory, her ancientry, her beauty, and her present wisdom.\nNot feared, save as men may fear the dignity of a man, old and wise.\"\n\nFaramir, The Two Towers, Book 2, \"Chapter V: The Window on the West,\" p. 678-9.");
            fprintf(filePointer, "%s", text);
            break;
        case 1:
            strcpy(text, "\"People,\" Geralt turned his head, \"like to invent monsters and monstrosities.\nThen they seem less monstrous themselves.\nWhen they get blind-drunk, cheat, steal, beat their wives, starve an old woman, when they kill a trapped fox with an axe or riddle the last existing unicorn with arrows, they like to think that the Bane entering cottages at daybreak is more monstrous than they are.\nThey feel better then.\nThey find it easier to live.\" - Andrzej Sapkowski, The Last Wish");
            fprintf(filePointer, "%s", text);
            break;
        case 2:
            strcpy(text, "Isshin: You're leaving, Sekiro? Severing immortality... That will be quite the battle.\nAnd in battle... The plans and desires of those involved churn endlessly.\nIf you hesitate you'll be swept away... and lose the battle. That's right. Best you keep it in mind. Sekiro. Hesitate, and you lose.\nIsshin: Don't forget, Sekiro. Hesitate, and you lose... That is the way of war."); 
            fprintf(filePointer, "%s", text);
            break;
        default:
            break;
    }
    printf("Successfully generated file!\n"); fflush(stdout);
    fclose(filePointer);
    return 0;
}