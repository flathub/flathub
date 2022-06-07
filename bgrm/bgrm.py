#!/usr/bin/env python3

# Author: Dylan Turner
# Description: Main loop for bgrm app

from cv2 import \
    GaussianBlur, imread, resize, copyMakeBorder, \
    BORDER_CONSTANT, cvtColor, COLOR_BGR2YUV_I420
from v4l2 import \
    v4l2_format, V4L2_BUF_TYPE_VIDEO_OUTPUT, V4L2_FIELD_NONE, \
    V4L2_PIX_FMT_YUV420, VIDIOC_S_FMT
from fcntl import ioctl

from cam import Cam
from settings import AppSettings

def main():
    settings = AppSettings.fromArguments()

    # Setup a background image to plug into the cam functions
    if settings.bgImg != '':
        bgImg = getCorrectlySizedBg(settings)

    with Cam(settings) as cam, open('/dev/video' + str(settings.virt_dev), 'wb') as virtCam:
        formatVirtualCamera(settings, virtCam, cam)

        # Loop over feed
        while True:
            # Get cam feed
            if not settings.blur and settings.bgImg != '':
                frame, noBgFrame = cam.getFrame(bgImg)
            else:
                frame, noBgFrame = cam.getFrame()
            
            # If blur enabled, then reapply the background, but blurrd
            if settings.blur:
                removedBg = frame - noBgFrame
                blurred = GaussianBlur(removedBg, (77, 77), 21)
                noBgFrame += blurred
            
            # Write to virtual camera
            virtCam.write(cvtColor(noBgFrame, COLOR_BGR2YUV_I420))
            
            # Display
            stackFrame = cam.stackFrames(frame, noBgFrame)
            if not cam.display(stackFrame):
                break

def formatVirtualCamera(settings, virtCam, actualCam):
    # Setup format info for writing to cam device
    format = v4l2_format()
    format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT
    format.fmt.pix.field = V4L2_FIELD_NONE
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420
    format.fmt.pix.width = settings.screenWidth
    format.fmt.pix.height = settings.screenHeight
    format.fmt.pix.bytesperline = settings.screenWidth * actualCam.channels
    format.fmt.pix.sizeimage = \
        settings.screenWidth * settings.screenHeight * actualCam.channels
    
    # Set device format
    print('Format loopback format result (0 good): {}'.format(
        ioctl(virtCam, VIDIOC_S_FMT, format)
    ))

def getCorrectlySizedBg(settings):
    bgImg = imread(settings.bgImg)

    # Scale to match y and be centered
    bgHeight, bgWidth, _channels = bgImg.shape
    aspect = float(bgWidth) / float(bgHeight)
    newWidth = int(settings.screenHeight * aspect)
    bgImg = resize(bgImg, (newWidth, settings.screenHeight))

    # Scale down
    if newWidth > settings.screenWidth:
        startx = int((newWidth - settings.screenWidth) / 2)
        endx = newWidth - startx
        bgImg = bgImg[0:settings.screenHeight, startx:endx]
    else:
        padding = int((settings.screenWidth - newWidth) / 2)
        bgImg = copyMakeBorder(
            bgImg, 0, 0, padding, padding, BORDER_CONSTANT
        )
    
    # Make sure correct size
    bgImg = resize(bgImg, (settings.screenWidth, settings.screenHeight))
    return bgImg
