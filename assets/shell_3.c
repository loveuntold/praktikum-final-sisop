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
  else if (strcmp(dirname, "/")) {
    *cwd = FS_NODE_P_ROOT;
  }
  else {
    for (i = 0; i < FS_MAX_NODE; i++) {
      if (strcmp(node_fs_buf.nodes[i].node_name, dirname) && node_fs_buf.nodes[i].parent_index == *cwd) {
        if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
          *cwd = i;
          return;
        }
        else {
          printString("cd: ");
          printString(dirname);
          printString(": Not a directory\n");
          return;
        }
      }
    }
    printString("cd: ");
    printString(dirname);
    printString(": No such file or directory\n");
  }
}

// TODO: 7. Implement ls function
void ls(byte cwd, char* dirname) {
  struct node_fs node_fs_buf;
  byte target_index = cwd;
  int i;

  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

  if (dirname[0] != '\0' && !(strlen(dirname) == 1 && dirname[0] == '.')) {
    bool dir_found = false;

    for (i = 0; i < FS_MAX_NODE; i++) {
      if (strcmp(node_fs_buf.nodes[i].node_name, dirname) && node_fs_buf.nodes[i].parent_index == cwd) {
        if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
          target_index = i;
          dir_found = true;
          break;
        }
        else {
          printString("ls: cannot access '");
          printString(dirname);
          printString("' : Not a directory\n");
          return;
        }
      }
    }

    if (!dir_found) {
      printString("ls: cannot access '");
      printString(dirname);
      printString("' : No such file or directory\n");
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
  byte src_index = 0xFF, dst_index = 0xFF, parent_index = cwd;
  char output_name[64], dirname[64];
  int i, dirname_len = 0;

  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

  for (i = 0; i < FS_MAX_NODE; i++) {
    if (strcmp(node_fs_buf.nodes[i].node_name, src) && node_fs_buf.nodes[i].parent_index == cwd) {
      src_index = i;
      break;
    }
  }

  if (src_index == 0xFF) {
    printString("mv: cannot stat '");
    printString(src);
    printString("': No such file or directory\n");
    return;
  }

  if (node_fs_buf.nodes[src_index].data_index == FS_NODE_D_DIR) {
    printString("mv: cannot move '");
    printString(src);
    printString("': Is a directory\n");
    return;
  }

  if (dst[0] == '/') {
    parent_index = FS_NODE_P_ROOT;
    strcpy(output_name, dst + 1);
  }
  else if (dst[0] == '.' && dst[1] == '.' && dst[2] == '/') {
    parent_index = node_fs_buf.nodes[cwd].parent_index;
    strcpy(output_name, dst + 3);
  }
  else {
    for (i = 0; dst[i] != '\0'; i++) {
      if (dst[i] == '/') {
        dirname_len = i;
        break;
      }
    }

    if (dirname_len > 0) {
      for (i = 0; i < dirname_len; i++) {
        dirname[i] = dst[i];
      }
      dirname[dirname_len] = '\0';
      strcpy(output_name, dst + dirname_len + 1);

      for (i = 0; i < FS_MAX_NODE; i++) {
        if (strcmp(node_fs_buf.nodes[i].node_name, dirname) && node_fs_buf.nodes[i].parent_index == cwd) {
          parent_index = i;
          break;
        }
      }

      if (i == FS_MAX_NODE) {
        printString("mv: cannot move '");
        printString(src);
        printString("' to '");
        printString(dst);
        printString("': No such file or directory\n");
        return;
      }

      if (node_fs_buf.nodes[parent_index].data_index != FS_NODE_D_DIR) {
        printString("mv: cannot move '");
        printString(src);
        printString("' to '");
        printString(dst);
        printString("': Not a directory\n");
        return;
      }
    }
    else {
      strcpy(output_name, dst);
    }
  }

  for (i = 0; i < FS_MAX_NODE; i++) {
    if (strcmp(node_fs_buf.nodes[i].node_name, output_name) && node_fs_buf.nodes[i].parent_index == parent_index) {
      printString("mv: cannot move '");
      printString(src);
      printString("' to '");
      printString(dst);
      printString("': File exists\n");
      return;
    }
  }

  strcpy(node_fs_buf.nodes[src_index].node_name, output_name);
  node_fs_buf.nodes[src_index].parent_index = parent_index;

  writeSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  writeSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
}

// TODO: 9. Implement cp function
void cp(byte cwd, char* src, char* dst) {
  struct file_metadata metadata_src, metadata_dst;
  struct node_fs node_fs_buf;
  enum fs_return status;
  byte src_index = 0xFF, dst_index = 0xFF, parent_index = cwd;
  char output_name[64], dirname[64];
  int i, dirname_len = 0;

  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

  for (i = 0; i < FS_MAX_NODE; i++) {
    if (strcmp(node_fs_buf.nodes[i].node_name, src) && node_fs_buf.nodes[i].parent_index == cwd) {
      src_index = i;
      break;
    }
  }

  if (src_index == 0xFF) {
    printString("cp: cannot stat '");
    printString(src);
    printString("': No such file or directory\n");
    return;
  }

  if (node_fs_buf.nodes[src_index].data_index == FS_NODE_D_DIR) {
    printString("cp: cannot copy '");
    printString(src);
    printString("': Is a directory\n");
    return;
  }

  if (dst[0] == '/') {
    parent_index = FS_NODE_P_ROOT;
    strcpy(output_name, dst + 1);
  }
  else if (dst[0] == '.' && dst[1] == '.' && dst[2] == '/') {
    parent_index = node_fs_buf.nodes[cwd].parent_index;
    strcpy(output_name, dst + 3);
  }
  else {
    for (i = 0; dst[i] != '\0'; i++) {
      if (dst[i] == '/') {
        dirname_len = i;
        break;
      }
    }

    if (dirname_len > 0) {
      for (i = 0; i < dirname_len; i++) {
        dirname[i] = dst[i];
      }
      dirname[dirname_len] = '\0';
      strcpy(output_name, dst + dirname_len + 1);

      for (i = 0; i < FS_MAX_NODE; i++) {
        if (strcmp(node_fs_buf.nodes[i].node_name, dirname) && node_fs_buf.nodes[i].parent_index == cwd) {
          parent_index = i;
          break;
        }
      }

      if (i == FS_MAX_NODE) {
        printString("cp: cannot create regular file '");
        printString(dst);
        printString("': No such file or directory\n");
        return;
      }

      if (node_fs_buf.nodes[parent_index].data_index != FS_NODE_D_DIR) {
        printString("cp: failed to access '");
        printString(dst);
        printString("': Not a directory\n");
        return;
      }
    }
    else {
      strcpy(output_name, dst);
    }
  }

  for (i = 0; i < FS_MAX_NODE; i++) {
    if (strcmp(node_fs_buf.nodes[i].node_name, output_name) && node_fs_buf.nodes[i].parent_index == parent_index) {
      printString("cp: cannot create regular file '");
      printString(dst);
      printString("': File exists\n");
      return;
    }
  }

  metadata_src.parent_index = cwd;
  strcpy(metadata_src.node_name, src);
  fsRead(&metadata_src, &status);

  metadata_dst.parent_index = parent_index;
  metadata_dst.filesize = metadata_src.filesize;
  strcpy(metadata_dst.node_name, output_name);
  memcpy(metadata_dst.buffer, metadata_src.buffer, metadata_src.filesize);
  fsWrite(&metadata_dst, &status);
}

// TODO: 10. Implement cat function
void cat(byte cwd, char* filename) {
  struct file_metadata metadata;
  struct node_fs node_fs_buf;
  enum fs_return status;
  int i;
  byte file_index = 0xFF;

  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

  for (i = 0; i < FS_MAX_NODE; i++) {
    if (strcmp(node_fs_buf.nodes[i].node_name, filename) && node_fs_buf.nodes[i].parent_index == cwd) {
      file_index = i;
      break;
    }
  }

  if (file_index == 0xFF) {
    printString("cat: ");
    printString(filename);
    printString(": No such file or directory\n");
    return;
  }

  if (node_fs_buf.nodes[file_index].data_index == FS_NODE_D_DIR) {
      printString("cat: ");
      printString(filename);
      printString(": Is a directory\n");
      return;
  }

  metadata.parent_index = cwd;
  strcpy(metadata.node_name, filename);
  fsRead(&metadata, &status);

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
      printString("mkdir: cannot create directory '");
      printString(dirname);
      printString("': File exists\n");
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
    printString("mkdir: cannot create directory: No space left on device\n");
    return;
  }

  node_fs_buf.nodes[free_node_index].parent_index = cwd;
  node_fs_buf.nodes[free_node_index].data_index = FS_NODE_D_DIR;
  strcpy(node_fs_buf.nodes[free_node_index].node_name, dirname);

  writeSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  writeSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
}