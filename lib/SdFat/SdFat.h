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
#ifndef SdFat_h
#define SdFat_h
/** Arduino Print support if not zero */
#define SD_FAT_PRINT_SUPPORT 1
#include "Sd2Card.h"
#include "FatStructs.h"
#if SD_FAT_PRINT_SUPPORT
#include "Print.h"
#endif // SD_FAT_PRINT_SUPPORT
//==============================================================================
// SdVolume class

union cache_t {
           /** Used to access cached file data blocks. */
  uint8_t  data[512];
           /** Used to access cached FAT16 entries. */
  uint16_t fat16[256];
           /** Used to access cached FAT32 entries. */
  uint32_t fat32[128];  
           /** Used to access cached directory entries. */
  dir_t    dir[16];
           /** Used to access a cached MasterBoot Record. */
  mbr_t    mbr;
           /** Used to access to a cached FAT boot sector. */
  fbs_t    fbs;       
};
#define CACHE_FOR_WRITE  1 
/**
 * \class SdVolume
 * \brief Access FAT16 and FAT32 volumes on SD and SDHC cards.
 */
class SdVolume {
  /** Allow SdFile access to SdVolume private data. */
  friend class SdFile;
//
  static cache_t cacheBuffer_;       // 512 byte cache for storage device blocks
  static uint32_t cacheBlockNumber_; // Logical number of block in the cache
  static Sd2Card *sdCard_;           // blockDevice for cache
  static uint8_t cacheDirty_;        // cacheFlush() will write block if true
  static uint32_t cacheMirrorBlock_; // block number for mirror FAT
//
  uint8_t blocksPerCluster_;
  uint32_t blocksPerFat_;
  uint32_t clusterCount_;
  uint8_t  clusterSizeShift_;
  uint32_t dataStartBlock_;
  uint8_t fatCount_;
  uint32_t fatStartBlock_;
  uint8_t fatType_;
  uint16_t rootDirEntryCount_;
  uint32_t rootDirStart_;
  //----------------------------------------------------------------------------
  uint8_t allocContiguous(uint32_t &curCluster, uint32_t count);
  uint8_t blockOfCluster(uint32_t position) {
          return (position >> 9) & (blocksPerCluster_ - 1);}
  uint32_t clusterStartBlock(uint32_t cluster) {
           return dataStartBlock_ + ((cluster - 2) << clusterSizeShift_);}
  uint32_t blockNumber(uint32_t cluster, uint32_t position) {
           return clusterStartBlock(cluster) + blockOfCluster(position);}
  static uint8_t cacheFlush(void);
  static uint8_t cacheRawBlock(uint32_t blockNumber, uint8_t action = 0);
  static void cacheSetDirty(void) {cacheDirty_ |= CACHE_FOR_WRITE;}
  static uint8_t cacheZeroBlock(uint32_t blockNumber);
  uint8_t chainSize(uint32_t &size, uint32_t beginCluster);
  uint8_t fatGet(uint32_t cluster, uint32_t &value);
  uint8_t fatPut(uint32_t cluster, uint32_t value);
  uint8_t fatPutEOC(uint32_t cluster) {return fatPut(cluster, 0X0FFFFFF8);}
  uint8_t findContiguous(uint32_t &start, uint32_t count);
  uint8_t freeChain(uint32_t cluster);
  uint8_t isEOC(uint32_t cluster)
      {return  cluster >= (fatType_ == 16 ? 0XFFF8 : 0X0FFFFFF8);}
  uint8_t readBlock(uint32_t block, uint8_t *dst) {
    return sdCard_->readBlock(block, dst);}
  uint8_t readData(uint32_t block, uint16_t offset, uint8_t *dst, uint16_t count)
      {return sdCard_->readData(block, offset, dst, count);}
  uint8_t writeBlock(uint32_t block, uint8_t *dst)
      {return sdCard_->writeBlock(block, dst);}   
public:
   /** Clears the cache and returns a pointer to the cache.  Used by the WaveRP
    *  recorder to do raw write to the SD card.  Not for normal apps.
    */       
   static uint8_t *cacheClear(void)
      {cacheFlush(); cacheBlockNumber_ = 0XFFFFFFFF; return cacheBuffer_.data;}
  /** Create an instance of SdVolume */
  SdVolume(void) : fatType_(0){}
  /**
   * Initialize a FAT volume.  Try partition one first then try super
   * floppy format.
   *
   * \param[in] dev The Sd2Card where the volume is located.
   *
   * \return The value one, true, is returned for success and
   * the value zero, false, is returned for failure.  Reasons for
   * failure include not finding a valid partition, not finding a valid
   * FAT file system or an I/O error.
   */
  uint8_t init(Sd2Card &dev) { return init(dev, 1) ? 1 : init(dev, 0);}
  uint8_t init(Sd2Card &dev, uint8_t vol);

  // inline functions that return volume info
  /** \return The volume's cluster size in blocks. */   
  uint8_t blocksPerCluster(void) {return blocksPerCluster_;}
  /** \return The number of blocks in one FAT. */ 
  uint32_t blocksPerFat(void)  {return blocksPerFat_;}
  /** \return The total number of clusters in the volume. */ 
  uint32_t clusterCount(void) {return clusterCount_;}
  /** \return The shift count required to multiply by blocksPerCluster. */
  uint8_t clusterSizeShift(void) {return clusterSizeShift_;}
  /** \return The logical block number for the start of file data. */ 
  uint32_t dataStartBlock(void) {return dataStartBlock_;}
  /** \return The number of FAT structures on the volume. */
  uint8_t fatCount(void) {return fatCount_;}
  /** \return The logical block number for the start of the first FAT. */
  uint32_t fatStartBlock(void) {return fatStartBlock_;}
  /** \return The FAT type of the volume. Values are 12, 16 or 32. */
  uint8_t fatType(void) {return fatType_;}
  /** \return The number of entries in the root directory for FAT16 volumes. */
  uint32_t rootDirEntryCount(void) {return rootDirEntryCount_;}
  /** \return The logical block number for the start of the root directory
       on FAT16 volumes or the first cluster number on FAT32 volumes. */
  uint32_t rootDirStart(void) {return rootDirStart_;}
};
//==============================================================================
// SdFile class
// unused attribute bit - use for dir entry needs to be updated on SD card
#define ATTR_FILE_DIR_DIRTY 0X80
// unused attribute bit - use to indicate desire for unbuffered SD read
#define ATTR_FILE_UNBUFFERED_READ 0X40
/** Directory entry is part of a long name */
#define DIR_IS_LONG_NAME(dir)\
           (((dir).attributes & DIR_ATT_LONG_NAME_MASK) == DIR_ATT_LONG_NAME)
/** Mask for file/subdirectory tests */
#define DIR_ATT_FILE_TYPE_MASK (DIR_ATT_VOLUME_ID | DIR_ATT_DIRECTORY)
/** Directory entry is for a file */
#define DIR_IS_FILE(dir) (((dir).attributes & DIR_ATT_FILE_TYPE_MASK) == 0)
/** Directory entry is for a subdirectory */
#define DIR_IS_SUBDIR(dir)\
            (((dir).attributes & DIR_ATT_FILE_TYPE_MASK) == DIR_ATT_DIRECTORY)
//------------------------------------------------------------------------------
/**
 * \class SdFile
 * \brief Access FAT16 and FAT32 files on SD and SDHC cards.
 */
#if SD_FAT_PRINT_SUPPORT
class SdFile : public Print {
#else //SD_FAT_PRINT_SUPPORT
class SdFile {
#endif //SD_FAT_PRINT_SUPPORT
// values for type_
/** This SdFile has not been opened. */
#define FAT_FILE_TYPE_CLOSED 0
/** SdFile for a file */
#define FAT_FILE_TYPE_NORMAL 1
/** SdFile for a FAT16 root directory */
#define FAT_FILE_TYPE_ROOT16 2
/** SdFile for a FAT32 root directory */
#define FAT_FILE_TYPE_ROOT32 3
/** SdFile for a subdirectory */
#define FAT_FILE_TYPE_SUBDIR 4
/** Test value for directory type */
#define FAT_FILE_TYPE_MIN_DIR FAT_FILE_TYPE_ROOT16

  uint8_t attributes_;
  uint8_t type_;  
  uint32_t curCluster_;  
  uint32_t curPosition_;
  uint32_t dirBlock_;
  uint8_t  dirIndex_;  
  uint32_t fileSize_;  
  uint32_t firstCluster_;
  SdVolume *vol_;
  uint8_t addCluster(void);
public:
/** Create an instance of SdFile. */
  SdFile(void) : type_(FAT_FILE_TYPE_CLOSED) {}
  /** Used to check for print errors. Clear before a sequence of prints
   *  to a file.  Non-zero after the prints indicates that an error occurred.
   */   
  uint8_t writeByteError;
  /** Close this instance of SdFile. */
  uint8_t close(void) {if (!sync())return false; type_ = 0; return true;}
  uint8_t contiguousRange(uint32_t &bgnBlock, uint32_t &endBlock);  
  uint8_t create(SdFile &dirFile, char *fileName);
  uint8_t createContiguous(SdFile &dirFile, char *fileName, uint32_t size);
  static void dirName(dir_t &dir, char *name);
  /** \return The current position for a file or directory. */
  uint32_t curPosition(void) {return curPosition_;}
  /** \return The total number of bytes in a file or directory. */
  uint32_t fileSize(void) {return fileSize_;}
  /** \return True if this is a SdFile for a directory else false. */
  uint8_t isDir(void) {return type_ >= FAT_FILE_TYPE_MIN_DIR;}
  /** \return True if this is a SdFile for a file else false. */
  uint8_t isFile(void) {return type_ == FAT_FILE_TYPE_NORMAL;}
  /** \return True if this is a SdFile for an open file/directory else false. */
  uint8_t isOpen(void) {return type_ != FAT_FILE_TYPE_CLOSED;}
  /** \return True if this is a read only file else false. */
  uint8_t isReadOnly(void) {return (attributes_ & DIR_ATT_READ_ONLY) != 0;}
  uint8_t openRoot(SdVolume &vol);
  uint8_t open(SdFile &dirFile, dir_t &entry);
  uint8_t open(SdFile &dirFile, char *fileName);
  int16_t read(uint8_t *dst, uint16_t count);
  /**
 * Read the next byte from a file.
 *  
 * \return For success read returns the next byte in the file as an int.
 * If an error occurs or end of file is reached -1 is returned.
 */ 
  int16_t read(void)
    {uint8_t b; return read(&b, 1) == 1 ? b : -1;}
  int8_t readDir(dir_t &dir);
  uint8_t remove(void);
  static uint8_t remove(SdFile &dirFile, char *fileName); 
  /** Sets the file's current position to zero. */ 
  void rewind(void) {curPosition_ = curCluster_ = 0;}
  /** Sets the files position to current position + \a pos. See seekSet(). */
  uint8_t seekCur(uint32_t pos) {return seekSet(curPosition_ + pos);}
  /** Sets the files current position to end of file.  Useful to position
   *  a file for append. See seekSet(). */  
  uint8_t seekEnd(void) {return seekSet(fileSize_);}
  uint8_t seekSet(uint32_t pos);
  uint8_t sync(void);
  uint8_t truncate(uint32_t size);
  void write(uint8_t b);
  int16_t write(uint8_t *src, uint16_t count);
  //----------------------------------------------------------------------------
  //inline functions mainly for debug
  /** Cancel unbuffered reads for this file.
   *  Not recommended for normal applications.  
   */
  void clearUnbufferedRead(void) 
    {attributes_ &= ~ATTR_FILE_UNBUFFERED_READ;}  
  /** \return Index of this file's directory in the block dirBlock. */
  uint8_t dirIndex(void)  {return dirIndex_;}
  /** \return Address of the block that contains this file's directory. */
  uint32_t dirBlock(void) {return dirBlock_;}
  /** \return The first cluster number for a file or directory. */
  uint32_t firstCluster(void) {return firstCluster_;}
  /** \return The current cluster number for a file or directory. */
  uint32_t curCluster(void) {return curCluster_;}
  /** Use unbuffered for reads to this file.  Used with Wave Shield ISR.
   *  Not recommended for normal applications.  
   */
  void setUnbufferedRead(void) 
    {if(isFile()) attributes_ |= ATTR_FILE_UNBUFFERED_READ;}
   /** \return SdCard object for this file. 
   *  Not recommended for normal applications.  
   */   
  Sd2Card *sdCard(void) {return vol_->sdCard_;}  
  /** Type of this SdFile.  You should use isFile() or isDir() instead of type()
   * if possible.
   *
   * \return The file or directory type. */
  uint8_t type(void) {return type_;}
  /** \return Unbuffered read attribute. */  
  uint8_t unbufferedRead(void) 
    {return attributes_ & ATTR_FILE_UNBUFFERED_READ;}
  /** \return SdVolume that contains this file. */
  SdVolume *volume(void) {return vol_;}
};
#endif // SdFat_h
