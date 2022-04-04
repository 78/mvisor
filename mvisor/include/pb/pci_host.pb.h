// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: pci_host.proto

#ifndef PROTOBUF_pci_5fhost_2eproto__INCLUDED
#define PROTOBUF_pci_5fhost_2eproto__INCLUDED

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 3005000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 3005000 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_table_driven.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/metadata.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)

namespace protobuf_pci_5fhost_2eproto {
// Internal implementation detail -- do not use these members.
struct TableStruct {
  static const ::google::protobuf::internal::ParseTableField entries[];
  static const ::google::protobuf::internal::AuxillaryParseTableField aux[];
  static const ::google::protobuf::internal::ParseTable schema[1];
  static const ::google::protobuf::internal::FieldMetadata field_metadata[];
  static const ::google::protobuf::internal::SerializationTable serialization_table[];
  static const ::google::protobuf::uint32 offsets[];
};
void AddDescriptors();
void InitDefaultsMchStateImpl();
void InitDefaultsMchState();
inline void InitDefaults() {
  InitDefaultsMchState();
}
}  // namespace protobuf_pci_5fhost_2eproto
class MchState;
class MchStateDefaultTypeInternal;
extern MchStateDefaultTypeInternal _MchState_default_instance_;

// ===================================================================

class MchState : public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:MchState) */ {
 public:
  MchState();
  virtual ~MchState();

  MchState(const MchState& from);

  inline MchState& operator=(const MchState& from) {
    CopyFrom(from);
    return *this;
  }
  #if LANG_CXX11
  MchState(MchState&& from) noexcept
    : MchState() {
    *this = ::std::move(from);
  }

  inline MchState& operator=(MchState&& from) noexcept {
    if (GetArenaNoVirtual() == from.GetArenaNoVirtual()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }
  #endif
  static const ::google::protobuf::Descriptor* descriptor();
  static const MchState& default_instance();

  static void InitAsDefaultInstance();  // FOR INTERNAL USE ONLY
  static inline const MchState* internal_default_instance() {
    return reinterpret_cast<const MchState*>(
               &_MchState_default_instance_);
  }
  static PROTOBUF_CONSTEXPR int const kIndexInFileMessages =
    0;

  void Swap(MchState* other);
  friend void swap(MchState& a, MchState& b) {
    a.Swap(&b);
  }

  // implements Message ----------------------------------------------

  inline MchState* New() const PROTOBUF_FINAL { return New(NULL); }

  MchState* New(::google::protobuf::Arena* arena) const PROTOBUF_FINAL;
  void CopyFrom(const ::google::protobuf::Message& from) PROTOBUF_FINAL;
  void MergeFrom(const ::google::protobuf::Message& from) PROTOBUF_FINAL;
  void CopyFrom(const MchState& from);
  void MergeFrom(const MchState& from);
  void Clear() PROTOBUF_FINAL;
  bool IsInitialized() const PROTOBUF_FINAL;

  size_t ByteSizeLong() const PROTOBUF_FINAL;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input) PROTOBUF_FINAL;
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const PROTOBUF_FINAL;
  ::google::protobuf::uint8* InternalSerializeWithCachedSizesToArray(
      bool deterministic, ::google::protobuf::uint8* target) const PROTOBUF_FINAL;
  int GetCachedSize() const PROTOBUF_FINAL { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const PROTOBUF_FINAL;
  void InternalSwap(MchState* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return NULL;
  }
  inline void* MaybeArenaPtr() const {
    return NULL;
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const PROTOBUF_FINAL;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // uint32 config = 1;
  void clear_config();
  static const int kConfigFieldNumber = 1;
  ::google::protobuf::uint32 config() const;
  void set_config(::google::protobuf::uint32 value);

  // @@protoc_insertion_point(class_scope:MchState)
 private:

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  ::google::protobuf::uint32 config_;
  mutable int _cached_size_;
  friend struct ::protobuf_pci_5fhost_2eproto::TableStruct;
  friend void ::protobuf_pci_5fhost_2eproto::InitDefaultsMchStateImpl();
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// MchState

// uint32 config = 1;
inline void MchState::clear_config() {
  config_ = 0u;
}
inline ::google::protobuf::uint32 MchState::config() const {
  // @@protoc_insertion_point(field_get:MchState.config)
  return config_;
}
inline void MchState::set_config(::google::protobuf::uint32 value) {
  
  config_ = value;
  // @@protoc_insertion_point(field_set:MchState.config)
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__

// @@protoc_insertion_point(namespace_scope)


// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_pci_5fhost_2eproto__INCLUDED