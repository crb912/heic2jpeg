#include <fstream>
#include <iostream>
#include "heifreader.h"
#include <vips/vips8>
extern "C" {
#include <libavutil/imgutils.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

using namespace std;
using namespace HEIF;
using namespace vips;
void haveThumb(struct FileInformation fileInfo);
int iphoneHeic(HEIF_DLL_PUBLIC Reader* reader, Grid gridData, const char* filename);
AVCodecContext* getHEVCDecoderContext();

static struct SwsContext* swsContext;
struct rgbData
{
    uint8_t* data;
    size_t size;
    int width;
    int height;
};

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        cout << "Usage: ./heif2jpeg <input.heic> <output.jpeg>" << endl;
        return 0;
    }
    const char* inputFileName = argv[1];
    const char* outputFileName = argv[2];

    // Make an instance of Heif Reader
    auto *reader = Reader::Create();
    reader->initialize(inputFileName);

    // Verify that the file is HEIF format.
    FileInformation fileInfo;
    if (reader->getFileInformation(fileInfo) != ErrorCode::OK) {
        cout << "Unable to get MetaBox! Wrong heif format." << endl;
        return 0;
    }
    if (!(fileInfo.features & FileFeatureEnum::HasSingleImage ||
          fileInfo.features & FileFeatureEnum::HasImageCollection)) {
        cout << "The file don't have images in the Metabox. " << endl;
        return 0;
    }

    haveThumb(fileInfo);

    // when the file is iPhone heic.
    Array<ImageId> gridIds;
    Grid gridData;
    if (reader->getItemListByType("grid",gridIds) == ErrorCode::OK &&
        (fileInfo.features & FileFeatureEnum::HasImageCollection) &&
        reader->getItem(gridIds[0].get(), gridData) == ErrorCode::OK)
    {
        return iphoneHeic(reader, gridData, outputFileName);
    }

    // The image have collections
    uint64_t memoryBufferSize = 1024 * 1024;
    ofstream outfile(outputFileName, ofstream::binary);
    auto *memoryBuffer = new uint8_t[memoryBufferSize];
    if (fileInfo.features & FileFeatureEnum::HasImageCollection) {
        Array<ImageId> itemIds;
        reader->getMasterImages(itemIds);
        // all the image items
        for (unsigned int i = 0; i < itemIds.size; i++) {
            const ImageId masterId = itemIds[i];
            if (reader->getItemDataWithDecoderParameters(
                    masterId.get(), memoryBuffer, memoryBufferSize) == ErrorCode::OK) {
                DecoderConfiguration decodeConf; // struct containing
                reader->getDecoderParameterSets(masterId.get(), decodeConf);
                auto decoSpeInfo = decodeConf.decoderSpecificInfo;

                for (unsigned int j = 0; j < decoSpeInfo.size; ++j) {
                    auto entry = decoSpeInfo[j];
                    outfile.write((const char *) entry.decSpecInfoData.begin(),
                                  entry.decSpecInfoData.size);
                }
                outfile.write(reinterpret_cast<const char*>(memoryBuffer), memoryBufferSize);
            } else { cout << "getItemDataWithDecoderParameters error" << endl; }
        };
        delete[] memoryBuffer;
        Reader::Destroy(reader);
        return 0;
    }

    // The image only have 1 master image.
    else if (fileInfo.features & FileFeatureEnum::HasSingleImage) {
        Array<ImageId> itemIds;
        reader->getMasterImages(itemIds);

        // Find the item ID of the first master image
        const ImageId masterId = itemIds[0];
        if (reader->getItemDataWithDecoderParameters(masterId.get(), memoryBuffer, memoryBufferSize) == ErrorCode::OK) {
            outfile.write(reinterpret_cast<const char*>(memoryBuffer), memoryBufferSize);
            cout << "Get the master image" << endl;
            delete[] memoryBuffer;
        }
        Reader::Destroy(reader);
        return 0;
    }
    cout << "There no image in the file MetaBox! ";
    Reader::Destroy(reader);
    return 0;
}


//  Verify that the file have Thumbnail.
void haveThumb(FileInformation fileInfo)
{
    const auto metaBoxFeatures = fileInfo.rootMetaBoxInformation.features;
    if (metaBoxFeatures & MetaBoxFeatureEnum::HasThumbnails)
    {
        cout << "The file have Thumbnail." << endl;
    }
}

int iphoneHeic(HEIF_DLL_PUBLIC Reader* reader, Grid gridData, const char* outputFileName)
{
    Array<ImageId> tileItemIds;
    cout << "Width: " << gridData.outputWidth << ",Height: "  << gridData.outputHeight << endl;
    cout << "Columns: "<< gridData.columns << ",Row: " << gridData.rows << endl;
    tileItemIds = gridData.imageIds;
    cout << "Tiles nums:" << tileItemIds.size << endl;
    // tiles container
    vector<vips::VImage> tiles;

    for (auto tileItemId : tileItemIds)
    {
        avcodec_register_all();
        uint64_t memoryBufferSize =  1024 * 1024;
        auto *memoryBuffer = new uint8_t[memoryBufferSize];

        if (reader->getItemDataWithDecoderParameters(tileItemId.get(),
            memoryBuffer, memoryBufferSize) ==ErrorCode::OK)
        {
            // Get HEVC decoder configuration
            AVCodecContext* c = getHEVCDecoderContext();
            AVFrame* frame = av_frame_alloc();
            AVPacket* packet = av_packet_alloc();

            packet->size = static_cast<int>(memoryBufferSize);
            packet->data = ((uint8_t*)(&memoryBuffer[0]));
            auto* errorDescription = new char[256];

            avcodec_register_all();
            // handle error!
            int sent = avcodec_send_packet(c, packet);
            if (sent < 0)
            {
                av_strerror(sent, errorDescription, 256);
                cerr << "Error sending packet to HEVC decoder: " << errorDescription << endl;
                throw sent;
            }
            int success = avcodec_receive_frame(c, frame);
            if (success != 0)
            {
                av_strerror(success, errorDescription, 256);
                cerr << "Error decoding frame: " << errorDescription << endl;
                throw success;
            }
            delete[] errorDescription;
            size_t bufferSize = static_cast<size_t>(av_image_get_buffer_size(
                AV_PIX_FMT_RGB24, frame->width, frame->height, 1));


            struct rgbData rgb =
            {
                .data = (uint8_t *) malloc(bufferSize),
                .size = bufferSize,
                .width = frame->width,
                .height = frame->height,
            };

            // Convert colorspace of decoded frame load into buffer
            // copyFrameInto(frame, rgb.data, rgb.size);
            AVFrame* imgFrame = av_frame_alloc();
            auto tempBuffer = (uint8_t*) av_malloc(rgb.size);
            struct SwsContext *sws_ctx = sws_getCachedContext(
              swsContext,
              rgb.width, rgb.height, AV_PIX_FMT_YUV420P,
              rgb.width, rgb.height, AV_PIX_FMT_RGB24,
              0, nullptr, nullptr, nullptr);

            av_image_fill_arrays(imgFrame->data, imgFrame->linesize,
              tempBuffer, AV_PIX_FMT_RGB24, rgb.width, rgb.height, 1);
            auto* const* frameDataPtr = (uint8_t const* const*)frame->data;

            // Convert YUV to RGB
            sws_scale(sws_ctx, frameDataPtr, frame->linesize, 0,
              rgb.height, imgFrame->data, imgFrame->linesize);

            // Move RGB data in pixel order into memory
            auto dataPtr = static_cast<const uint8_t* const*>(imgFrame->data);
            auto size = static_cast<int>(rgb.size);
            int ret = av_image_copy_to_buffer(rgb.data, size, dataPtr,
               imgFrame->linesize, AV_PIX_FMT_RGB24,  rgb.width, rgb.height, 1);

            av_free(imgFrame);
            av_free(tempBuffer);
            avcodec_close(c);
            av_free(c);
            av_free(frame);

            vips::VImage imgTile = vips::VImage::new_from_memory(rgb.data, rgb.size,
                rgb.width, rgb.height, 3, VIPS_FORMAT_UCHAR);
            tiles.push_back(imgTile);
        }
        delete[] memoryBuffer;
    }

    // Stitch tiles together
    vips::VImage iphonePic = vips::VImage::new_memory();
    iphonePic = iphonePic.arrayjoin(tiles, vips::VImage::option()->set("across", (int)gridData.columns));
    iphonePic = iphonePic.extract_area(0, 0, gridData.outputWidth, gridData.outputHeight);

    // Write out
    VipsBlob* jpegBuffer = iphonePic.jpegsave_buffer(vips:: VImage::option()->set("Q", 90));
    ofstream outfile(outputFileName, ofstream::binary);
    outfile.write(static_cast<const char *>(jpegBuffer->area.data), jpegBuffer->area.length);

    cout << ":) Congratulations! Finished." << endl;
    Reader::Destroy(reader);
    return 0;
}

// Verify that the heif file is encoded with HEVC.
AVCodecContext* getHEVCDecoderContext()
{
    AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_HEVC);
    if (codec == NULL)
    {
        throw logic_error("avcodec_find_decoder retun NULL");
    }
    AVCodecContext *c = avcodec_alloc_context3(codec);

    if (!c) {
        throw logic_error("Could not allocate video codec context");
    }

    if (avcodec_open2(c, c->codec, nullptr) < 0) {
        throw logic_error("Could not open codec");
    }
    return c;
}
