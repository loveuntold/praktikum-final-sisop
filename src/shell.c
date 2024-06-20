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
  int i;
  byte path[64];
  int depth = 0;

  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER);

  while (cwd != FS_NODE_P_ROOT) {
    path[depth++] = cwd;
    cwd = node_fs_buf.nodes[cwd].parent_index;
  }

  printString("/");
  for (i = depth-1; i >= 0 ; i--) {
    printString(node_fs_buf.nodes[path[i]].node_name);
    printString("/");
  }
}

// TODO: 5. Implement parseCommand function
void parseCommand(char* buf, char* cmd, char arg[2][64]) {
  int i, j, k;

  for (i = 0; i < 64; i++) {
    cmd[i] = '\0';
    arg[0][i] = '\0';
    arg[1][i] = '\0';
  }
  i = 0;

  while (buf[i] != ' ' && buf[i] != '\0') {
    cmd[i] = buf[i];
    i++;
  }
  cmd[i] = '\0';

  if (buf[i++] == '\0') {
    return;
  }

  for (k=0; k<2; k++) {
    j=0;
    while (buf[i] != ' ' && buf[i] != '\0') {
      arg[k][j++] = buf[i++];
    }
    arg[k][j] = '\0';
    if (buf[i] != '\0') i++;
  }
}

// TODO: 6. Implement cd function
void cd(byte* cwd, char* dirname) {
  struct node_fs node_fs_buf;
  int i;

  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

  if (strcmp(dirname, "..")) {
    if (*cwd != FS_NODE_P_ROOT) {
      *cwd = node_fs_buf.nodes[*cwd].parent_index;
    }
  }
  else {
    for (i = 0; i < FS_MAX_NODE; i++) {
      if (strcmp(node_fs_buf.nodes[i].node_name, dirname) && node_fs_buf.nodes[i].parent_index == *cwd) {
        if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
          *cwd = i;
          return;
        }
        else {
          printString("Not a directory\n");
          return;
        }
      }
    }
    printString("Directory not found\n");
  }
}

// TODO: 7. Implement ls function
void ls(byte cwd, char* dirname) {
  struct node_fs node_fs_buf;
  byte target_index = cwd;
  int i;

  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

  if (dirname[0] != '\0') {
    bool dir_found = false;

    for (i = 0; i < FS_MAX_NODE; i++) {
      if (strcmp(node_fs_buf.nodes[i].node_name, dirname) && node_fs_buf.nodes[i].parent_index == cwd) {
        if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
          target_index = i;
          dir_found = true;
          break;
        }
        else {
          printString("Not a directory\n");
          return;
        }
      }
    }

    if (!dir_found) {
      printString("Directory not found\n");
      return;
    }
  }

  for (i = 0; i < FS_MAX_NODE; i++) {
    if (node_fs_buf.nodes[i].parent_index == target_index) {
      printString(node_fs_buf.nodes[i].node_name);
      printString(" ");
    }
  }
  printString("\n");
}

// TODO: 8. Implement mv function
void mv(byte cwd, char* src, char* dst) {
  struct node_fs node_fs_buf;
  int i;
  byte src_index = 0xFF, dst_index = 0xFF;

  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

  for (i = 0; i < FS_MAX_NODE; i++) {
    if (strcmp(node_fs_buf.nodes[i].node_name, src) && node_fs_buf.nodes[i].parent_index == cwd) {
      src_index = i;
      break;
    }
  }

  if (src_index == 0xFF) {
    printString("Source file not found\n");
    return;
  }

  for (i = 0; i < FS_MAX_NODE; i++) {
    if (strcmp(node_fs_buf.nodes[i].node_name, dst) && node_fs_buf.nodes[i].parent_index == cwd) {
      dst_index = i;
      break;
    }
  }

  if (dst_index != 0xFF) {
    printString("Destination already exists\n");
    return;
  }

  strcpy(node_fs_buf.nodes[src_index].node_name, dst);

  writeSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  writeSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
}

// TODO: 9. Implement cp function
void cp(byte cwd, char* src, char* dst) {
  struct file_metadata metadata_src, metadata_dst;
  enum fs_return status;
  struct node_fs node_fs_buf;
  int i;

  metadata_src.parent_index = cwd;
  strcpy(metadata_src.node_name, src);

  fsRead(&metadata_src, &status);
  if (status != FS_SUCCESS) {
    printString("Source file not found\n");
    return;
  }

  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

  for (i = 0; i < FS_MAX_NODE; i++) {
    if (strcmp(node_fs_buf.nodes[i].node_name, dst) && node_fs_buf.nodes[i].parent_index == cwd) {
      printString("Destination already exists\n");
      return;
    }
  }

  metadata_dst.parent_index = cwd;
  metadata_dst.filesize = metadata_src.filesize;
  strcpy(metadata_dst.node_name, dst);
  memcpy(metadata_dst.buffer, metadata_src.buffer, metadata_src.filesize);

  fsWrite(&metadata_dst, &status);
  if (status != FS_SUCCESS) {
    printString("Failed to copy file\n");
    return;
  }
}

// TODO: 10. Implement cat function
void cat(byte cwd, char* filename) {
  struct file_metadata metadata;
  enum fs_return status;
  int i;

  metadata.parent_index = cwd;
  strcpy(metadata.node_name, filename);

  fsRead(&metadata, &status);
  if (status != FS_SUCCESS) {
    printString("File not found\n");
    return;
  }

  for (i = 0; i < metadata.filesize; i++) {
    char c = metadata.buffer[i];
    interrupt(0x10, 0xE << 8 | c, 0, 0, 0);
  }
  printString("\n");
}

// TODO: 11. Implement mkdir function
void mkdir(byte cwd, char* dirname) {
  struct node_fs node_fs_buf;
  int i, free_node_index = -1;

  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

  for (i = 0; i < FS_MAX_NODE; i++) {
    if (strcmp(node_fs_buf.nodes[i].node_name, dirname) && node_fs_buf.nodes[i].parent_index == cwd) {
      printString("Directory already exists\n");
      return;
    }
  }

  for (i = 0; i < FS_MAX_NODE; i++) {
    if (node_fs_buf.nodes[i].parent_index == 0x00 && node_fs_buf.nodes[i].data_index == 0x00) {
      free_node_index = i;
      break;
    }
  }

  if (free_node_index == -1) {
    printString("No free node available\n");
    return;
  }

  node_fs_buf.nodes[free_node_index].parent_index = cwd;
  node_fs_buf.nodes[free_node_index].data_index = FS_NODE_D_DIR;
  strcpy(node_fs_buf.nodes[free_node_index].node_name, dirname);

  writeSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  writeSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
}

