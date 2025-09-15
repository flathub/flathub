
INCLUDE_DIR=/app/include
DATA_DIR=/app/share
LIB_DIR=/app/lib
BIN_DIR=/app/bin




install -Dm644 -t $INCLUDE_DIR/stk include/*
install -Dm644 -t $LIB_DIR src/libstk.*
install -Dm644 -t $DATA_DIR/stk/rawwaves rawwaves/*.raw

#cp -pr projects/demo/tcl $DATA_DIR/stk/demo
#cp -pr projects/demo/scores $DATA_DIR/stk/demo
#cp -p projects/demo/demo $BIN_DIR/stk-demo
#cp -p projects/demo/Md2Skini $BIN_DIR/Md2Skini
#for f in Banded Drums Modal Physical Shakers StkDemo Voice ; do
#  chmod +x projects/demo/$f
#  sed -e 's,\./demo,$BIN_DIR/stk-demo,' -e '1i#! /bin/sh' \
#    -i projects/demo/$f
#  cp -p projects/demo/$f $DATA_DIR/stk/demo
#done

#cp -pr projects/examples/midifiles $DATA_DIR/stk/examples
#cp -pr projects/examples/rawwaves $DATA_DIR/stk/examples
#cp -pr projects/examples/scores $DATA_DIR/stk/examples
#for f in sine sineosc foursine audioprobe midiprobe duplex play \
#    record inetIn inetOut rtsine crtsine bethree controlbee \
#    threebees playsmf grains ; do
#  cp -p projects/examples/$f $BIN_DIR/stk-$f
#  # absolute links, will be shortened later
#  ln -s $BIN_DIR/stk-$f $DATA_DIR/stk/examples/$f
#done

install -Dm644 -t $DATA_DIR/stk/effects projects/effects/tcl
install -Dm644 -t $BIN_DIR/stk-effects projects/effects/effects
sed -e 's,\./effects,$BIN_DIR/stk-effects,' -e '1i#! /bin/sh' \
    -i projects/effects/StkEffects
install -Dm644 -t $DATA_DIR/stk/effects projects/effects/StkEffects

install -Dm644 -t $DATA_DIR/stk/ragamatic projects/ragamatic/tcl
install -Dm644 -t $DATA_DIR/stk/ragamatic projects/ragamatic/rawwaves
install -Dm644 -t $BIN_DIR/stk-ragamat projects/ragamatic/ragamat
sed -e 's,\./ragamat,$BIN_DIR/stk-ragamat,' -e '1i#! /bin/sh' \
  -i projects/ragamatic/Raga
install -Dm644 -t $DATA_DIR/stk/ragamatic projects/ragamatic/Raga

install -Dm644 -t $DATA_DIR/stk/eguitar projects/eguitar/tcl
install -Dm644 -t $DATA_DIR/stk/eguitar projects/eguitar/scores
install -Dm644 -t $BIN_DIR/stk-eguitar projects/eguitar/eguitar
sed -e 's,\./eguitar,$BIN_DIR/stk-eguitar,' -e '1i#! /bin/sh' \
  -i projects/eguitar/ElectricGuitar
install -Dm644 -t $DATA_DIR/stk/eguitar projects/eguitar/ElectricGuitar
