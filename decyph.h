#ifndef GOSTAK
#define GOSTAK
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cbcurses.h"
#define MAXPOINTERS 32767
#define MAXLINKS 8
#define MAXMORPHS 8
#define MAXORIGLEN 32
#define MAXTRANLEN 20
#define SPEC_ORIG "%31s"
#define SPEC_TRAN "%19s"
#define DISPLAY_WIDTH 1024
#define TOKEN_MAXLEN 64

struct globaldata{
    void **pointerpool;
    int pointerinuse;
    int dbpointer;
    int pointerreserve;
} global={NULL, 0, 0, 32000};
int fullmatch(char *str, const char *str2){
    if(str==NULL && str2==NULL)return 1;
    if(str==NULL || str2==NULL)return 0;
    if(strlen(str)<strlen(str2))return 0;
    if(str[0]!=str2[0])return 0;
    if(strlen(str)>strlen(str2)){
        if(str[strlen(str2)]!=' ' && str[strlen(str2)]!='\n')return 0;
        for(size_t i=0; i<strlen(str2); i++){
            if(str[i]!=str2[i])return 0;
        }
        return 1;
    }
    else{
        return strcmp(str, str2)==0;
    }
}
void *mallocpointer(size_t bytes){
    if(global.pointerinuse>MAXPOINTERS || global.pointerinuse>global.dbpointer+global.pointerreserve){
        return NULL;
    }
    void *pointer=NULL;
    pointer=malloc(bytes);
    if(pointer==NULL)return NULL;
    global.pointerinuse++;
    for(int i=0; i<MAXPOINTERS; i++){
        if(global.pointerpool[i]==NULL){
            global.pointerpool[i]=pointer;
            break;
        }
    }
    return pointer;
}
void *reallocpointer(void *pointer, size_t bytes){
    int i=0, found=0;
    for(; i<MAXPOINTERS; i++){
        if(global.pointerpool[i]==pointer){
            found=1;
            break;
        }
        if(i==MAXPOINTERS-1 && found==0){
            return NULL;
        }
    }
    pointer=realloc(pointer, bytes);
    global.pointerpool[i]=pointer;
    if(pointer==NULL)return NULL;
    return pointer;
}
void freepointer(void *pointer){
    if(pointer==NULL)return;
    for(int i=0; i<MAXPOINTERS; i++){
        if(global.pointerpool[i]==pointer){
            global.pointerpool[i]=NULL;
            break;
        }
        if(i==MAXPOINTERS-1){
            return;
        }
    }
    free(pointer);
    global.pointerinuse--;
    pointer=NULL;
}
void freeall(){
    for(int i=0; i<MAXPOINTERS && global.pointerinuse>0; i++){
        if(global.pointerpool[i]!=NULL){
            free(global.pointerpool[i]);
            global.pointerinuse--;
        }
    }
}

struct fio{
    size_t workingdirmaxlen;
    char *workingdir;
} fio={0, NULL};
void fio_init(const char *execpath){
    i32 execpathlen=strlen(execpath);
    fio.workingdir=malloc(execpathlen+1);
    fio.workingdirmaxlen=execpathlen+32;
    memset(fio.workingdir, 0, execpathlen+1);
    strcpy(fio.workingdir,execpath);
    for(i32 i=execpathlen-1; i>=0; i--){
        if(fio.workingdir[i]=='\\' || fio.workingdir[i]=='/'){
            break;
        }else{
            fio.workingdir[i]=0;
        }
    }
}
FILE *fopenrelative(const char *relative_path, const char *restrict_mode){
    char *tmppath=malloc(fio.workingdirmaxlen);
    memset(tmppath, 0, fio.workingdirmaxlen);
    strcpy(tmppath, fio.workingdir);
    strcat(tmppath, relative_path);
    FILE *fp=fopen(tmppath, restrict_mode);
    free(tmppath);
    return fp;
}

char *printr_dest=NULL;
char *printr_token=NULL;
size_t pos=0;
void printc(i32 color, const char *format, ...){
    cbc_setcolor(color);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    fflush(stdout);
}
void charstate(char ch, i32 *state, i32 *arg, i32 color, i32 *tint, size_t *width){
    // states
    // 0  - regular character
    // 1  - valid indicator '$'
    // 2  - color indicator
    if(*state==2)*state=0;
    if(ch=='\n'){
        *state=0;
        return;
    }
    *width=*width+1;
    if(ch==' '){
        *state=0;
        return;
    }
    if(ch=='$' && *state==0){
        *state=1;
    }
    else if(*state==1){
        if((ch<='Z' && ch>='A') || (ch<='z' && ch>='a') || ch=='@'){
            *state=2;
            switch(ch){
            case 'o': // $o original color
                *tint=color;
                break;
            case 'd': // $d, $Gd debug
                *tint=Blue;
                break;
            case 'w': // $w, $Gw warning
                *tint=Red;
                break;
            case 'c': // $c, $Gc command
                *tint=Yellow|Bright;
                break;
            }
            *width=*width-2;
        }else{
            *state=0;
        }
    }else{
        *state=0;
    }
}
size_t strwidth(const char *str){
    size_t len=0;
    i32 state=0, tint=0, arg=0;
    for(size_t i=0; i<strlen(str); i++){
        charstate(str[i], &state, &arg, 0, &tint, &len);
    }
    return len;
}
void printword(const char *word){
    size_t wordlen=strlen(word);
    if(wordlen==0)return;
    size_t strw=strwidth(word);
    if(strw+pos<DISPLAY_WIDTH){
        printf("%s", word);
        pos+=strw;
        if(word[wordlen-1]=='\n')pos=0;
    }else if(strw<DISPLAY_WIDTH){
        printf("\n%s", word);
        pos=strw;
        if(word[wordlen-1]=='\n')pos=0;
    }else{
        char printword_token[TOKEN_MAXLEN];
        memset(printword_token, 0, TOKEN_MAXLEN);
        i32 len=0;
        for(size_t i=0; i<strlen(word); i++){
            printword_token[len]=word[i];
            len++;
            if(len+pos>=DISPLAY_WIDTH){
                printf("%s\n", printword_token);
                memset(printword_token, 0, TOKEN_MAXLEN);
                len=0;
            }
        }
        printf("%s", printword_token);
        pos=len;
        if(word[wordlen-1]=='\n')pos=0;
    }
}
void printr(i32 color, const char *format, ...){
    cbc_setcolor(color);
    memset(printr_dest, 0, 4096);
    memset(printr_token, 0, TOKEN_MAXLEN);
    va_list args;
    va_start(args, format);
    vsnprintf(printr_dest, 4096, format, args);
    va_end(args);

    size_t strl=0, strw=0, strld=strlen(printr_dest);
    i32 state=0, tint=cbc_getcolor(), arg=0;
    for(size_t i=0; i<strld; i++){
        char ch=printr_dest[i];
        charstate(ch, &state, &arg, color, &tint, &strw);
        printr_token[strl]=ch;
        strl++;
        if(state==0){
            if(ch==' ' || ch=='\n'){
                if(printr_dest[0]!=ch){
                    printword(printr_token);
                    memset(printr_token, 0, TOKEN_MAXLEN);
                    strl=0;
                }else if(i<strld-1){
                    char ch_1=printr_dest[i+1];
//                    if(ch_1!=ch || printr_dest[0]!=ch){
                    if(ch_1!=ch){
                        printword(printr_token);
                        memset(printr_token, 0, TOKEN_MAXLEN);
                        strl=0;
                    }
                }
            }
        }
        else if(state==2){
            if(strl>2){
                printr_token[strl-1]=0;
                printr_token[strl-2]=0;
                printword(printr_token);
            }
            memset(printr_token, 0, TOKEN_MAXLEN);
            strl=0;
            cbc_setcolor(tint);
        }
        else if(state==4){
            if(strl>3){
                printr_token[strl-1]=0;
                printr_token[strl-2]=0;
                printr_token[strl-3]=0;
                printword(printr_token);
            }
            memset(printr_token, 0, TOKEN_MAXLEN);
            strl=0;
            cbc_setcolor(tint);
        }
    }
    printword(printr_token);
    memset(printr_token, 0, TOKEN_MAXLEN);
    fflush(stdout);
}
void strlower(char *target_str_pos){
    for(size_t i=0; i<strlen(target_str_pos); i++){
        if(target_str_pos[i]<=90 && (target_str_pos)[i]>=65){
            target_str_pos[i]+=32;
        }
    }
}

typedef struct entry entry;
struct entry {
    char *orig;
    char *tran;
    size_t len;
    size_t tranlen;
    int code;
    int cert;
    int links_count;
    int links[MAXLINKS];
    int morphs_count;
    char *morphs[MAXMORPHS];
};
entry *entries=NULL;
int entries_count=0;
int entries_size=0;

int isMark(char c);
entry newEntry(void);
entry *findEntryWithTransl(const char *tran);
entry *findEntry(const char *orig);
void parseNewEntry(entry *ent);
void loadDictionary(FILE *fp);
void saveDictionary(FILE *fp);
void initEntries(void);
void pushEntry(entry ent);
void printTranslation(entry *ent, int certainize);
void copyEntry(entry *ent, entry *ent2);
void linkEntry(entry *ent, entry *ent2);
void unlinkEntry(entry *ent, entry *ent2);
void changeEntryLinks(int oldcode, int newcode);
void unmorphEntry(entry *ent);
void showEntryInfo(entry *ent);

int isMark(char c){
    return ispunct(c);
    switch(c){
        case '.': case ',': case '?': case '!': case '\n':
        case '"': case '\'': case ':': case ';': case '/': return 1;
    }
    return 0;
}
entry newEntry(void){
    entry entry_new={0};
    entry_new.orig=mallocpointer(MAXORIGLEN*sizeof(char));
    entry_new.tran=mallocpointer(MAXTRANLEN*sizeof(char));
    memset(entry_new.orig, 0, MAXORIGLEN);
    memset(entry_new.tran, 0, MAXTRANLEN);
    strcpy(entry_new.tran, "...");
    entry_new.len=0;
    entry_new.cert=0;
    entry_new.code=entries_count;
    entry_new.links_count=0;
    entry_new.links[0]=-1;
    entry_new.morphs_count=0;
    entry_new.morphs[0]=NULL;
    return entry_new;
}
entry *findEntryWithTransl(const char *tran){
    size_t len=strlen(tran);
    if(len)while(isMark(tran[len-1])){
        len--;
        if(len<=1)break;
    }
    entry *ent=NULL;
    if(len>=MAXTRANLEN || !len)return ent;
    for(int i=0; i<entries_count; i++){
        size_t len2=entries[i].tranlen;
        if(!strncmp(entries[i].tran, tran, len2>len?len2:len)){
            ent=&entries[i];
            break;
        }
    }
    return ent;
}
entry *findEntry(const char *orig){
    size_t len=strlen(orig);
    if(len)while(isMark(orig[len-1])){
        len--;
        if(len<=1)break;
    }
    entry *ent=NULL;
    if(len>=MAXORIGLEN || !len)return ent;
    for(int i=0; i<entries_count; i++){
        if(entries[i].len==len){
            int found=1;
            for(size_t j=0; j<len; j++){
                if(entries[i].orig[j]!=orig[j]){
                    found=0;
                    break;
                }
            }
            if(found){
                ent=&entries[i];
                break;
            }
        }
        if(entries[i].morphs_count){
            int found=0;
            for(int j=0; j<entries[i].morphs_count; j++){
                int same=1;
                size_t len2=strlen(entries[i].morphs[j]);
                if(len2!=len)continue;
                for(size_t k=0; k<len; k++){
                    if(entries[i].morphs[j][k]!=orig[k]){
                        same=0;
                        break;
                    }
                }
                if(same){
                    found=1;
                    break;
                }
            }
            if(found){
                ent=&entries[i];
                break;
            }
        }
    }
    return ent;
}
void parseNewEntry(entry *ent){
    ent->len=strlen(ent->orig);
    ent->code=entries_count;
}
void loadDictionary(FILE *fp){
    fp=fopenrelative("dic.txt", "r");
    if(fp){
        while(1){
            entry tmpent=newEntry();
            fscanf(fp, "%s %s %d %d %d", tmpent.orig, tmpent.tran, &tmpent.cert, &tmpent.links_count, &tmpent.morphs_count);
            if(tmpent.links_count){
                for(int i=0; i<tmpent.links_count; i++){
                    fscanf(fp, "%d", &tmpent.links[i]);
                }
            }
            if(tmpent.morphs_count){
                for(int i=0; i<tmpent.morphs_count; i++){
                    tmpent.morphs[i]=mallocpointer(MAXORIGLEN*sizeof(char));
                    fscanf(fp, "%s", tmpent.morphs[i]);
                }
            }
            parseNewEntry(&tmpent);
            if(!tmpent.len)break;
            pushEntry(tmpent);
        }
        fclose(fp);
    }
}
void saveDictionary(FILE *fp){
    fp=fopenrelative("dic.txt", "w");
    for(int i=0; i<entries_count; i++){
        fprintf(fp, "%s %s %d %d %d", entries[i].orig, entries[i].tran, entries[i].cert, entries[i].links_count, entries[i].morphs_count);
        for(int j=0; j<entries[i].links_count; j++){
            fprintf(fp, " %d", entries[i].links[j]);
        }
        for(int j=0; j<entries[i].morphs_count; j++){
            if(entries[i].morphs[j]){
                fprintf(fp, " %s", entries[i].morphs[j]);
            }
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
}
void initEntries(void){
    global.pointerpool=malloc(MAXPOINTERS*sizeof(void *));
    for(int i=0; i<MAXPOINTERS; i++)global.pointerpool[i]=NULL;
    entries=mallocpointer(100*sizeof(entry));
    entries_size=100;
}
void pushEntry(entry ent){
    if(entries_count>=entries_size){
        entries_size+=50+entries_size/4;
        entries=reallocpointer(entries, entries_size*sizeof(entry));
    }
    copyEntry(&entries[entries_count], &ent);
    for(int i=ent.links_count; i<MAXLINKS; i++){
        entries[entries_count].links[i]=-1;
    }
    entries_count++;
}
void printTranslation(entry *ent, int certainize){
    cbc_setcolor(Default);
    if(!ent->tran){
        printf("... ");
        return;
    }
    if(!ent->tran[0]){
        printf("... ");
        return;
    }
    if(ent->cert>=1000)cbc_setcolor(Green|Bright);
    else if(ent->cert>300)cbc_setcolor(Cyan|Bright);
    else if(ent->cert>50)cbc_setcolor(Yellow|Bright);
    else cbc_setcolor(Red);
    printf("%s", ent->tran);
    if(certainize){
        if(ent->cert<1000){
            ent->cert+=10;
            if(ent->cert>=1000)ent->cert=999;
        }
    }
}
void copyEntry(entry *dest, entry *src){
    dest->orig=src->orig;
    dest->tran=src->tran;
    dest->len=src->len;
    dest->tranlen=strlen(src->tran);
    dest->code=src->code;
    dest->cert=src->cert;
    dest->links_count=src->links_count;
    for(int i=0; i<src->links_count; i++){
        dest->links[i]=src->links[i];
    }
    dest->morphs_count=src->morphs_count;
    for(int i=0; i<src->morphs_count; i++){
        dest->morphs[i]=src->morphs[i];
    }
}
void linkEntry(entry *ent, entry *ent2){
    int found=-1, found2=-1;
    for(int i=0; i<MAXLINKS; i++){
        if(ent->links[i]==-1){found=i; break;}
        else if(ent->links[i]==ent2->code){found=-2; break;}
    }
    if(found==-1){
        printf("No avaiable linking slot for \"%s\".\n", ent->orig);
        return;
    }
    for(int i=0; i<MAXLINKS; i++){
        if(ent2->links[i]==-1){found2=i; break;}
        else if(ent2->links[i]==ent->code){found2=-2; break;}
    }
    if(found2==-1){
        printf("No avaiable linking slot for \"%s\".\n", ent2->orig);
        return;
    }
    if(found>=0){
        ent->links[found]=ent2->code;
        ent->links_count+=1;
    }
    if(found2>=0){
        ent2->links[found2]=ent->code;
        ent2->links_count+=1;
    }
    if(found==-2 && found2==-2){
        printf("Linkage between \"%s\" and \"%s\" already exists.\n", ent->orig, ent2->orig);
    }else{
        printf("Linkage between \"%s\" and \"%s\" is created.\n", ent->orig, ent2->orig);
    }
}
void unlinkEntry(entry *ent, entry *ent2){
    int found=-1, found2=-1;
    for(int i=0; i<ent->links_count; i++){
        if(ent->links[i]==ent2->code){found=i; break;}
    }
    for(int i=0; i<ent2->links_count; i++){
        if(ent2->links[i]==ent->code){found2=i; break;}
    }
    if(found>=0){
        ent->links_count-=1;
        ent->links[found]=ent->links[ent->links_count];
        ent->links[ent->links_count]=-1;
    }
    if(found2>=0){
        ent2->links_count-=1;
        ent2->links[found]=ent2->links[ent2->links_count];
        ent2->links[ent2->links_count]=-1;
    }
    if(found==-1 && found2==-1){
        printf("Linkage between \"%s\" and \"%s\" does not exist.\n", ent->orig, ent2->orig);
    }else{
        printf("Linkage between \"%s\" and \"%s\" is removed.\n", ent->orig, ent2->orig);
    }
}
void changeEntryLinks(int oldcode, int newcode){
    for(int i=0; i<entries_count; i++){
        for(int j=0; j<MAXLINKS; j++){
            if(entries[i].links[j]<0)break;
            if(entries[i].links[j]==oldcode){
                if(newcode==-1){
                    entries[i].links_count--;
                    entries[i].links[j]=entries[i].links[entries[i].links_count];
                    entries[i].links[entries[i].links_count]=-1;
                }else entries[i].links[j]=newcode;
                break;
            }
        }
    }
}
void unmorphEntry(entry *ent){
    for(int i=0; i<ent->morphs_count; i++){
        printf(" %s", ent->morphs[i]);
        if(i<ent->morphs_count-1)printf(",");
        freepointer(ent->morphs[i]);
    }
    ent->morphs[0]=NULL;
    ent->morphs_count=0;
}
void showEntryInfo(entry *ent){
    if(!ent)return;
    cbc_setcolor(Default);
    printf("\nOriginal Form: %s\nTranslation: %s", ent->orig, ent->tran);
    if(ent->morphs_count){
        cbc_setcolor(Default);
        printf("\n  Morphs: %s", ent->morphs[0]);
        for(int i=1; i<ent->morphs_count; i++){
            printf(", %s", ent->morphs[i]);
        }
    }
    if(ent->links_count){
        cbc_setcolor(Default);
        printf("\n  Linked words: %s", entries[ent->links[0]].orig);
        for(int i=1; i<ent->links_count; i++){
            printf(", %s", entries[ent->links[i]].orig);
        }
    }
}
#endif
