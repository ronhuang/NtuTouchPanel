/* Arduino SdFat Library
 * Copyright (C) 2009 by William Greiman
 *  
 * This file is part of the Arduino SdFat Library
 *  
 * This Library is free software: you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with the Arduino SdFat Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "SdFat.h"
#include <avr/pgmspace.h>
#include "WProgram.h"
//------------------------------------------------------------------------------
/**
 *  Format the name field of \a dir into the 13 byte array
 *  \a name in standard 8.3 short name format.
 */
void SdFile::dirName(dir_t &dir, char *name)
{
  uint8_t j = 0;
  for (uint8_t i = 0; i < 11; i++) {
    if (dir.name[i] == ' ')continue;
    if (i == 8) name[j++] = '.';
    name[j++] = dir.name[i];
  }
  name[j] = 0;
}
//------------------------------------------------------------------------------
// form 8.3 name for directory entry
static uint8_t make83Name(char *str, uint8_t *name)
{
  uint8_t c;
  uint8_t n = 7;
  uint8_t i = 0;
  //blank fill name and extension
  while (i < 11)name[i++] = ' ';
  i = 0;
  while ((c = *str++) != '\0') {
    if (c == '.') {
      if (n == 10) return false;// only one dot allowed
      n = 10;
      i = 8;
    }
    else {
#define SD_FAT_SAVE_RAM 1
#if SD_FAT_SAVE_RAM
      // using PSTR gives incorrect warning in C++ files for Arduino
      // illegal FAT16 characters
      char b, *p = (char *)PSTR("|<>^+=?/[];,*\"\\");
      while ((b = pgm_read_byte(p++))) if (b == c) return false;
#else //SD_FAT_SAVE_RAM
      // illegal FAT16 characters
      const char *p = "|<>^+=?/[];,*\"\\";     
      while (*p) if (*p++ == c) return false;
#endif //SD_FAT_SAVE_RAM
      // check size and only allow ASCII printable characters
      if (i > n || c < 0X21 || c > 0X7E)return false;
      //only upper case allowed in 8.3 names - convert lower to upper
      name[i++] = c < 'a' || c > 'z' ?  c : c + ('A' - 'a');
    }
  }
  //must have a file name, extension is optional
  if (name[0] == ' ') return false;
  return true;
}
//==============================================================================
// add a cluster to a file for write
uint8_t SdFile::addCluster(void)
{
  if (!vol_->allocContiguous(curCluster_, 1)) return false;
  if (firstCluster_ == 0) {
    // first cluster of file so link to directory entry  
    firstCluster_ = curCluster_;
    attributes_ |= ATTR_FILE_DIR_DIRTY;
  }
  return true;
}
//-----------------------------------------------------------------------------
/**
 * Check for contiguous file and return it's cluster range.
 *  
 * \param[out] bgnBlock the first block address for the file.
 * \param[out] endBlock the last  block address for the file.
 *  
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 * Reasons for failure include file is not contiguous, file has zero length
 * or an I/O error occurred. 
 */
uint8_t SdFile::contiguousRange(uint32_t &bgnBlock, uint32_t &endBlock)
{
  if (firstCluster_ == 0) return false;
  for (uint32_t c = firstCluster_; ; c++) {
    uint32_t next;
    if (!vol_->fatGet(c, next)) return false;
    if (next != (c + 1)) {
      if (!vol_->isEOC(next)) return false;
      bgnBlock = vol_->clusterStartBlock(firstCluster_);
      endBlock = vol_->clusterStartBlock(c)
                  + vol_->blocksPerCluster_ - 1;
      return true;
    }
  }
}
//------------------------------------------------------------------------------
/**
 * Create and open a new file.
 * 
 * \note This function only supports short DOS 8.3 names. 
 * See open() for more information.
 * 
 * \param[in] dirFile The directory where the file will be created.
 * \param[in] fileName A valid DOS 8.3 file name.
 *  
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 * Reasons for failure include \a fileName contains
 * an invalid DOS 8.3 file name, the FAT volume has not been initialized,
 * a file is already open, the file already exists, the root
 * directory is full or an I/O error. 
 *
 */  
uint8_t SdFile::create(SdFile &dirFile, char *fileName)
{
  dir_t dir;
  uint8_t name[11];
  if (!dirFile.isDir())return false;
  //error if bad file name
  if (!make83Name(fileName, name)) return false;
  dirFile.rewind();
  //error if name exists
  while (dirFile.curPosition_ < dirFile.fileSize_) {
    if (dirFile.read((uint8_t *)&dir, 32) != 32) return false;
    if (dir.name[0] == 0) break;
    if (strncmp((char *)dir.name, (char*)name, 11) == 0) return false;
  }
  uint8_t found = 0;
  dirFile.rewind();
  // find unused dir entry
  while (dirFile.curPosition_ < dirFile.fileSize_) {
    if (dirFile.read((uint8_t *)&dir, 32) != 32) return false;
    if (dir.name[0] == DIR_NAME_FREE || dir.name[0] == DIR_NAME_DELETED) {
      found = 1;
      break;
    } 
  }
  vol_ = dirFile.vol_;
  if (found) {
    dirBlock_ = vol_->cacheBlockNumber_;
    dirIndex_ = ((dirFile.curPosition_ - 32) >> 5) & 0XF;
  }
  else {
    if (dirFile.type_ == FAT_FILE_TYPE_ROOT16) return false;
    // add and zero cluster for dirFile
    if (!vol_->allocContiguous(dirFile.curCluster_, 1)) return false;
    dirFile.fileSize_ += 512UL << vol_->clusterSizeShift_;
    // use first entry in new cluster
    dirBlock_ = vol_->clusterStartBlock(dirFile.curCluster_);
    dirIndex_ = 0;    
    // zero cluster - make sure last block zeroed is dirBlock_
    for (uint8_t i = vol_->blocksPerCluster_; i != 0; i--) {
      if (!vol_->cacheZeroBlock(dirBlock_ + i - 1)) return false;
    }
  }
  uint8_t *p = (uint8_t*)(vol_->cacheBuffer_.dir + dirIndex_);
  //initialize as empty file
  for (uint8_t i = 0; i < sizeof(dir_t); i++) {
    p[i] = i < sizeof(name) ? name[i] : 0;
  }
  type_ = FAT_FILE_TYPE_NORMAL;
  attributes_ = 0;
  curCluster_ = 0;
  curPosition_ = 0;
  firstCluster_ = 0;
  fileSize_ = 0;
  //insure created directory entry will be written to storage device
  vol_->cacheSetDirty();
  return vol_->cacheFlush();
}
//------------------------------------------------------------------------------
/**
 * Create and open a new contiguous file of a specified size.
 * 
 * \note This function only supports short DOS 8.3 names. 
 * See open() for more information.
 * 
 * \param[in] dirFile The directory where the file will be created.
 * \param[in] fileName A valid DOS 8.3 file name.
 * \param[in] size The desired file size.
 *  
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 * Reasons for failure include \a fileName contains
 * an invalid DOS 8.3 file name, the FAT volume has not been initialized,
 * a file is already open, the file already exists, the root
 * directory is full or an I/O error. 
 *
 */  
uint8_t SdFile::createContiguous(SdFile &dirFile, char *fileName, uint32_t size)
{
  if (size == 0) return false;
  if (!create(dirFile, fileName)) return false;
  uint32_t count = ((size - 1) >> (vol_->clusterSizeShift_ + 9)) + 1;
  if (!vol_->allocContiguous(firstCluster_, count)) {
    remove();
    return false;
  }
  fileSize_ = size;
  attributes_ |= ATTR_FILE_DIR_DIRTY;
  return sync();
}
//------------------------------------------------------------------------------
/**
 * Open a file or subdirectory by name.
 * 
 * \note The file or subdirectory, \a name, must be in the specified
 * directory, \a dir, and must have a DOS 8.3 name.
 * 
 * \param[in] dir An open SdFile instance for the directory.
 *  
 * \param[in] name A valid 8.3 DOS name for a file or subdirectory in the
 * directory \a dir.
 * 
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 * Reasons for failure include the FAT volume has not been initialized, \a dir
 * is not a directory, \a name is invalid, the file or subdirectory does not
 *  exist, or an I/O error occurred.  
 */  
uint8_t SdFile::open(SdFile &dirFile, char *name)
{
  uint8_t dname[11];
  if (!make83Name(name, dname)) return false;
  // is there a better cast?
  return open(dirFile, *((dir_t *)dname));
}
//------------------------------------------------------------------------------
/**
 * Open a file or subdirectory by directory structure.
 * 
 * \param[in] dirFile SdFile object for the directory containing the file.
 *  
 * \param[in] entry The directory structure with file name.
 * 
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 */  
uint8_t SdFile::open(SdFile &dirFile, dir_t &entry)
{
  dir_t dir;
  if (isOpen()) return false;
  dirFile.rewind();
  while (1) {
    if (dirFile.readDir(dir) <= 0) return false;
    if (!strncasecmp((char *)entry.name, (char *)dir.name, 11)) {
      dirIndex_ = ((dirFile.curPosition_ - 32) >> 5) & 0XF;    
      dirBlock_ = dirFile.vol_->cacheBlockNumber_;
      break;
    }
  }
  vol_ = dirFile.vol_;    
  if (DIR_IS_FILE(dir)) {
    type_ = FAT_FILE_TYPE_NORMAL;
    fileSize_ = dir.fileSize;
  }
  else if (DIR_IS_SUBDIR(dir)) {
    type_ = FAT_FILE_TYPE_SUBDIR;
    if (!vol_->chainSize(fileSize_, firstCluster_)) return false;
  }
  else {
    return false;
  }
  attributes_ = dir.attributes & DIR_ATT_DEFINED_BITS;  
  curPosition_ = 0;
  curCluster_ = 0;
  firstCluster_ = (uint32_t)dir.firstClusterHigh << 16;
  firstCluster_ |= dir.firstClusterLow; 
  fileSize_ = dir.fileSize;
  return true;
}
//------------------------------------------------------------------------------
/**
 * Open a volume's root directory.
 *
 * \param[in] vol The FAT volume containing the root directory to be opened.
 *
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 * Reasons for failure include the FAT volume has not been initialized
 * or it a FAT12 volume.
 */
uint8_t SdFile::openRoot(SdVolume &vol)
{
  if (isOpen()) return false;
  if(vol.fatType() == 16) {
    type_ = FAT_FILE_TYPE_ROOT16;
    firstCluster_ = 0;
    fileSize_ = 32*vol.rootDirEntryCount();
  }
  else if (vol.fatType() == 32) {
    type_ = FAT_FILE_TYPE_ROOT32;
    firstCluster_ = vol.rootDirStart();
    if (!vol.chainSize(fileSize_, firstCluster_)) return false;
  }
  else {
    return false;
  }
  attributes_ = DIR_ATT_DIRECTORY;
  vol_ = &vol;
  curCluster_ = 0;
  curPosition_ = 0;
  dirBlock_ = 0;
  dirIndex_ = 0;
  return true;
}
//------------------------------------------------------------------------------
/**
 * Read data from a file starting at the current position.
 * 
 * \param[out] dst Pointer to the location that will receive the data.
 * 
 * \param[in] count Maximum number of bytes to read.
 * 
 * \return For success read() returns the number of bytes read.
 * A value less than \a count, including zero, will be returned
 * if end of file is reached. 
 * If an error occurs, read() returns -1.  Possible errors include
 * read() called before a file has been opened, corrupt file system
 * or an I/O error occurred.
 */ 
int16_t SdFile::read(uint8_t *dst, uint16_t count)
{
  if (!isOpen()) return false;
  if (count > (fileSize_ - curPosition_)) count = fileSize_ - curPosition_;
  uint16_t toRead = count;
  while (toRead > 0) {
    uint32_t block; // raw device block number
    uint16_t offset = curPosition_ & 0X1FF; // offset in block
    if (type_ == FAT_FILE_TYPE_ROOT16) {
      block = vol_->rootDirStart() + (curPosition_ >> 9);
    }
    else {
      uint8_t blockOfCluster = vol_->blockOfCluster(curPosition_);
      if (offset == 0 && blockOfCluster == 0) {
        // start of new cluster
        if (curPosition_ == 0) {
          // use first cluster in file
          curCluster_ = firstCluster_;
        }
        else {
          // get next cluster from FAT
          if (!vol_->fatGet(curCluster_, curCluster_)) return -1;
        }
      }
      block = vol_->clusterStartBlock(curCluster_) + blockOfCluster;
    }
    uint16_t n = toRead;
    if (n > (512 - offset)) n = 512 - offset;
    if (unbufferedRead() && block != vol_->cacheBlockNumber_) {        
      if (!vol_->readData(block, offset, dst, n)) return -1;
    }
    else {
      if (!vol_->cacheRawBlock(block)) return -1;
      uint8_t *src = vol_->cacheBuffer_.data + offset;
      uint8_t *end = src + n;
      while (src != end) *dst++ = *src++;
    }
    curPosition_ += n;
    toRead -= n;
  }
  return count;
}
//------------------------------------------------------------------------------
/**
 * Read the next directory entry from a directory file.
 *
 * \param[out] dir The dir_t struct that will receive the data.
 *
 * \return For success readDir() returns the number of bytes read.
 * A value of zero will be returned if end of file is reached.
 * If an error occurs, readDir() returns -1.  Possible errors include
 * readDir() called before a directory has been opened, this is not
 * a directory file or an I/O error occurred.
 */
int8_t SdFile::readDir(dir_t &dir)
{
  int8_t n;
  //if not a directory file return an error
  if (!isDir()) return -1;
  while ((n = read((uint8_t *)&dir, sizeof(dir_t))) == sizeof(dir_t)) {
    if (dir.name[0] == 0) break;
    if (dir.name[0] == DIR_NAME_DELETED) continue;
    if (DIR_IS_FILE(dir) || DIR_IS_SUBDIR(dir)) return n;
  }
  return n < 0 ? n : 0;
}
//------------------------------------------------------------------------------
/**
 * Remove a file.  The directory entry and all data for the file are deleted.
 *
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 * Reasons for failure include the file is a directory, is read only
 * or an I/O error occurred. 
 */
uint8_t SdFile::remove(void)
{
  if (!isFile() || isReadOnly()) return false;
  if (firstCluster_) {
    if (!vol_->freeChain(firstCluster_)) return false;
  }
  if (!vol_->cacheRawBlock(dirBlock_, CACHE_FOR_WRITE)) return false;

  dir_t *d = vol_->cacheBuffer_.dir + dirIndex_;
  d->name[0] = DIR_NAME_DELETED;
  type_ = FAT_FILE_TYPE_CLOSED;
  return vol_->cacheFlush();
}
//------------------------------------------------------------------------------
/**
 * Remove a file.  The directory entry and all data for the file are deleted.
 *
 * \param[in] dirFile The directory that contains the file.
 * \param[in] fileName The name of the file to be removed.
 *  
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 * Reasons for failure include the file is a directory, is read only,
 * \a dirFile is not a directory, \a fileName is not found 
 * or an I/O error occurred. 
 */
uint8_t SdFile::remove(SdFile &dirFile, char *fileName) 
{
  SdFile file;
  if (!file.open(dirFile, fileName)) return false;
  return file.remove();
}
//------------------------------------------------------------------------------
/**
 * Sets the file's position.
 *
 * \param[in] pos The new position in bytes from the beginning of the file.
 * 
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.   
 */
uint8_t SdFile::seekSet(uint32_t pos)
{
  // error if file not open or seek past end of file
  if (!isOpen() || pos > fileSize_) return false;
  if (type_ == FAT_FILE_TYPE_ROOT16) {
    curPosition_ = pos;
    return true;
  }
  if (pos == 0) {
    //set position to start of file
    curCluster_ = 0;
    curPosition_ = 0;
    return true;
  }
  // calculate cluster index for cur and new position
  uint32_t nCur = (curPosition_ - 1) >> (vol_->clusterSizeShift_ + 9);
  uint32_t nNew = (pos - 1) >> (vol_->clusterSizeShift_ + 9);
  if (nNew < nCur || curPosition_ == 0) {
    // must follow chain from first cluster
    curCluster_ = firstCluster_;
  }
  else {
    // advance from curPosition
    nNew -= nCur;
  }
  while (nNew--) {
    if (!vol_->fatGet(curCluster_, curCluster_)) return false;
  }
  curPosition_ = pos;
  return true;
}
//------------------------------------------------------------------------------
/**
 *  The sync() call causes all modified data and directory fields
 *  to be written to the storage device.
 *
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 * Reasons for failure include a call to sync() before a file has been
 * opened or an I/O error.   
 */  
uint8_t SdFile::sync(void)
{
  if (!isOpen()) return false;
  if (attributes_ & ATTR_FILE_DIR_DIRTY) {
    if (!vol_->cacheRawBlock(dirBlock_, CACHE_FOR_WRITE)) return false;
    dir_t *d = &(vol_->cacheBuffer_.dir[dirIndex_]);
    d->fileSize = fileSize_;
    d->firstClusterLow = firstCluster_ & 0XFFFF;
    d->firstClusterHigh = firstCluster_ >> 16;
    attributes_ &= ~ATTR_FILE_DIR_DIRTY;
  }
  return vol_->cacheFlush();
}
//------------------------------------------------------------------------------
/**
 * Truncate a file to a specified length.  The current file position
 * will be maintained if it is less than or equal to \a length otherwise
 * it will be set to end of file.
 *  
 * \param[in] length The desired length for the file. 
 * 
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 * Reasons for failure include file is read only, file is a directory,
 * \a length is greater than the current file size or an I/O error occurs.
 */ 
uint8_t SdFile::truncate(uint32_t length)
{
  if (!isFile()|| isReadOnly()) return false;
  if (length > fileSize_) return false;
  // fileSize and length are zero - nothing to do
  if (fileSize_ == 0) return true;
  // 
  uint32_t newPos = curPosition_ > length ? length : curPosition_;
  if (!seekSet(length)) return false;
  if (curCluster_ == 0) {
    if (!vol_->freeChain(firstCluster_)) return false;
    firstCluster_ = 0;
  }
  else {
    uint32_t toFree;
    if (!vol_->fatGet(curCluster_, toFree)) return false;
    if (!vol_->isEOC(toFree)) {
      // free extra clusters
      if (!vol_->freeChain(toFree)) return false;   
      // current cluster is end of chain   
      if (!vol_->fatPutEOC(curCluster_)) return false;
    }
  }    
  fileSize_ = length;
  attributes_ |= ATTR_FILE_DIR_DIRTY;
  if (!sync()) return false;
  return seekSet(newPos);
}
//------------------------------------------------------------------------------
/**
 * Append one byte to a file.  This function is called by Arduino's Print class.
 *
 * \note The byte is moved to the cache but may not be written to the
 * storage device until sync() is called.
 *
 * \param[in] b The byte to be written. 
 */ 
void SdFile::write(uint8_t b)
{
  if (write(&b, 1) != 1) writeByteError = 1;
}
//------------------------------------------------------------------------------
/**
 * Write data to an open file.
 * 
 * \note Data is moved to the cache but may not be written to the
 * storage device until sync() is called.   
 * 
 * \param[in] src Pointer to the location of the data to be written.
 * 
 * \param[in] count Number of bytes to write.
 * 
 * \return For success write() returns the number of bytes written, always
 * \a count.  If an error occurs, write() returns -1.  Possible errors
 * include write() is called before a file has been opened, write is called
 * for a read-only file, device is full, a corrupt file system or an I/O error. 
 *
 */   
int16_t SdFile::write(uint8_t *src, uint16_t count)
{
  if (!isFile() || isReadOnly()) return -1;
  uint16_t nToWrite = count;
  while (nToWrite > 0) {
    uint8_t blockOfCluster = vol_->blockOfCluster(curPosition_);
    uint16_t blockOffset = curPosition_ & 0X1FF;
    if (blockOfCluster == 0 && blockOffset == 0) {
      //start of new cluster
      if (curCluster_ == 0) {
        if (firstCluster_ == 0) {
          // allocate first cluster of file
        if (!addCluster()) return -1;
        }
        else {
          curCluster_ = firstCluster_;
        }
      }
      else {
        uint32_t next;
        if (!vol_->fatGet(curCluster_, next)) return false;
        if (vol_->isEOC(next)) {
          // add cluster if at end of chain
          if (!addCluster()) return -1; 
        }
        else {
          curCluster_ = next;
        }
      }
    }
    // max space in block
    uint16_t n = 512 - blockOffset;
    // lesser of space and amount to write
    if(n > nToWrite) n = nToWrite;
    uint32_t block = vol_->clusterStartBlock(curCluster_) + blockOfCluster;
    if(n == 512) {
      // full block - don't need to use cache
      if (!vol_->writeBlock(block, src)) return -1;
      src += 512;
    }
    else {
      if (blockOffset == 0 && curPosition_ == fileSize_) {
        // start of new block don't need to read into cache
        if (!vol_->cacheFlush()) return -1;
        vol_->cacheBlockNumber_ = block;
        vol_->cacheSetDirty();
      }
      else {
        // rewrite part of block
        if (!vol_->cacheRawBlock(block, CACHE_FOR_WRITE)) return -1;
      }
      uint8_t *dst = vol_->cacheBuffer_.data + blockOffset;
      uint8_t *end = dst + n;
      while (dst != end) *dst++ = *src++;
    }
    nToWrite -= n;
    curPosition_ += n;
    if (curPosition_ > fileSize_) {
      // update fileSize and insure sync will update dir entry    
      fileSize_ = curPosition_;
      attributes_ |= ATTR_FILE_DIR_DIRTY;
    }
  }
  return count - nToWrite;
}
//==============================================================================
// raw block cache
// init to invalid block number
uint32_t SdVolume::cacheBlockNumber_ = 0XFFFFFFFF; 
cache_t  SdVolume::cacheBuffer_;     //512 byte cache for Sd2Card
Sd2Card *SdVolume::sdCard_;
uint8_t  SdVolume::cacheDirty_ = 0;  //cacheFlush() will write block if true
uint32_t SdVolume::cacheMirrorBlock_ = 0; // mirror  block for second FAT

//------------------------------------------------------------------------------
uint8_t SdVolume::allocContiguous(uint32_t &curCluster, uint32_t count)
{
  uint32_t bgnCluster = curCluster ? curCluster + 1 : 2;
  uint32_t endCluster = bgnCluster;
  uint32_t fatEnd = clusterCount_ + 1;
  for (uint32_t n = 0;; n++, endCluster++) {
    if (n >= clusterCount_) return false;
    if (endCluster > fatEnd) {
      bgnCluster = endCluster = 2;
    }
    uint32_t f;
    if (!fatGet(endCluster, f)) return false;
    if (f != 0) {
      bgnCluster = endCluster + 1;
    }
    else if ((endCluster - bgnCluster + 1) == count){
      break;
    }
  }
  // mark end of chain
  if (!fatPutEOC(endCluster)) return false;
  // link clusters
  while (endCluster > bgnCluster) {
    if (!fatPut(endCluster - 1, endCluster)) return false;
    endCluster--;
  }
  if (curCluster != 0) {
    // connect chains
    if (!fatPut(curCluster, bgnCluster)) return false;
  }
  curCluster = bgnCluster;
  return true;
}
//------------------------------------------------------------------------------
uint8_t SdVolume::cacheFlush(void)
{
  if (cacheDirty_) {
    if (!sdCard_->writeBlock(cacheBlockNumber_, cacheBuffer_.data)) {
      return false;
    }
    // mirror FAT tables
    if (cacheMirrorBlock_) {
      if (!sdCard_->writeBlock(cacheMirrorBlock_, cacheBuffer_.data)) {
        return false;
      }
      cacheMirrorBlock_ = 0;
    }
    cacheDirty_ = 0;
  }
  return true;
}
//------------------------------------------------------------------------------
uint8_t SdVolume::cacheRawBlock(uint32_t blockNumber, uint8_t action)
{
  if (cacheBlockNumber_ != blockNumber) {
    if (!cacheFlush()) return false;
    if (!sdCard_->readBlock(blockNumber, cacheBuffer_.data)) return false;
    cacheBlockNumber_ = blockNumber;
  }
  cacheDirty_ |= action;
  return true;
}
//------------------------------------------------------------------------------
// cache a zero block for blockNumber
uint8_t SdVolume::cacheZeroBlock(uint32_t blockNumber)
{
  if (!cacheFlush()) return false;
  for (uint16_t i = 0; i < 512; i++) cacheBuffer_.data[i] = 0;
  cacheBlockNumber_ = blockNumber;
  cacheSetDirty();
  return true;
}
//------------------------------------------------------------------------------
// return the size in bytes of a cluster chain
uint8_t SdVolume::chainSize(uint32_t &size, uint32_t cluster)
{
  size = 0;
  while(1) {
    if (!fatGet(cluster, cluster)) return false;
    size += 512UL << clusterSizeShift_;
    if (isEOC(cluster)) return true;
  }
}
//------------------------------------------------------------------------------
// Fetch a FAT entry
uint8_t SdVolume::fatGet(uint32_t cluster, uint32_t &value)
{
  if (cluster > (clusterCount_ + 1)) return false;
  uint32_t lba = fatStartBlock_ ;
  lba += fatType_ == 16 ? cluster >> 8 : cluster >> 7;
  if (lba != cacheBlockNumber_) {
    if (!cacheRawBlock(lba)) return false;
  }
  if (fatType_ == 16) {
    value = cacheBuffer_.fat16[cluster & 0XFF];
  }
  else {
    value = cacheBuffer_.fat32[cluster & 0X7F];
  }
  return true;
}
//------------------------------------------------------------------------------
// Store a FAT entry
uint8_t SdVolume::fatPut(uint32_t cluster, uint32_t value)
{
  if (cluster < 2) return false;
  if (cluster > (clusterCount_ + 1)) return false;  
  uint32_t lba = fatStartBlock_ ;
  lba += fatType_ == 16 ? cluster >> 8 : cluster >> 7;
  if (lba != cacheBlockNumber_) {
    if (!cacheRawBlock(lba)) return false;
  }
  if (fatType_ == 16) {
    cacheBuffer_.fat16[cluster & 0XFF] = value;
  }
  else {
    cacheBuffer_.fat32[cluster & 0X7F] = value;
  }
  cacheSetDirty();
  // mirror second FAT
  if (fatCount_ > 1) cacheMirrorBlock_ = lba + blocksPerFat_;
  return true;
}
//------------------------------------------------------------------------------
// free a cluster chain
uint8_t SdVolume::freeChain(uint32_t cluster)
{
  while (1) {
    uint32_t next;
    if (!fatGet(cluster, next)) return false;
    if (!fatPut(cluster, 0)) return false;
    if (isEOC(next)) return true;
    cluster = next;
  }
}
//------------------------------------------------------------------------------
/**
 *  Initialize a FAT volume.
 *
 * \param[in] dev The SD card where the volume is located.
 *
 * \param[in] part The partition to be used.  Legal values for \a part are
 * 1-4 to use the corresponding partition on a device formatted with
 * a MBR, Master Boot Record, or zero if the device is formatted as
 * a super floppy with the FAT boot sector in block zero.
 *
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.  Reasons for
 * failure include not finding a valid partition, not finding a valid
 *  FAT file system in the specified partition or an I/O error.
 */
uint8_t SdVolume::init(Sd2Card &dev, uint8_t part)
{
  uint32_t volumeStartBlock = 0;
  sdCard_ = &dev;
  // if part == 0 assume super floppy with FAT boot sector in block zero
  // if part > 0 assume mbr volume with partition table
  if (part) {
    if (part > 4)return false;
    if (!cacheRawBlock(volumeStartBlock)) return false;
    part_t *p = &cacheBuffer_.mbr.part[part-1];
    if ((p->boot & 0X7F) !=0  ||
      p->totalSectors < 100 ||
      p->firstSector == 0) {
      //not a valid partition
      return false;
    }
    volumeStartBlock = p->firstSector;
  }
  if (!cacheRawBlock(volumeStartBlock)) return false;
  bpb_t *bpb = &cacheBuffer_.fbs.bpb;
  if (bpb->bytesPerSector != 512 ||
    bpb->fatCount == 0 ||
    bpb->reservedSectorCount == 0 ||
    bpb->sectorsPerCluster == 0) {
       // not valid FAT volume
      return false;
  }
  fatCount_ = bpb->fatCount;
  blocksPerCluster_ = bpb->sectorsPerCluster;
  clusterSizeShift_ = 0;
  while (blocksPerCluster_ != (1 << clusterSizeShift_)) {
    // error if not power of 2
    if (clusterSizeShift_++ > 7) return false;
  }
  blocksPerFat_ = bpb->sectorsPerFat16 ? 
                    bpb->sectorsPerFat16 : bpb->sectorsPerFat32;
  rootDirEntryCount_ = bpb->rootDirEntryCount;
  fatStartBlock_ = volumeStartBlock + bpb->reservedSectorCount;
  rootDirStart_ = fatStartBlock_ + bpb->fatCount*blocksPerFat_;
  dataStartBlock_ = rootDirStart_ + ((32*bpb->rootDirEntryCount + 511)/512);
  uint32_t totalBlocks = bpb->totalSectors16 ? 
                           bpb->totalSectors16 : bpb->totalSectors32;
  // total data blocks                        
  clusterCount_ = totalBlocks - (dataStartBlock_ - volumeStartBlock);
  // divide by cluster size
  clusterCount_ >>= clusterSizeShift_;
  if (clusterCount_ < 4085) {
    fatType_ = 12;
  }
  else if (clusterCount_ < 65525) {
    fatType_ = 16;
  }
  else {
    rootDirStart_ = bpb->fat32RootCluster;
    fatType_ = 32;
  }
  return true;
}
