/*
 *
 *  Copyright (c) 2021
 *  name : Francis Banyikwa
 *  email: mhogomchungu@gmail.com
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "yt-dlp.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#include "../networkAccess.h"
#include "../utility.h"

#include "../configure.h"

#include "aria2c.h"

const char * yt_dlp::testYtDlp()
{
	return R"R(youtube] Extracting URL: https://www.youtube.com/watch?v=tn2USd5KeVM
[youtube] tn2USd5KeVM: Downloading webpage
[youtube] tn2USd5KeVM: Downloading ios player API JSON
[youtube] tn2USd5KeVM: Downloading android player API JSON
[youtube] tn2USd5KeVM: Downloading m3u8 information
[info] tn2USd5KeVM: Downloading 1 format(s): 242+250
[download] Destination: 16 years ago, LeBron James put up one of the greatest playoff performances ever 😤 ｜ NBA on ESPN-tn2USd5KeVM.f242.webm
[download] downloaded_bytes:1024 ETA:86 total_bytes_estimate:NA total_bytes:3006209 progress.speed:34781.85091065167 filename:16 years ago, LeBron James put up one of the greatest playoff performances ever 😤 ｜ NBA on ESPN-tn2USd5KeVM.f242.webm
[download] downloaded_bytes:3072 ETA:33 total_bytes_estimate:NA total_bytes:3006209 progress.speed:89097.34668363113 filename:16 years ago, LeBron James put up one of the greatest playoff performances ever 😤 ｜ NBA on ESPN-tn2USd5KeVM.f242.webm
[download] downloaded_bytes:7168 ETA:15 total_bytes_estimate:NA total_bytes:3006209 progress.speed:189275.88640212538 filename:16 years ago, LeBron James put up one of the greatest playoff performances ever 😤 ｜ NBA on ESPN-tn2USd5KeVM.f242.webm
[download] downloaded_bytes:15360 ETA:8 total_bytes_estimate:NA total_bytes:3006209 progress.speed:371957.3995981617 filename:16 years ago, LeBron James put up one of the greatest playoff performances ever 😤 ｜ NBA on ESPN-tn2USd5KeVM.f242.webm
[download] downloaded_bytes:31744 ETA:4 total_bytes_estimate:NA total_bytes:3006209 progress.speed:652337.2325541516 filename:16 years ago, LeBron James put up one of the greatest playoff performances ever 😤 ｜ NBA on ESPN-tn2USd5KeVM.f242.webm
[download] downloaded_bytes:64512 ETA:9 total_bytes_estimate:NA total_bytes:3006209 progress.speed:295201.8918201228 filename:16 years ago, LeBron James put up one of the greatest playoff performances ever 😤 ｜ NBA on ESPN-tn2USd5KeVM.f242.webm
[download] downloaded_bytes:130048 ETA:8 total_bytes_estimate:NA total_bytes:3006209 progress.speed:359000.1287306937 filename:16 years ago, LeBron James put up one of the greatest playoff performances ever 😤 ｜ NBA on ESPN-tn2USd5KeVM.f242.webm
[download] downloaded_bytes:261120 ETA:5 total_bytes_estimate:NA total_bytes:3006209 progress.speed:479032.6506522084 filename:16 years ago, LeBron James put up one of the greatest playoff performances ever 😤 ｜ NBA on ESPN-tn2USd5KeVM.f242.webm
[download] downloaded_bytes:523264 ETA:3 total_bytes_estimate:NA total_bytes:3006209 progress.speed:668734.2472276848 filename:16 years ago, LeBron James put up one of the greatest playoff performances ever 😤 ｜ NBA on ESPN-tn2USd5KeVM.f242.webm
[download] downloaded_bytes:1047552 ETA:2 total_bytes_estimate:NA total_bytes:3006209 progress.speed:676620.700437195 filename:16 years ago, LeBron James put up one of the greatest playoff performances ever 😤 ｜ NBA on ESPN-tn2USd5KeVM.f242.webm
[download] downloaded_bytes:1732231 ETA:1 total_bytes_estimate:NA total_bytes:3006209 progress.speed:834853.6513007934 filename:16 years ago, LeBron James put up one of the greatest playoff performances ever 😤 ｜ NBA on ESPN-tn2USd5KeVM.f242.webm
[download] downloaded_bytes:3006209 ETA:0 total_bytes_estimate:NA total_bytes:3006209 progress.speed:902139.0218883085 filename:16 years ago, LeBron James put up one of the greatest playoff performances ever 😤 ｜ NBA on ESPN-tn2USd5KeVM.f242.webm
[download] downloaded_bytes:3006209 ETA:NA total_bytes_estimate:NA total_bytes:3006209 progress.speed:495882.2212164972 filename:16 years ago, LeBron James put up one of the greatest playoff performances ever 😤 ｜ NBA on ESPN-tn2USd5KeVM.f242.webm
[download] Destination: 16 years ago, LeBron James put up one of the greatest playoff performances ever 😤 ｜ NBA on ESPN-tn2USd5KeVM.f250.webm
[download] downloaded_bytes:1024 ETA:31 total_bytes_estimate:NA total_bytes:801810 progress.speed:25365.37818619924 filename:16 years ago, LeBron James put up one of the greatest playoff performances ever 😤 ｜ NBA on ESPN-tn2USd5KeVM.f250.webm
[download] downloaded_bytes:3072 ETA:11 total_bytes_estimate:NA total_bytes:801810 progress.speed:67111.66031917997 filename:16 years ago, LeBron James put up one of the greatest playoff performances ever 😤 ｜ NBA on ESPN-tn2USd5KeVM.f250.webm
[download] downloaded_bytes:7168 ETA:5 total_bytes_estimate:NA total_bytes:801810 progress.speed:143245.38467622438 filename:16 years ago, LeBron James put up one of the greatest playoff performances ever 😤 ｜ NBA on ESPN-tn2USd5KeVM.f250.webm
[download] downloaded_bytes:15360 ETA:2 total_bytes_estimate:NA total_bytes:801810 progress.speed:278861.3860717578 filename:16 years ago, LeBron James put up one of the greatest playoff performances ever 😤 ｜ NBA on ESPN-tn2USd5KeVM.f250.webm
[download] downloaded_bytes:31744 ETA:6 total_bytes_estimate:NA total_bytes:801810 progress.speed:126219.34070617768 filename:16 years ago, LeBron James put up one of the greatest playoff performances ever 😤 ｜ NBA on ESPN-tn2USd5KeVM.f250.webm
[download] downloaded_bytes:64512 ETA:5 total_bytes_estimate:NA total_bytes:801810 progress.speed:145522.65862243847 filename:16 years ago, LeBron James put up one of the greatest playoff performances ever 😤 ｜ NBA on ESPN-tn2USd5KeVM.f250.webm
[download] downloaded_bytes:130048 ETA:3 total_bytes_estimate:NA total_bytes:801810 progress.speed:189624.4667509348 filename:16 years ago, LeBron James put up one of the greatest playoff performances ever 😤 ｜ NBA on ESPN-tn2USd5KeVM.f250.webm
[download] downloaded_bytes:261120 ETA:2 total_bytes_estimate:NA total_bytes:801810 progress.speed:247524.8816082002 filename:16 years ago, LeBron James put up one of the greatest playoff performances ever 😤 ｜ NBA on ESPN-tn2USd5KeVM.f250.webm
[download] downloaded_bytes:523264 ETA:0 total_bytes_estimate:NA total_bytes:801810 progress.speed:365075.6637308399 filename:16 years ago, LeBron James put up one of the greatest playoff performances ever 😤 ｜ NBA on ESPN-tn2USd5KeVM.f250.webm
[download] downloaded_bytes:801810 ETA:0 total_bytes_estimate:NA total_bytes:801810 progress.speed:477969.5544142013 filename:16 years ago, LeBron James put up one of the greatest playoff performances ever 😤 ｜ NBA on ESPN-tn2USd5KeVM.f250.webm
[download] downloaded_bytes:801810 ETA:NA total_bytes_estimate:NA total_bytes:801810 progress.speed:219337.62226918363 filename:16 years ago, LeBron James put up one of the greatest playoff performances ever 😤 ｜ NBA on ESPN-tn2USd5KeVM.f250.webm
[Merger] Merging formats into "16 years ago, LeBron James put up one of the greatest playoff performances ever 😤 ｜ NBA on ESPN-tn2USd5KeVM.webm"
Deleting original file 16 years ago, LeBron James put up one of the greatest playoff performances ever 😤 ｜ NBA on ESPN-tn2USd5KeVM.f250.webm (pass -k to keep)
Deleting original file 16 years ago, LeBron James put up one of the greatest playoff performances ever 😤 ｜ NBA on ESPN-tn2USd5KeVM.f242.webm (pass -k to keep))R" ;
}

const char * yt_dlp::testFfmpeg()
{
	return R"R([youtube] Extracting URL: https://www.youtube.com/watch?v=LOpES1ZlISk
[youtube] LOpES1ZlISk: Downloading webpage
[youtube] LOpES1ZlISk: Downloading ios player API JSON
[youtube] LOpES1ZlISk: Downloading android player API JSON
[youtube] LOpES1ZlISk: Downloading m3u8 information
[info] LOpES1ZlISk: Downloading 1 format(s): 242+250
[download] Destination: Bobby Marks on NBA free agency： Damian Lillard's future, A+ for the Lakers & more ｜ NBA on ESPN-LOpES1ZlISk.webm
Input #0, matroska,webm, from 'https://rr2---sn-8vq5jvhu1-q5ge.googlevideo.com/videoplayback?expire=1688404862&ei=Hq-iZJu0IMfIxwL-mLaQCQ&ip=197.250.197.106&id=o-AN-Mhp8xBo0FkzjLttQZTYA4rS_YvpOaxlWk1fVEvdeI&itag=242&source=youtube&requiressl=yes&mh=cM&mm=31%2C29&mn=sn-8vq5jvhu1-q5ge%2Csn-hgn7rnee&ms=au%2Crdu&mv=m&mvi=2&pl=18&initcwndbps=187500&vprv=1&svpuc=1&mime=video%2Fwebm&gir=yes&clen=9897290&dur=990.366&lmt=1688337165712349&mt=1688383058&fvip=1&keepalive=yes&fexp=24007246%2C51000012%2C51000024&beids=24350017&c=IOS&txp=5535434&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Csvpuc%2Cmime%2Cgir%2Cclen%2Cdur%2Clmt&sig=AOq0QJ8wRQIhAKFiRG-SYmS_lgbtCtWeuEt80RrWW03pFxKBfpl90zbpAiBo74goFCLSHhlXu1TmepMystawvD9u91oXcTxxlE_Zpw%3D%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=AG3C_xAwRAIgEDiulbviVETAsGB8xZdRwUWvGvdjaqc5dX_8JmLs27sCID6Ysy7lQXN6gofN0OULiok1qwjLUSDRXfHkiKbmQw2G':
  Metadata:
    encoder         : google/video-file
  Duration: 00:16:30.37, start: 0.000000, bitrate: 79 kb/s
  Stream #0:0(eng): Video: vp9 (Profile 0), yuv420p(tv, bt709), 426x240, SAR 1:1 DAR 71:40, 30 fps, 30 tbr, 1k tbn (default)
Input #1, matroska,webm, from 'https://rr2---sn-8vq5jvhu1-q5ge.googlevideo.com/videoplayback?expire=1688404863&ei=H6-iZNPWL5WEWJHlpcAP&ip=197.250.197.106&id=o-AMrfanCUV2RL8GcaUhUIcOvDF8mBSvI4uKoL5tI8o6Kt&itag=250&source=youtube&requiressl=yes&mh=cM&mm=31%2C29&mn=sn-8vq5jvhu1-q5ge%2Csn-hgn7rnee&ms=au%2Crdu&mv=m&mvi=2&pl=18&initcwndbps=187500&spc=Ul2Sq81k0298IGzHwpxvOkPsaZQG910&vprv=1&svpuc=1&mime=audio%2Fwebm&gir=yes&clen=8067527&dur=990.401&lmt=1688330975379663&mt=1688382819&fvip=1&keepalive=yes&fexp=24007246&c=ANDROID&txp=5532434&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cspc%2Cvprv%2Csvpuc%2Cmime%2Cgir%2Cclen%2Cdur%2Clmt&sig=AOq0QJ8wRgIhAMIMTZi-dS4vJWFNBY61qmGTiGyDzYDUsLx8yxbstjlkAiEAxs8cQNQ8oemMY-2q4GQF9iG64LbUtuG5Xebl1p0okWQ%3D&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=AG3C_xAwRQIhAMfnOj8rkjCHX6bbkJ7KAPZvzOKwldrhtfw55zQxNyU7AiAYCXEerg-s936B6uuGEbJk_0BcvPstHGF4obpbe9ocaQ%3D%3D':
  Metadata:
    encoder         : google/video-file
  Duration: 00:16:30.40, start: -0.007000, bitrate: 65 kb/s
  Stream #1:0(eng): Audio: opus, 48000 Hz, stereo, fltp (default)
Output #0, webm, to 'file:Bobby Marks on NBA free agency： Damian Lillard's future, A+ for the Lakers & more ｜ NBA on ESPN-LOpES1ZlISk.webm.part':
  Metadata:
    encoder         : Lavf59.27.100
  Stream #0:0(eng): Video: vp9 (Profile 0), yuv420p(tv, bt709), 426x240 [SAR 1:1 DAR 71:40], q=2-31, 30 fps, 30 tbr, 1k tbn (default)
  Stream #0:1(eng): Audio: opus, 48000 Hz, stereo, fltp (default)
Stream mapping:
  Stream #0:0 -> #0:0 (copy)
  Stream #1:0 -> #0:1 (copy)
Press [q] to stop, [?] for help
frame=    1 fps=0.0 q=-1.0 size=       1kB time=00:00:00.00 bitrate=4768.0kbits/s speed=6.85x
frame=  871 fps=0.0 q=-1.0 size=     256kB time=00:00:29.00 bitrate=  72.3kbits/s speed=54.4x
frame= 1142 fps=1102 q=-1.0 size=     512kB time=00:00:38.03 bitrate= 110.3kbits/s speed=36.7x
frame= 1628 fps=1046 q=-1.0 size=     768kB time=00:00:54.23 bitrate= 116.0kbits/s speed=34.8x
frame= 2204 fps=1066 q=-1.0 size=    1280kB time=00:01:13.43 bitrate= 142.8kbits/s speed=35.5x
frame= 3151 fps=1226 q=-1.0 size=    1792kB time=00:01:45.00 bitrate= 139.8kbits/s speed=40.9x
frame= 4214 fps=1372 q=-1.0 size=    2304kB time=00:02:20.43 bitrate= 134.4kbits/s speed=45.7x
frame= 5128 fps=1434 q=-1.0 size=    2816kB time=00:02:50.90 bitrate= 135.0kbits/s speed=47.8x
frame= 6522 fps=1599 q=-1.0 size=    3584kB time=00:03:37.36 bitrate= 135.1kbits/s speed=53.3x
frame= 7621 fps=1653 q=-1.0 size=    4352kB time=00:04:14.00 bitrate= 140.4kbits/s speed=55.1x
frame= 9241 fps=1802 q=-1.0 size=    5376kB time=00:05:08.00 bitrate= 143.0kbits/s speed=60.1x
frame=11018 fps=1931 q=-1.0 size=    6400kB time=00:06:07.23 bitrate= 142.8kbits/s speed=64.4x
frame=11881 fps=1913 q=-1.0 size=    6912kB time=00:06:36.00 bitrate= 143.0kbits/s speed=63.8x
frame=13166 fps=1962 q=-1.0 size=    7680kB time=00:07:18.83 bitrate= 143.4kbits/s speed=65.4x
frame=14758 fps=2046 q=-1.0 size=    8448kB time=00:08:11.90 bitrate= 140.7kbits/s speed=68.2x
frame=16750 fps=2170 q=-1.0 size=    9728kB time=00:09:18.30 bitrate= 142.7kbits/s speed=72.3x
frame=17993 fps=2177 q=-1.0 size=   10496kB time=00:09:59.73 bitrate= 143.4kbits/s speed=72.6x
frame=20038 fps=2282 q=-1.0 size=   11520kB time=00:11:07.90 bitrate= 141.3kbits/s speed=76.1x
frame=21848 fps=2354 q=-1.0 size=   12544kB time=00:12:08.23 bitrate= 141.1kbits/s speed=78.5x
frame=24166 fps=2468 q=-1.0 size=   14080kB time=00:13:25.50 bitrate= 143.2kbits/s speed=82.3x
frame=26536 fps=2577 q=-1.0 size=   15360kB time=00:14:44.50 bitrate= 142.3kbits/s speed=85.9x
frame=28441 fps=2634 q=-1.0 size=   16640kB time=00:15:48.00 bitrate= 143.8kbits/s speed=87.8x
frame=29711 fps=2654 q=-1.0 Lsize=   17543kB time=00:16:30.38 bitrate= 145.1kbits/s speed=88.5x
video:9475kB audio:7554kB subtitle:0kB other streams:0kB global headers:0kB muxing overhead: 3.017891%
[download] 100% of   17.13MiB in 00:00:12 at 1.40MiB/s)R" ;
}

static QString _OSXBinaryName()
{
	return "yt-dlp_macos" ;
}

static QString _Windows32BitBinaryName()
{
	return "yt-dlp_x86.exe" ;
}

static QString _Windows64BitBinaryName()
{
	return "yt-dlp.exe" ;
}

void yt_dlp::checkIfBinaryExist( const QString& runTimeBinPath,const QString& thirdPartyBinPath )
{
	if( utility::platformIsWindows() ){

		auto destPath = runTimeBinPath ;

		if( utility::platformIs32Bit() ){

			destPath += "/" + _Windows32BitBinaryName() ;
		}else{
			destPath += "/" + _Windows64BitBinaryName() ;
		}

		if( !QFile::exists( destPath ) ){

			auto srcPath = thirdPartyBinPath + "/ytdlp/" + _Windows32BitBinaryName() ;

			utility::copyFile( srcPath,destPath ) ;
		}

	}else if( utility::platformIsOSX() ){

		auto destPath = runTimeBinPath + "/" + _OSXBinaryName() ;
		auto srcPath = utility::OSX3rdPartyDirPath() + "/" + _OSXBinaryName() ;

		if( !QFile::exists( destPath ) && QFile::exists( srcPath ) ){

			utility::copyFile( srcPath,destPath ) ;
		}
	}
}

static const char * _jsonFullArguments()
{
	return R"R({"uploader":%(uploader)j,"id":%(id)j,"thumbnail":%(thumbnail)j,"duration":%(duration)j,"title":%(title)j,"upload_date":%(upload_date)j,"webpage_url":%(webpage_url)j,"formats":%(formats.:.{url,format_id,ext,resolution,filesize,filesize_approx,tbr,vbr,abr,asr,container,protocol,vcodec,video_ext,acodec,audio_ext,format_note})j,"playlist_id":%(playlist_id)j,"playlist_title":%(playlist_title)j,"playlist":%(playlist)j,"playlist_uploader":%(playlist_uploader)j,"playlist_uploader_id":%(playlist_uploader_id)j})R" ;
}

QStringList yt_dlp::jsonNoFormatsArgumentList()
{
	auto a = R"R({"uploader":%(uploader)j,"id":%(id)j,"thumbnail":%(thumbnail)j,"duration":%(duration)j,"title":%(title)j,"upload_date":%(upload_date)j,"webpage_url":%(webpage_url)j,"playlist_id":%(playlist_id)j,"playlist_title":%(playlist_title)j,"playlist":%(playlist)j,"playlist_uploader":%(playlist_uploader)j,"playlist_uploader_id":%(playlist_uploader_id)j})R" ;

	return { "--newline","--print",a } ;
}

static QJsonObject _defaultControlStructure()
{
	QJsonObject obj ;

	obj.insert( "Connector","&&" ) ;

	obj.insert( "lhs",[](){

		QJsonObject obj ;

		obj.insert( "startsWith","[download]" ) ;

		return obj ;
	}() ) ;

	obj.insert( "rhs",[](){

		QJsonObject obj ;

		obj.insert( "contains","ETA" ) ;

		return obj ;
	}() ) ;

	return obj ;
}

static void _arr_imp( QJsonArray& )
{
}

template< typename First,typename ... Rest >
static void _arr_imp( QJsonArray& arr,const First& f,Rest&& ... rest )
{
	arr.append( f ) ;
	_arr_imp( arr,std::forward< Rest >( rest ) ... ) ;
}

template< typename ... Args >
static QJsonArray _arr( Args&& ... args )
{
	QJsonArray arr ;
	_arr_imp( arr,std::forward< Args >( args ) ... ) ;
	return arr ;
}

QJsonObject yt_dlp::init( const QString& name,
			  const QString& configFileName,
			  Logger& logger,
			  const engines::enginePaths& enginePath )
{
	auto m = enginePath.enginePath( configFileName ) ;

	if( QFile::exists( m ) ){

		return QJsonObject() ;
	}

	QJsonObject mainObj ;

	utility::addJsonCmd json( mainObj ) ;

	auto x86Name = _Windows32BitBinaryName() ;
	auto amd64   = _Windows64BitBinaryName() ;
	auto macos   = _OSXBinaryName() ;

	json.add( { { "Generic" },{ { "x86","yt-dlp",{ "yt-dlp" } },
				    { "amd64","yt-dlp",{ "yt-dlp" } } } } ) ;

	json.add( { { "Windows" },{ { "x86",x86Name,{ x86Name } },
				    { "amd64",amd64,{ amd64 } } } } ) ;

	json.add( { { "MacOS" },{ { "x86",macos,{ macos } },
				  { "amd64",macos,{ macos } } } } ) ;

	json.done() ;

	mainObj.insert( "DefaultListCmdOptions",_arr( "--newline","--print",_jsonFullArguments() ) ) ;

	mainObj.insert( "DumptJsonArguments",_arr( "--newline","--print",_jsonFullArguments() ) ) ;

	mainObj.insert( "DefaultCommentsCmdOptions",_arr( "--get-comments","--no-download","--print","{\"title\":%(title)j,\"comments\":%(comments)j}" ) ) ;

	mainObj.insert( "DefaultSubstitlesCmdOptions",_arr( "--no-download","--print","{\"title\":%(title)j,\"automatic_captions\":%(automatic_captions)j,\"subtitles\":%(subtitles)j}" ) ) ;

	mainObj.insert( "DefaultSubtitleDownloadOptions",_arr( "--embed-subs" ) ) ;

	mainObj.insert( "DefaultDownLoadCmdOptions",_arr( "--newline","--ignore-config","--no-playlist","-o","%(title).200s-%(id)s.%(ext)s" ) ) ;

	mainObj.insert( "SkipLineWithText",_arr( "(pass -k to keep)" ) ) ;

	mainObj.insert( "RemoveText",_arr() ) ;

	mainObj.insert( "SplitLinesBy",_arr( "\n" ) ) ;

	mainObj.insert( "DownloadUrl","https://api.github.com/repos/yt-dlp/yt-dlp/releases/latest" ) ;

	mainObj.insert( "AutoUpdate",true ) ;

	mainObj.insert( "EncodingArgument","--encoding" ) ;

	mainObj.insert( "RequiredMinimumVersionOfMediaDownloader","2.2.0" ) ;

	mainObj.insert( "Name",name ) ;

	mainObj.insert( "CookieArgument","--cookies" ) ;

	mainObj.insert( "PlaylistItemsArgument","--playlist-items" ) ;

	mainObj.insert( "ControlJsonStructure",_defaultControlStructure() ) ;

	mainObj.insert( "VersionArgument","--version" ) ;

	mainObj.insert( "OptionsArgument","-f" ) ;

	mainObj.insert( "BackendPath",utility::stringConstants::defaultPath() ) ;

	mainObj.insert( "VersionStringLine",0 ) ;

	mainObj.insert( "VersionStringPosition",0 ) ;

	mainObj.insert( "BatchFileArgument","-a" ) ;

	mainObj.insert( "CanDownloadPlaylist",true ) ;

	mainObj.insert( "LikeYoutubeDl",true ) ;

	mainObj.insert( "ReplaceOutputWithProgressReport",false ) ;

	engines::file( m,logger ).write( mainObj ) ;

	return mainObj ;
}

yt_dlp::yt_dlp( const engines& engines,
		const engines::engine& engine,
		QJsonObject& obj,
		Logger& logger,
		const engines::enginePaths& enginePath,
		const util::version& version,
		const QString& downloadFolder,
		bool deleteFilesOnCancel ) :
	engines::engine::baseEngine( engines.Settings(),engine,engines.processEnvironment() ),
	m_engine( engine ),
	m_version( version ),
	m_deleteFilesOnCancel( deleteFilesOnCancel ),
	m_downloadFolder( downloadFolder )
{
	Q_UNUSED( m_version )

	auto name = obj.value( "Name" ).toString() ;

	if( name == "yt-dlp" ){

		if( obj.value( "Cmd" ).isUndefined() ){

			auto configFileName = name + ".json" ;

			auto m = enginePath.enginePath( configFileName ) ;

			QFile::remove( m ) ;

			obj = yt_dlp::init( name,configFileName,logger,enginePath ) ;
		}
	}

	if( !obj.contains( "EncodingArgument" ) ){

		obj.insert( "EncodingArgument","--encoding" ) ;
	}

	auto arr = _arr( "--newline","--print",_jsonFullArguments() ) ;

	obj.insert( "DumptJsonArguments",arr ) ;

	obj.insert( "DefaultListCmdOptions",arr ) ;

	if( !obj.contains( "DefaultCommentsCmdOptions" ) ){

		auto a = "--get-comments" ;
		auto b = "--no-download" ;
		auto c = "--print" ;
		auto d = "{\"title\":%(title)j,\"comments\":%(comments)j}" ;

		obj.insert( "DefaultCommentsCmdOptions",_arr( a,b,c,d ) ) ;
	}

	if( !obj.contains( "DefaultSubstitlesCmdOptions" ) ){

		auto a = "--no-download" ;
		auto b = "--print" ;
		auto c = "{\"title\":%(title)j,\"automatic_captions\":%(automatic_captions)j,\"subtitles\":%(subtitles)j}" ;

		obj.insert( "DefaultSubstitlesCmdOptions",_arr( a,b,c ) ) ;
	}

	if( !obj.contains( "DefaultSubtitleDownloadOptions" ) ){

		obj.insert( "DefaultSubtitleDownloadOptions",_arr( "--embed-subs" ) ) ;
	}
}

#define COMPACTYEAR "2022"

void yt_dlp::appendCompatOption( QStringList& e )
{
	e.append( "--compat-options" ) ;
	e.append( COMPACTYEAR ) ;
}

const char * yt_dlp::yt_dlplFilter::compatYear()
{
	return "yt-dlp: error: wrong OPTS for --compat-options: " COMPACTYEAR ;
}

yt_dlp::~yt_dlp()
{
}

static bool _yt_dlp( const engines::engine&,const QByteArray& e )
{
	return e.startsWith( "[download]" ) && e.contains( "ETA" ) ;
}

static bool _fragment_output( const QByteArray& e )
{
	return utils::misc::startsWithAny( e,"[https @ ","[hls @ ","Opening '" ) ;
}

static bool _ffmpeg( const engines::engine&,const QByteArray& e )
{
	if( _fragment_output( e ) ){

		return true ;
	}else{
		return utils::misc::startsWithAny( e,"frame=","size=" ) ;
	}
}

static bool _aria2c( const engines::engine& s,const QByteArray& e )
{
	return aria2c::meetCondition( s,e ) ;
}

static bool _ffmpeg_internal( const engines::engine&,const QByteArray& e )
{
	return e.contains( " / ~" ) || e.startsWith( "Frame: " ) ;
}

static bool _shouldNotGetCalled( const engines::engine&,const QByteArray& )
{
	return false ;
}

class parseTemplateOutPut
{
public:
	static void setTemplate( QStringList& e )
	{
		e.append( "--progress-template" ) ;
		e.append( "download:[download] downloaded_bytes:%(progress.downloaded_bytes)s ETA:%(progress.eta)s total_bytes_estimate:%(progress.total_bytes_estimate)s total_bytes:%(progress.total_bytes)s progress.speed:%(progress.speed)s filename:%(progress.filename)s" ) ;
	}
	parseTemplateOutPut( const QByteArray& e ) :
		m_totalSize( this->findEntry( e,"total_bytes:" ) ),
		m_eta( this->findEntry( e,"ETA:" ) ),
		m_dataDownloaded( this->findEntry( e,"downloaded_bytes:" ) ),
		m_totaSizeEstimate( this->findEntry( e,"total_bytes_estimate:" ) ),
		m_speed( this->findEntry( e,"speed:" ) )
	{
		auto m = e.indexOf( "filename" ) ;

		if( m != -1 ){

			m_fileName = e.mid( m + 9 ) ;
		}
	}
	const QByteArray& fileName() const
	{
		return m_fileName ;
	}
	const QByteArray& dataDownloaded() const
	{
		return m_dataDownloaded ;
	}
	const QByteArray& totalSizeEstimate() const
	{
		return m_totaSizeEstimate ;
	}
	const QByteArray& speed() const
	{
		return m_speed ;
	}
	const QByteArray& ETA() const
	{
		return m_eta ;
	}
	const QByteArray& totalSize() const
	{
		return m_totalSize ;
	}
private:
	QByteArray findEntry( const QByteArray& data,const QByteArray& e )
	{
		auto m = data.indexOf( e ) ;

		if( m != -1 ){

			auto s = data.mid( m + e.size() ) ;

			m = s.indexOf( " " ) ;

			if( m != -1 ){

				return s.mid( 0,m ) ;
			}
		}

		return "NA" ;
	}
	QByteArray m_totalSize ;
	QByteArray m_eta ;
	QByteArray m_dataDownloaded ;
	QByteArray m_totaSizeEstimate ;
	QByteArray m_speed ;
	QByteArray m_fileName ;
};

class ytDlpFilter : public engines::engine::baseEngine::filterOutPut
{
public:
	ytDlpFilter( const engines::engine& engine ) :
		m_engine( engine )
	{
	}
	engines::engine::baseEngine::filterOutPut::result
	formatOutput( const engines::engine::baseEngine::filterOutPut::args& args ) const override
	{
		if( m_function == _yt_dlp ){

			m_tmp = this->outPutFormat( args ) ;

			return { m_tmp,m_engine,m_function } ;

		}else if( m_function == _ffmpeg_internal ){

			m_tmp = this->outPutFfmpeg( args ) ;

			return { m_tmp,m_engine,m_function } ;
		}else{
			return { args.outPut,m_engine,m_function } ;
		}
	}
	bool meetCondition( const engines::engine::baseEngine::filterOutPut::args& args ) const override
	{
		const auto& e = args.outPut ;

		if( _yt_dlp( m_engine,e ) ){

			m_function = _yt_dlp ;

		}else if( _ffmpeg( m_engine,e ) ){

			m_function = _ffmpeg_internal ;

		}else if( _aria2c( m_engine,e ) ){

			m_function = _aria2c ;
		}else{
			m_function = _shouldNotGetCalled ;

			return false ;
		}

		return true ;
	}
	const engines::engine& engine() const override
	{
		return m_engine ;
	}
private:
	QByteArray outPutFormat( const engines::engine::baseEngine::filterOutPut::args& args ) const
	{
		const auto& e = args.outPut ;
		const auto& locale = args.locale ;

		parseTemplateOutPut outPut( e ) ;

		args.data.ytDlpData().setFilePath( outPut.fileName() ) ;

		QString progress = e.mid( 0,11 ) ;

		if( outPut.dataDownloaded() == "NA" ){

			progress += "NA / " ;
		}else{
			auto m = qint64( outPut.dataDownloaded().toDouble() ) ;

			progress += locale.formattedDataSize( m )  + " / " ;
		}

		double percentage = 0 ;

		if( outPut.totalSize() != "NA" ){

			auto mm = outPut.totalSize().toDouble() ;

			if( mm != 0 ){

				percentage = outPut.dataDownloaded().toDouble() * 100 / mm ;
			}

			progress += locale.formattedDataSize( qint64( mm ) ) ;

		}else if( outPut.totalSizeEstimate() != "NA" ){

			auto mm = outPut.totalSizeEstimate().toDouble() ;

			if( mm != 0 ){

				percentage = outPut.dataDownloaded().toDouble() * 100 / mm ;
			}

			progress += "~" + locale.formattedDataSize( qint64( mm ) ) ;
		}else{
			progress += "NA" ;
		}

		if( percentage < 100 ){

			progress += " (" + QString::number( percentage,'f',2 ) + "%)" ;
		}else{
			progress += " (100%)" ;
		}

		if( outPut.speed() != "NA" ){

			auto mm = outPut.speed().toDouble() ;

			progress += " at " + locale.formattedDataSize( qint64( mm ) ) + "/s" ;
		}

		if( outPut.ETA() == "NA" ){

			progress += ", ETA NA" ;
		}else{
			progress += ", ETA " + locale.secondsToString( outPut.ETA().toInt() ) ;
		}

		return progress.toUtf8() ;
	}
	double toSeconds( const QByteArray& s ) const
	{
		if( s.isEmpty() ){

			return 0 ;
		}else{
			auto mm = util::split( util::split( s,'.' )[ 0 ],':' ) ;

			if( mm.size() > 2 ){

				auto h  = mm[ 0 ].toDouble() ;
				auto m  = mm[ 1 ].toDouble() ;
				auto ss = mm[ 2 ].toDouble() ;

				ss += m * 60 ;
				ss += h * 60 * 60 ;

				return ss ;
			}else{
				return 0 ;
			}
		}
	}
	QByteArray duration( const QByteArray& e ) const
	{
		if( e.contains( "  Duration: " ) ){

			auto m = util::split( e,' ' ) ;

			for( int a = 0 ; a < m.size() ; a++ ){

				if( m[ a ] == "Duration:" && a + 1 < m.size() ){

					auto mm = m[ a + 1 ].replace( ",","" ) ;

					return mm ;
				}
			}

			return {} ;
		}else{
			return {} ;
		}
	}
	QString getOption( const QList< QByteArray >& m,const char * opt ) const
	{
		for( int i = 0 ; i < m.size() ; i++ ){

			const auto& s = m[ i ] ;

			if( s == opt ){

				if( i + 1 < m.size() ){

					return m[ i + 1 ] ;
				}

			}else if( s.startsWith( opt ) ){

				auto m = s.indexOf( '=' ) ;

				if( m == -1 ){

					return "NA" ;
				}else{
					return s.mid( m + 1 ) ;
				}
			}
		}

		return "NA" ;
	}
	qint64 size( QByteArray e ) const
	{
		return e.replace( "kB","" ).toLongLong() * 1024 ;
	}
	QByteArray outPutFfmpeg( const filterOutPut::args& args ) const
	{
		const auto& data   = args.outPut ;
		auto& s            = args.data ;
		const auto& locale = args.locale ;

		double totalTime = 0 ;

		if( _fragment_output( data ) ){

			return args.data.lastText() ;
		}

		QByteArray totalTimeString ;

		s.forEach( [ & ]( int,const QByteArray& e ){

			auto d = this->duration( e ) ;

			if( d.isEmpty() ){

				return false ;
			}else{
				totalTime = this->toSeconds( d ) ;

				totalTimeString = std::move( d ) ;

				return true ;
			}
		} ) ;

		auto m = util::split( data,' ' ) ;

		auto currentTimeString = this->getOption( m,"time=" ) ;
		auto size = this->getOption( m,"size=" ) ;

		if( size == "NA" ){

			size = this->getOption( m,"Lsize=" ) ;
		}

		auto currentTime = this->toSeconds( currentTimeString.toUtf8() ) ;

		auto currentSize = this->size( size.toUtf8() ) ;

		if( currentTime == 0 || totalTime == 0 || currentSize == 0 || size == "NA" ){

			auto frame   = this->getOption( m,"frame=" ) ;
			auto fps     = this->getOption( m,"fps=" ) ;
			auto bitrate = this->getOption( m,"bitrate=" ) ;
			auto speed   = this->getOption( m,"speed=" ) ;

			QString result = "Frame: %1, Fps: %2, Size: %3, Bitrate: %4, Speed: %5" ;

			return result.arg( frame,fps,size,bitrate,speed ).toUtf8() ;
		}else{
			auto r = currentTime * 100 / totalTime ;

			auto totalSize = totalTime * currentSize / currentTime ;

			auto totalSizeString = locale.formattedDataSize( totalSize ) ;
			auto currentSizeString = locale.formattedDataSize( currentSize ) ;

			auto completed = QString::number( r,'f',2 ) ;

			if( completed == "100.00" ){

				completed = "100" ;
			}

			auto frame   = this->getOption( m,"frame=" ) ;
			auto fps     = this->getOption( m,"fps=" ) ;
			auto bitrate = this->getOption( m,"bitrate=" ) ;
			auto speed   = this->getOption( m,"speed=" ) ;

			QString a1 = "%1 / ~%2 (%3%) at %4" ;
			auto a = a1.arg( currentSizeString,totalSizeString,completed,speed ).toUtf8() ;

			QString b1 = "Frame: %1, Fps: %2, Bitrate: %3" ;
			auto b = b1.arg( frame,fps,bitrate ).toUtf8() ;

			if( s.mainLogger() ){

				return a + ", " + b ;
			}else{
				return a + "\n" + b ;
			}
		}
	}

	mutable QByteArray m_tmp ;
	const engines::engine& m_engine ;
	mutable bool( *m_function )( const engines::engine&,const QByteArray& ) ;
} ;

engines::engine::baseEngine::FilterOutPut yt_dlp::filterOutput()
{
	return { util::types::type_identity< ytDlpFilter >(),m_engine } ;
}

class ytDlpMediainfo
{
public:
	ytDlpMediainfo( const QJsonArray& array )
	{
		for( const auto& it : array ){

			this->add( it.toObject() ) ;
		}
	}
	std::vector< engines::engine::baseEngine::mediaInfo > sort()
	{
		std::vector< engines::engine::baseEngine::mediaInfo > m ;

		std::sort( m_medias.begin(),m_medias.end(),std::less<int>() ) ;

		for( auto& it : m_medias ){

			m.emplace_back( it.mInfo() ) ;
		}

		return m ;
	}
private:
	enum class mediaType:int{ mhtml,videoOnly,audioOnly,audioVideo,unknown } ;

	void add( const QJsonObject& obj )
	{
		auto url       = obj.value( "url" ).toString() ;
		auto id        = obj.value( "format_id" ).toString() ;
		auto ext       = obj.value( "ext" ).toString() ;
		auto rsn       = obj.value( "resolution" ).toString() ;

		auto tbr       = QString::number( obj.value( "tbr" ).toDouble() ) ;
		auto vbr       = QString::number( obj.value( "vbr" ).toDouble() ) ;
		auto abr       = QString::number( obj.value( "abr" ).toDouble() ) ;
		auto asr       = QString::number( obj.value( "asr" ).toInt() ) ;

		auto container = obj.value( "container" ).toString() ;
		auto proto     = obj.value( "protocol" ).toString() ;
		auto vcodec    = obj.value( "vcodec" ).toString() ;
		auto acodec    = obj.value( "acodec" ).toString() ;
		auto fmtNotes  = obj.value( "format_note" ).toString() ;

		QStringList s ;

		QString ss ;

		if( container.isEmpty() ){

			ss = QString( "Proto: %1\n" ).arg( proto ) ;
		}else{
			auto m = QString( "Proto: %1%2\ncontainer: %2\n" ) ;
			ss = m.arg( proto,container ) ;
		}

		this->append( s,"acodec: ",acodec,false ) ;
		this->append( s,"vcodec: ",vcodec,false ) ;

		if( tbr != "0" ){

			this->append( s,"tbr: ",tbr,true ) ;
		}

		if( asr != "0" ){

			this->append( s,"asr: ",asr + "Hz",false ) ;
		}

		ytDlpMediainfo::mediaType mt = ytDlpMediainfo::mediaType::unknown ;

		if( ext == "mhtml" ){

			mt = ytDlpMediainfo::mediaType::mhtml ;
		}else{
			bool hasVideo = vcodec != "none" ;
			bool hasAudio = acodec != "none" ;

			if( hasVideo && hasAudio ){

				rsn += "\naudio video" ;

				this->append( s,"vbr: ",vbr,true ) ;
				this->append( s,"abr: ",abr,true ) ;

				mt = ytDlpMediainfo::mediaType::audioVideo ;

			}else if( hasVideo && !hasAudio ){

				if( !rsn.contains( "video only" ) ){

					rsn += "\nvideo only" ;
				}

				this->append( s,"vbr: ",vbr,true ) ;

				mt = ytDlpMediainfo::mediaType::videoOnly ;

			}else if( !hasVideo && hasAudio ){

				if( !rsn.contains( "audio only" ) ){

					rsn += "\naudio only" ;
				}

				this->append( s,"abr: ",abr,true ) ;

				mt = ytDlpMediainfo::mediaType::audioOnly ;
			}
		}

		if( !fmtNotes.isEmpty() ){

			rsn += "\n" + fmtNotes ;
		}

		QStringList arr{ url } ;

		auto size = this->fileSize( obj ) ;
		auto sizeRaw = this->fileSizeRaw( obj ) ;

		ss = ss + s.join( ", " ) ;

		m_medias.emplace_back( mt,arr,id,ext,rsn,size,sizeRaw,ss ) ;
	}
	QString fileSizeRaw( const QJsonObject& e )
	{
		auto m = e.value( "filesize" ).toInt( -1 ) ;

		if( m == -1 ){

			m = e.value( "filesize_approx" ).toInt( -1 ) ;

			if( m == -1 ){

				return "0" ;
			}else{
				return QString::number( m ) ;
			}
		}else{
			return QString::number( m ) ;
		}
	}
	QString fileSize( const QJsonObject& e )
	{
		auto m = e.value( "filesize" ).toInt( -1 ) ;

		if( m == -1 ){

			m = e.value( "filesize_approx" ).toInt( -1 ) ;

			if( m == -1 ){

				return "NA" ;
			}else{
				return "~" + m_locale.formattedDataSize( m ) ;
			}
		}else{
			return m_locale.formattedDataSize( m ) ;
		}
	}
	void append( QStringList& s,const char * str,const QString& sstr,bool formatBitrate )
	{
		if( sstr != "none" && !sstr.isEmpty() ){

			if( formatBitrate ){

				auto m = sstr.indexOf( '.' ) ;

				if( m == -1 ){

					s.append( str + sstr + "k" ) ;
				}else{
					s.append( str + sstr.mid( 0,m ) + "k" ) ;
				}
			}else{
				s.append( str + sstr ) ;
			}
		}
	}

	class str
	{
	public:
		template< typename ... T >
		str( ytDlpMediainfo::mediaType e,T&& ... t ) :
			m_media( std::forward< T >( t ) ... ),m_type( e )
		{
		}
		operator int()
		{
			return static_cast< int >( m_type ) ;
		}
		engines::engine::baseEngine::mediaInfo mInfo()
		{
			return std::move( m_media ) ;
		}
	private:
		engines::engine::baseEngine::mediaInfo m_media ;
		mediaType m_type ;
	};

	std::vector< str > m_medias ;
	Logger::locale m_locale ;
};

std::vector< engines::engine::baseEngine::mediaInfo >
yt_dlp::mediaProperties( Logger&,const QJsonArray& array )
{
	if( array.isEmpty() ){

		return {} ;
	}else{
		return ytDlpMediainfo( array ).sort() ;
	}
}

std::vector< engines::engine::baseEngine::mediaInfo >
yt_dlp::mediaProperties( Logger& l,const QByteArray& e )
{
	QJsonParseError err ;

	auto json = QJsonDocument::fromJson( e,&err ) ;

	if( err.error == QJsonParseError::NoError ){

		auto arr = json.object().value( "formats" ).toArray() ;

		return this->mediaProperties( l,arr ) ;
	}else{
		utility::failedToParseJsonData( l,err ) ;

		return {} ;
	}
}

bool yt_dlp::supportsShowingComments()
{
	return true ;
}

bool yt_dlp::updateVersionInfo()
{
	return false ;
}

void yt_dlp::updateLocalOptions( QStringList& opts )
{
	opts.prepend( "--break-on-reject" ) ;
	opts.prepend( "!playlist" ) ;
	opts.prepend( "--match-filter" ) ;
}

void yt_dlp::setProxySetting( QStringList& e,const QString& s )
{
	e.append( "--proxy" ) ;
	e.append( s ) ;
}

void yt_dlp::setTextEncondig( const QString& args,QStringList& opts )
{
	const auto& e = engines::engine::baseEngine::Settings().textEncoding() ;

	if( !e.isEmpty() && !args.isEmpty() ){

		opts.append( args ) ;
		opts.append( e ) ;
	}
}

engines::engine::baseEngine::DataFilter yt_dlp::Filter( int id )
{
	return { util::types::type_identity< yt_dlp::yt_dlplFilter >(),id,m_engine,*this } ;
}

QString yt_dlp::updateTextOnCompleteDownlod( const QString& uiText,
					     const QString& bkText,
					     const QString& dopts,
					     const engines::engine::baseEngine::finishedState& f )
{
	using functions = engines::engine::baseEngine ;

	if( f.cancelled() ){

		return functions::updateTextOnCompleteDownlod( bkText,dopts,f ) ;

	}else if( f.success() ){

		auto e = engines::engine::mediaAlreadInArchiveText() ;

		if( uiText.contains( e ) ){

			auto m = engines::engine::baseEngine::updateTextOnCompleteDownlod( bkText,dopts,f ) ;

			return m + "\n" + e  ;
		}

		QStringList a ;

		for( const auto& it : util::split( uiText,'\n',true ) ){

			auto x = engines::engine::baseEngine::postProcessing::processingText() ;
			auto y = engines::engine::baseEngine::preProcessing::processingText() ;

			if( !it.contains( x ) && !it.contains( y ) ){

				a.append( it ) ;
			}
		}

		return engines::engine::baseEngine::updateTextOnCompleteDownlod( a.join( "\n" ),dopts,f ) ;

	}else if( uiText == "EngineNeedUpdating" ){

		const auto& name = this->engine().name() ;
		auto version = "2023.01.06" ;

		return QObject::tr( "Please Update \"%1\" To Atleast Version \"%2\"" ).arg( name,version ) ;

	}else if( uiText.contains( "Requested format is not available" ) ){

		return functions::errorString( f,functions::errors::unknownFormat,bkText ) ;

	}else if( uiText.contains( "Temporary failure in name resolution" ) ){

		return functions::errorString( f,functions::errors::noNetwork,bkText ) ;

	}else if( uiText.contains( " is not a valid URL" ) ){

		return functions::errorString( f,functions::errors::unknownUrl,bkText ) ;

	}else if( uiText.contains( "ERROR: Unsupported URL:" ) ){

		return functions::errorString( f,functions::errors::notSupportedUrl,bkText ) ;
	}else{
		auto m = engines::engine::baseEngine::updateTextOnCompleteDownlod( uiText,dopts,f ) ;
		return m + "\n" + bkText ;
	}
}

void yt_dlp::updateDownLoadCmdOptions( const engines::engine::baseEngine::updateOpts& s,bool e )
{
	if( s.userOptions.contains( "--yes-playlist" ) ){

		s.ourOptions.removeAll( "--no-playlist" ) ;
	}

	if( !s.ourOptions.contains( "--newline" ) ){

		s.ourOptions.append( "--newline" ) ;
	}

	s.ourOptions.append( "--output-na-placeholder" ) ;
	s.ourOptions.append( "NA" ) ;

	QStringList mm ;

	auto _add = [ & ]( const QString& txt,const QString& original,const QString& New ){

		if( txt.contains( original ) ){

			mm.append( "--parse-metadata" ) ;
			mm.append( New + ":" + original ) ;
		}
	} ;

	for( int m = s.ourOptions.size() - 1 ; m > -1 ; m-- ){

		if( s.ourOptions[ m ] == "-o" ){

			if( m + 1 < s.ourOptions.size() ){

				auto& e = s.ourOptions[ m + 1 ] ;

				auto w = s.uiIndex.toString( true,s.ourOptions ) ;
				auto ww = s.uiIndex.toString( false,s.ourOptions ) ;

				_add( e,"%(autonumber)s",ww ) ;
				_add( e,"%(playlist_index)s",w ) ;
				_add( e,"%(playlist_autonumber)s",w ) ;
				_add( e,"%(playlist_id)s",s.playlist_id ) ;
				_add( e,"%(playlist_title)s",s.playlist_title ) ;
				_add( e,"%(playlist)s",s.playlist ) ;
				_add( e,"%(playlist_count)s",s.playlist_count ) ;
				_add( e,"%(playlist_uploader)s",s.playlist_uploader ) ;
				_add( e,"%(playlist_uploader_id)s",s.playlist_uploader_id ) ;
				_add( e,"%(n_entries)s",s.uiIndex.total() ) ;
			}

			break ;
		}
	}

	if( !mm.isEmpty() ){

		s.ourOptions.append( mm ) ;
	}

	engines::engine::baseEngine::updateDownLoadCmdOptions( s,e ) ;

	if( !s.ourOptions.contains( "-f" ) && !s.ourOptions.contains( "-S" ) ){

		s.ourOptions.append( "-f" ) ;

		s.ourOptions.append( configure::defaultDownloadOption() ) ;
	}

	this->appendCompatOption( s.ourOptions ) ;

	while( s.ourOptions.contains( "--progress-template" ) ){

		utility::arguments( s.ourOptions ).removeOptionWithArgument( "--progress-template" ) ;
	}

	parseTemplateOutPut::setTemplate( s.ourOptions ) ;
}

void yt_dlp::updateGetPlaylistCmdOptions( QStringList& e )
{
	e.append( "--lazy-playlist" ) ;

	this->appendCompatOption( e ) ;

	e.append( "--output-na-placeholder" ) ;
	e.append( "\"NA\"" ) ;
}

void yt_dlp::updateCmdOptions( QStringList& e )
{
	this->appendCompatOption( e ) ;

	e.append( "--output-na-placeholder" ) ;
	e.append( "\"NA\"" ) ;
}

yt_dlp::yt_dlplFilter::yt_dlplFilter( int processId,const engines::engine& engine,yt_dlp& p ) :
	engines::engine::baseEngine::filter( engine,processId ),m_engine( engine ),m_parent( p )
{
}

const QByteArray& yt_dlp::yt_dlplFilter::operator()( Logger::Data& s )
{
	if( s.lastText() == "[media-downloader] Download Cancelled" ){

		if( m_parent.m_deleteFilesOnCancel ){

			utility::deleteTmpFiles( m_parent.m_downloadFolder,std::move( m_fileNames ) ) ;
		}

	}else if( s.lastLineIsProgressLine() ){

		const auto& m = s.lastText() ;

		if( m.startsWith( "[download] " ) ){

			m_tmp = this->fileName() + "\n" + m.mid( 11 ) ;

		}else if( m_engine.name().contains( "aria2c" ) ){

			auto n = m.indexOf( ' ' ) ;

			if( n != -1 ){

				m_tmp = this->fileName() + "\n" + m.mid( n + 1 ) ;
			}else{
				m_tmp = this->fileName() + "\n" + m ;
			}

			aria2c::trimProgressLine( m_tmp ) ;
		}else{
			m_tmp = this->fileName() + "\n" + m ;
		}

		return m_tmp ;
	}

	const auto m = s.toStringList() ;

	if( s.doneDownloading() ){

		if( utility::stringConstants::downloadFailed( s.lastText() ) ){

			for( auto it = m.rbegin() ; it != m.rend() ; it++ ){

				const QByteArray& e = *it ;

				if( e.startsWith( "ERROR: " ) ){

					m_tmp = e ;

					return m_tmp ;
				}
			}
		}

		if( m_fileNames.empty() ){

			/*
			 * Failed to find file name, try again.
			 */

			this->parseOutput( m ) ;

			if( m_fileNames.empty() ){

				const auto& m = s.ytDlpData().filePath() ;

				if( !m.isEmpty() ){

					return m ;
				}else{
					//????
					return m_tmp ;
				}
			}else{
				s.addFileName( m_fileNames.back() ) ;

				return m_fileNames.back() ;
			}
		}else{
			s.addFileName( m_fileNames.back() ) ;

			return m_fileNames.back() ;
		}
	}

	this->setFileName( s.ytDlpData().filePath() ) ;

	return this->parseOutput( m ) ;
}

yt_dlp::yt_dlplFilter::~yt_dlplFilter()
{
}

QByteArray yt_dlp::yt_dlplFilter::fileName()
{
	if( m_fileNames.empty() ){

		return {} ;
	}else{
		return m_fileNames.back() ;
	}
}

const QByteArray& yt_dlp::yt_dlplFilter::parseOutput( const Logger::Data::QByteArrayList& data )
{
	for( const auto& m : data ){

		const QByteArray& e = m ;

		if( e.contains( this->compatYear() ) ){

			m_tmp = "EngineNeedUpdating" ;

			return m_tmp ;
		}
		if( e.startsWith( "ERROR: " ) || e.startsWith( "yt-dlp: error:" ) ){

			m_tmp = e ;

			return m_tmp ;
		}
		if( e.startsWith( "[download] " ) && e.contains( " has already been downloaded" ) ){

			auto m = e.mid( e.indexOf( " " ) + 1 ) ;
			m.truncate( m.indexOf( " has already been downloaded" ) ) ;

			this->setFileName( m ) ;
		}
		if( e.contains( "] Destination: " ) ){

			this->setFileName( e.mid( e.indexOf( "] Destination: " ) + 15 ) ) ;
		}
		if( e.contains( " Merging formats into \"" ) ){

			auto m = e.mid( e.indexOf( '"' ) + 1 ) ;
			auto s = m.lastIndexOf( '"' ) ;

			if( s != -1 ){

				m.truncate( s ) ;
			}

			this->setFileName( m ) ;
		}
		if( e.contains( "has already been recorded" ) ){

			m_tmp = engines::engine::mediaAlreadInArchiveText().toUtf8() ;

			return m_tmp ;
		}
	}

	return m_preProcessing.text() ;
}

void yt_dlp::yt_dlplFilter::setFileName( const QByteArray& fileName )
{
	if( fileName.isEmpty() ){

		return ;
	}

	for( const auto& it : m_fileNames ){

		if( it == fileName ){

			return ;
		}
	}

	m_fileNames.emplace_back( fileName ) ;
}
