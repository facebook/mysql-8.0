/* Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

/*
  Bug#16403708 SUBOPTIMAL CODE IN MY_STRNXFRM_SIMPLE()
  Bug#68476    Suboptimal code in my_strnxfrm_simple()

  Below we test some alternative implementations for my_strnxfrm_simple.
  In order to do benchmarking, configure in optimized mode, and
  generate a separate executable for this file:
    cmake -DMERGE_UNITTESTS=0
  You may want to tweak some constants below:
   - experiment with num_iterations
  run './strings_strnxfrm-t --disable-tap-output'
    to see timing reports for your platform.


  Benchmarking with gcc and clang indicates that:

  There is insignificant difference between my_strnxfrm_simple and strnxfrm_new
  when src != dst

  my_strnxfrm_simple() is significantly faster than strnxfrm_new
  when src == dst, especially for long strings.

  Loop unrolling gives significant speedup for large strings.
 */

// First include (the generated) my_config.h, to get correct platform defines.
#include "my_config.h"
#include <gtest/gtest.h>
#include <vector>

#include "benchmark.h"
#include "strnxfrm.h"
#include "template_utils.h"

namespace strnxfrm_unittest {

#if defined(GTEST_HAS_PARAM_TEST)

#if !defined(DBUG_OFF)
// There is no point in benchmarking anything in debug mode.
const size_t num_iterations= 1ULL;
#else
// Set this so that each test case takes a few seconds.
// And set it back to a small value before pushing!!
// const size_t num_iterations= 20000000ULL;
const size_t num_iterations= 2ULL;
#endif

class StrnxfrmTest : public ::testing::TestWithParam<size_t>
{
protected:
  virtual void SetUp()
  {
    m_length= GetParam();
    m_src.assign(m_length, 0x20);
    m_dst.assign(m_length, 0x20);
  }
  std::vector<uchar> m_src;
  std::vector<uchar> m_dst;
  size_t m_length;
};

size_t test_values[]= {1, 10, 100, 1000};

INSTANTIATE_TEST_CASE_P(Strnxfrm, StrnxfrmTest,
                        ::testing::ValuesIn(test_values));

TEST_P(StrnxfrmTest, OriginalSrcDst)
{
  for (size_t ix= 0; ix < num_iterations; ++ix)
    strnxfrm_orig(&my_charset_latin1,
                  &m_dst[0], m_length, m_length,
                  &m_src[0], m_length, 192);
}

TEST_P(StrnxfrmTest, OriginalUnrolledSrcDst)
{
  for (size_t ix= 0; ix < num_iterations; ++ix)
    strnxfrm_orig_unrolled(&my_charset_latin1,
                           &m_dst[0], m_length, m_length,
                           &m_src[0], m_length, 192);
}

TEST_P(StrnxfrmTest, ModifiedSrcDst)
{
  for (size_t ix= 0; ix < num_iterations; ++ix)
    strnxfrm_new(&my_charset_latin1,
                 &m_dst[0], m_length, m_length,
                 &m_src[0], m_length, 192);
}

TEST_P(StrnxfrmTest, ModifiedUnrolledSrcDst)
{
  for (size_t ix= 0; ix < num_iterations; ++ix)
    strnxfrm_new_unrolled(&my_charset_latin1,
                          &m_dst[0], m_length, m_length,
                          &m_src[0], m_length, 192);
}

TEST_P(StrnxfrmTest, OriginalSrcSrc)
{
  for (size_t ix= 0; ix < num_iterations; ++ix)
    strnxfrm_orig(&my_charset_latin1,
                  &m_src[0], m_length, m_length,
                  &m_src[0], m_length, 192);
}

TEST_P(StrnxfrmTest, OriginalUnrolledSrcSrc)
{
  for (size_t ix= 0; ix < num_iterations; ++ix)
    strnxfrm_orig_unrolled(&my_charset_latin1,
                           &m_src[0], m_length, m_length,
                           &m_src[0], m_length, 192);
}

TEST_P(StrnxfrmTest, ModifiedSrcSrc)
{
  for (size_t ix= 0; ix < num_iterations; ++ix)
    strnxfrm_new(&my_charset_latin1,
                 &m_src[0], m_length, m_length,
                 &m_src[0], m_length, 192);
}

TEST_P(StrnxfrmTest, ModifiedUnrolledSrcSrc)
{
  for (size_t ix= 0; ix < num_iterations; ++ix)
    strnxfrm_new_unrolled(&my_charset_latin1,
                          &m_src[0], m_length, m_length,
                          &m_src[0], m_length, 192);
}

#endif  // GTEST_HAS_PARAM_TEST

TEST(StrXfrmTest, SimpleUTF8Correctness)
{
  const char* src= "abc æøå 日本語";
  unsigned char buf[32];

  static const unsigned char full_answer_with_pad[32] = {
    0x00, 0x61, 0x00, 0x62, 0x00, 0x63,  // abc
    0x00, 0x20,  // space
    0x00, 0xe6, 0x00, 0xf8, 0x00, 0xe5,  // æøå
    0x00, 0x20,  // space
    0x65, 0xe5, 0x67, 0x2c, 0x8a, 0x9e,  // 日本語
    0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20  // space for padding
  };

  for (size_t maxlen= 0; maxlen < sizeof(buf); ++maxlen) {
    memset(buf, 0xff, sizeof(buf));
    my_strnxfrm(&my_charset_utf8_bin, buf, maxlen, reinterpret_cast<const uchar *>(src), strlen(src));
    EXPECT_EQ(0, memcmp(buf, full_answer_with_pad, maxlen))
      << "Wrong answer for maxlen " << maxlen;
  }
}

// Benchmark based on reduced test case in Bug #83247 / #24788778.
//
// Note: This benchmark does not exercise any real multibyte characters;
// it is mostly exercising padding. If we change the test string to contain
// e.g. Japanese characters, performance goes down by ~20%.
static void BM_SimpleUTF8(size_t num_iterations)
{
  StopBenchmarkTiming();

  static constexpr int key_cols = 12;
  static constexpr int set_key_cols = 6;  // Only the first half is set.
  static constexpr int key_col_chars = 80;
  static constexpr int bytes_per_char = 3;
  static constexpr int key_bytes = key_col_chars * bytes_per_char;
  static constexpr int buffer_bytes = key_cols * key_bytes;

  unsigned char source[buffer_bytes];
  unsigned char dest[buffer_bytes];

  const char *content= "PolyFilla27773";
  const int len= strlen(content);

  for (int k= 0, offset= 0; k < set_key_cols; ++k, offset+= key_bytes)
  {
    memcpy(source + offset, content, len);
  }

  StartBenchmarkTiming();
  for (size_t i= 0; i < num_iterations; ++i)
  {
    for (int k= 0, offset= 0; k < key_cols; ++k, offset+= key_bytes)
    {
      if (k < set_key_cols)
      {
        my_strnxfrm(&my_charset_utf8_bin, dest + offset, key_bytes, source + offset, len);
      }
      else
      {
        my_strnxfrm(&my_charset_utf8_bin, dest + offset, key_bytes, source + offset, 0);
      }
    }
  }
  StopBenchmarkTiming();
}
BENCHMARK(BM_SimpleUTF8);

// Verifies using my_charpos to find the length of a string.
// hp_hash.c does this extensively. Not really a strnxfrm benchmark,
// but belongs to the same optimization effort.
static void BM_UTF8MB4StringLength(size_t num_iterations)
{
  StopBenchmarkTiming();

  CHARSET_INFO *cs = &my_charset_utf8mb4_0900_ai_ci;

  // Some English text, then some Norwegian text, then some Japanese,
  // and then a few emoji (the last with skin tone modifiers).
  const char *content= "Premature optimization is the root of all evil. "
    "Våre norske tegn bør æres. 日本語が少しわかります。 ✌️🐶👩🏽";
  const int len= strlen(content);
  int tot_len= 0;

  StartBenchmarkTiming();
  for (size_t i= 0; i < num_iterations; ++i)
  {
    tot_len+= my_charpos(cs, content, content + len, len / cs->mbmaxlen);
  }
  StopBenchmarkTiming();

  EXPECT_NE(0, tot_len);
}
BENCHMARK(BM_UTF8MB4StringLength);

// Benchmark testing the default recommended collation for 8.0, without
// stressing padding as much, but still testing only Latin letters.
static void BM_SimpleUTF8MB4(size_t num_iterations)
{
  StopBenchmarkTiming();

  const char *content= "This is a rather long string that contains only "
    "simple letters that are available in ASCII. This is a common special "
    "case that warrants a benchmark on its own, even if the character set "
    "and collation supports much more complicated scenarios.";
  const int len= strlen(content);

  // Just recorded from a trial run on the string above. The last six
  // bytes are padding.
  static constexpr uchar expected[]= {
    0x1e, 0x95, 0x1d, 0x18, 0x1d, 0x32, 0x1e, 0x71,
    0x02, 0x09, 0x1d, 0x32, 0x1e, 0x71, 0x02, 0x09,
    0x1c, 0x47, 0x02, 0x09, 0x1e, 0x33, 0x1c, 0x47,
    0x1e, 0x95, 0x1d, 0x18, 0x1c, 0xaa, 0x1e, 0x33,
    0x02, 0x09, 0x1d, 0x77, 0x1d, 0xdd, 0x1d, 0xb9,
    0x1c, 0xf4, 0x02, 0x09, 0x1e, 0x71, 0x1e, 0x95,
    0x1e, 0x33, 0x1d, 0x32, 0x1d, 0xb9, 0x1c, 0xf4,
    0x02, 0x09, 0x1e, 0x95, 0x1d, 0x18, 0x1c, 0x47,
    0x1e, 0x95, 0x02, 0x09, 0x1c, 0x7a, 0x1d, 0xdd,
    0x1d, 0xb9, 0x1e, 0x95, 0x1c, 0x47, 0x1d, 0x32,
    0x1d, 0xb9, 0x1e, 0x71, 0x02, 0x09, 0x1d, 0xdd,
    0x1d, 0xb9, 0x1d, 0x77, 0x1f, 0x0b, 0x02, 0x09,
    0x1e, 0x71, 0x1d, 0x32, 0x1d, 0xaa, 0x1e, 0x0c,
    0x1d, 0x77, 0x1c, 0xaa, 0x02, 0x09, 0x1d, 0x77,
    0x1c, 0xaa, 0x1e, 0x95, 0x1e, 0x95, 0x1c, 0xaa,
    0x1e, 0x33, 0x1e, 0x71, 0x02, 0x09, 0x1e, 0x95,
    0x1d, 0x18, 0x1c, 0x47, 0x1e, 0x95, 0x02, 0x09,
    0x1c, 0x47, 0x1e, 0x33, 0x1c, 0xaa, 0x02, 0x09,
    0x1c, 0x47, 0x1e, 0xe3, 0x1c, 0x47, 0x1d, 0x32,
    0x1d, 0x77, 0x1c, 0x47, 0x1c, 0x60, 0x1d, 0x77,
    0x1c, 0xaa, 0x02, 0x09, 0x1d, 0x32, 0x1d, 0xb9,
    0x02, 0x09, 0x1c, 0x47, 0x1e, 0x71, 0x1c, 0x7a,
    0x1d, 0x32, 0x1d, 0x32, 0x02, 0x77, 0x02, 0x09,
    0x1e, 0x95, 0x1d, 0x18, 0x1d, 0x32, 0x1e, 0x71,
    0x02, 0x09, 0x1d, 0x32, 0x1e, 0x71, 0x02, 0x09,
    0x1c, 0x47, 0x02, 0x09, 0x1c, 0x7a, 0x1d, 0xdd,
    0x1d, 0xaa, 0x1d, 0xaa, 0x1d, 0xdd, 0x1d, 0xb9,
    0x02, 0x09, 0x1e, 0x71, 0x1e, 0x0c, 0x1c, 0xaa,
    0x1c, 0x7a, 0x1d, 0x32, 0x1c, 0x47, 0x1d, 0x77,
    0x02, 0x09, 0x1c, 0x7a, 0x1c, 0x47, 0x1e, 0x71,
    0x1c, 0xaa, 0x02, 0x09, 0x1e, 0x95, 0x1d, 0x18,
    0x1c, 0x47, 0x1e, 0x95, 0x02, 0x09, 0x1e, 0xf5,
    0x1c, 0x47, 0x1e, 0x33, 0x1e, 0x33, 0x1c, 0x47,
    0x1d, 0xb9, 0x1e, 0x95, 0x1e, 0x71, 0x02, 0x09,
    0x1c, 0x47, 0x02, 0x09, 0x1c, 0x60, 0x1c, 0xaa,
    0x1d, 0xb9, 0x1c, 0x7a, 0x1d, 0x18, 0x1d, 0xaa,
    0x1c, 0x47, 0x1e, 0x33, 0x1d, 0x65, 0x02, 0x09,
    0x1d, 0xdd, 0x1d, 0xb9, 0x02, 0x09, 0x1d, 0x32,
    0x1e, 0x95, 0x1e, 0x71, 0x02, 0x09, 0x1d, 0xdd,
    0x1e, 0xf5, 0x1d, 0xb9, 0x02, 0x22, 0x02, 0x09,
    0x1c, 0xaa, 0x1e, 0xe3, 0x1c, 0xaa, 0x1d, 0xb9,
    0x02, 0x09, 0x1d, 0x32, 0x1c, 0xe5, 0x02, 0x09,
    0x1e, 0x95, 0x1d, 0x18, 0x1c, 0xaa, 0x02, 0x09,
    0x1c, 0x7a, 0x1d, 0x18, 0x1c, 0x47, 0x1e, 0x33,
    0x1c, 0x47, 0x1c, 0x7a, 0x1e, 0x95, 0x1c, 0xaa,
    0x1e, 0x33, 0x02, 0x09, 0x1e, 0x71, 0x1c, 0xaa,
    0x1e, 0x95, 0x02, 0x09, 0x1c, 0x47, 0x1d, 0xb9,
    0x1c, 0x8f, 0x02, 0x09, 0x1c, 0x7a, 0x1d, 0xdd,
    0x1d, 0x77, 0x1d, 0x77, 0x1c, 0x47, 0x1e, 0x95,
    0x1d, 0x32, 0x1d, 0xdd, 0x1d, 0xb9, 0x02, 0x09,
    0x1e, 0x71, 0x1e, 0xb5, 0x1e, 0x0c, 0x1e, 0x0c,
    0x1d, 0xdd, 0x1e, 0x33, 0x1e, 0x95, 0x1e, 0x71,
    0x02, 0x09, 0x1d, 0xaa, 0x1e, 0xb5, 0x1c, 0x7a,
    0x1d, 0x18, 0x02, 0x09, 0x1d, 0xaa, 0x1d, 0xdd,
    0x1e, 0x33, 0x1c, 0xaa, 0x02, 0x09, 0x1c, 0x7a,
    0x1d, 0xdd, 0x1d, 0xaa, 0x1e, 0x0c, 0x1d, 0x77,
    0x1d, 0x32, 0x1c, 0x7a, 0x1c, 0x47, 0x1e, 0x95,
    0x1c, 0xaa, 0x1c, 0x8f, 0x02, 0x09, 0x1e, 0x71,
    0x1c, 0x7a, 0x1c, 0xaa, 0x1d, 0xb9, 0x1c, 0x47,
    0x1e, 0x33, 0x1d, 0x32, 0x1d, 0xdd, 0x1e, 0x71,
    0x02, 0x77, 0x02, 0x09, 0x02, 0x09, 0x02, 0x09,
  };
  uchar dest[sizeof(expected)];

  StartBenchmarkTiming();
  for (size_t i= 0; i < num_iterations; ++i)
  {
    my_strnxfrm(&my_charset_utf8mb4_0900_ai_ci, dest, sizeof(dest),
      reinterpret_cast<const uchar *>(content), len);
  }
  StopBenchmarkTiming();

  for (size_t i= 0; i < sizeof(dest); ++i) {
    EXPECT_EQ(expected[i], dest[i])
      << "Weights differ in position " << i;
  }
}
BENCHMARK(BM_SimpleUTF8MB4);

// Benchmark testing a wider variety of character sets on a more complicated
// collation (the recommended default collation for 8.0), without stressing
// padding as much.
static void BM_MixedUTF8MB4(size_t num_iterations)
{
  StopBenchmarkTiming();

  // Some English text, then some Norwegian text, then some Japanese,
  // and then a few emoji (the last with skin tone modifiers).
  const char *content= "Premature optimization is the root of all evil. "
    "Våre norske tegn bør æres. 日本語が少しわかります。 ✌️🐶👩🏽";
  const int len= strlen(content);

  // Just recorded from a trial run on the string above. The last four
  // bytes are padding.
  static constexpr uchar expected[]= {
    0x1e, 0x0c, 0x1e, 0x33, 0x1c, 0xaa, 0x1d, 0xaa, 0x1c,
    0x47, 0x1e, 0x95, 0x1e, 0xb5, 0x1e, 0x33, 0x1c, 0xaa,
    0x02, 0x09, 0x1d, 0xdd, 0x1e, 0x0c, 0x1e, 0x95, 0x1d,
    0x32, 0x1d, 0xaa, 0x1d, 0x32, 0x1f, 0x21, 0x1c, 0x47,
    0x1e, 0x95, 0x1d, 0x32, 0x1d, 0xdd, 0x1d, 0xb9, 0x02,
    0x09, 0x1d, 0x32, 0x1e, 0x71, 0x02, 0x09, 0x1e, 0x95,
    0x1d, 0x18, 0x1c, 0xaa, 0x02, 0x09, 0x1e, 0x33, 0x1d,
    0xdd, 0x1d, 0xdd, 0x1e, 0x95, 0x02, 0x09, 0x1d, 0xdd,
    0x1c, 0xe5, 0x02, 0x09, 0x1c, 0x47, 0x1d, 0x77, 0x1d,
    0x77, 0x02, 0x09, 0x1c, 0xaa, 0x1e, 0xe3, 0x1d, 0x32,
    0x1d, 0x77, 0x02, 0x77, 0x02, 0x09, 0x1e, 0xe3, 0x1c,
    0x47, 0x1e, 0x33, 0x1c, 0xaa, 0x02, 0x09, 0x1d, 0xb9,
    0x1d, 0xdd, 0x1e, 0x33, 0x1e, 0x71, 0x1d, 0x65, 0x1c,
    0xaa, 0x02, 0x09, 0x1e, 0x95, 0x1c, 0xaa, 0x1c, 0xf4,
    0x1d, 0xb9, 0x02, 0x09, 0x1c, 0x60, 0x1d, 0xdd, 0x1e,
    0x33, 0x02, 0x09, 0x1c, 0x47, 0x1c, 0xaa, 0x1e, 0x33,
    0x1c, 0xaa, 0x1e, 0x71, 0x02, 0x77, 0x02, 0x09, 0xfb,
    0x40, 0xe5, 0xe5, 0xfb, 0x40, 0xe7, 0x2c, 0xfb, 0x41,
    0x8a, 0x9e, 0x3d, 0x60, 0xfb, 0x40, 0xdc, 0x11, 0x3d,
    0x66, 0x3d, 0x87, 0x3d, 0x60, 0x3d, 0x83, 0x3d, 0x79,
    0x3d, 0x67, 0x02, 0x8a, 0x02, 0x09, 0x0a, 0x2d, 0x13,
    0xdf, 0x14, 0x12, 0x13, 0xa6, 0x02, 0x09, 0x02, 0x09
  };
  uchar dest[sizeof(expected)];

  StartBenchmarkTiming();
  for (size_t i= 0; i < num_iterations; ++i)
  {
    my_strnxfrm(&my_charset_utf8mb4_0900_ai_ci, dest, sizeof(dest),
      reinterpret_cast<const uchar *>(content), len);
  }
  StopBenchmarkTiming();

  for (size_t i= 0; i < sizeof(dest); ++i) {
    EXPECT_EQ(expected[i], dest[i])
      << "Weights differ in position " << i;
  }
}
BENCHMARK(BM_MixedUTF8MB4);

// Case-sensitive, accent-sensitive benchmark, using the same string as
// BM_SimpleUTF8MB4. This will naturally be slower, since many more weights
// need to be generated.
static void BM_MixedUTF8MB4_AS_CS(size_t num_iterations)
{
  StopBenchmarkTiming();

  // Some English text, then some Norwegian text, then some Japanese,
  // and then a few emoji (the last with skin tone modifiers).
  const char *content= "Premature optimization is the root of all evil. "
    "Våre norske tegn bør æres. 日本語が少しわかります。 ✌️🐶👩🏽";
  const int len= strlen(content);

  // Just recorded from a trial run on the string above. The last four
  // bytes are padding.
  static constexpr uchar expected[]= {
    // Primary weights.
    0x1e, 0x0c, 0x1e, 0x33, 0x1c, 0xaa, 0x1d, 0xaa, 0x1c,
    0x47, 0x1e, 0x95, 0x1e, 0xb5, 0x1e, 0x33, 0x1c, 0xaa,
    0x02, 0x09, 0x1d, 0xdd, 0x1e, 0x0c, 0x1e, 0x95, 0x1d,
    0x32, 0x1d, 0xaa, 0x1d, 0x32, 0x1f, 0x21, 0x1c, 0x47,
    0x1e, 0x95, 0x1d, 0x32, 0x1d, 0xdd, 0x1d, 0xb9, 0x02,
    0x09, 0x1d, 0x32, 0x1e, 0x71, 0x02, 0x09, 0x1e, 0x95,
    0x1d, 0x18, 0x1c, 0xaa, 0x02, 0x09, 0x1e, 0x33, 0x1d,
    0xdd, 0x1d, 0xdd, 0x1e, 0x95, 0x02, 0x09, 0x1d, 0xdd,
    0x1c, 0xe5, 0x02, 0x09, 0x1c, 0x47, 0x1d, 0x77, 0x1d,
    0x77, 0x02, 0x09, 0x1c, 0xaa, 0x1e, 0xe3, 0x1d, 0x32,
    0x1d, 0x77, 0x02, 0x77, 0x02, 0x09, 0x1e, 0xe3, 0x1c,
    0x47, 0x1e, 0x33, 0x1c, 0xaa, 0x02, 0x09, 0x1d, 0xb9,
    0x1d, 0xdd, 0x1e, 0x33, 0x1e, 0x71, 0x1d, 0x65, 0x1c,
    0xaa, 0x02, 0x09, 0x1e, 0x95, 0x1c, 0xaa, 0x1c, 0xf4,
    0x1d, 0xb9, 0x02, 0x09, 0x1c, 0x60, 0x1d, 0xdd, 0x1e,
    0x33, 0x02, 0x09, 0x1c, 0x47, 0x1c, 0xaa, 0x1e, 0x33,
    0x1c, 0xaa, 0x1e, 0x71, 0x02, 0x77, 0x02, 0x09, 0xfb,
    0x40, 0xe5, 0xe5, 0xfb, 0x40, 0xe7, 0x2c, 0xfb, 0x41,
    0x8a, 0x9e, 0x3d, 0x60, 0xfb, 0x40, 0xdc, 0x11, 0x3d,
    0x66, 0x3d, 0x87, 0x3d, 0x60, 0x3d, 0x83, 0x3d, 0x79,
    0x3d, 0x67, 0x02, 0x8a, 0x02, 0x09, 0x0a, 0x2d, 0x13,
    0xdf, 0x14, 0x12, 0x13, 0xa6,
    // Padding (of primary weights).
    0x02, 0x09, 0x02, 0x09, 0x02, 0x09, 0x02, 0x09, 0x02,
    0x09,
    // Level separator.
    0x00, 0x00,
    // Secondary weights.
    0x00,
    0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
    0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
    0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
    0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
    0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
    0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
    0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
    0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
    0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
    0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
    0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
    0x00, 0x29, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
    0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
    0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
    0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
    0x00, 0x2f, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x01,
    0x10, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
    0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
    0x20, 0x00, 0x20, 0x00, 0x37, 0x00, 0x20, 0x00, 0x20,
    0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
    0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
    0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
    0x20, 0x00, 0x20, 0x00, 0x20,
    // Level separator.
    0x00, 0x00,
    // Tertiary weights.
    0x00, 0x08,
    0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00,
    0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02,
    0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00,
    0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02,
    0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00,
    0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02,
    0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00,
    0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02,
    0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00,
    0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02,
    0x00, 0x02, 0x00, 0x02, 0x00, 0x08, 0x00, 0x02, 0x00,
    0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02,
    0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00,
    0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02,
    0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00,
    0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x04, 0x00, 0x04,
    0x00, 0x04, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00,
    0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02,
    0x00, 0x0e, 0x00, 0x02, 0x00, 0x02, 0x00, 0x0e, 0x00,
    0x0e, 0x00, 0x0e, 0x00, 0x0e, 0x00, 0x0e, 0x00, 0x0e,
    0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00,
    0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02,
    0x00, 0x02, 0x00, 0x02,

    // Empty filler bytes; seemingly this collation happily
    // returns four byte less than we ask it to.
    0xff, 0xff, 0xff, 0xff
  };
  uchar dest[sizeof(expected)];

  size_t ret= 0;
  StartBenchmarkTiming();
  for (size_t i= 0; i < num_iterations; ++i)
  {
    ret = my_strnxfrm(&my_charset_utf8mb4_0900_as_cs, dest, sizeof(dest),
      pointer_cast<const uchar *>(content), len);
  }
  StopBenchmarkTiming();

  EXPECT_EQ(616u, ret);
  for (size_t i= 0; i < ret; ++i) {
    EXPECT_EQ(expected[i], dest[i])
      << "Weights differ in position " << i;
  }
}
BENCHMARK(BM_MixedUTF8MB4_AS_CS);

TEST(PadCollationTest, BasicTest)
{
  constexpr char foo[] = "foo";
  constexpr char foosp[] = "foo    ";
  constexpr char bar[] = "bar";
  constexpr char foobar[] = "foobar";

  auto my_strnncollsp= my_charset_utf8mb4_0900_ai_ci.coll->strnncollsp;

  // "foo" == "foo"
  EXPECT_EQ(my_strnncollsp(&my_charset_utf8mb4_0900_ai_ci,
                           pointer_cast<const uchar *>(foo), strlen(foo),
                           pointer_cast<const uchar *>(foo), strlen(foo)),
            0);
  // "foo" == "foo    "
  EXPECT_EQ(my_strnncollsp(&my_charset_utf8mb4_0900_ai_ci,
                           pointer_cast<const uchar *>(foo), strlen(foo),
                           pointer_cast<const uchar *>(foosp), strlen(foosp)),
            0);
  // "foo" > "bar"
  EXPECT_GT(my_strnncollsp(&my_charset_utf8mb4_0900_ai_ci,
                           pointer_cast<const uchar *>(foo), strlen(foo),
                           pointer_cast<const uchar *>(bar), strlen(bar)),
            0);
  // "foo" < "foobar" because "foo    " < "foobar"
  EXPECT_LT(my_strnncollsp(&my_charset_utf8mb4_0900_ai_ci,
                           pointer_cast<const uchar *>(foo), strlen(foo),
                           pointer_cast<const uchar *>(foobar), strlen(foobar)),
            0);

  // Exactly the same tests in reverse.

  // "foo    " == "foo"
  EXPECT_EQ(my_strnncollsp(&my_charset_utf8mb4_0900_ai_ci,
                           pointer_cast<const uchar *>(foosp), strlen(foosp),
                           pointer_cast<const uchar *>(foo), strlen(foo)),
            0);
  // "bar" < "foo"
  EXPECT_LT(my_strnncollsp(&my_charset_utf8mb4_0900_ai_ci,
                           pointer_cast<const uchar *>(bar), strlen(bar),
                           pointer_cast<const uchar *>(foo), strlen(foo)),
            0);
  // "foobar" > "foo" because "foobar" > "foo    "
  EXPECT_GT(my_strnncollsp(&my_charset_utf8mb4_0900_ai_ci,
                           pointer_cast<const uchar *>(foobar), strlen(foobar),
                           pointer_cast<const uchar *>(foo), strlen(foo)),
            0);
}

}  // namespace
