#include "shell.h"
#include "kernel.h"
#include "std_lib.h"
#include "filesystem.h"

void shell(){
  char buf[64];
  char cmd[64];
  char arg[2][64];

  byte cwd = FS_NODE_P_ROOT;

  while(true){
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
void printCWD(byte cwd){
  struct node_fs node_fs_buf;
  byte path[64];  
  int i;
  int index = 0;

  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

  while(cwd != FS_NODE_P_ROOT){
    path[index] = cwd;
    cwd = node_fs_buf.nodes[cwd].parent_index;
    index++;
  }

  printString("/");

  for(i=index - 1; i >= 0; i--){
    printString(node_fs_buf.nodes[path[i]].node_name);
    printString("/"); 
  }

}

// TODO: 5. Implement parseCommand function
void parseCommand(char* buf, char* cmd, char arg[2][64]){
  int i = 0;
  int j = 0;


  for(i=0; i<FS_MAX_NODE; i++){
    cmd[i] = '\0';
    arg[0][i] = '\0';
    arg[1][i] = '\0';
  }

  i = 0;

  while(buf[i] != ' ' && buf[i] != '\0'){
    cmd[i] = buf[i];
    i++;
  }

  cmd[i] = '\0';

  if(buf[i] == '\0'){
    return;
  }

  i++;

  while(buf[i] == ' '){
    i++;
  }

  j = 0;
  while(buf[i] != ' ' && buf[i] != '\0'){
        arg[0][j++] = buf[i++];
  }

  arg[0][j] = '\0';

  while(buf[i] == ' '){
    i++;
  }

  j = 0;
  while(buf[i] != ' ' && buf[i] != '\0'){
    arg[1][j++] = buf[i++];
  }

  arg[1][j] = '\0';
}

// TODO: 6. Implement cd function
void cd(byte* cwd, char* dirname){
  struct node_fs node_fs_buf;
  int i;

  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

  if(strcmp(dirname, "/")){
    *cwd = FS_NODE_P_ROOT;
    return;
  }else if(strcmp(dirname, "..")){
    if(*cwd != FS_NODE_P_ROOT){
      *cwd = node_fs_buf.nodes[*cwd].parent_index;
      return;
    }
  }else{
    for(i=0; i<FS_MAX_NODE; i++){
      if(strcmp(node_fs_buf.nodes[i].node_name, dirname) && node_fs_buf.nodes[i].parent_index == *cwd){
        if(node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR){
          *cwd = i;
          return;
        }else{
          printString("Error: Not a directory\n");
          return;
        }
      }
    }
    printString("Error: Directory not found\n");
  }
}


// TODO: 7. Implement ls function
void ls(byte cwd, char* dirname){
  struct node_fs node_fs_buf;
  int i;
  byte target = cwd;

  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

// for ls and ls .
  if(strlen(dirname) > 0 && !strcmp(dirname, ".")){
    
    for(i=0; i<FS_MAX_NODE; i++){
      if(strcmp(node_fs_buf.nodes[i].node_name, dirname) && node_fs_buf.nodes[i].parent_index == cwd){
        if(node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR){
          target = i;
          break;
        }else{
          printString("Error: Not a directory\n");
          return;
        }
      }
    }

    if(i == FS_MAX_NODE){
      printString("Directory Not Found\n");
      return;
    }
  }
  
  for(i=0; i<FS_MAX_NODE; i++){
    if(node_fs_buf.nodes[i].parent_index == target){
      printString(node_fs_buf.nodes[i].node_name);
      printString(" ");
    }
  }

  printString("\n");
}


// TODO: 8. Implement mv function
void mv(byte cwd, char* src, char* dst){
    struct node_fs node_fs_buf;
    int i;
    int src_index = -1;
    byte parent = cwd;
    char directoryName[64];

    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    for(i=0; i<FS_MAX_NODE; i++){
        if(strcmp(node_fs_buf.nodes[i].node_name, src) && node_fs_buf.nodes[i].parent_index == cwd){
            src_index = i;
            break;
        }
    }

    if(src_index == -1){
        printString("Error: Source not found\n");
        return;
    }

    if(node_fs_buf.nodes[src_index].data_index == FS_NODE_D_DIR){
        printString("Error: Source is a directory\n");
        return;
    }

    if(dst[0] == '/'){
        parent = FS_NODE_P_ROOT;
        strcpy(directoryName, dst + 1);
    }else if(dst[0] == '.' && dst[1] == '.' && dst[2] == '/'){ 
        parent = node_fs_buf.nodes[cwd].parent_index;
        strcpy(directoryName, dst + 3);
    } else{
        for(i=0; i<FS_MAX_NODE; i++){
            if(strcmp(node_fs_buf.nodes[i].node_name, dst) && node_fs_buf.nodes[i].parent_index == cwd){
                parent = i;
                int len = strlen(node_fs_buf.nodes[i].node_name) + 1;
                strcpy(directoryName, len + dst);
                break;
            }
        }
    }

    if(i == FS_MAX_NODE){
        printString("Error: Destination not found\n");
        return;
    }

    if(node_fs_buf.nodes[parent].data_index != FS_NODE_D_DIR){
        printString("Error: Destination is not a directory\n");
        return;
    }

    for(i=0; i<FS_MAX_NODE; i++){
        if(strcmp(node_fs_buf.nodes[i].node_name, directoryName) && node_fs_buf.nodes[i].parent_index == parent){
            printString("Error: Destination already exists\n");
            return;
        }
    }

    node_fs_buf.nodes[src_index].parent_index = parent;
    strcpy(node_fs_buf.nodes[src_index].node_name, directoryName);

    writeSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    writeSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

} 



// TODO: 9. Implement cp function
void cp(byte cwd, char* src, char* dst) {}

// TODO: 10. Implement cat function
void cat(byte cwd, char* filename) {}

// TODO: 11. Implement mkdir function
void mkdir(byte cwd, char* dirname) {}

