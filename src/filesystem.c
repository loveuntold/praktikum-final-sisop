#include "kernel.h"
#include "std_lib.h"
#include "filesystem.h"

void fsInit() {
  struct map_fs map_fs_buf;
  int i = 0;

  readSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
  for (i = 0; i < 16; i++) map_fs_buf.is_used[i] = true;
  for (i = 256; i < 512; i++) map_fs_buf.is_used[i] = true;
  writeSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
}

// TODO: 2. Implement fsRead function
void fsRead(struct file_metadata* metadata, enum fs_return* status) {
  struct node_fs node_fs_buf;
  struct data_fs data_fs_buf;

  int i;
  int j;
  int found = -1;

  readSector(&data_fs_buf, FS_DATA_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER);

  for(i=0; i<FS_MAX_NODE; i++){
    if(strcmp(node_fs_buf.nodes[i].node_name, metadata->node_name) == 0 && node_fs_buf.nodes[i].parent_index == metadata->parent_index){
      found = i;
      break;
    }
  }

  if(found == -1){
    *status = FS_R_NODE_NOT_FOUND;
    return;
  }

  if(node_fs_buf.nodes[found].data_index == FS_NODE_D_DIR){
    *status = FS_R_TYPE_IS_DIRECTORY;
    return;
    
    metadata->filesize = 0;
    for(i=0; i<FS_MAX_SECTOR; i++){
      if(data_fs_buf.datas[node_fs_buf.nodes[found].data_index].sectors[i] == 0){
        break;
      }
      readSector(&(metadata->buffer[i*SECTOR_SIZE]), data_fs_buf.datas[node_fs_buf.nodes[found].data_index].sectors[i]);
      metadata->filesize += SECTOR_SIZE;
    }

    *status = FS_SUCCESS;
  }
}

// TODO: 3. Implement fsWrite function
void fsWrite(struct file_metadata* metadata, enum fs_return* status) {
  struct map_fs map_fs_buf;
  struct node_fs node_fs_buf;
  struct data_fs data_fs_buf;
}
