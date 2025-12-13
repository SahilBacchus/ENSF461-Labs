#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <dmtx.h>
#include <wand/magick-wand.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

#define TRUE 1
#define FALSE 0
#define MAX_FNAME_LENGTH 256
#define MAX_MESSAGE_LENGTH 8000
#define SEM_NAME "/dmtx_sem"

typedef struct {
    char filename[MAX_FNAME_LENGTH];
    char message[MAX_MESSAGE_LENGTH];
} filedata;

typedef struct {
    int next_index;
} shared_info;

shared_info* info = NULL;
filedata* filelist = NULL;
int sharedfd;
char* sharedmem = NULL;
int numfiles = 0;
char folderpath[MAX_FNAME_LENGTH];

int initialized = 0;

void initdmtx() {
    if (!initialized) {
        MagickWandGenesis();
        initialized = 1;
    }
}

void closedmtx() {
    if (initialized) {
        MagickWandTerminus();
        initialized = 0;
    }
}

char* scandmtx(char* filepath) {
    int width, height;
    unsigned char* pxl = NULL;
    DmtxImage* img = NULL;
    DmtxDecode* dec = NULL;
    DmtxRegion* reg = NULL;
    DmtxMessage* msg = NULL;
    char* result = NULL;
    MagickWand* wand;

    if (!initialized) initdmtx();

    wand = NewMagickWand();
    if (MagickReadImage(wand, filepath) == MagickFalse) {
        DestroyMagickWand(wand);
        return NULL;
    }

    width = MagickGetImageWidth(wand);
    height = MagickGetImageHeight(wand);

    pxl = malloc(3 * width * height);
    if (!pxl) { DestroyMagickWand(wand); return NULL; }

    if (MagickExportImagePixels(wand, 0, 0, width, height, "RGB", CharPixel, pxl) == MagickFalse) {
        free(pxl); DestroyMagickWand(wand); return NULL;
    }

    img = dmtxImageCreate(pxl, width, height, DmtxPack24bppRGB);
    if (!img) { free(pxl); DestroyMagickWand(wand); return NULL; }

    dec = dmtxDecodeCreate(img, 1);
    if (!dec) { dmtxImageDestroy(&img); free(pxl); DestroyMagickWand(wand); return NULL; }

    reg = dmtxRegionFindNext(dec, NULL);
    if (!reg) { dmtxDecodeDestroy(&dec); dmtxImageDestroy(&img); free(pxl); DestroyMagickWand(wand); return NULL; }

    msg = dmtxDecodeMatrixRegion(dec, reg, 0);
    if (!msg) { dmtxRegionDestroy(&reg); dmtxDecodeDestroy(&dec); dmtxImageDestroy(&img); free(pxl); DestroyMagickWand(wand); return NULL; }

    result = malloc(msg->outputIdx + 1);
    memcpy(result, msg->output, msg->outputIdx);
    result[msg->outputIdx] = '\0';

    dmtxMessageDestroy(&msg);
    dmtxRegionDestroy(&reg);
    dmtxDecodeDestroy(&dec);
    dmtxImageDestroy(&img);
    free(pxl);
    DestroyMagickWand(wand);

    return result;
}

int generate_file_list(char* path, int include_info) {
    DIR* dir;
    struct dirent* ent;
    numfiles = 0;

    if ((dir = opendir(path)) == NULL) { perror("opendir"); return FALSE; }
    while ((ent = readdir(dir)) != NULL) {
        if (strstr(ent->d_name, ".png")) numfiles++;
    }
    closedir(dir);

    int total_size = sizeof(filedata) * numfiles + (include_info ? sizeof(shared_info) : 0);

    shm_unlink("filelist");
    sharedfd = shm_open("filelist", O_CREAT | O_RDWR, 0666);
    if (sharedfd < 0) { perror("shm_open"); return FALSE; }
    if (ftruncate(sharedfd, total_size) < 0) { perror("ftruncate"); return FALSE; }

    sharedmem = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, sharedfd, 0);
    if (sharedmem == MAP_FAILED) { perror("mmap"); return FALSE; }

    filelist = include_info ? (filedata*)(sharedmem + sizeof(shared_info)) : (filedata*)sharedmem;

    if ((dir = opendir(path)) == NULL) { perror("opendir"); return FALSE; }
    int idx = 0;
    while ((ent = readdir(dir)) != NULL) {
        if (strstr(ent->d_name, ".png")) {
            strncpy(filelist[idx].filename, ent->d_name, MAX_FNAME_LENGTH);
            filelist[idx].message[0] = '\0';
            idx++;
        }
    }
    closedir(dir);
    return TRUE;
}

void generate_dmtx_seq() {
    for (int i = 0; i < numfiles; i++) {
        char fullpath[512];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", folderpath, filelist[i].filename);
        char* msg = scandmtx(fullpath);
        if (msg) { strncpy(filelist[i].message, msg, MAX_MESSAGE_LENGTH); free(msg); }
        else snprintf(filelist[i].message, MAX_MESSAGE_LENGTH, "[ERROR decoding %s]\n", filelist[i].filename);
    }
    closedmtx();
}

void generate_dmtx_par(int numprocesses) {
    info = (shared_info*)sharedmem;
    info->next_index = 0;

    sem_t* sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) { perror("sem_open"); exit(1); }

    for (int p = 0; p < numprocesses; p++) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); exit(1); }
        if (pid == 0) {
            sem_t* child_sem = sem_open(SEM_NAME, 0);
            if (child_sem == SEM_FAILED) { perror("sem_open child"); exit(1); }
            while (1) {
                sem_wait(child_sem);
                int idx = info->next_index++;
                sem_post(child_sem);
                if (idx >= numfiles) break;

                char fullpath[512];
                snprintf(fullpath, sizeof(fullpath), "%s/%s", folderpath, filelist[idx].filename);
                char* msg = scandmtx(fullpath);
                if (msg) { strncpy(filelist[idx].message, msg, MAX_MESSAGE_LENGTH); free(msg); }
                else snprintf(filelist[idx].message, MAX_MESSAGE_LENGTH, "[ERROR decoding %s]\n", filelist[idx].filename);
            }
            sem_close(child_sem);
            closedmtx();
            exit(0);
        }
    }

    for (int p = 0; p < numprocesses; p++) wait(NULL);
    sem_close(sem);
    sem_unlink(SEM_NAME);
    closedmtx();
}

int main(int argc, char** argv) {
    if (argc != 4) { printf("Usage: %s <#processes> <folder> <output>\n", argv[0]); return 1; }
    int numprocesses = atoi(argv[1]);
    strncpy(folderpath, argv[2], MAX_FNAME_LENGTH);

    if (numprocesses == 0) generate_file_list(folderpath, 0);
    else generate_file_list(folderpath, 1);

    if (numprocesses == 0) generate_dmtx_seq();
    else generate_dmtx_par(numprocesses);

    FILE* fp = fopen(argv[3], "w");
    if (!fp) { perror("fopen"); return -1; }
    for (int i = 0; i < numfiles; i++) fprintf(fp, "%s", filelist[i].message);
    fclose(fp);

    int total_size = sizeof(filedata) * numfiles + (numprocesses > 0 ? sizeof(shared_info) : 0);
    munmap(sharedmem, total_size);
    shm_unlink("filelist");

    return 0;
}
