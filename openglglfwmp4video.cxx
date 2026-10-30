#include <iostream>
#include <vector>

// FFmpeg C-API headers wrapped for C++
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

// Using GLFW for simple windowing and legacy context setup
#include <GLFW/glfw3.h>

// Structural tracking for video decoding state
struct VideoState {
    AVFormatContext* formatContext = nullptr;
    AVCodecContext*  codecContext  = nullptr;
    SwsContext*      swsContext    = nullptr;
    AVFrame*         avFrame       = nullptr;
    AVFrame*         glFrame       = nullptr;
    AVPacket*        packet        = nullptr;
    int              videoStreamIndex = -1;
    uint8_t*         buffer        = nullptr;
    GLuint           textureID     = 0;
};

// Initialise FFmpeg video decoder context
bool init_video(const char* filename, VideoState& state) {
    // 1. Open the file context
    if (avformat_open_input(&state.formatContext, filename, nullptr, nullptr) != 0) {
        std::cerr << "Error: Could not open video file.\n";
        return false;
    }

    // 2. Locate stream info
    if (avformat_find_stream_info(state.formatContext, nullptr) < 0) {
        std::cerr << "Error: Could not find stream parameters.\n";
        return false;
    }

    // 3. Find the primary video stream index
    const AVCodec* codec = nullptr;
    state.videoStreamIndex = av_find_best_stream(state.formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
    if (state.videoStreamIndex < 0) {
        std::cerr << "Error: Could not find a valid video stream.\n";
        return false;
    }

    // 4. Setup decoder context
    state.codecContext = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(state.codecContext, state.formatContext->streams[state.videoStreamIndex]->codecpar);
    
    if (avcodec_open2(state.codecContext, codec, nullptr) < 0) {
        std::cerr << "Error: Could not open codec.\n";
        return false;
    }

    // 5. Allocate frame structures
    state.avFrame = av_frame_alloc();
    state.glFrame = av_frame_alloc();
    state.packet  = av_packet_alloc();

    // 6. Set up software scaling context for YUV420P to RGB24 conversion
    int width = state.codecContext->width;
    int height = state.codecContext->height;
    
    int size = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
    state.buffer = (uint8_t*)av_malloc(size * sizeof(uint8_t));
    
    av_image_fill_arrays(state.glFrame->data, state.glFrame->linesize, state.buffer, 
                         AV_PIX_FMT_RGB24, width, height, 1);

    state.swsContext = sws_getContext(
        width, height, state.codecContext->pix_fmt,
        width, height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );

    return true;
}

// Grab and process the next single frame from the media stream
bool read_next_frame(VideoState& state) {
    while (av_read_frame(state.formatContext, state.packet) >= 0) {
        if (state.packet->stream_index == state.videoStreamIndex) {
            
            // Supply raw data packets to decoder
            if (avcodec_send_packet(state.codecContext, state.packet) == 0) {
                // Pull decoded frame back out
                if (avcodec_receive_frame(state.codecContext, state.avFrame) == 0) {
                    
                    // Convert color space on the CPU: YUV420P -> RGB24
                    sws_scale(state.swsContext, state.avFrame->data, state.avFrame->linesize, 0,
                              state.codecContext->height, state.glFrame->data, state.glFrame->linesize);
                    
                    av_packet_unref(state.packet);
                    return true; // We successfully extracted an RGB frame
                }
            }
        }
        av_packet_unref(state.packet);
    }
    return false; // End of file or error
}

void cleanup(VideoState& state) {
    if (state.textureID) glDeleteTextures(1, &state.textureID);
    if (state.buffer) av_free(state.buffer);
    if (state.swsContext) sws_freeContext(state.swsContext);
    if (state.avFrame) av_frame_free(&state.avFrame);
    if (state.glFrame) av_frame_free(&state.glFrame);
    if (state.packet) av_packet_free(&state.packet);
    if (state.codecContext) avcodec_free_context(&state.codecContext);
    if (state.formatContext) avformat_close_input(&state.formatContext);
}

int main() {
    VideoState state;
    const char* filename = "your_video.mp4";

    // Initialize Video Stream Decoder
    if (!init_video(filename, state)) return -1;

    // Setup Windowing Environment (GLFW)
    if (!glfwInit()) return -1;
    
    // Request a backward-compatible profile targeting Legacy OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    
    GLFWwindow* window = glfwCreateWindow(state.codecContext->width, state.codecContext->height, "Legacy OpenGL Video Playback", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // --- Legacy OpenGL Setup ---
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &state.textureID);
    glBindTexture(GL_TEXTURE_2D, state.textureID);
    
    // Pixel layout configurations
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Main Engine / Rendering Loop
    while (!glfwWindowShouldClose(window)) {
        // Read next frame from video file
        if (!read_next_frame(state)) {
            // Loop video when hitting the file termination point
            av_seek_frame(state.formatContext, state.videoStreamIndex, 0, AVSEEK_FLAG_BACKWARD);
            avcodec_flush_buffers(state.codecContext);
            continue;
        }

        // Upload new texture bitmap buffer synchronously to the GPU
        glBindTexture(GL_TEXTURE_2D, state.textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 
                    state.codecContext->width, state.codecContext->height, 
                    0, GL_RGB, GL_UNSIGNED_BYTE, state.glFrame->data[0]);

        // --- Traditional Immediate Mode Rendering Pipeline ---
        glClear(GL_COLOR_BUFFER_BIT);
        glLoadIdentity();

        glBegin(GL_QUADS);
            // Front-facing viewport quad mapping texture coordinates
            glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, -1.0f); // Bottom-Left (Note: flipped coordinates due to video storage layout)
            glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f, -1.0f); // Bottom-Right
            glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f,  1.0f); // Top-Right
            glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f,  1.0f); // Top-Left
        glEnd();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    cleanup(state);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
