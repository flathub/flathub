#include "custom_math.h"

guint32 c_pow (guint32 num,
               guint32 p)
{
  for (guint32 i = 1; i < p; i++) {
    num += num;
  }

  return num;
}
