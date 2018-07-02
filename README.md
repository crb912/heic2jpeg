# Convert heif to jpeg/png
**About project**：   

based on `nokiatech/heif`: https://github.com/nokiatech/heif  
Inspired by @yohhoy https://github.com/yohhoy/heic2hevc  

When I had really pulled my hair out that I didn't know how to deal with iPhone heic file,the following library helped me.  
https://github.com/monostream/tifig  
I use the code in my project.

This project is in developing, and it needs some tweak.

**Knowledge about HEIF format(written in Chinese)**:
[HEIF格式详细解读](https://www.crb912.top/computer%20science/2018/05/01/HEIF-format.html)

---
## Part Ⅰ: Dependencies
### 1. glib2.0-dev
Since `libvips` has to have `glib2.0-dev`, you must make sure that you have installed it. If you had done it, you could circumvent the step.
#### How to install`glib2.0-dev` ?
**ubuntu**：
```
sudo apt-get install libglib2.0-dev
```
**MSYS**：
```
pacman -S glib2-devel
pacman -S glib2 2.48.2-1
```

### 2. libvips
libvips is a 2D image processing library. I use the library for generating JPEG and PNG, including cropping tiles of iPhone heic file.
https://github.com/jcupitt/libvips
#### 2.1 Download Link
https://github.com/jcupitt/libvips/releases  
Recommended version:  vips  8.6.3
#### 2.2 Building libvips
To aviod using many dependencies, you'd better configure with the following arguments.

```
./configure --enable-debug=[yes] --enable-static --without-gsf  \
--without-fftw --without-magick --without-orc --without-lcms  --without-OpenEXR \
--without-poppler --without-rsvg  --without-zlib --without-openslide --without-matio \
--without-ppm  --without-analyze  --without-radiance  --without-cfitsio --without-libwebp\
 --without-pangoft2 --without-tiff --without-giflib --without-libexif
```
Check the summary at the end of `configure` carefully!  
Check the summary at the end of `configure` carefully!!  
Check the summary at the end of `configure` carefully!!!

When ending of `configure`, checking and make sure:
```
file import/export with libpng: 	yes (pkg-config libpng >= 1.2.9)
file import/export with libjpeg:	yes (pkg-config)
```

**Error handle 1**: lack of `Expat library`  
If you don't meet the problem, please skip.  
```
checking for XML_ParserCreate in -lexpat... no
configure: error: Could not find the
```
Error Source:libvips must have `libexpat1-dev`. So:
```
sudo apt install libexpat1-dev   // Ubuntu
```

**Error handle 2**: lack of `libpng`,`libjpeg`
```
file import/export with libpng: 	no
file import/export with libjpeg:	no   // or find by search
```
If you meet the problem, you must install them. For Ubuntu:

```
sudo apt install libpng16-dev   
sudo apt install libjpeg-dev
```

##### Continue
Once `configure` is looking OK, compile and install with the usual:
By default this will install files to `/usr/local`.
```
make
sudo make install  
```

### 3. FFmpeg
ffmpeg: https://www.ffmpeg.org/  
FFmpeg library support encoding and decoding of HEVC(H.254).  
Check：
- `libavcodec` >=3.1
- `libswscale` >=3.1

**Install or update FFmpeg library**：  
Ubuntu
```markdown
sudo apt install libavcodec-dev
sudo apt install libswscale-dev
```

Msys + mingw64
```
pacman -S mingw-w64-x86_64-ffmpeg
```

If you can't install FFmpeg with the above method, you should build FFmpeg source code.

FFmpeg Download Link：https://www.ffmpeg.org/download.html#releases  
Recommand Version：FFmpeg 3.4.2  "Cantor"

## Part Ⅱ: Build
1. At first, build Nokia heif library.
```
cd heif/build         
cmake ../srcs
cmake --build .
```
2. Waiting a few minutes,then
```
cd ..
cd ..
ls     // CMakeLists.txt  heif  heif2jpeg.cpp  READNE.md
madir build && cd build
cmake ..
cmake --build .
```
Check executable file(/build).  

**Error handle 1**: can't find `glib-object.h` or `glibconfig.h`  
The cause of the error is compiler don't have find the header file('glib2.0').  
Check file:
`/srcs/example/CmakeList.txt`
```
INCLUDE_DIRECTORIES(right path)  # glib-object.h
```

**Example**(ubuntu 16.04)：  
```
INCLUDE_DIRECTORIES(/usr/include/glib-2.0)  # glib-object.h
```

**Error handle 2**:  undefined reference  
I think you may not follow the guide build step by step, so that some additional dependencies are needed. Re-build or add those dependencies by yourself.

## Part Ⅲ: Usages
When compiler success,there is a executable file `heic2jpg` in `/build/`.

```
./heic2jpg inputFileName.heic outputFileName.jpeg
```
If you want to support more picture format, please change `libvips` configure arguments.

**Error handle**:  `libvips-cpp.so.42`
```
./heic2jpeg: error while loading shared libraries: libvips-cpp.so.42:
cannot open shared object file: No such file or directory
```
update your ldconfig cache with this command:
```
sudo ldconfig
```

**Suggestions for improvements are highly welcome!**
