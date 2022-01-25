#!/bin/sh

mkdir -p /app/lib/codecs/lib/gstreamer-1.0
export GST_PLUGIN_SYSTEM_PATH=/app/lib/gstreamer-1.0
export LD_LIBRARY_PATH=/app/lib/codecs/lib/:/app/lib/

for i in /app/lib/gstreamer-1.0/*.so; do
  SUITE=`gst-inspect-1.0 $i | grep "Source module" | sed 's,Source module,,' | tr -d [:blank:]`
  if [ x$SUITE == x'gst-plugins-bad' ] ||
     [ x$SUITE == x'gst-plugins-ugly' ] ||
     [ x$SUITE == x'gst-libav' ]; then
    mv $i /app/lib/codecs/lib/gstreamer-1.0
  fi
done

