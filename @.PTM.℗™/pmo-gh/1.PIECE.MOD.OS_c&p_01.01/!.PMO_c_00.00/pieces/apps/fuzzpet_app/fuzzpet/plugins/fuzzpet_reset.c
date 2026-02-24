#include <stdio.h>
#include <stdlib.h>

int main() {
    // Reset state.txt
    FILE *f = fopen("pieces/apps/fuzzpet_app/fuzzpet/state.txt", "w");
    if (f) {
        fprintf(f, "name=Fuzzball\n");
        fprintf(f, "hunger=50\n");
        fprintf(f, "happiness=75\n");
        fprintf(f, "energy=100\n");
        fprintf(f, "level=1\n");
        fprintf(f, "pos_x=5\n");
        fprintf(f, "pos_y=5\n");
        fprintf(f, "status=active\n");
        fprintf(f, "response=Welcome to a new game!\n");
        fclose(f);
    }
    
    // Reset map.txt
    FILE *mf = fopen("pieces/apps/fuzzpet_app/world/map.txt", "w");
    if (mf) {
        fprintf(mf, "####################\n");
        fprintf(mf, "#  ...............T#\n");
        fprintf(mf, "#  .......@.......T#\n");
        fprintf(mf, "#  ....R..........T#\n");
        fprintf(mf, "#  ....R..........T#\n");
        fprintf(mf, "#  ....R..........T#\n");
        fprintf(mf, "#  ....R..........T#\n");
        fprintf(mf, "#  ................#\n");
        fprintf(mf, "#                  #\n");
        fprintf(mf, "####################\n");
        fclose(mf);
    }
    
    return 0;
}
