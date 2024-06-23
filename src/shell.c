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
    int len;
    int src_index = -1;
    int dst_index = -1;
    byte parent_index;
    char filename[64];

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
        parent_index = FS_NODE_P_ROOT;
        strcpy(filename, dst + 1);
    }else if(dst[0] == '.' && dst[1] == '.' && dst[2] == '/'){ 
        parent_index = node_fs_buf.nodes[cwd].parent_index;
        strcpy(filename, dst + 3);
    }else{
        for(i = strlen(dst) - 1; i >= 0; i--){
            if(dst[i] == '/'){
                len = i;
                dst[i] = '\0';
                break;
            }
        }

        if(i>=0){
            for(i=0; i<FS_MAX_NODE; i++){
                if(strcmp(node_fs_buf.nodes[i].node_name, dst) && node_fs_buf.nodes[i].parent_index == cwd){
                    if(node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR){
                        parent_index = i;
                        break;
                    }else{
                        printString("Error: Destination is not a directory\n");
                        return;
                    }
                }
            }

          strcpy(filename, dst + len + 1);
        }else{
            parent_index = cwd;
            strcpy(filename, dst);
        }
    
    }
  
    for(i=0; i<FS_MAX_NODE; i++){
        if(strcmp(node_fs_buf.nodes[i].node_name, filename) && node_fs_buf.nodes[i].parent_index == parent_index){
            printString("Error: File already exists\n");
            return;
        }
    }

    strcpy(node_fs_buf.nodes[src_index].node_name, filename);
    node_fs_buf.nodes[src_index].parent_index = parent_index;

    writeSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    writeSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);


    printString("File moved\n");
}

// TODO: 9. Implement cp function
void cp(byte cwd, char* src, char* dst){
  struct node_fs node_fs_buf;
  struct file_metadata file_metadata_buf;
  struct file_metadata metadata_src;
  struct file_metadata metadata_dst;
  enum fs_return status;
  int i;
  int len;
  int src_index = -1;
  int dst_index = -1;
  byte parent_index;
  char filename[64];

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
    parent_index = FS_NODE_P_ROOT;
    strcpy(filename, dst + 1);
  }else if(dst[0] == '.' && dst[1] == '.' && dst[2] == '/'){
    parent_index = node_fs_buf.nodes[cwd].parent_index;
    strcpy(filename, dst + 3);
  }else{
    for(i = strlen(dst) - 1; i >= 0; i--){
      if(dst[i] == '/'){
        len = i;
        dst[i] = '\0';
        break;
      }
    }

    if(i>=0){
      for(i=0; i<FS_MAX_NODE; i++){
        if(strcmp(node_fs_buf.nodes[i].node_name, dst) && node_fs_buf.nodes[i].parent_index == cwd){
          if(node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR){
            parent_index = i;
            break;
          }else{
            printString("Error: Destination is not a directory\n");
            return;
          }
        }
      }

      strcpy(filename, dst + len + 1);
    }else{
      parent_index = cwd;
      strcpy(filename, dst);
    }
  }  

  for(i=0; i<FS_MAX_NODE; i++){
    if(strcmp(node_fs_buf.nodes[i].node_name, filename) && node_fs_buf.nodes[i].parent_index == parent_index){
      printString("Error: File already exists\n");
      return;
    }
  }

  metadata_src.parent_index = cwd;
  strcpy(metadata_src.node_name, src);
  fsRead(&metadata_src, &status);

  if(status != FS_SUCCESS){
    printString("Error: Failed to read source file\n");
    return;
  }
  
  metadata_dst.parent_index = parent_index;
  metadata_dst.filesize = metadata_src.filesize;
  strcpy(metadata_dst.node_name, filename);
  
  
  memcpy(metadata_dst.buffer, metadata_src.buffer, metadata_src.filesize);
  fsWrite(&metadata_dst, &status);
  
  if(status != FS_SUCCESS){
    printString("Error: Failed to write destination file\n");
    return;
  }

  printString("File copied\n");
  
}

// TODO: 10. Implement cat function
void cat(byte cwd, char* filename) {
    struct node_fs node_fs_buf;
    struct file_metadata file_metadata_buf;
    enum fs_return status;
    int i;
    byte file_index = cwd; 

    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    for(i=0; i<FS_MAX_NODE; i++){
        if(strcmp(node_fs_buf.nodes[i].node_name, filename) && node_fs_buf.nodes[i].parent_index == cwd){
            file_index = i;
            break;
        }
    }

    if(file_index == cwd){
        printString("Error: File not found\n");
        return;
    }

    file_metadata_buf.parent_index = node_fs_buf.nodes[file_index].parent_index;
    strcpy(file_metadata_buf.node_name, node_fs_buf.nodes[file_index].node_name);
    fsRead(&file_metadata_buf, &status);

    if(status == FS_SUCCESS){
        printString(file_metadata_buf.buffer);
        printString("\n");
    }else{
        printString("Error: Failed to read file\n");
    }
}


// TODO: 11. Implement mkdir function
void mkdir(byte cwd, char* dirname) {
  struct node_fs node_fs_buf;
  byte free_node_index = -1;
  int i;

  readSector((byte*)&node_fs_buf.nodes[0], FS_NODE_SECTOR_NUMBER);
  readSector((byte*)&node_fs_buf.nodes[32], FS_NODE_SECTOR_NUMBER + 1);

  for(i=0; i<FS_MAX_NODE; i++){
      if(node_fs_buf.nodes[i].parent_index == cwd && strcmp(node_fs_buf.nodes[i].node_name, dirname)){
        printString("Error: Directory already exists\n");
        return;
    }
  }
 
  for(i=0; i<FS_MAX_NODE; i++){
    if(node_fs_buf.nodes[i].parent_index == 0 && strlen(node_fs_buf.nodes[i].node_name) == 0){
      free_node_index = i;
      break;
    }
  }

  if(free_node_index == -1){
    printString("Error: No free node available for mkdir\n");
    return;
  }

    node_fs_buf.nodes[free_node_index].parent_index = cwd;
    strcpy(node_fs_buf.nodes[free_node_index].node_name, dirname);
    node_fs_buf.nodes[free_node_index].data_index = FS_NODE_D_DIR; 

    writeSector((byte*)&node_fs_buf.nodes[0], FS_NODE_SECTOR_NUMBER);
    writeSector((byte*)&node_fs_buf.nodes[32], FS_NODE_SECTOR_NUMBER + 1);

    printString("Directory created\n");
}