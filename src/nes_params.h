#pragma once

typedef struct _NesParamPoint {
  gint32 blockx;
  gint32 blocky;
  gint32 x;
  gint32 y;
} NesParamPoint;

typedef struct _NesTilePoint {
  gint32 blockx;
  gint32 blocky;
  gint32 x;
  gint32 y;
  gint32 index;
} NesTilePoint;
