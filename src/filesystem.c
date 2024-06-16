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
  bool not_found = false;
  int data_index;
  int index = -1; 


  readSector(&data_fs_buf, FS_DATA_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER+1);

  for(i=0; i<FS_MAX_NODE; i++){
    if(strcmp(node_fs_buf.nodes[i].node_name, metadata->node_name) == 0 && node_fs_buf.nodes[i].parent_index == metadata->parent_index){
      not_found = false;
      index = i;
      break;
    }
    else{
      not_found = true;
    }
  }

  if(not_found){
    *status = FS_R_NODE_NOT_FOUND;
    return;
  }

  if(node_fs_buf.nodes[index].data_index == FS_NODE_D_DIR){
    *status = FS_R_TYPE_IS_DIRECTORY;
    return;
  }

  data_index = node_fs_buf.nodes[index].data_index;

  for(i=0; i<FS_MAX_SECTOR; i++){
    if(data_fs_buf.datas[data_index].sectors[i] == 0){
      break;
    }
    readSector(&(metadata->buffer[i*SECTOR_SIZE]), data_fs_buf.datas[data_index].sectors[i]);
  }

  *status = FS_SUCCESS;
  
}

// TODO: 3. Implement fsWrite function
void fsWrite(struct file_metadata* metadata, enum fs_return* status) {
  struct map_fs map_fs_buf;
  struct node_fs node_fs_buf;
  struct data_fs data_fs_buf;

  int i;
  int j;
  bool found = false;
  int empty_node_index = -1;
  int empty_data_index = -1;
  int free_block = 0;
  int sector_index = 0;
  int sector_needed = metadata->filesize / SECTOR_SIZE + 1;

  readSector(&data_fs_buf, FS_DATA_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER+1);

  for(i=0; i<FS_MAX_NODE; i++){
    if(strcmp(node_fs_buf.nodes[i].node_name, metadata->node_name) == 0 && node_fs_buf.nodes[i].parent_index == metadata->parent_index){
      found = true;
      break;
    }
    if(node_fs_buf.nodes[i].node_name[0] == 0 && empty_node_index == -1){
      empty_node_index = i;
    }
  }

  if(found){
    *status = FS_W_NODE_ALREADY_EXISTS;
    return;
  }

  if(empty_node_index == -1){
    *status = FS_W_NO_FREE_NODE;
    return;
  }

  for(i=0; i<FS_MAX_DATA; i++){
    if(data_fs_buf.datas[i].sectors[0] == 0){
      empty_data_index = i;
      break;
    }
  }

  if(empty_data_index == -1){
    *status = FS_W_NO_FREE_DATA;
    return;
  }

  for(i=0; i<FS_MAX_SECTOR; i++){
    if(data_fs_buf.datas[empty_data_index].sectors[i] == 0){
      free_block++;
    }
  }

  if(free_block < sector_needed){
    *status = FS_W_NOT_ENOUGH_SPACE;
    return;
  }

  for(i=0; i<FS_MAX_SECTOR; i++){
    if(data_fs_buf.datas[empty_data_index].sectors[i] == 0){
      data_fs_buf.datas[empty_data_index].sectors[i] = sector_index + 1;
      writeSector(&(metadata->buffer[i*SECTOR_SIZE]), data_fs_buf.datas[empty_data_index].sectors[i]);
      sector_index++;
    }
  }

  node_fs_buf.nodes[empty_node_index].parent_index = metadata->parent_index;
  node_fs_buf.nodes[empty_node_index].data_index = empty_data_index;
  strcpy(node_fs_buf.nodes[empty_node_index].node_name, metadata->node_name);

  writeSector(&data_fs_buf, FS_DATA_SECTOR_NUMBER);
  writeSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  writeSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER+1);

  *status = FS_SUCCESS;

}
