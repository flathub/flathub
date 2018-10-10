#!/bin/sh
cd "$(dirname "$0")"
if ['getconfig LONG_BIT' = "64" ]
then  
	exec ./voxels.x86_64 "$@"
else
	exec ./voxels.x86 "$@"
fi
