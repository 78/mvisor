/*
 * MVisor
 * fuse.c is based on libfuse3
 * Copyright (C) 2022 cair <rui.cai@tenclass.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "fuse.h"

Fuse *Fuse::instance_ = nullptr;

Fuse::Fuse(std::string &mount_path) {
  // start fuse init process
  lo_data_ = (struct lo_data *)malloc(sizeof(struct lo_data));
  bzero(lo_data_, sizeof(struct lo_data));
  lo_data_->cache = CACHE_NORMAL;
  lo_data_->timeout = 1.0;
  lo_data_->writeback = 1;
  lo_data_->debug = 0;

  // set mount path from config
  mount_path += '\0';
  lo_data_->source = (const char *)malloc(mount_path.length());
  memcpy((void *)lo_data_->source, mount_path.c_str(), mount_path.length());

  // check mount path must be director
  struct stat stat;
  bzero(&stat, sizeof(stat));
  MV_ASSERT(lstat(lo_data_->source, &stat) != -1);
  MV_ASSERT(S_ISDIR(stat.st_mode));

  // open root dir
  lo_data_->root = (struct lo_inode *)calloc(1, sizeof(struct lo_inode));
  bzero(lo_data_->root, sizeof(struct lo_inode));

  lo_data_->root->refcount = 1;
  lo_data_->root->next = lo_data_->root->prev = lo_data_->root;
  lo_data_->root->ino = stat.st_ino;
  lo_data_->root->dev = stat.st_dev;
  lo_data_->root->fd = open(lo_data_->source, O_PATH);
  MV_ASSERT(lo_data_->root->fd != -1);
  inode_list_.push_front(lo_data_->root);
}

Fuse::~Fuse() {
  for (auto inode : inode_list_) {
    close(inode->fd);
    free(inode);
  }

  if (lo_data_->source) {
    free((void *)lo_data_->source);
    lo_data_->source = nullptr;
  }

  if (lo_data_) {
    free(lo_data_);
    lo_data_ = nullptr;
  }
}

struct lo_inode *Fuse::GetLowLevelInode(fuse_ino_t inode) {
  if (inode == FUSE_ROOT_ID) {
    return lo_data_->root;
  }
  auto iter =
      std::find(inode_list_.begin(), inode_list_.end(), (struct lo_inode *)(uintptr_t)inode);
  if (iter != inode_list_.end()) {
    return *iter;
  } else {
    return nullptr;
  }
}

int Fuse::LowLevelFd(fuse_ino_t ino) {
  lo_inode *inode = GetLowLevelInode(ino);
  if (inode == nullptr) {
    return -1;
  }
  return inode->fd;
}

struct lo_inode *Fuse::LowLevelFind(struct stat *stat) {
  auto inode = std::find_if(inode_list_.begin(), inode_list_.end(), [=](lo_inode *inode) -> bool {
    return inode->dev == stat->st_dev && inode->ino == stat->st_ino;
  });

  if (inode == inode_list_.end()) {
    return nullptr;
  } else {
    (*inode)->refcount++;
    return *inode;
  }
}

void Fuse::ConvertStatfs(const struct statvfs *stbuf, struct fuse_kstatfs *kstatfs) {
  kstatfs->bsize = stbuf->f_bsize;
  kstatfs->frsize = stbuf->f_frsize;
  kstatfs->blocks = stbuf->f_blocks;
  kstatfs->bfree = stbuf->f_bfree;
  kstatfs->bavail = stbuf->f_bavail;
  kstatfs->files = stbuf->f_files;
  kstatfs->ffree = stbuf->f_ffree;
  kstatfs->namelen = stbuf->f_namemax;
}

int Fuse::LowLevelLookup(fuse_ino_t parent, const char *name,
                         struct fuse_entry_param *entry_param) {
  auto lo_data = get_lo_data();
  bzero(entry_param, sizeof(fuse_entry_param));
  entry_param->attr_timeout = lo_data->timeout;
  entry_param->entry_timeout = lo_data->timeout;

  int new_fd = openat(LowLevelFd(parent), name, O_PATH | O_NOFOLLOW);
  if (new_fd == -1) {
    return errno;
  }

  fstatat(new_fd, "", &entry_param->attr, AT_EMPTY_PATH | AT_SYMLINK_NOFOLLOW);
  MV_ASSERT(new_fd != -1);

  lo_inode *inode = LowLevelFind(&entry_param->attr);
  if (inode) {
    close(new_fd);
  } else {
    inode = (struct lo_inode *)calloc(1, sizeof(struct lo_inode));
    MV_ASSERT(inode != nullptr);
    inode->refcount = 1;
    inode->fd = new_fd;
    inode->ino = entry_param->attr.st_ino;
    inode->dev = entry_param->attr.st_dev;
    inode_list_.push_front(inode);
  }
  entry_param->ino = (uintptr_t)inode;
  return 0;
}

void Fuse::ConvertStat(const struct stat *stbuf, struct fuse_attr *attr) {
  attr->ino = stbuf->st_ino;
  attr->mode = stbuf->st_mode;
  attr->nlink = stbuf->st_nlink;
  attr->uid = stbuf->st_uid;
  attr->gid = stbuf->st_gid;
  attr->rdev = stbuf->st_rdev;
  attr->size = stbuf->st_size;
  attr->blksize = stbuf->st_blksize;
  attr->blocks = stbuf->st_blocks;
  attr->atime = stbuf->st_atime;
  attr->mtime = stbuf->st_mtime;
  attr->ctime = stbuf->st_ctime;
  attr->atimensec = 0;
  attr->mtimensec = 0;
  attr->ctimensec = 0;
}

int Fuse::MakeNodeWrapper(int dirfd, const char *path, const char *link, int mode, dev_t rdev) {
  int res;

  if (S_ISREG(mode)) {
    res = openat(dirfd, path, O_CREAT | O_EXCL | O_WRONLY, mode);
    if (res >= 0) res = close(res);
  } else if (S_ISDIR(mode)) {
    res = mkdirat(dirfd, path, mode);
  } else if (S_ISLNK(mode) && link != NULL) {
    res = symlinkat(link, dirfd, path);
  } else if (S_ISFIFO(mode)) {
    res = mkfifoat(dirfd, path, mode);
  } else {
    res = mknodat(dirfd, path, mode, rdev);
  }

  return res;
}

void Fuse::UnrefInode(struct lo_data *lo, struct lo_inode *inode, uint64_t n) {
  if (!inode) {
    return;
  }

  MV_ASSERT(inode->refcount >= n);
  inode->refcount -= n;
  if (!inode->refcount) {
    inode_list_.remove(inode);
    close(inode->fd);
    free(inode);
    inode = nullptr;
  }
}

void Fuse::LowLevelForgetOne(fuse_ino_t ino, uint64_t nlookup) {
  auto lo_data = get_lo_data();
  auto inode = GetLowLevelInode(ino);
  if (lo_data && inode) {
    UnrefInode(lo_data, inode, nlookup);
  }
}

void Fuse::FuseFillEntry(struct fuse_entry_out *arg, const struct fuse_entry_param *entry_param) {
  arg->nodeid = entry_param->ino;
  arg->generation = entry_param->generation;
  arg->entry_valid = CalcTimeoutSecond(entry_param->entry_timeout);
  arg->entry_valid_nsec = CalcTimeoutNsecond(entry_param->entry_timeout);
  arg->attr_valid = CalcTimeoutSecond(entry_param->attr_timeout);
  arg->attr_valid_nsec = CalcTimeoutNsecond(entry_param->attr_timeout);
  ConvertStat(&entry_param->attr, &arg->attr);
}

/* `buf` is allowed to be empty so that the proper size may be
   allocated by the caller */
size_t Fuse::FuseAddDirentryPlus(char *buf, size_t buf_size, const char *name,
                                 const struct fuse_entry_param *entry_param, off_t off) {
  size_t name_length = strlen(name);
  size_t entry_length = FUSE_NAME_OFFSET_DIRENTPLUS + name_length;
  size_t entry_length_padded = FUSE_DIRENT_ALIGN(entry_length);
  if ((buf == nullptr) || (entry_length_padded > buf_size)) {
    return entry_length_padded;
  }

  struct fuse_direntplus *dp = (struct fuse_direntplus *)buf;
  memset(&dp->entry_out, 0, sizeof(dp->entry_out));
  FuseFillEntry(&dp->entry_out, entry_param);

  struct fuse_dirent *dirent = &dp->dirent;
  dirent->ino = entry_param->attr.st_ino;
  dirent->off = off;
  dirent->namelen = name_length;
  dirent->type = (entry_param->attr.st_mode & S_IFMT) >> 12;
  memcpy(dirent->name, name, name_length);
  memset(dirent->name + name_length, 0, entry_length_padded - entry_length);
  return entry_length_padded;
}

/* `buf` is allowed to be empty so that the proper size may be
   allocated by the caller */
size_t Fuse::FuseAddDirentry(char *buf, size_t buf_size, const char *name, const struct stat *stat,
                             off_t off) {
  size_t name_length = strlen(name);
  size_t entry_length = FUSE_NAME_OFFSET + name_length;
  size_t enttry_length_padded = FUSE_DIRENT_ALIGN(entry_length);
  if ((buf == nullptr) || (enttry_length_padded > buf_size)) {
    return enttry_length_padded;
  }

  struct fuse_dirent *dirent;
  dirent = (struct fuse_dirent *)buf;
  dirent->ino = stat->st_ino;
  dirent->off = off;
  dirent->namelen = name_length;
  dirent->type = (stat->st_mode & S_IFMT) >> 12;
  memcpy(dirent->name, name, name_length);
  memset(dirent->name + name_length, 0, enttry_length_padded - entry_length);
  return enttry_length_padded;
}

bool Fuse::LowLevelReadDir(fuse_in_header *request, char *result_buf, uint32_t *remain_size,
                           bool is_plus) {
  auto in = (fuse_read_in *)OFFSET(request);
  *remain_size = in->size;

  auto dirp = (lo_dirp *)in->fh;
  if (in->offset != (uint64_t)dirp->offset) {
    seekdir(dirp->dp, in->offset);
    dirp->entry = nullptr;
    dirp->offset = in->offset;
  }

  // get result in loop
  bool lookup_err = false;
  auto result_buf_pointer = result_buf;
  while (true) {
    uint64_t entry_size;
    off_t next_off;
    const char *name;

    if (!dirp->entry) {
      dirp->entry = readdir(dirp->dp);
      if (!dirp->entry) {
        break;
      }
    }

    next_off = dirp->entry->d_off;
    name = dirp->entry->d_name;
    fuse_ino_t entry_ino = 0;

    if (is_plus) {
      struct fuse_entry_param entry_param = {0};
      if (IsDotOrDotdot(name)) {
        entry_param.attr.st_ino = dirp->entry->d_ino;
        entry_param.attr.st_mode = dirp->entry->d_type << 12;
      } else {
        LowLevelLookup(request->nodeid, name, &entry_param);
        if (entry_param.ino == 0) {
          lookup_err = true;
          break;
        }
        entry_ino = entry_param.ino;
      }
      entry_size =
          FuseAddDirentryPlus(result_buf_pointer, *remain_size, name, &entry_param, next_off);
    } else {
      struct stat stat = {0};
      stat.st_ino = dirp->entry->d_ino;
      stat.st_mode = (mode_t)dirp->entry->d_type << 12,
      entry_size = FuseAddDirentry(result_buf_pointer, *remain_size, name, &stat, next_off);
    }

    if (entry_size > *remain_size) {
      if (entry_ino != 0) {
        LowLevelForgetOne(entry_ino, 1);
      }
      break;
    }

    result_buf_pointer += entry_size;
    *remain_size -= entry_size;
    dirp->entry = nullptr;
    dirp->offset = next_off;
  }

  return !lookup_err;
}

size_t Fuse::FuseBufSize(const struct fuse_bufvec *buf_vec) {
  size_t index;
  size_t size = 0;

  for (index = 0; index < buf_vec->count; index++) {
    if (buf_vec->buf[index].size == SIZE_MAX) {
      size = SIZE_MAX;
      break;
    } else if (SIZE_MAX - buf_vec->buf[index].size < size) {
      size = SIZE_MAX;
      break;
    } else {
      size += buf_vec->buf[index].size;
    }
  }
  return size;
}

ssize_t Fuse::FuseBufWrite(const struct fuse_buf *dst_buf, size_t dst_offset,
                           const struct fuse_buf *src_buf, size_t src_offset, size_t length) {
  ssize_t res = 0;
  size_t copied = 0;

  while (length) {
    if (dst_buf->flags & FUSE_BUF_FD_SEEK) {
      res =
          pwrite(dst_buf->fd, (char *)src_buf->mem + src_offset, length, dst_buf->pos + dst_offset);
    } else {
      res = write(dst_buf->fd, (char *)src_buf->mem + src_offset, length);
    }
    if (res == -1) {
      if (!copied) {
        return -errno;
      }
      break;
    }

    if (res == 0) {
      break;
    }
    copied += res;
    if (!(dst_buf->flags & FUSE_BUF_FD_RETRY)) {
      break;
    }
    src_offset += res;
    dst_offset += res;
    length -= res;
  }

  return copied;
}

ssize_t Fuse::FuseBufRead(const struct fuse_buf *dst_buf, size_t dst_offset,
                          const struct fuse_buf *src_buf, size_t src_offset, size_t length) {
  ssize_t res = 0;
  size_t copied = 0;

  while (length) {
    if (src_buf->flags & FUSE_BUF_FD_SEEK) {
      res =
          pread(src_buf->fd, (char *)dst_buf->mem + dst_offset, length, src_buf->pos + src_offset);
    } else {
      res = read(src_buf->fd, (char *)dst_buf->mem + dst_offset, length);
    }
    if (res == -1) {
      if (!copied) {
        return -errno;
      }
      break;
    }

    if (res == 0) {
      break;
    }
    copied += res;
    if (!(src_buf->flags & FUSE_BUF_FD_RETRY)) {
      break;
    }
    dst_offset += res;
    src_offset += res;
    length -= res;
  }

  return copied;
}

ssize_t Fuse::FuseBufCopyOne(const struct fuse_buf *dst, size_t dst_offset,
                             const struct fuse_buf *src, size_t src_offset, size_t length,
                             enum fuse_buf_copy_flags flags) {
  if (src->flags & FUSE_BUF_IS_FD) {
    return FuseBufRead(dst, dst_offset, src, src_offset, length);
  }

  if (dst->flags & FUSE_BUF_IS_FD) {
    return FuseBufWrite(dst, dst_offset, src, src_offset, length);
  }

  MV_PANIC("not implement this kind of copy operation");
  return 0;
}

int Fuse::FuseBufvecAdvance(struct fuse_bufvec *buf_vec, size_t length) {
  const struct fuse_buf *buf = FuseBufvecCurrent(buf_vec);
  if (!buf) {
    return 0;
  }

  buf_vec->off += length;
  MV_ASSERT(buf_vec->off <= buf->size);
  if (buf_vec->off == buf->size) {
    MV_ASSERT(buf_vec->idx < buf_vec->count);
    buf_vec->idx++;
    if (buf_vec->idx == buf_vec->count) {
      return 0;
    }
    buf_vec->off = 0;
  }
  return 1;
}

ssize_t Fuse::FuseBufCopy(struct fuse_bufvec *dst_buf_vec, struct fuse_bufvec *src_buf_vec,
                          enum fuse_buf_copy_flags flags) {
  if (dst_buf_vec == src_buf_vec) {
    return FuseBufSize(dst_buf_vec);
  }

  ssize_t res = -1;
  size_t copied = 0;
  size_t src_buf_length = 0;
  size_t dst_buf_length = 0;
  size_t length = 0;
  for (;;) {
    const struct fuse_buf *src_buf = FuseBufvecCurrent(src_buf_vec);
    const struct fuse_buf *dst_buf = FuseBufvecCurrent(dst_buf_vec);
    if (src_buf == nullptr || dst_buf == nullptr) {
      break;
    }

    src_buf_length = src_buf->size - src_buf_vec->off;
    dst_buf_length = dst_buf->size - dst_buf_vec->off;
    length = std::min(src_buf_length, dst_buf_length);
    res = FuseBufCopyOne(dst_buf, dst_buf_vec->off, src_buf, src_buf_vec->off, length, flags);
    if (res < 0) {
      if (!copied) {
        return res;
      }
      break;
    }
    copied += res;

    if (!FuseBufvecAdvance(src_buf_vec, res) || !FuseBufvecAdvance(dst_buf_vec, res)) break;

    if (res < (ssize_t)length) {
      break;
    }
  }

  return copied;
}
