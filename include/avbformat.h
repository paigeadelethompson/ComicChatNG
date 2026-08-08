#pragma once

#include <QtGlobal>
#include <cstdint>

#pragma pack(push, 1)

using AvbInt8 = std::uint8_t;
using AvbInt16 = std::uint16_t;
using AvbInt32 = std::uint32_t;

constexpr AvbInt16 AF_MAGICNUM = 0x81;
constexpr AvbInt16 AF_MAGICNUM_NEW = 0x8181;

constexpr AvbInt16 AT_SIMPLE = 1;
constexpr AvbInt16 AT_COMPLEX = 2;
constexpr AvbInt16 AT_BACKDROP = 3;

constexpr AvbInt16 AVATAR_CURRENT_VERSION = 2;
constexpr AvbInt16 INVALID_POSE_ID = 0;
constexpr int MAX_COMPRESS_BUFFER_SIZE = 2048 * 1024;
constexpr int MAX_PALETTE_SIZE = 2048;

enum AvbImageFormat : AvbInt8 {
  AIF_DIB = 0,
  AIF_LZDEFLATE = 1,
};

enum AvbImagePalette : AvbInt8 {
  AIP_NOPALETTE = 0,
  AIP_GLOBALPALETTE = 1,
  AIP_LOCALPALETTE = 2,
  AIP_MONOCHROME = 3,
  AIP_MASKEDMONO = 4,
  AIP_DUALMASK = 5,
};

struct AvbHeader {
  AvbInt16 magic = 0;
  AvbInt16 type = 0;
  AvbInt16 version = 0;
};

struct AvbIconData {
  AvbInt32 offset = 0;
  AvbInt8 format = 0;
  AvbInt8 palette = 0;
};

struct AvbBodyDataNew {
  AvbInt32 imageOffset = 0;
  AvbInt32 maskOffset = 0;
  AvbInt32 auraOffset = 0;
  AvbInt16 emotion = 0;
  AvbInt8 intensity = 0;
  AvbInt16 x = 0;
  AvbInt16 y = 0;
  AvbInt8 imageFormat = 0;
  AvbInt8 maskFormat = 0;
  AvbInt8 auraFormat = 0;
  AvbInt8 imagePalette = 0;
  AvbInt8 maskPalette = 0;
  AvbInt8 auraPalette = 0;
};

struct AvbBodyDataOld {
  AvbInt32 imageOffset = 0;
  AvbInt32 maskOffset = 0;
  AvbInt32 auraOffset = 0;
  AvbInt16 emotion = 0;
  AvbInt8 intensity = 0;
  AvbInt16 x = 0;
  AvbInt16 y = 0;
  AvbInt8 padding[16] = {};
};

struct AvbFaceDataNew {
  AvbInt32 imageOffset = 0;
  AvbInt32 maskOffset = 0;
  AvbInt32 auraOffset = 0;
  AvbInt16 emotion = 0;
  AvbInt8 intensity = 0;
  AvbInt16 cx = 0;
  AvbInt16 cy = 0;
  AvbInt16 cxDelta = 0;
  AvbInt16 cyDelta = 0;
  AvbInt16 x = 0;
  AvbInt16 y = 0;
  AvbInt8 imageFormat = 0;
  AvbInt8 maskFormat = 0;
  AvbInt8 auraFormat = 0;
  AvbInt8 imagePalette = 0;
  AvbInt8 maskPalette = 0;
  AvbInt8 auraPalette = 0;
};

struct AvbFaceDataOld {
  AvbInt32 imageOffset = 0;
  AvbInt32 maskOffset = 0;
  AvbInt32 auraOffset = 0;
  AvbInt16 emotion = 0;
  AvbInt8 intensity = 0;
  AvbInt16 cx = 0;
  AvbInt16 cy = 0;
  AvbInt16 cxDelta = 0;
  AvbInt16 cyDelta = 0;
  AvbInt16 x = 0;
  AvbInt16 y = 0;
  AvbInt8 padding[16] = {};
};

struct AvbTorsoDataNew {
  AvbInt32 imageOffset = 0;
  AvbInt32 maskOffset = 0;
  AvbInt32 auraOffset = 0;
  AvbInt16 emotion = 0;
  AvbInt8 intensity = 0;
  AvbInt16 cx = 0;
  AvbInt16 cy = 0;
  AvbInt8 imageFormat = 0;
  AvbInt8 maskFormat = 0;
  AvbInt8 auraFormat = 0;
  AvbInt8 imagePalette = 0;
  AvbInt8 maskPalette = 0;
  AvbInt8 auraPalette = 0;
};

struct AvbTorsoDataOld {
  AvbInt32 imageOffset = 0;
  AvbInt32 maskOffset = 0;
  AvbInt32 auraOffset = 0;
  AvbInt16 emotion = 0;
  AvbInt8 intensity = 0;
  AvbInt16 cx = 0;
  AvbInt16 cy = 0;
  AvbInt8 padding[16] = {};
};

enum AvbRecordType : AvbInt16 {
  AK_NAME = 1,
  AK_FLAGS = 2,
  AK_ICON = 3,
  AK_NFACES = 4,
  AK_NTORSOS = 5,
  AK_STARTDATA = 6,
  AK_ENDDATA = 7,
  AK_STYLE = 8,
  AK_NBODIES = 9,
  AK_NFACES2 = 10,
  AK_NTORSOS2 = 11,
  AK_NBODIES2 = 12,

  AK_ICON_NEW = 256,
  AK_COLORPALETTE = 257,
  AK_BACKDROP = 258,
  AK_COPYRIGHT = 259,
  AK_ORIGINAL_URL = 260,
  AK_OVERRIDE_URL = 261,
  AK_USAGE_FLAGS = 262,
  AK_OFFSET_ADJUSTMENT = 263,
};

#pragma pack(pop)

inline void adjustOffset(AvbInt32 &offset, qint32 by) {
  if (offset)
    offset = static_cast<AvbInt32>(static_cast<qint32>(offset) + by);
}
