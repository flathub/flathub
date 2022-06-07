# Author: Dylan Turner
# Description: Handle OpenCV processing of images

from cv2 import \
    VideoCapture, resize, namedWindow, moveWindow, imshow, waitKey, \
    destroyAllWindows, imread
from numpy import shape
from cvzone import stackImages
from cvzone.SelfiSegmentationModule import SelfiSegmentation
from time import sleep

class Cam:
    def __init__(self, settings):
        self._settings = settings

        self._vidFeed = VideoCapture(settings.camera)
        self._vidFeed.set(3, settings.screenWidth)
        self._vidFeed.set(4, settings.screenHeight)

        self._segmentor = SelfiSegmentation()

        _success, baseFrame = self._vidFeed.read()
        if not _success:
            print('Failed to read from camera!')
            quit()
        _height, _width, self.channels = baseFrame.shape

        if not settings.disableWin:
            # Set window to be at a specific spot
            namedWindow(self._settings.winTitle)
            moveWindow(
                self._settings.winTitle,
                self._settings.winStartX, self._settings.winStartY
            )
    
    def __enter__(self):
        return self
    
    def getFrame(self, bgImg = None):
        _success, frame = self._vidFeed.read()

        if shape(bgImg) == ():
            noBgFrame = self._segmentor.removeBG(
                frame, self._settings.fillColor,
                threshold = self._settings.rmThresh
            )
        else:
            noBgFrame = self._segmentor.removeBG(
                frame, bgImg,
                threshold = self._settings.rmThresh
            )
        return (frame, noBgFrame)
    
    def stackFrames(self, leftFrame, rightFrame):
        newWidth = int(self._settings.screenWidth * self._settings.viewScale)
        newHeight = int(self._settings.screenHeight * self._settings.viewScale)
        return stackImages(
            [
                resize(leftFrame, (newWidth, newHeight)),
                resize(rightFrame, (newWidth, newHeight))
            ], 1, 1
        )
    
    def display(self, frame):
        if not self._settings.disableWin:
            imshow(self._settings.winTitle, frame)

        if waitKey(1) & 0xFF == self._settings.quitKey:
            return False
        return True
    
    def __exit__(self, exc_type, exc_value, traceback):
        self._vidFeed.release()
        destroyAllWindows()
