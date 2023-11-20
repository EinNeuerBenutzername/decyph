#include <stdio.h>
#include <string.h>
#include "decyph.h"
// todo:
// [x] fix string leakage
// [x] fix empty pushes and removes
// [x] batch push
// [x] batch remove
// [x] batch morph
// [ ] notes are appended to words
// [ ] make morphs hold different translations
// [ ] encyph

int main(int argc, char *argv[]){
    if(argc)fio_init(argv[0]);
    char input[4096];
    cbc_init();
#ifdef CBCurses_Windows
    system("chcp 65001");
#endif
    global.pointerpool=malloc(MAXPOINTERS*sizeof(void *));
    for(i32 i=0; i<MAXPOINTERS; i++)global.pointerpool[i]=NULL;
    printr_dest=mallocpointer(4096*sizeof(char));
    memset(printr_dest, 0, 4096);
    printr_token=mallocpointer(TOKEN_MAXLEN);
    memset(printr_token, 0, TOKEN_MAXLEN);
    char *input_orig=mallocpointer(4096*sizeof(char));
    FILE *dic_file=NULL;
    initEntries();
    loadDictionary(dic_file);
    cbc_clearscreen();
    cbc_setcolor(Default|Bright);
    printf("    DECYPH\n");
    cbc_setcolor(Cyan|Bright);
    printf("        \"The gostak distims the doshes\"\n");
    cbc_setcolor(Default);
    while(1){
        cbc_setcolor(Yellow);
        fflush(stdout);
        printf("> ");
        memset(input, 0, 4096);
        fgets(input, 4000, stdin);
        while(input[strlen(input)-1]=='\n')input[strlen(input)-1]=0;
        memset(input_orig, 0, 4096);
        strcpy(input_orig, input);
        strlower(input);
        cbc_setcolor(Default);
        if(fullmatch(input, "q") || fullmatch(input, "quit") || fullmatch(input, "exit")){
            break;
        }
        else if(fullmatch(input, "h") || fullmatch(input, "help")){
            pos=0;
            printr(Default, "Help\n"
            "    Notes: Decyph splits words with space.\n"
            "$ccls$o         - clear the screen\n"
            "$c[c]ount$o     - count current entries\n"
            "$c[h]elp$o      - show commands list\n"
            "$c[s]ave$o      - save current entries\n"
            "$c[d]etermine$o - determine a $c<word>$o's $c<meaning>$o\n"
            "$c[g]uess$o     - guess a $c<word>$o's $c<meaning>$o\n"
            "$c[l]ink$o      - link two existing $c<word>$os\n"
            "$c[unl]ink$o    - unlink two existing $c<word>$os\n"
            "$c[p]ush$o      - push $c<words>$o into dictionary entries\n"
//            "$c[n]ote$o      - write down about a $c<word>$o its $c<notes>$o\n"
            "$c[m]orph$o     - add to a $c<word>$o its $c<variants>$o\n"
            "$c[unm]orph$o   - remove all variants from a $c<word>$o\n"
            "$c[r]emove$o    - remove $c<words>$o' entry\n"
            "$c[tr]anslate$o - translate a $c<sentence>$o\n"
            "$c<word>$o      - shows information about the $c<word>$o\n"
            "$c<line>$o      - attempt to translate the $c<line>$o\n");
        }
        else if(fullmatch(input, "cls") || fullmatch(input, "clear")){
            cbc_clearscreen();
        }
        else if(fullmatch(input, "c") || fullmatch(input, "count")){
            printf("Pointers in use: %d\n", global.pointerinuse);
            printf("Current entries: %d\n", entries_count);
            if(entries_count<35){
                for(int i=0; i<entries_count; i++){
                    printf("%-4d %12s - ", entries[i].code, entries[i].orig);
                    printTranslation(&entries[i], 0);
                    cbc_setcolor(Default);
                    if(entries[i].links_count){
                        printf(" Linked: %s", entries[entries[i].links[0]].orig);
                        for(int j=1; j<entries[i].links_count; j++){
                            printf(", %s", entries[entries[i].links[j]].orig);
                        }
                    }
                    printf("\n");
                }
            }
            else{
                printf("%s (", entries[0].orig);
                printTranslation(&entries[0], 0);
                cbc_setcolor(Default);
                printf(")");
                for(int i=1; i<entries_count; i++){
                    printf(",     %s (", entries[i].orig);
                    printTranslation(&entries[i], 0);
                    cbc_setcolor(Default);
                    printf(")");
                }
                printf("\n");
            }
        }
        else if(fullmatch(input, "s") || fullmatch(input, "save")){
            saveDictionary(dic_file);
            printf("Saved all %d entries.\n", entries_count);
        }
        else if(fullmatch(input, "r") || fullmatch(input, "remove")){
            char orig[MAXORIGLEN];
            memset(orig, 0, MAXORIGLEN);
            int offset=0, doffset=0;
            sscanf(input, "%*s%n", &offset);
            while(sscanf(input+offset, SPEC_ORIG "%n", orig, &doffset)==1){
                offset+=doffset;
                entry *ent=findEntry(orig);
                if(ent==NULL){
                    printf("Entry ");
                    cbc_setcolor(Green|Bright);
                    printf("%s", orig);
                    cbc_setcolor(Default);
                    printf(" not found.\n");
                    continue;
                }
                if(strcmp(ent->orig, orig)){
                    printf("Cannot delete an entry from a morph.\n");
                    continue;
                }
                entries_count--;
                entries[entries_count].code=ent->code;
                destroyEntry(ent);
                if(entries_count){
                    copyEntry(ent, &entries[entries_count]);
                    // here, change all the links.
                    changeEntryLinks(ent->code, -1);
                    changeEntryLinks(entries[entries_count].code, ent->code);
                }
                printf("Entry ");
                cbc_setcolor(Green|Bright);
                printf("%s", orig);
                cbc_setcolor(Default);
                printf(" deleted.\n");
            }
            if(!doffset){
                printf("Please enter a valid word.\n");
                continue;
            }
        }
        else if(fullmatch(input, "p") || fullmatch(input, "push")){
            entry tmpent=newEntry();
            int offset=0, doffset=0;
            sscanf(input, "%*s%n", &offset);
            while(sscanf(input+offset, SPEC_ORIG "%n", tmpent.orig, &doffset)==1){
                offset+=doffset;
                if(findEntry(tmpent.orig)){
                    printf("Word already exists: ");
                    cbc_setcolor(Green|Bright);
                    printf("%s\n", tmpent.orig);
                    cbc_setcolor(Default);
                    destroyEntry(&tmpent);
                    tmpent=newEntry();
                    continue;
                }
                parseNewEntry(&tmpent);
                pushEntry(tmpent);
                printf("Word added: ");
                cbc_setcolor(Green|Bright);
                printf("%s", tmpent.orig);
                cbc_setcolor(Default);
                printf(".\n");
                tmpent=newEntry();
            }
            if(!doffset){
                printf("Please enter a valid word.\n");
                continue;
            }
            destroyEntry(&tmpent);
        }
        else if(fullmatch(input, "g") || fullmatch(input, "guess")){
            char orig[MAXORIGLEN];
            memset(orig, 0, MAXORIGLEN);
            sscanf(input, "%*s " SPEC_ORIG, orig);
            entry *ent=findEntry(orig);
            if(ent==NULL){
                printf("Word is not found.\n");
                continue;
            }
            if(ent->cert<1000){
                sscanf(input, "%*s %*s " SPEC_TRAN, ent->tran);
                printf("You make a guess: %s = %s\n", orig, ent->tran);
                ent->cert=0;
            }else{
                printf("Word is already determined.\n");
                continue;
            }
        }
        else if(fullmatch(input, "d") || fullmatch(input, "determine")){
            char orig[MAXORIGLEN];
            memset(orig, 0, MAXORIGLEN);
            sscanf(input, "%*s " SPEC_ORIG, orig);
            entry *ent=findEntry(orig);
            if(ent==NULL){
                printf("Word is not found.\n");
                continue;
            }
            if(ent->cert<1000){
                sscanf(input, "%*s %*s " SPEC_TRAN, ent->tran);
                printf("The word is determined: %s = ", orig);
                cbc_setcolor(Green|Bright);
                printf("%s\n", ent->tran);
                cbc_setcolor(Default);
                ent->cert=1000;
            }else{
                printf("You redetermine the word: %s = %s -> ", orig, ent->tran);
                sscanf(input, "%*s %*s " SPEC_TRAN, ent->tran);
                cbc_setcolor(Green|Bright);
                printf("%s\n", ent->tran);
                cbc_setcolor(Default);
                ent->cert=1000;
            }
        }
        else if(fullmatch(input, "l") || fullmatch(input, "link")){
            char orig[MAXORIGLEN];
            memset(orig, 0, MAXORIGLEN);
            sscanf(input, "%*s " SPEC_ORIG, orig);
            entry *ent=findEntry(orig);
            if(ent==NULL){
                printf("Word \"%s\" is not found.\n", orig);
                continue;
            }
            memset(orig, 0, MAXORIGLEN);
            sscanf(input, "%*s %*s " SPEC_ORIG, orig);
            entry *ent2=findEntry(orig);
            if(ent2==NULL){
                printf("Word \"%s\" is not found.\n", orig);
                continue;
            }
            linkEntry(ent, ent2);
        }
        else if(fullmatch(input, "unl") || fullmatch(input, "unlink")){
            char orig[MAXORIGLEN];
            memset(orig, 0, MAXORIGLEN);
            sscanf(input, "%*s " SPEC_ORIG, orig);
            entry *ent=findEntry(orig);
            if(ent==NULL){
                printf("Word \"%s\" is not found.\n", orig);
                continue;
            }
            memset(orig, 0, MAXORIGLEN);
            sscanf(input, "%*s %*s " SPEC_ORIG, orig);
            entry *ent2=findEntry(orig);
            if(ent2==NULL){
                printf("Word \"%s\" is not found.\n", orig);
                continue;
            }
            unlinkEntry(ent, ent2);
        }
        else if(fullmatch(input, "m") || fullmatch(input, "morph")){
            char orig[MAXORIGLEN];
            memset(orig, 0, MAXORIGLEN);
            sscanf(input, "%*s " SPEC_ORIG, orig);
            entry *ent=findEntry(orig);
            if(ent==NULL){
                printf("Word is not found.\n");
                continue;
            }
            if(ent->morphs_count<MAXMORPHS){
                memset(orig, 0, MAXORIGLEN);
                int offset=0, doffset=0;
                sscanf(input, "%*s %*s%n", &offset);
                while(sscanf(input+offset, SPEC_ORIG "%n", orig, &doffset)==1){
                    if(ent->morphs_count>=MAXMORPHS){
                        printf("Word morphs has reached the limit.\n");
                        break;
                    }
                    offset+=doffset;
                    if(findEntry(orig)){
                        printf("Morph ");
                        cbc_setcolor(Green|Bright);
                        printf("%s", orig);
                        cbc_setcolor(Default);
                        printf(" already exists.\n");
                        continue;
                    }
                    ent->morphs[ent->morphs_count]=mallocpointer(MAXORIGLEN);
                    memset(ent->morphs[ent->morphs_count], 0, MAXORIGLEN);
                    strcpy(ent->morphs[ent->morphs_count], orig);
                    printf("You create a morph of the word: %s = %s\n", ent->orig, ent->morphs[ent->morphs_count]);
                    ent->morphs_count++;
                }
                if(!doffset){
                    printf("Invalid morph.\n");
                }
            }else{
                printf("Word morphs has reached the limit.\n");
            }
        }
//        else if(fullmatch(input, "n") || fullmatch(input, "note")){
//
//        }
        else if(fullmatch(input, "unm") || fullmatch(input, "unmorph")){
            char orig[MAXORIGLEN];
            memset(orig, 0, MAXORIGLEN);
            sscanf(input, "%*s " SPEC_ORIG, orig);
            entry *ent=findEntry(orig);
            if(ent==NULL){
                printf("Word is not found.\n");
                continue;
            }
            if(ent->morphs_count){
                printf("Removed morphs:");
                unmorphEntry(ent);
                printf("\nAll of the morphs of \"%s\" are cleared.\n", ent->orig);
            }else{
                printf("Word has no morphs.\n");
            }
        }
        else if(fullmatch(input, "tr") || fullmatch(input, "translate")){
            int offset=0, doffset=0, words=0, prevcert=0;
            entry *ent=NULL;
            char token[MAXORIGLEN];
            memset(token, 0, MAXORIGLEN);
            sscanf(input, "%*s%n", &offset);
            for(; sscanf(input+offset, "%s%n", token, &doffset)==1; words++){
                offset+=doffset;
                ent=findEntry(token);
                if(ent){
                    if(words==0){
                        prevcert=ent->cert;
                    }
                    printTranslation(ent, 1);
                    size_t len=strlen(token);
                    while(isMark(token[len-1])){
                        len--;
                        if(len<=1)break;
                    }
                    cbc_setcolor(Default);
                    printf("%s", token+len);
                    printf(" ");
                }else{
                    cbc_setcolor(Default);
                    printf("%s ", token);
                }
            }
            if(ent && words==1){
                ent->cert=prevcert;
                showEntryInfo(ent);
            }
            if(!doffset){
                printf("Please enter a valid sentence.");
            }
            printf("\n");
            cbc_setcolor(Default);
        }
        else{
            int offset=0, doffset=0, words=0, prevcert=0;
            entry *ent=NULL;
            char token[MAXORIGLEN];
            memset(token, 0, MAXORIGLEN);
            for(; sscanf(input+offset, "%s%n", token, &doffset)==1; words++){
                offset+=doffset;
                ent=findEntry(token);
                if(ent){
                    if(words==0){
                        prevcert=ent->cert;
                    }
                    printTranslation(ent, 1);
                    size_t len=strlen(token);
                    while(isMark(token[len-1])){
                        len--;
                        if(len<=1)break;
                    }
                    cbc_setcolor(Default);
                    printf("%s", token+len);
                    printf(" ");
                }else{
                    cbc_setcolor(Default);
                    printf("%s ", token);
                }
            }
            if(!doffset){
                printf("Please enter a valid sentence.");
            }
            if(ent && words==1){
                ent->cert=prevcert;
                showEntryInfo(ent);
            }
            printf("\n");
            cbc_setcolor(Default);
        }
    }
    saveDictionary(dic_file);
    freeall();
}
