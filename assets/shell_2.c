#include "shell.h"
#include "kernel.h"
#include "std_lib.h"
#include "filesystem.h"

void shell() {
  char buf[64];
  char cmd[64];
  char arg[2][64];

  byte cwd = FS_NODE_P_ROOT;

  while (true) {
    printString("MengOS:");
    printCWD(cwd);
    printString("$ ");
    readString(buf);
    parseCommand(buf, cmd, arg);

    if (strcmp(cmd, "cd")) cd(&cwd, arg[0]);
    else if (strcmp(cmd, "ls")) ls(cwd, arg[0]);
    else if (strcmp(cmd, "mv")) mv(cwd, arg[0], arg[1]);
    else if (strcmp(cmd, "cp")) cp(cwd, arg[0], arg[1]);
    else if (strcmp(cmd, "cat")) cat(cwd, arg[0]);
    else if (strcmp(cmd, "mkdir")) mkdir(cwd, arg[0]);
    else if (strcmp(cmd, "clear")) clearScreen();
    else printString("Invalid command\n");
  }
}

// TODO: 4. Implement printCWD function
void printCWD(byte cwd) {
    struct node_fs node_fs_buf;
    char path[128];
    char temp[128];
    int x = 0;
    int y = 0;
    int i,j,z;

    readSector((byte*)&node_fs_buf.nodes[0], FS_NODE_SECTOR_NUMBER);


    readSector((byte*)&node_fs_buf.nodes[32], FS_NODE_SECTOR_NUMBER);

    if (cwd == FS_NODE_P_ROOT) {
        printString("/");
        return;
    }

    while (cwd != FS_NODE_P_ROOT) {
        strcpy(temp, node_fs_buf.nodes[cwd].node_name);

        y=strlen(temp);


        for (i=x, j=y; i>=0; i--, j--) {
            z=i+y+1;
            path[z] = path[i];
        }


        for (i = 0; i < y; i++) {
            path[i] = temp[i];
        }


        path[y]='/';
        x = x + y + 1;
        cwd=node_fs_buf.nodes[cwd].parent_index;

    }

    path[x] = '\0';

    printString(path);
}


// TODO: 5. Implement parseCommand function
void parseCommand(char* buf, char* cmd, char arg[2][64]) {
    int buf_index = 0;
    int cmd_index = 0;
    int arg_index = 0;
    int char_index = 0;
    
    clear((byte*)cmd, 64);
    clear((byte*)arg[0], 64);
    clear((byte*)arg[1], 64);
    
    while (buf[buf_index] == ' ' || buf[buf_index] == '\t') {
        buf_index++;
    }

    while (buf[buf_index] != ' ' && buf[buf_index] != '\t' && buf[buf_index] != '\0') {
        cmd[cmd_index++] = buf[buf_index++];
    }

    cmd[cmd_index] = '\0';
    
    while (buf[buf_index] == ' ' || buf[buf_index] == '\t') {
        buf_index++;
    }

    while (buf[buf_index] != ' ' && buf[buf_index] != '\t' && buf[buf_index] != '\0') {
        arg[0][char_index++] = buf[buf_index++];
    }

    arg[0][char_index] = '\0';
    char_index = 0;

    while (buf[buf_index] == ' ' || buf[buf_index] == '\t') {
        buf_index++;
    }

    while (buf[buf_index] != ' ' && buf[buf_index] != '\t' && buf[buf_index] != '\0') {
        arg[1][char_index++] = buf[buf_index++];
    }

    arg[1][char_index] = '\0';
}

// TODO: 6. Implement cd function
void cd(byte* cwd, char* dirname) {
    struct node_fs node_fs_buf;
    int i;

    if (strcmp(dirname, "/") == 1) {
        *cwd = FS_NODE_P_ROOT;
        return;
    } 
    
    if (strcmp(dirname, "..") == 1) {
        readSector((byte*)&node_fs_buf, FS_NODE_SECTOR_NUMBER);
        readSector((byte*)&node_fs_buf + SECTOR_SIZE, FS_NODE_SECTOR_NUMBER + 1);
        *cwd = node_fs_buf.nodes[*cwd].parent_index;
        return;
    }

    readSector((byte*)&node_fs_buf, FS_NODE_SECTOR_NUMBER);
    readSector((byte*)&node_fs_buf + SECTOR_SIZE, FS_NODE_SECTOR_NUMBER + 1);

    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].parent_index == *cwd && strcmp(node_fs_buf.nodes[i].node_name, dirname) == 1) {
            if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
                *cwd = i;
                return;
            } else {
                printString("Not a directory\n");
                return;
            }
        }
    }

    printString("Directory not found\n");
}


// TODO: 7. Implement ls function
void ls(byte cwd, char* dirname) {
    struct node_fs node_fs_buf;

    int i;

    byte x = cwd;

    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);



    if (strcmp(dirname, ".")==0 && strlen(dirname)>0) {
        for (i = 0; i < FS_MAX_NODE; i++) {

            if (node_fs_buf.nodes[i].parent_index==cwd && strcmp(node_fs_buf.nodes[i].node_name, dirname) == 1) {
                if (node_fs_buf.nodes[i].data_index != FS_NODE_D_DIR){
                    printString("Not a directory\n");
                    return;
                }
                
                x=i;

                break;
            }
        }

        if (i==FS_MAX_NODE) {
            printString("Directory not found\n");
            return;
        }
    }

    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].parent_index==x) {
            printString(node_fs_buf.nodes[i].node_name);
            printString(" ");
        }

    }


    printString("\n");

}


// TODO: 8. Implement mv function
void mv(byte cwd, char* src, char* dst) {
    struct node_fs node_fs_buf;
    int i, j;
    byte dst_index = FS_NODE_P_ROOT;

    readSector((byte*)&node_fs_buf, FS_NODE_SECTOR_NUMBER);
    readSector((byte*)&node_fs_buf + SECTOR_SIZE, FS_NODE_SECTOR_NUMBER + 1);

    if (dst[0] == '/') {
        dst_index = FS_NODE_P_ROOT;
        dst++;
    } else if (dst[0] == '.' && dst[1] == '.' && dst[2] == '/') {
        readSector((byte*)&node_fs_buf, FS_NODE_SECTOR_NUMBER);
        dst_index = node_fs_buf.nodes[cwd].parent_index;
        dst += 3;
    } else {
        for (j = 0; j < FS_MAX_NODE; ++j) {
            if (node_fs_buf.nodes[j].parent_index == cwd && strcmp(node_fs_buf.nodes[j].node_name, dst) == 0) {
                dst_index = j;
                dst += strlen(node_fs_buf.nodes[j].node_name) + 1;
                break;
            }
        }
    }

    for (i = 0; i < FS_MAX_NODE; ++i) {
        if (node_fs_buf.nodes[i].parent_index == cwd && strcmp(node_fs_buf.nodes[i].node_name, src) == 0) {
            if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
                printString("Cannot move a directory\n");
                return;
            }
            node_fs_buf.nodes[i].parent_index = dst_index;
            strcpy(node_fs_buf.nodes[i].node_name, dst);
            writeSector((byte*)&node_fs_buf, FS_NODE_SECTOR_NUMBER);
            writeSector((byte*)&node_fs_buf + SECTOR_SIZE, FS_NODE_SECTOR_NUMBER + 1);
            return;
        }
    }
    printString("File not found\n");
}

// TODO: 9. Implement cp function
void cp(byte cwd, char* src, char* dst) {
    struct node_fs node_fs_buf;
    struct file_metadata src_meta, dst_meta;
    int i, j, free_node = -1;
    enum fs_return status;
    byte dst_index = FS_NODE_P_ROOT;

    readSector((byte*)&node_fs_buf, FS_NODE_SECTOR_NUMBER);
    readSector((byte*)&node_fs_buf + SECTOR_SIZE, FS_NODE_SECTOR_NUMBER + 1);

    if (dst[0] == '/') {
        dst_index = FS_NODE_P_ROOT;
        dst++;
    } else if (dst[0] == '.' && dst[1] == '.' && dst[2] == '/') {
        dst_index = node_fs_buf.nodes[cwd].parent_index;
        dst += 3;
    } else {
        for (j = 0; j < FS_MAX_NODE; ++j) {
            if (node_fs_buf.nodes[j].parent_index == cwd && strcmp(node_fs_buf.nodes[j].node_name, dst) == 0) {
                dst_index = j;
                dst += strlen(node_fs_buf.nodes[j].node_name) + 1;
                break;
            }
        }
    }

    for (i = 0; i < FS_MAX_NODE; ++i) {
        if (node_fs_buf.nodes[i].node_name[0] == '\0') {
            free_node = i;
            break;
        }
    }

    if (free_node == -1) {
        printString("No free node available\n");
        return;
    }

    for (i = 0; i < FS_MAX_NODE; ++i) {
        if (node_fs_buf.nodes[i].parent_index == cwd && strcmp(node_fs_buf.nodes[i].node_name, src) == 0) {
            if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
                printString("Cannot copy a directory\n");
                return;
            }

            src_meta.parent_index = cwd;
            strcpy(src_meta.node_name, src);
            fsRead(&src_meta, &status);

            if (status != FS_SUCCESS) {
                printString("Error reading source file\n");
                return;
            }

            dst_meta.parent_index = dst_index;
            strcpy(dst_meta.node_name, dst);
            dst_meta.filesize = src_meta.filesize;
            memcpy(dst_meta.buffer, src_meta.buffer, src_meta.filesize);

            fsWrite(&dst_meta, &status);

            if (status == FS_W_NO_FREE_DATA) {
                printString("No free data blocks available\n");
                return;
            } else if (status == FS_W_NO_FREE_NODE) {
                printString("No free nodes available\n");
                return;
            } else if (status != FS_SUCCESS) {
                printString("Error writing destination file\n");
                return;
            }

            node_fs_buf.nodes[free_node].parent_index = dst_index;
            node_fs_buf.nodes[free_node].data_index = dst_meta.buffer[0]; // Assuming data_index is the first sector
            strcpy(node_fs_buf.nodes[free_node].node_name, dst);

            writeSector((byte*)&node_fs_buf, FS_NODE_SECTOR_NUMBER);
            writeSector((byte*)&node_fs_buf + SECTOR_SIZE, FS_NODE_SECTOR_NUMBER + 1);
            return;
        }
    }

    printString("File not found\n");
}


// TODO: 10. Implement cat function
void cat(byte cwd, char* filename) {
    struct node_fs node_fs_buf;
    struct file_metadata file_meta;
    enum fs_return status;
    int i,j;

    readSector((byte*)&node_fs_buf, FS_NODE_SECTOR_NUMBER);

    for (i = 0; i < FS_MAX_NODE; ++i) {
        if (node_fs_buf.nodes[i].parent_index == cwd && strcmp(node_fs_buf.nodes[i].node_name, filename) == 0) {
            if (node_fs_buf.nodes[i].data_index != FS_NODE_D_DIR) {
                file_meta.parent_index = node_fs_buf.nodes[i].parent_index;
                file_meta.filesize = 0;
                strcpy(file_meta.node_name, filename);

                fsRead(&file_meta, &status);

                if (status == FS_SUCCESS) {
                    for ( j = 0; j < file_meta.filesize; ++j) {
                        printString(file_meta.buffer[j]);
                    }
                    printString("\n");
                    return;
                } else {
                    printString("Error reading file\n");
                    return;
                }
            } else {
                printString("Not a file\n");
                return;
            }
        }
    }

    printString("File not found\n");
}

// TODO: 11. Implement mkdir function
void mkdir(byte cwd, char* dirname) {
    struct node_fs node_fs_buf;
    int i;
    int free_node = -1;

    if (strlen(dirname) == 0 || strlen(dirname) >= 14) {
        printString("Invalid directory name\n");
        return;
    }

    readSector((byte*)&node_fs_buf.nodes[0], FS_NODE_SECTOR_NUMBER);
    readSector((byte*)&node_fs_buf.nodes[32], FS_NODE_SECTOR_NUMBER);

    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].parent_index == cwd && strcmp(node_fs_buf.nodes[i].node_name, dirname) == 1) {
            printString("Directory already exists\n");
            return;
        }
    }

    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].node_name[0] == '\0') {
            free_node = i;
            break;
        }
    }

    if (free_node == -1) {
        printString("No space left in the filesystem\n");
        return;
    }

    node_fs_buf.nodes[free_node].parent_index = cwd;
    node_fs_buf.nodes[free_node].data_index = FS_NODE_D_DIR;
    strcpy(node_fs_buf.nodes[free_node].node_name, dirname);

    writeSector((byte*)&node_fs_buf.nodes[0], FS_NODE_SECTOR_NUMBER);
    writeSector((byte*)&node_fs_buf.nodes[32], FS_NODE_SECTOR_NUMBER + 1);

    printString("Directory created\n");
}