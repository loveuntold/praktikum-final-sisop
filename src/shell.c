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
    
    if(i > 0){
      printString("/");
    
    } 
  }

}

// TODO: 5. Implement parseCommand function
void parseCommand(char* buf, char* cmd, char arg[2][64]){
  int i = 0;
  int arg_index = 0;
  int buf_index = 0;

  while(buf[buf_index] != ' ' && buf[buf_index] != '\0'){
    cmd[i] = buf[buf_index];
    i++;
    buf_index++;
  }

  cmd[i] = '\0';

  if(buf[buf_index] == '\0') return;

  buf_index++;

  while(buf[buf_index] != '\0'){
    i = 0;

    while(buf[buf_index] != ' ' && buf[buf_index] != '\0'){
      arg[arg_index][i] = buf[buf_index];
      i++;
      buf_index++;
    }

    arg[arg_index][i] = '\0';
    arg_index++;

    if(buf[buf_index] == '\0') return;

    buf_index++;
  }

  arg[arg_index][0] = '\0';

}

// TODO: 6. Implement cd function
void cd(byte* cwd, char* dirname){
  struct node_fs node_fs_buf;
  byte i;

  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

  if(strcmp(dirname, "/") == 0){
    *cwd = FS_NODE_P_ROOT;
    return;
  }

  if(strcmp(dirname, "..") == 0){
    *cwd = node_fs_buf.nodes[*cwd].parent_index;
    return;
  }

  for(i=0; i<FS_MAX_NODE; i++){
    if(strcmp(node_fs_buf.nodes[i].node_name, dirname) == 0 && node_fs_buf.nodes[i].parent_index == *cwd){
      *cwd = i;
      return;
    }
  }

  printString("Directory not found\n");

}

// TODO: 7. Implement ls function
void ls(byte cwd, char* dirname) {}

// TODO: 8. Implement mv function
void mv(byte cwd, char* src, char* dst) {}

// TODO: 9. Implement cp function
void cp(byte cwd, char* src, char* dst) {}

// TODO: 10. Implement cat function
void cat(byte cwd, char* filename) {}

// TODO: 11. Implement mkdir function
void mkdir(byte cwd, char* dirname) {}

