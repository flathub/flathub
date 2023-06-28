<?php

/*
    ALIEN ARENA LIVE IMAGE GENERATOR V1.0
    Copyright (C) 2007 Tony Jackson

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    Tony Jackson can be contacted at tonyj@cooldark.com
*/

include ('config.php');

/* Open the remote URL and read the contents */
$handle = fopen($CONFIG['remoteurl'], "rb");
$content = '';
while (!feof($handle)) {
  $content .= fread($handle, 8192);
}
fclose($handle);

/* Take the serialised array data and return it to it's original form */
$data = unserialize($content);

/* The text to overlay on the image */
$text = "Live: {$data['servers']} servers, {$data['players']} players";

/* Load the background image from a file */
$image = imagecreatefromjpeg($CONFIG['bgimage']);

$font = $CONFIG['font'];
$font_size = $CONFIG['fontsize'];

/* Colour of text to be overlayed */
$fgcolor = imagecolorallocate($image, 255,255,255);

/* Colour of dropshadow */
$bgcolor = imagecolorallocate($image, 0,150,0);

$xoffset = $CONFIG['xoffset'];
$yoffset = $CONFIG['yoffset'];
$shadowdepth = 1;

/* Overlay text on image */
ImageTTFText ($image, $font_size, 0, $xoffset + $shadowdepth, $yoffset + $shadowdepth, $bgcolor, $font, $text);
ImageTTFText ($image, $font_size, 0, $xoffset, $yoffset, $fgcolor, $font, $text);

/* Change the php output type */
header("Content-type: image/jpeg");

/* Push out the image */
imagejpeg($image,"",$CONFIG['jpg_quality']);
imagedestroy($image);

?> 
