/*
 *  Copyright (c) 2014 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <memory>
#include <string>
#include "common/tools_common.h"
#include "config/aom_config.h"
#include "test/codec_factory.h"
#include "test/decode_test_driver.h"
#include "test/ivf_video_source.h"
#include "test/md5_helper.h"
#include "test/test_vectors.h"
#include "test/util.h"
#if CONFIG_WEBM_IO
#include "test/webm_video_source.h"
#endif

namespace {

const int kVideoNameParam = 1;

struct ExternalFrameBuffer {
  uint8_t *data;
  size_t size;
  int in_use;
};

// Class to manipulate a list of external frame buffers.
class ExternalFrameBufferList {
 public:
  ExternalFrameBufferList()
      : num_buffers_(0), num_used_buffers_(0), ext_fb_list_(NULL) {}

  virtual ~ExternalFrameBufferList() {
    for (int i = 0; i < num_buffers_; ++i) {
      delete[] ext_fb_list_[i].data;
    }
    delete[] ext_fb_list_;
  }

  // Creates the list to hold the external buffers. Returns true on success.
  bool CreateBufferList(int num_buffers) {
    if (num_buffers < 0) return false;

    num_buffers_ = num_buffers;
    ext_fb_list_ = new ExternalFrameBuffer[num_buffers_];
    EXPECT_TRUE(ext_fb_list_ != NULL);
    memset(ext_fb_list_, 0, sizeof(ext_fb_list_[0]) * num_buffers_);
    return true;
  }

  // Searches the frame buffer list for a free frame buffer. Makes sure
  // that the frame buffer is at least |min_size| in bytes. Marks that the
  // frame buffer is in use by libaom. Finally sets |fb| to point to the
  // external frame buffer. Returns < 0 on an error.
  int GetFreeFrameBuffer(size_t min_size, aom_codec_frame_buffer_t *fb) {
    EXPECT_TRUE(fb != NULL);
    const int idx = FindFreeBufferIndex();
    if (idx == num_buffers_) return -1;

    if (ext_fb_list_[idx].size < min_size) {
      delete[] ext_fb_list_[idx].data;
      ext_fb_list_[idx].data = new uint8_t[min_size];
      memset(ext_fb_list_[idx].data, 0, min_size);
      ext_fb_list_[idx].size = min_size;
    }

    SetFrameBuffer(idx, fb);

    num_used_buffers_++;
    return 0;
  }

  // Test function that will not allocate any data for the frame buffer.
  // Returns < 0 on an error.
  int GetZeroFrameBuffer(size_t min_size, aom_codec_frame_buffer_t *fb) {
    EXPECT_TRUE(fb != NULL);
    const int idx = FindFreeBufferIndex();
    if (idx == num_buffers_) return -1;

    if (ext_fb_list_[idx].size < min_size) {
      delete[] ext_fb_list_[idx].data;
      ext_fb_list_[idx].data = NULL;
      ext_fb_list_[idx].size = min_size;
    }

    SetFrameBuffer(idx, fb);
    return 0;
  }

  // Marks the external frame buffer that |fb| is pointing to as free.
  // Returns < 0 on an error.
  int ReturnFrameBuffer(aom_codec_frame_buffer_t *fb) {
    if (fb == NULL) {
      EXPECT_TRUE(fb != NULL);
      return -1;
    }
    ExternalFrameBuffer *const ext_fb =
        reinterpret_cast<ExternalFrameBuffer *>(fb->priv);
    if (ext_fb == NULL) {
      EXPECT_TRUE(ext_fb != NULL);
      return -1;
    }
    EXPECT_EQ(1, ext_fb->in_use);
    ext_fb->in_use = 0;
    num_used_buffers_--;
    return 0;
  }

  // Checks that the aom_image_t data is contained within the external frame
  // buffer private data passed back in the aom_image_t.
  void CheckImageFrameBuffer(const aom_image_t *img) {
    const struct ExternalFrameBuffer *const ext_fb =
        reinterpret_cast<ExternalFrameBuffer *>(img->fb_priv);

    ASSERT_TRUE(img->planes[0] >= ext_fb->data &&
                img->planes[0] < (ext_fb->data + ext_fb->size));
  }

  int num_used_buffers() const { return num_used_buffers_; }

 private:
  // Returns the index of the first free frame buffer. Returns |num_buffers_|
  // if there are no free frame buffers.
  int FindFreeBufferIndex() {
    int i;
    // Find a free frame buffer.
    for (i = 0; i < num_buffers_; ++i) {
      if (!ext_fb_list_[i].in_use) break;
    }
    return i;
  }

  // Sets |fb| to an external frame buffer. idx is the index into the frame
  // buffer list.
  void SetFrameBuffer(int idx, aom_codec_frame_buffer_t *fb) {
    ASSERT_TRUE(fb != NULL);
    fb->data = ext_fb_list_[idx].data;
    fb->size = ext_fb_list_[idx].size;
    ASSERT_EQ(0, ext_fb_list_[idx].in_use);
    ext_fb_list_[idx].in_use = 1;
    fb->priv = &ext_fb_list_[idx];
  }

  int num_buffers_;
  int num_used_buffers_;
  ExternalFrameBuffer *ext_fb_list_;
};

#if CONFIG_WEBM_IO

// Callback used by libaom to request the application to return a frame
// buffer of at least |min_size| in bytes.
int get_aom_frame_buffer(void *user_priv, size_t min_size,
                         aom_codec_frame_buffer_t *fb) {
  ExternalFrameBufferList *const fb_list =
      reinterpret_cast<ExternalFrameBufferList *>(user_priv);
  return fb_list->GetFreeFrameBuffer(min_size, fb);
}

// Callback used by libaom to tell the application that |fb| is not needed
// anymore.
int release_aom_frame_buffer(void *user_priv, aom_codec_frame_buffer_t *fb) {
  ExternalFrameBufferList *const fb_list =
      reinterpret_cast<ExternalFrameBufferList *>(user_priv);
  return fb_list->ReturnFrameBuffer(fb);
}

// Callback will not allocate data for frame buffer.
int get_aom_zero_frame_buffer(void *user_priv, size_t min_size,
                              aom_codec_frame_buffer_t *fb) {
  ExternalFrameBufferList *const fb_list =
      reinterpret_cast<ExternalFrameBufferList *>(user_priv);
  return fb_list->GetZeroFrameBuffer(min_size, fb);
}

// Callback will allocate one less byte than |min_size|.
int get_aom_one_less_byte_frame_buffer(void *user_priv, size_t min_size,
                                       aom_codec_frame_buffer_t *fb) {
  ExternalFrameBufferList *const fb_list =
      reinterpret_cast<ExternalFrameBufferList *>(user_priv);
  return fb_list->GetFreeFrameBuffer(min_size - 1, fb);
}

// Callback will not release the external frame buffer.
int do_not_release_aom_frame_buffer(void *user_priv,
                                    aom_codec_frame_buffer_t *fb) {
  (void)user_priv;
  (void)fb;
  return 0;
}

#endif  // CONFIG_WEBM_IO

// Class for testing passing in external frame buffers to libaom.
class ExternalFrameBufferMD5Test
    : public ::libaom_test::DecoderTest,
      public ::libaom_test::CodecTestWithParam<const char *> {
 protected:
  ExternalFrameBufferMD5Test()
      : DecoderTest(GET_PARAM(::libaom_test::kCodecFactoryParam)),
        md5_file_(NULL), num_buffers_(0) {}

  virtual ~ExternalFrameBufferMD5Test() {
    if (md5_file_ != NULL) fclose(md5_file_);
  }

  virtual void PreDecodeFrameHook(
      const libaom_test::CompressedVideoSource &video,
      libaom_test::Decoder *decoder) {
    if (num_buffers_ > 0 && video.frame_number() == 0) {
      // Have libaom use frame buffers we create.
      ASSERT_TRUE(fb_list_.CreateBufferList(num_buffers_));
      ASSERT_EQ(AOM_CODEC_OK,
                decoder->SetFrameBufferFunctions(GetAV1FrameBuffer,
                                                 ReleaseAV1FrameBuffer, this));
    }
  }

  void OpenMD5File(const std::string &md5_file_name_) {
    md5_file_ = libaom_test::OpenTestDataFile(md5_file_name_);
    ASSERT_TRUE(md5_file_ != NULL)
        << "Md5 file open failed. Filename: " << md5_file_name_;
  }

  virtual void DecompressedFrameHook(const aom_image_t &img,
                                     const unsigned int frame_number) {
    ASSERT_TRUE(md5_file_ != NULL);
    char expected_md5[33];
    char junk[128];

    // Read correct md5 checksums.
    const int res = fscanf(md5_file_, "%s  %s", expected_md5, junk);
    ASSERT_NE(EOF, res) << "Read md5 data failed";
    expected_md5[32] = '\0';

    ::libaom_test::MD5 md5_res;

    const aom_img_fmt_t shifted_fmt =
        (aom_img_fmt)(img.fmt & ~AOM_IMG_FMT_HIGHBITDEPTH);
    if (img.bit_depth == 8 && shifted_fmt != img.fmt) {
      aom_image_t *img_shifted =
          aom_img_alloc(NULL, shifted_fmt, img.d_w, img.d_h, 16);
      img_shifted->bit_depth = img.bit_depth;
      img_shifted->monochrome = img.monochrome;
      aom_img_downshift(img_shifted, &img, 0);
      md5_res.Add(img_shifted);
      aom_img_free(img_shifted);
    } else {
      md5_res.Add(&img);
    }
    const char *const actual_md5 = md5_res.Get();

    // Check md5 match.
    ASSERT_STREQ(expected_md5, actual_md5)
        << "Md5 checksums don't match: frame number = " << frame_number;

    const struct ExternalFrameBuffer *const ext_fb =
        reinterpret_cast<ExternalFrameBuffer *>(img.fb_priv);

    ASSERT_TRUE(img.planes[0] >= ext_fb->data &&
                img.planes[0] < (ext_fb->data + ext_fb->size));
  }

  // Callback to get a free external frame buffer. Return value < 0 is an
  // error.
  static int GetAV1FrameBuffer(void *user_priv, size_t min_size,
                               aom_codec_frame_buffer_t *fb) {
    ExternalFrameBufferMD5Test *const md5Test =
        reinterpret_cast<ExternalFrameBufferMD5Test *>(user_priv);
    return md5Test->fb_list_.GetFreeFrameBuffer(min_size, fb);
  }

  // Callback to release an external frame buffer. Return value < 0 is an
  // error.
  static int ReleaseAV1FrameBuffer(void *user_priv,
                                   aom_codec_frame_buffer_t *fb) {
    ExternalFrameBufferMD5Test *const md5Test =
        reinterpret_cast<ExternalFrameBufferMD5Test *>(user_priv);
    return md5Test->fb_list_.ReturnFrameBuffer(fb);
  }

  void set_num_buffers(int num_buffers) { num_buffers_ = num_buffers; }
  int num_buffers() const { return num_buffers_; }

 private:
  FILE *md5_file_;
  int num_buffers_;
  ExternalFrameBufferList fb_list_;
};

#if CONFIG_WEBM_IO
const char kAV1TestFile[] = "av1-1-b8-03-sizeup.mkv";
const char kAV1NonRefTestFile[] = "av1-1-b8-01-size-226x226.ivf";

// Class for testing passing in external frame buffers to libaom.
class ExternalFrameBufferTest : public ::testing::Test {
 protected:
  ExternalFrameBufferTest() : video_(NULL), decoder_(NULL), num_buffers_(0) {}

  virtual void SetUp() {
    video_ = new libaom_test::WebMVideoSource(kAV1TestFile);
    ASSERT_TRUE(video_ != NULL);
    video_->Init();
    video_->Begin();

    aom_codec_dec_cfg_t cfg = aom_codec_dec_cfg_t();
    cfg.allow_lowbitdepth = 0;
    decoder_ = new libaom_test::AV1Decoder(cfg, 0);
    ASSERT_TRUE(decoder_ != NULL);
  }

  virtual void TearDown() {
    delete decoder_;
    decoder_ = NULL;
    delete video_;
    video_ = NULL;
  }

  // Passes the external frame buffer information to libaom.
  aom_codec_err_t SetFrameBufferFunctions(
      int num_buffers, aom_get_frame_buffer_cb_fn_t cb_get,
      aom_release_frame_buffer_cb_fn_t cb_release) {
    if (num_buffers > 0) {
      num_buffers_ = num_buffers;
      EXPECT_TRUE(fb_list_.CreateBufferList(num_buffers_));
    }

    return decoder_->SetFrameBufferFunctions(cb_get, cb_release, &fb_list_);
  }

  aom_codec_err_t DecodeOneFrame() {
    const aom_codec_err_t res =
        decoder_->DecodeFrame(video_->cxdata(), video_->frame_size());
    CheckDecodedFrames();
    if (res == AOM_CODEC_OK) video_->Next();
    return res;
  }

  aom_codec_err_t DecodeRemainingFrames() {
    for (; video_->cxdata() != NULL; video_->Next()) {
      const aom_codec_err_t res =
          decoder_->DecodeFrame(video_->cxdata(), video_->frame_size());
      if (res != AOM_CODEC_OK) return res;
      CheckDecodedFrames();
    }
    return AOM_CODEC_OK;
  }

 protected:
  void CheckDecodedFrames() {
    libaom_test::DxDataIterator dec_iter = decoder_->GetDxData();
    const aom_image_t *img = NULL;

    // Get decompressed data
    while ((img = dec_iter.Next()) != NULL) {
      fb_list_.CheckImageFrameBuffer(img);
    }
  }

  libaom_test::CompressedVideoSource *video_;
  libaom_test::AV1Decoder *decoder_;
  int num_buffers_;
  ExternalFrameBufferList fb_list_;
};

class ExternalFrameBufferNonRefTest : public ExternalFrameBufferTest {
 protected:
  virtual void SetUp() {
    video_ = new libaom_test::IVFVideoSource(kAV1NonRefTestFile);
    ASSERT_TRUE(video_ != NULL);
    video_->Init();
    video_->Begin();

    aom_codec_dec_cfg_t cfg = aom_codec_dec_cfg_t();
    cfg.allow_lowbitdepth = 0;
    decoder_ = new libaom_test::AV1Decoder(cfg, 0);
    ASSERT_TRUE(decoder_ != NULL);
  }

  virtual void CheckFrameBufferRelease() {
    TearDown();
    ASSERT_EQ(0, fb_list_.num_used_buffers());
  }
};
#endif  // CONFIG_WEBM_IO

// This test runs through the set of test vectors, and decodes them.
// Libaom will call into the application to allocate a frame buffer when
// needed. The md5 checksums are computed for each frame in the video file.
// If md5 checksums match the correct md5 data, then the test is passed.
// Otherwise, the test failed.
TEST_P(ExternalFrameBufferMD5Test, ExtFBMD5Match) {
  const std::string filename = GET_PARAM(kVideoNameParam);
  aom_codec_dec_cfg_t cfg = aom_codec_dec_cfg_t();

  // Number of buffers equals #AOM_MAXIMUM_REF_BUFFERS +
  // #AOM_MAXIMUM_WORK_BUFFERS + four jitter buffers.
  const int jitter_buffers = 4;
  const int num_buffers =
      AOM_MAXIMUM_REF_BUFFERS + AOM_MAXIMUM_WORK_BUFFERS + jitter_buffers;
  set_num_buffers(num_buffers);

  // Open compressed video file.
  std::unique_ptr<libaom_test::CompressedVideoSource> video;
  if (filename.substr(filename.length() - 3, 3) == "ivf") {
    video.reset(new libaom_test::IVFVideoSource(filename));
  } else {
#if CONFIG_WEBM_IO
    video.reset(new libaom_test::WebMVideoSource(filename));
#else
    fprintf(stderr, "WebM IO is disabled, skipping test vector %s\n",
            filename.c_str());
    return;
#endif
  }
  ASSERT_TRUE(video.get() != NULL);
  video->Init();

  // Construct md5 file name.
  const std::string md5_filename = filename + ".md5";
  OpenMD5File(md5_filename);

  // Set decode config.
  cfg.allow_lowbitdepth = 0;
  set_cfg(cfg);

  // Decode frame, and check the md5 matching.
  ASSERT_NO_FATAL_FAILURE(RunLoop(video.get(), cfg));
}

#if CONFIG_WEBM_IO
TEST_F(ExternalFrameBufferTest, MinFrameBuffers) {
  // Minimum number of external frame buffers for AV1 is
  // #AOM_MAXIMUM_REF_BUFFERS + #AOM_MAXIMUM_WORK_BUFFERS.
  const int num_buffers = AOM_MAXIMUM_REF_BUFFERS + AOM_MAXIMUM_WORK_BUFFERS;
  ASSERT_EQ(AOM_CODEC_OK,
            SetFrameBufferFunctions(num_buffers, get_aom_frame_buffer,
                                    release_aom_frame_buffer));
  ASSERT_EQ(AOM_CODEC_OK, DecodeRemainingFrames());
}

TEST_F(ExternalFrameBufferTest, EightJitterBuffers) {
  // Number of buffers equals #AOM_MAXIMUM_REF_BUFFERS +
  // #AOM_MAXIMUM_WORK_BUFFERS + eight jitter buffers.
  const int jitter_buffers = 8;
  const int num_buffers =
      AOM_MAXIMUM_REF_BUFFERS + AOM_MAXIMUM_WORK_BUFFERS + jitter_buffers;
  ASSERT_EQ(AOM_CODEC_OK,
            SetFrameBufferFunctions(num_buffers, get_aom_frame_buffer,
                                    release_aom_frame_buffer));
  ASSERT_EQ(AOM_CODEC_OK, DecodeRemainingFrames());
}

TEST_F(ExternalFrameBufferTest, NotEnoughBuffers) {
  // Minimum number of external frame buffers for AV1 is
  // #AOM_MAXIMUM_REF_BUFFERS + #AOM_MAXIMUM_WORK_BUFFERS. Most files will
  // only use 5 frame buffers at one time.
  const int num_buffers = 2;
  ASSERT_EQ(AOM_CODEC_OK,
            SetFrameBufferFunctions(num_buffers, get_aom_frame_buffer,
                                    release_aom_frame_buffer));
  ASSERT_EQ(AOM_CODEC_OK, DecodeOneFrame());
  // Only run this on long clips. Decoding a very short clip will return
  // AOM_CODEC_OK even with only 2 buffers.
  ASSERT_EQ(AOM_CODEC_MEM_ERROR, DecodeRemainingFrames());
}

TEST_F(ExternalFrameBufferTest, NoRelease) {
  const int num_buffers = AOM_MAXIMUM_REF_BUFFERS + AOM_MAXIMUM_WORK_BUFFERS;
  ASSERT_EQ(AOM_CODEC_OK,
            SetFrameBufferFunctions(num_buffers, get_aom_frame_buffer,
                                    do_not_release_aom_frame_buffer));
  ASSERT_EQ(AOM_CODEC_OK, DecodeOneFrame());
  ASSERT_EQ(AOM_CODEC_MEM_ERROR, DecodeRemainingFrames());
}

TEST_F(ExternalFrameBufferTest, NullRealloc) {
  const int num_buffers = AOM_MAXIMUM_REF_BUFFERS + AOM_MAXIMUM_WORK_BUFFERS;
  ASSERT_EQ(AOM_CODEC_OK,
            SetFrameBufferFunctions(num_buffers, get_aom_zero_frame_buffer,
                                    release_aom_frame_buffer));
  ASSERT_EQ(AOM_CODEC_MEM_ERROR, DecodeOneFrame());
}

TEST_F(ExternalFrameBufferTest, ReallocOneLessByte) {
  const int num_buffers = AOM_MAXIMUM_REF_BUFFERS + AOM_MAXIMUM_WORK_BUFFERS;
  ASSERT_EQ(AOM_CODEC_OK, SetFrameBufferFunctions(
                              num_buffers, get_aom_one_less_byte_frame_buffer,
                              release_aom_frame_buffer));
  ASSERT_EQ(AOM_CODEC_MEM_ERROR, DecodeOneFrame());
}

TEST_F(ExternalFrameBufferTest, NullGetFunction) {
  const int num_buffers = AOM_MAXIMUM_REF_BUFFERS + AOM_MAXIMUM_WORK_BUFFERS;
  ASSERT_EQ(
      AOM_CODEC_INVALID_PARAM,
      SetFrameBufferFunctions(num_buffers, NULL, release_aom_frame_buffer));
}

TEST_F(ExternalFrameBufferTest, NullReleaseFunction) {
  const int num_buffers = AOM_MAXIMUM_REF_BUFFERS + AOM_MAXIMUM_WORK_BUFFERS;
  ASSERT_EQ(AOM_CODEC_INVALID_PARAM,
            SetFrameBufferFunctions(num_buffers, get_aom_frame_buffer, NULL));
}

TEST_F(ExternalFrameBufferTest, SetAfterDecode) {
  const int num_buffers = AOM_MAXIMUM_REF_BUFFERS + AOM_MAXIMUM_WORK_BUFFERS;
  ASSERT_EQ(AOM_CODEC_OK, DecodeOneFrame());
  ASSERT_EQ(AOM_CODEC_ERROR,
            SetFrameBufferFunctions(num_buffers, get_aom_frame_buffer,
                                    release_aom_frame_buffer));
}

TEST_F(ExternalFrameBufferNonRefTest, ReleaseNonRefFrameBuffer) {
  const int num_buffers = AOM_MAXIMUM_REF_BUFFERS + AOM_MAXIMUM_WORK_BUFFERS;
  ASSERT_EQ(AOM_CODEC_OK,
            SetFrameBufferFunctions(num_buffers, get_aom_frame_buffer,
                                    release_aom_frame_buffer));
  ASSERT_EQ(AOM_CODEC_OK, DecodeRemainingFrames());
  CheckFrameBufferRelease();
}
#endif  // CONFIG_WEBM_IO

AV1_INSTANTIATE_TEST_SUITE(
    ExternalFrameBufferMD5Test,
    ::testing::ValuesIn(libaom_test::kAV1TestVectors,
                        libaom_test::kAV1TestVectors +
                            libaom_test::kNumAV1TestVectors));
}  // namespace
