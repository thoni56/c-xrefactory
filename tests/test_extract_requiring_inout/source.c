#include "tennis.h"

#include <strings.h>


char *call_out(int player1score, int player2score) {
    static char score[100] = "Fifteen";
    if (player2score == 3)
        strcat(score, " Fourty");
    else
        strcat(score, " Thirty");
    return score;
}
